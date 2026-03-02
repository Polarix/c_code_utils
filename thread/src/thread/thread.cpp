//===========================================================//
//= Include files.                                          =//
//===========================================================//
#include <thread/thread.h>
#include <pthread.h>
#ifdef _WIN32
#include <windows.h>
#endif  // for 32-bit windows
#ifdef __linux
#include <sched.h>
#include <time.h>
#endif

#include <log/misc_log.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <ctime>

//===========================================================//
//= Static constants.                                       =//
//===========================================================//
const pthread_t INVALID_THREAD_HANDLE = static_cast<pthread_t>(-1); ///< 无效线程句柄

//===========================================================//
//= mutex class implementation.                             =//
//===========================================================//

/**
 * @brief 互斥锁构造函数
 * @param recursive 是否创建递归互斥锁
 */
mutex::mutex(bool recursive)
 : m_is_ok(false)
{
    int api_result;
    do
    {
        api_result = ::pthread_mutexattr_init(&m_attr);
        if (0 != api_result)
        {
            MISC_ERR_LOG("pthread_mutexattr_init() failed, %s", strerror(api_result));
            break;
        }
        api_result = pthread_mutexattr_settype(&m_attr,
            recursive ? PTHREAD_MUTEX_RECURSIVE : PTHREAD_MUTEX_NORMAL);
        if (0 != api_result)
        {
            MISC_ERR_LOG("pthread_mutexattr_settype() failed, %s", strerror(api_result));
            break;
        }
        api_result = ::pthread_mutex_init(&m_mutex, &m_attr);
        if (0 != api_result)
        {
            MISC_ERR_LOG("pthread_mutex_init() failed, %s", strerror(api_result));
            break;
        }
        /* 互斥锁构造成功 */
        m_is_ok = true;
    }while(0);
}

/**
 * @brief 互斥锁析构函数
 */
mutex::~mutex(void)
{
    if (m_is_ok)
    {
        int api_result = ::pthread_mutex_destroy(&m_mutex);
        if (api_result)
        {
            MISC_ERR_LOG("pthread_mutex_destroy() failed, %s", strerror(api_result));
        }
    }
    pthread_mutexattr_destroy(&m_attr);
}

/**
 * @brief 锁定互斥锁
 * @return 错误码
 */
mutex_err_t mutex::lock(void)
{
    mutex_err_t err = mutex_err_misc;

    if (!m_is_ok)
    {
        MISC_ERR_LOG("Mutex not initialized");
        err = mutex_err_invalid;
    }
    else
    {
        int api_result = ::pthread_mutex_lock(&m_mutex);
        if (0 != api_result)
        {
            if (api_result == EDEADLK)
            {
                MISC_ERR_LOG("pthread_mutex_lock() deadlock detected");
                err = mutex_err_dead_lock;
            }
            else
            {
                MISC_ERR_LOG("pthread_mutex_lock() failed with error %s", strerror(api_result));
                err = mutex_err_misc;
            }
        }
        else
        {
            err = mutex_no_err;
        }
    }

    return err;
}

/**
 * @brief 尝试锁定互斥锁
 * @return 错误码
 */
mutex_err_t mutex::try_lock(void)
{
    mutex_err_t err = mutex_err_misc;

    if (!m_is_ok)
    {
        MISC_ERR_LOG("Mutex not initialized");
        err = mutex_err_invalid;
    }
    else
    {
        int api_result = ::pthread_mutex_trylock(&m_mutex);
        if(0 != api_result)
        {
            if (api_result == EBUSY)
            {
                err = mutex_err_busy;
            }
            else
            {
                MISC_ERR_LOG("pthread_mutex_trylock() failed with error %s", strerror(api_result));
                err = mutex_err_misc;
            }
        }
        else
        {
            err = mutex_no_err;
        }
    }

    return err;
}

/**
 * @brief 解锁互斥锁
 * @return 错误码
 */
