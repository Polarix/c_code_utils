#include <thread/thread.h>
#include <iostream>
#include <cstdio>

// 日志宏简单实现（用于测试）
#define MISC_ERR_LOG(fmt, ...)   fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#define MISC_WRN_LOG(fmt, ...)   fprintf(stdout, "[WARN]  " fmt "\n", ##__VA_ARGS__)
#define MISC_INF_LOG(fmt, ...)   fprintf(stdout, "[INFO]  " fmt "\n", ##__VA_ARGS__)
#define MISC_DBG_LOG(fmt, ...)   fprintf(stdout, "[DEBUG] " fmt "\n", ##__VA_ARGS__)

/**
 * @brief 测试用线程类
 */
class TestThread : public thread
{
private:
    int m_id;
    bool m_stop_flag;
    mutex m_mutex;
    condition m_cond;

protected:
    virtual void* entry() override
    {
        printf("TestThread %d started, name: %s\n", m_id, get_name());

        mutex_locker locker(m_mutex);
        while (!m_stop_flag)
        {
            // 等待条件变量，超时 2 秒
            if (m_cond.wait(2000) == cond_err_timeout)
            {
                printf("Thread %d wait timeout, still running...\n", m_id);
            }
        }
        printf("Thread %d exiting.\n", m_id);
        return nullptr;
    }

public:
    TestThread(int id, const char* name) : thread(name), m_id(id), m_stop_flag(false), m_cond(m_mutex) {}

    void stop()
    {
        {
            mutex_locker locker(m_mutex);
            m_stop_flag = true;
        }
        m_cond.signal();  // 唤醒线程
    }

    void trigger()
    {
        m_cond.signal();  // 仅唤醒，不修改停止标志
    }
};

/**
 * @brief 测试取消的线程
 */
class CancelTestThread : public thread
{
protected:
    virtual void* entry() override
    {
        printf("CancelTestThread started, will run loop.\n");
        enable_cancel(true);   // 允许取消
        set_cancelable(THREAD_CANCEL_ENABLE, THREAD_CANCEL_TYPE_DEFERRED);

        int count = 0;
        while (true)
        {
            printf("Loop %d\n", ++count);
            thread::sleep_ms(500);
            test_cancel();      // 设置取消点
        }
        return nullptr;
    }
};

int main()
{
    printf("=== Test mutex ===\n");
    mutex mtx;
    if (mtx.is_ok())
    {
        mtx.lock();
        printf("Mutex locked\n");
        mtx.unlock();
        printf("Mutex unlocked\n");
    }

    printf("\n=== Test semaphore ===\n");
    semaphore sem(0);
    if (sem.is_ok())
    {
        sem.post();
        if (sem.try_wait() == sema_no_error)
        {
            printf("Semaphore try_wait succeeded\n");
        }
    }

    printf("\n=== Test thread and condition ===\n");
    TestThread t1(1, "Worker1");
    TestThread t2(2, "Worker2");

    t1.start();
    t2.start();

    thread::sleep_ms(1000);
    printf("Main: triggering t1\n");
    t1.trigger();

    thread::sleep_ms(1000);
    printf("Main: stopping t1 and t2\n");
    t1.stop();
    t2.stop();

    t1.join();
    t2.join();

    printf("\n=== Test thread cancellation ===\n");
    CancelTestThread ct;
    ct.start();
    thread::sleep_ms(3000);
    printf("Main: canceling thread\n");
    ct.cancel();
    ct.join();   // 等待取消完成
    printf("Thread canceled and joined.\n");

    printf("\n=== Test self thread handler ===\n");
    self_thread_handler& self = thread::self();
    char name_buf[THREAD_NAME_LEN_MAX + 1];
    self.get_name(name_buf);
    printf("Main thread name: %s\n", name_buf);
    self.set_name("MainThread");
    self.get_name(name_buf);
    printf("After set, main thread name: %s\n", name_buf);

    return 0;
}