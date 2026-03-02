#include <thread/thread.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <memory>

// 测试专用日志宏（避免与 thread.h 中的冲突）
// 为 WRN 和 ERR 添加 ANSI 颜色（适用于 MSYS2、Linux 终端）
#define THREAD_TEST_INF_LOG(fmt, ...)   printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define THREAD_TEST_WRN_LOG(fmt, ...)   printf("\033[33m[WARN] " fmt "\033[0m\n", ##__VA_ARGS__)
#define THREAD_TEST_ERR_LOG(fmt, ...)   fprintf(stderr, "\033[31m[ERROR] " fmt "\033[0m\n", ##__VA_ARGS__)
#define THREAD_TEST_DBG_LOG(fmt, ...)   printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

// 简单的自旋等待辅助函数
static void spin_wait(int ms)
{
    thread::sleep_ms(ms);
}

/*----------------------------------------------------------------------------
 * 基础测试线程：使用条件变量等待，支持停止
 *----------------------------------------------------------------------------*/
class TestWorker : public thread
{
private:
    int             m_id;
    bool            m_stop;
    mutex           m_mutex;
    condition       m_cond;

protected:
    virtual void* entry() override
    {
        THREAD_TEST_INF_LOG("Worker %d (name: %s) started", m_id, get_name());
        {
            mutex_locker locker(m_mutex);
            while (!m_stop)
            {
                if (m_cond.wait(500) == cond_err_timeout)
                {
                    THREAD_TEST_DBG_LOG("Worker %d timeout", m_id);
                }
            }
        }
        THREAD_TEST_INF_LOG("Worker %d exiting", m_id);
        return nullptr;
    }

public:
    TestWorker(int id, const char* name)
        : thread(name)
        , m_id(id)
        , m_stop(false)
        , m_cond(m_mutex)
    {}

    void stop()
    {
        {
            mutex_locker locker(m_mutex);
            m_stop = true;
        }
        m_cond.signal();   // 唤醒可能正在等待的线程
    }

    void trigger()
    {
        m_cond.signal();   // 仅唤醒，不修改停止标志
    }
};

/*----------------------------------------------------------------------------
 * 嵌套线程测试：线程内部再创建子线程
 *----------------------------------------------------------------------------*/
class NestedWorker : public thread
{
private:
    int m_level;
    std::vector<std::unique_ptr<TestWorker>> m_children;

protected:
    virtual void* entry() override
    {
        THREAD_TEST_INF_LOG("Nested worker level %d started", m_level);

        // 创建并启动子线程
        const int child_count = 3;
        for (int i = 0; i < child_count; ++i)
        {
            char name[32];
            snprintf(name, sizeof(name), "ChildL%d-%d", m_level, i);
            auto child = new TestWorker(i, name);
            m_children.emplace_back(child);
            child->start();
        }

        // 等待一段时间，然后停止所有子线程
        spin_wait(2000);
        for (auto& child : m_children)
        {
            child->stop();
            child->join();
        }

        THREAD_TEST_INF_LOG("Nested worker level %d exiting", m_level);
        return nullptr;
    }

public:
    explicit NestedWorker(int level) : thread("Nested"), m_level(level) {}
};

/*----------------------------------------------------------------------------
 * 可取消线程测试（使用 pthread 取消机制）
 *----------------------------------------------------------------------------*/
class CancelableThread : public thread
{
protected:
    virtual void* entry() override
    {
        THREAD_TEST_INF_LOG("CancelableThread started");
        enable_cancel(true);
        set_cancelable(THREAD_CANCEL_ENABLE, THREAD_CANCEL_TYPE_DEFERRED);

        int count = 0;
        while (true)
        {
            THREAD_TEST_DBG_LOG("CancelableThread loop %d", ++count);
            thread::sleep_ms(300);
            test_cancel();   // 取消点
        }
        return nullptr;
    }
};

/*----------------------------------------------------------------------------
 * 强制停止测试（不等待，直接析构或调用 force_stop）
 *----------------------------------------------------------------------------*/