mutex_err_t mutex::unlock(void)
{
    mutex_err_t err = mutex_err_misc;

    if (!m_is_ok)
    {
        MISC_ERR_LOG("Mutex not initialized");
        err = mutex_err_invalid;
    }
    else
    {
        int api_result = ::pthread_mutex_unlock(&m_mutex);
        switch (api_result)
        {
            case 0:
            {
                err = mutex_no_err;
                break;
            }
            case EPERM:
            {
                MISC_ERR_LOG("pthread_mutex_unlock(): we don't own the mutex");
                err = mutex_err_unlocked;
                break;
            }
            case EINVAL:
            {
                MISC_ERR_LOG("pthread_mutex_unlock(): mutex not initialized.");
                err = mutex_err_invalid;
                break;
            }
            default:
            {
                MISC_ERR_LOG("pthread_mutex_unlock() failed with error %d", api_result);
                err = mutex_err_misc;
                break;
            }
        }
    }

    return err;
}

//===========================================================//
//= condition class implementation.                         =//
//===========================================================//

/**
 * @brief 条件变量构造函数
 * @param mutex 关联的互斥锁
 */
condition::condition(mutex& mutex)
 : m_mutex_instance(mutex)
 , m_is_ok(false)
 , m_clock_id(CLOCK_MONOTONIC)
{
    int api_result;
    do
    {
        api_result = ::pthread_condattr_init(&m_attr);
        if (0 != api_result)
        {
            MISC_ERR_LOG("pthread_condattr_init() result error %d.", api_result);
            break;
        }
        /* 设置时钟源为CLOCK_MONOTONIC */
        api_result = ::pthread_condattr_setclock(&m_attr, CLOCK_MONOTONIC);
        if (0 != api_result)
        {
            // MISC_WRN_LOG("pthread_condattr_setclock(CLOCK_MONOTONIC) result error %d.", api_result);
            /* 如果失败则尝试设置REALTIME */
            api_result = ::pthread_condattr_setclock(&m_attr, CLOCK_REALTIME);
            if (0 != api_result)
            {
                MISC_ERR_LOG("pthread_condattr_setclock(CLOCK_REALTIME) result error %d.", api_result);
                break;
            }
            else
            {
                MISC_WRN_LOG("pthread_condattr_setclock(CLOCK_REALTIME) result successed.");
                m_clock_id = CLOCK_REALTIME;
            }
        }
        else
        {
            MISC_WRN_LOG("pthread_condattr_setclock(CLOCK_MONOTONIC) result successed.");
        }
        api_result = ::pthread_cond_init(&m_cond, &m_attr);
        if (0 != api_result)
        {
            MISC_ERR_LOG("pthread_cond_init() result error %d.", api_result);
            break;
        }
        m_is_ok = true;
    }while(0);
}

/**
 * @brief 条件变量析构函数
 */
condition::~condition()
{
    if (m_is_ok)
    {
        /* 唤醒所有等待线程，避免销毁时有线程在等待 */
        broadcast();
        /* 销毁条件对象 */
        int api_result = ::pthread_cond_destroy(&m_cond);
        if (0 != api_result)
        {
            MISC_ERR_LOG("pthread_cond_destroy() result error %d.", api_result);
        }
        /* 销毁配置参数 */
        ::pthread_condattr_destroy(&m_attr);
    }
}

/**
 * @brief 等待条件变量
 * @param timeout_ms 超时时间（毫秒），0表示无限等待
 * @return 错误码
 */
cond_err_t condition::wait(uint32_t timeout_ms)
{
    cond_err_t err = cond_err_misc_error;

    if (!m_is_ok)
    {
        MISC_ERR_LOG("Condition variable not initialized");
        err = cond_err_invalid;
    }
    else
    {
        int api_result;
        if(timeout_ms > 0)
        {
            struct timespec ts = {0, 0};
            /* 获取当前时间 */
            if (0 != ::clock_gettime(m_clock_id, &ts))
            {
                MISC_ERR_LOG("clock_gettime() failed");
                return cond_err_misc_error;
            }

            /* 计算超时时间 */
            ts.tv_sec += timeout_ms / 1000;
            ts.tv_nsec += (timeout_ms % 1000) * 1000000;

            /* 处理纳秒溢出 */
            if (ts.tv_nsec >= 1000000000)
            {
                ts.tv_sec += 1;
                ts.tv_nsec -= 1000000000;
            }

            MISC_DBG_LOG("Wait for %u ms.", timeout_ms);
            api_result = ::pthread_cond_timedwait(&m_cond, &(m_mutex_instance.m_mutex), &ts);
        }
        else
        {
            MISC_DBG_LOG("Wait forever.");
            api_result = ::pthread_cond_wait(&m_cond, &(m_mutex_instance.m_mutex));
        }

        switch(api_result)
        {
            case 0: // No error.
            {
                err = cond_no_error;
                break;
            }
            case ETIMEDOUT:
            {
                err = cond_err_timeout;
                MISC_WRN_LOG("Waiting timeout.");
                break;
            }
            case EINVAL:
            {
                MISC_ERR_LOG("pthread_cond_wait/timedwait(): invalid arguments");
                err = cond_err_invalid;
                break;
            }
            default:
            {
                MISC_ERR_LOG("pthread_cond_wait/timedwait() result error %d.", api_result);
                err = cond_err_misc_error;
                break;
            }
        }
    }

    return err;
}

/**
 * @brief 唤醒一个等待线程
 * @return 错误码
 */
cond_err_t condition::signal()
{
    cond_err_t err = cond_err_misc_error;

    if (!m_is_ok)
    {
        MISC_ERR_LOG("Condition variable not initialized");
        err = cond_err_invalid;
    }
    else
    {
        int api_result = ::pthread_cond_signal(&m_cond);
        if (0 != api_result)
        {
            MISC_ERR_LOG("pthread_cond_signal() result error %d.", api_result);
            err = cond_err_misc_error;
        }
        else
        {
            err = cond_no_error;
        }
    }

    return err;
}

/**
 * @brief 唤醒所有等待线程
 * @return 错误码
 */
cond_err_t condition::broadcast()
{
    cond_err_t err = cond_err_misc_error;

    if (!m_is_ok)
    {
        MISC_ERR_LOG("Condition variable not initialized");
        err = cond_err_invalid;
    }
    else
    {
        int api_result = ::pthread_cond_broadcast(&m_cond);
        if (0 != api_result)
        {
            MISC_ERR_LOG("pthread_cond_broadcast() result error %d.", api_result);
            err = cond_err_misc_error;
        }
        else
        {
            err = cond_no_error;
        }
    }
    return err;
}

//===========================================================//
//= mutex_locker class implementation.                      =//
//===========================================================//

/**
 * @brief 互斥锁包装器构造函数
 * @param mutex 要管理的互斥锁
 */
mutex_locker::mutex_locker(mutex& mutex)
 :m_mutex_obj(mutex)
 , m_ok(false)
{
    if(mutex_no_err == m_mutex_obj.lock())
    {
        m_ok = true;
    }
    else
    {
        m_ok = false;
        MISC_ERR_LOG("Failed to lock mutex");
    }
}

/**
 * @brief 互斥锁包装器析构函数
 */
mutex_locker::~mutex_locker(void)
{
    if(m_ok)
    {
        m_mutex_obj.unlock();
    }
}

//===========================================================//
//= semaphore class implementation.                         =//
//===========================================================//

/**
 * @brief 信号量构造函数
 * @param init_val 初始值
 */
semaphore::semaphore(unsigned int init_val)
 : m_ok(false)
{
    memset(&m_handle, 0x00, sizeof(sem_t));

    if(0 != ::sem_init(&m_handle, 0, init_val))
    {
        MISC_ERR_LOG("semaphore init failed: %s", strerror(errno));
        m_ok = false;
    }
    else
    {
        m_ok = true;
    }
}

/**
 * @brief 信号量析构函数
 */
semaphore::~semaphore(void)
{
    if (m_ok)
    {
        if(0 != sem_destroy(&m_handle))
        {
            MISC_ERR_LOG("semaphore destroy failed: %s", strerror(errno));
        }
    }
}

/**
 * @brief 尝试等待信号量（非阻塞）
 * @return 错误码
 */
sema_err_t semaphore::try_wait(void)
{
    sema_err_t err = sema_err_invalid;

    if(!m_ok)
    {
        err = sema_err_invalid;
    }
    else
    {
        int api_result = sem_trywait(&m_handle);
        switch(api_result)
        {
            case 0:
            {
                err = sema_no_error;
                break;
            }
            case EAGAIN:
            {
                err = sema_err_busy;
                break;
            }
            default:
            {
                MISC_ERR_LOG("sem_trywait() failed: %s", strerror(errno));
                err = sema_err_misc;
                break;
            }
        }
    }
    return err;
}

/**
 * @brief 等待信号量（阻塞）
 * @return 错误码
 */