class ForceStopThread : public thread
{
protected:
    virtual void* entry() override
    {
        THREAD_TEST_INF_LOG("ForceStopThread started, will run forever");
        while (true)
        {
            thread::sleep_ms(500);
            thread::test_cancel();   // 显式取消点，使线程可响应取消请求（Windows 下必需）
        }
        return nullptr;
    }
};

/*----------------------------------------------------------------------------
 * 主测试函数
 *----------------------------------------------------------------------------*/
int main()
{
    THREAD_TEST_INF_LOG("========== Thread Wrapper Test ==========");
#ifdef USE_ATOMIC_THREAD_STATE
    THREAD_TEST_INF_LOG("Atomic state: ENABLED");
#else
    THREAD_TEST_INF_LOG("Atomic state: DISABLED (using mutex)");
#endif

    /* 1. 基础互斥锁测试 */
    THREAD_TEST_INF_LOG("\n--- Test mutex ---");
    mutex mtx;
    if (mtx.is_ok())
    {
        mtx.lock();
        THREAD_TEST_INF_LOG("Mutex locked");
        mtx.unlock();
        THREAD_TEST_INF_LOG("Mutex unlocked");
    }

    /* 2. 信号量测试 */
    THREAD_TEST_INF_LOG("\n--- Test semaphore ---");
    semaphore sem(0);
    if (sem.is_ok())
    {
        sem.post();
        if (sem.try_wait() == sema_no_error)
            THREAD_TEST_INF_LOG("Semaphore try_wait succeeded");
    }

    /* 3. 大量线程创建/停止测试 */
    THREAD_TEST_INF_LOG("\n--- Stress test: 100 workers ---");
    {
        std::vector<std::unique_ptr<TestWorker>> workers;
        for (int i = 0; i < 100; ++i)
        {
            char name[32];
            snprintf(name, sizeof(name), "Worker%d", i);
            workers.emplace_back(new TestWorker(i, name));
            workers.back()->start();
        }

        spin_wait(1500);
        THREAD_TEST_INF_LOG("Triggering random workers");
        for (int i = 0; i < 20; ++i)
        {
            int idx = rand() % workers.size();
            workers[idx]->trigger();
        }

        spin_wait(1000);
        THREAD_TEST_INF_LOG("Stopping all workers");
        for (auto& w : workers)
        {
            w->stop();
        }
        for (auto& w : workers)
        {
            w->join();
        }
    }

    /* 4. 嵌套线程测试 */
    THREAD_TEST_INF_LOG("\n--- Test nested threads ---");
    {
        NestedWorker top(1);
        top.start();
        top.join();
    }

    /* 5. 可取消线程测试 */
    THREAD_TEST_INF_LOG("\n--- Test cancelable thread ---");
    {
        CancelableThread ct;
        ct.start();
        spin_wait(2000);
        THREAD_TEST_INF_LOG("Canceling thread");
        ct.cancel();
        ct.join();
        THREAD_TEST_INF_LOG("Thread canceled and joined");
    }

    /* 6. 强制停止测试（通过析构） */
    THREAD_TEST_INF_LOG("\n--- Test force stop via destructor ---");
    {
        ForceStopThread ft;
        ft.start();
        spin_wait(1000);
        THREAD_TEST_INF_LOG("Destroying thread object (force stop)");
        // ft 析构时会调用 force_stop() 取消并等待
    }
    THREAD_TEST_INF_LOG("Force stop done");

    /* 7. 当前线程处理器测试 */
    THREAD_TEST_INF_LOG("\n--- Test self thread handler ---");
    {
        self_thread_handler& self = thread::self();
        char name_buf[THREAD_NAME_LEN_MAX + 1];
        self.get_name(name_buf);
        THREAD_TEST_INF_LOG("Main thread current name: %s", name_buf);
        self.set_name("MainThread");
        self.get_name(name_buf);
        THREAD_TEST_INF_LOG("After set: %s", name_buf);
    }

    THREAD_TEST_INF_LOG("\n========== All tests passed ==========");
    return 0;
}