sema_err_t semaphore::wait(void)
{
    sema_err_t err = sema_err_invalid;

    if(!m_ok)
    {
        err = sema_err_invalid;
    }
    else
    {
        if(0 == sem_wait(&m_handle))
        {
            err = sema_no_error;
        }
        else
        {
            MISC_ERR_LOG("sem_wait() failed: %s", strerror(errno));
            err = sema_err_misc;
        }
    }

    return err;
}

/**
 * @brief 释放信号量
 * @return 错误码
 */
sema_err_t semaphore::post(void)
{
    sema_err_t err = sema_err_invalid;

    if(!m_ok)
    {
        err = sema_err_invalid;
    }
    else
    {
        if(0 == sem_post(&m_handle))
        {
            err = sema_no_error;
        }
        else
        {
            MISC_ERR_LOG("sem_post() failed: %s", strerror(errno));
            err = sema_err_misc;
        }
    }

    return err;
}

//===========================================================//
//= self_thread_handler class implementation.               =//
//===========================================================//

/**
 * @brief 当前线程处理器构造函数
 */
self_thread_handler::self_thread_handler(void)
{
    m_handle = ::pthread_self();
}

/**
 * @brief 当前线程处理器析构函数
 */
self_thread_handler::~self_thread_handler(void)
{
    /* Do nothing. */
}

/**
 * @brief 获取线程ID
 * @return 线程ID
 */
pthread_t self_thread_handler::get_id(void) const
{
    return m_handle;
}

/**
 * @brief 获取线程名称
 * @param buf 名称缓冲区
 * @param len 缓冲区长度
 */
void self_thread_handler::get_name(char* buf, unsigned int len)
{
    if (buf == nullptr || len == 0)
    {
        MISC_ERR_LOG("Invalid buffer or length");
    }
    else
    {
        int api_result;
        char thread_name_buf[THREAD_NAME_LEN_MAX+1] = {0x00};
#ifdef __linux__
        api_result = ::pthread_getname_np(m_handle, thread_name_buf, sizeof(thread_name_buf));
#elif defined(_WIN32)
        /* Windows pthread实现可能不支持此功能。 */
        const char* default_name = "Unknown";
        ::strncpy(thread_name_buf, default_name, sizeof(thread_name_buf)-1);
        api_result = 0;
#else
        api_result = -1;
#endif
        if(0 != api_result)
        {
            MISC_ERR_LOG("Get thread name failed with error %d", api_result);
            buf[0] = '\0';
        }
        else
        {
            size_t name_len = (len > THREAD_NAME_LEN_MAX) ? THREAD_NAME_LEN_MAX : len;
            ::strncpy(buf, thread_name_buf, name_len);
            buf[name_len-1] = '\0'; // 确保以null结尾
        }
    }
}

/**
 * @brief 获取线程名称（使用默认缓冲区大小）
 * @param buf 名称缓冲区（必须不小于THREAD_NAME_LEN_MAX+1）
 */
void self_thread_handler::get_name(char* buf)
{
    get_name(buf, THREAD_NAME_LEN_MAX+1);
}

/**
 * @brief 设置线程名称
 * @param new_name 新名称
 */
void self_thread_handler::set_name(const char* new_name)
{
    if(new_name == nullptr)
    {
        MISC_ERR_LOG("Invalid argument.");
    }
    else
    {
        char thread_name_buf[THREAD_NAME_LEN_MAX+1] = {0x00};
        size_t name_len = ::strlen(new_name);
        if(name_len > THREAD_NAME_LEN_MAX)
        {
            ::strncpy(thread_name_buf, new_name, THREAD_NAME_LEN_MAX);
            thread_name_buf[THREAD_NAME_LEN_MAX] = '\0';
        }
        else
        {
            ::strcpy(thread_name_buf, new_name);
        }
        int api_result;
#ifdef __linux__
        api_result = ::pthread_setname_np(m_handle, thread_name_buf);
#elif defined(__APPLE__)
        api_result = ::pthread_setname_np(thread_name_buf);
#elif defined(_WIN32)
        /* Windows pthread实现可能不支持此功能。 */
        MISC_WRN_LOG("Thread name setting not supported on Windows");
        api_result = 0;
#else
        api_result = -1;
#endif
        if(0 != api_result)
        {
            MISC_ERR_LOG("Set thread name failed with error %d", api_result);
        }
    }
}

bool self_thread_handler::set_sched(thread_sched_t new_sched, int priority)
{
    bool result = false;
    if(m_handle != INVALID_THREAD_HANDLE)
    {
        int policy;
        struct sched_param param;
        if(0 == ::pthread_getschedparam(m_handle, &policy, &param))
        {
            MISC_INF_LOG("Now thread policy is %d, priority is %d.", policy, param.sched_priority);
            param.sched_priority = priority; /* Used 0 for default priority. */
            if(0 != ::pthread_setschedparam(m_handle, new_sched, &param))
            {
                MISC_ERR_LOG("Get thread sched attr failed, %s.", ::strerror(errno));
            }
            else
            {
                MISC_INF_LOG("Set thread policy to %d, priority to %d.", policy, param.sched_priority);
                result = true;
            }
        }
        else
        {
            MISC_ERR_LOG("Get thread sched attr failed %s.", ::strerror(errno));
        }
    }
    return result;
}

//===========================================================//
//= thread class implementation.                            =//
//===========================================================//

/**
 * @brief 线程启动函数（静态）
 * @param ptr 线程对象指针
 * @return 线程返回值
 */
void* thread::thread_start_proc(void* ptr)
{
    void* rtn = nullptr;

    thread* thread_obj = reinterpret_cast<thread*>(ptr);

    if(thread_obj != nullptr)
    {
        rtn = thread_obj->call_entry();
    }
    else
    {
        MISC_ERR_LOG("CANNOT reinterpret thread object, exit.");
        // rtn = reinterpret_cast<void*>(thread_err_no_resource);
    }

    return rtn;
}

/**
 * @brief 线程构造函数
 * @param thread_name 线程名称
 */
thread::thread(const char* thread_name)
 : m_handle(INVALID_THREAD_HANDLE)
 , m_state(THREAD_STATE_IDLE)
 , m_is_joinable(true)
{
    if (thread_name != nullptr)
    {
        set_name(thread_name);
    }
    else
    {
        m_thread_name[0] = '\0';
    }
}

/**
 * @brief 线程析构函数
 */
thread::~thread(void)
{
    /* 避免预期之外的异常操作，不使用自动锁控制。 */
    m_mutex.lock();
    /* 与cancel方法相比，析构处理无视任何错误，强制终止和释放线程资源。 */
    if(m_handle != INVALID_THREAD_HANDLE)
    {
        const thread_state_t now_state = get_state();
        /* 线程运行中则尝试结束。 */
        if (now_state == THREAD_STATE_RUNNING)
        {
            MISC_WRN_LOG("Thread still running, forcing cancellation");
            /* 尝试取消线程。 */
            if (0 != ::pthread_cancel(m_handle))
            {
                MISC_ERR_LOG("pthread_cancel() failed");
            }
        }
        set_state(THREAD_STATE_CANCELED);
        m_handle = INVALID_THREAD_HANDLE;
    }

    m_mutex.unlock();
}

/**
 * @brief 设置线程状态
 * @param new_state 新状态
 */
void thread::set_state(thread_state_t new_state)
{
    mutex_locker locker(m_mutex);
    m_state = new_state;
}

/**
 * @brief 获取线程状态
 * @return 线程状态
 */
thread::thread_state_t thread::get_state(void)
{
    thread_state_t current_state;

    m_mutex.lock();
    current_state = m_state;
    m_mutex.unlock();

    return current_state;
}

/**
 * @brief 调用线程入口函数
 * @return 线程返回值
 */
void* thread::call_entry(void)
{
    void* exit_code = nullptr;
    /* 更新线程状态为"运行中" */
    set_state(THREAD_STATE_RUNNING);
    /* 启动线程体并等待结束或取消 */
    exit_code = entry();
    /* 更新线程状态为"已退出" */
    set_state(THREAD_STATE_EXITED);
    return exit_code;
}

/**
 * @brief 让出CPU
 */
void thread::yield(void)
{
#ifdef _WIN32
    ::Sleep(0);
#elif defined(__linux__)
    // ::pthread_yield();
    sched_yield();
#endif
}

/**
 * @brief 启动线程
 * @return 错误码
 */
thread_err_t thread::start(void)
{
    thread_err_t err = thread_err_misc;

    do
    {
        mutex_locker locker(m_mutex);

        /* 检查线程状态 */
        if(m_state == THREAD_STATE_RUNNING)
        {
            MISC_WRN_LOG("Thread already running.");
            err = thread_err_running;
            break;
        }
        if(m_handle != INVALID_THREAD_HANDLE)
        {
            MISC_WRN_LOG("Thread handle already exists.");
            err = thread_err_misc;
            break;
        }
        /* 线程参数属性。 */
        pthread_attr_t attr;
        int api_result;
        api_result = pthread_attr_init(&attr);
        if (0 != api_result)
        {
            MISC_ERR_LOG("pthread_attr_init() failed, result %d.", api_result);
            err = thread_err_no_resource;
            break;
        }
        /* 设置为可连接线程。 */
        api_result = ::pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        if (0 != api_result)
        {
            MISC_ERR_LOG("pthread_attr_setdetachstate() failed, result %d.", api_result);
            err = thread_err_misc;
            break;
        }
        /* 创建线程。 */
        api_result = ::pthread_create(&m_handle, &attr, thread_start_proc, this);
        if (0 != api_result)
        {
            MISC_ERR_LOG("Create thread failed, result %d.", api_result);
            m_handle = INVALID_THREAD_HANDLE;
            err = thread_err_no_resource;
            break;
        }
        /* 释放属性资源 */
        api_result = pthread_attr_destroy(&attr);
        if(0 != api_result)
        {
            MISC_WRN_LOG("pthread_attr_destroy() failed, result %d.", api_result);
        }
        MISC_INF_LOG("Create thread(%s) done.", m_thread_name);
        /* 设置线程名称 */
        if(m_thread_name[0] != '\0')
        {
#ifdef __linux__
            api_result = ::pthread_setname_np(m_handle, m_thread_name);
#elif defined(__APPLE__)
            api_result = ::pthread_setname_np(m_thread_name);
#endif
            if(0 != api_result)
            {
                MISC_WRN_LOG("pthread_setname_np() failed, result %d.", api_result);
            }
        }
        /* 线程已启动。 */
        err = thread_no_err;
    }while(0);

    return err;
}

/**
 * @brief 取消线程
 * @return 错误码
 */
thread_err_t thread::cancel(void)
{
    thread_err_t err = thread_err_misc;

    mutex_locker auto_locker(m_mutex);

    if(m_handle != INVALID_THREAD_HANDLE)
    {
        int api_result = ::pthread_cancel(m_handle);
        if(0 != api_result)
        {
            MISC_ERR_LOG("pthread_cancel() failed with error %d", api_result);
            err = thread_err_misc;
        }
        else
        {
#ifdef THREAD_JOIN_AFTER_CANCEL
            err = thread_no_err;
            set_state(THREAD_STATE_CANCELED);
            if(m_is_joinable)
            {
                // 等待线程结束
                void* exit_code = nullptr;
                if (0 != ::pthread_join(m_handle, &exit_code))
                {
                    MISC_ERR_LOG("pthread_join() failed after cancel");
                }
            }
#endif /* THREAD_JOIN_AFTER_CANCEL */
            m_handle = INVALID_THREAD_HANDLE;
        }
    }
    else
    {
        err = thread_err_not_running;
    }

    return err;
}

/**
 * @brief 测试取消点
 */
void thread::test_cancel(void)
{
    ::pthread_testcancel();
}

/**
 * @brief 启用/禁用取消
 * @param enable true-启用，false-禁用
 */
void thread::enable_cancel(bool enable)
{
    ::pthread_setcancelstate(enable ? PTHREAD_CANCEL_ENABLE : PTHREAD_CANCEL_DISABLE, nullptr);
}

/**
 * @brief 获取当前线程ID
 * @return 线程ID
 */
unsigned long thread::current_thread_id(void)
{
    return static_cast<unsigned long>(::pthread_self());
}

/**
 * @brief 休眠指定毫秒数
 * @param sleep_ms 休眠时间（毫秒）
 */
void thread::sleep_ms(unsigned int sleep_ms)
{
#ifdef _WIN32
    ::Sleep(sleep_ms);
#else
    // unistd.h
    ::usleep(sleep_ms * 1000);
#endif
}

/**
 * @brief 等待线程结束
 * @param exit_code 返回的退出码指针，默认为nullptr
 * @return 0-成功，其他-错误码
 */
int thread::join(void** exit_code)
{
    int rtn = -1;

    if(m_handle != INVALID_THREAD_HANDLE)
    {
        rtn = ::pthread_join(m_handle, exit_code);
        if (rtn == 0)
        {
            m_mutex.lock();
            m_handle = INVALID_THREAD_HANDLE;
            m_state = THREAD_STATE_EXITED;
            m_mutex.unlock();
        }
    }
    else
    {
        // MISC_ERR_LOG("Invalid thread handle");
    }

    return rtn;
}

/**
 * @brief 获取线程名称
 * @return 线程名称
 */
const char* thread::get_name(void)
{
    return m_thread_name;
}

/**
 * @brief 设置线程名称
 * @param new_name 新名称
 */
int thread::set_name(const char* new_name)
{
    int result;
    if(new_name == nullptr)
    {
        result = EINVAL;
        MISC_ERR_LOG("Invalid parameter.");
    }
    else
    {
        size_t name_len = ::strlen(new_name);
        if(name_len > THREAD_NAME_LEN_MAX)
        {
            ::strncpy(m_thread_name, new_name, THREAD_NAME_LEN_MAX);
            m_thread_name[THREAD_NAME_LEN_MAX] = '\0';
        }
        else
        {
            ::strcpy(m_thread_name, new_name);
        }

        /* 如果线程已存在，设置系统线程名称 */
        if(m_handle != INVALID_THREAD_HANDLE)
        {
    #ifdef __linux__
            ::pthread_setname_np(m_handle, m_thread_name);
    #elif defined(__APPLE__)
            ::pthread_setname_np(m_thread_name);
    #endif
        }

        result = S_OK;
    }

    return result;
}

bool thread::set_sched(thread_sched_t new_sched, int priority)
{
    bool result = false;
    if(m_handle != INVALID_THREAD_HANDLE)
    {
        int policy;
        struct sched_param param;
        if(0 == ::pthread_getschedparam(m_handle, &policy, &param))
        {
            MISC_INF_LOG("Now thread policy is %d, priority is %d.", policy, param.sched_priority);
            param.sched_priority = priority; /* Used 0 for default priority. */
            if(0 != ::pthread_setschedparam(m_handle, new_sched, &param))
            {
                MISC_ERR_LOG("Get thread sched attr failed, %s.", ::strerror(errno));
            }
            else
            {
                MISC_INF_LOG("Set thread policy to %d, priority to %d.", policy, param.sched_priority);
                result = true;
            }
        }
        else
        {
            MISC_ERR_LOG("Get thread sched attr failed %s.", ::strerror(errno));
        }
    }
    return result;
}

thread_err_t thread::set_cancelable(thread_cancel_state_t state, thread_cancel_type_t type)
{
    thread_err_t err = thread_err_misc;
    do
    {
        /* 确保线程已存在 */
        if(m_handle == INVALID_THREAD_HANDLE)
        {
            MISC_WRN_LOG("Thread is not running.");
            /* 线程尚未启动 */
            err = thread_err_not_running;
            break;
        }
        int api_result;
        /* 设置取消状态 */
        api_result = ::pthread_setcancelstate((THREAD_CANCEL_ENABLE == state)?PTHREAD_CANCEL_ENABLE:PTHREAD_CANCEL_DISABLE, nullptr);
        if(0 != api_result)
        {
            MISC_ERR_LOG("pthread_cancel() failed with error %d", api_result);
            break;
        }
        /* 设置取消状态类型 */
        api_result = ::pthread_setcanceltype((THREAD_CANCEL_TYPE_ASYNC == type)?PTHREAD_CANCEL_ASYNCHRONOUS:PTHREAD_CANCEL_DEFERRED, nullptr);
        if(0 != api_result)
        {
            MISC_ERR_LOG("pthread_setcanceltype() failed with error %d", api_result);
            break;
        }
        /* 参数设置完成 */
        err = thread_no_err;
    }while(0);

    return err;
}

/**
 * @brief 获取当前线程处理器
 * @return 当前线程处理器引用
 */
self_thread_handler& thread::self(void)
{
#ifdef __GNUC__
    static __thread self_thread_handler self_handler;
#elif defined(_MSC_VER)
    static __declspec(thread) self_thread_handler self_handler;
#elif __STDC_VERSION__ >= 201112L
    static _Thread_local self_thread_handler self_handler;
#else
    #error "Thread-local storage not supported"
#endif
    return self_handler;
}
