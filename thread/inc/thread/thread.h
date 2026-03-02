/////////////////////////////////////////////////////////////////////////////
// Name:        thread_base.h
// Purpose:     thread, message queue and mutex object declare.
// Author:      Xu Yulin, Pantum Dalian.
// Modified by: 2021-11-05 New creation
// Created:     2021-11-05
// Copyright:   Company internal allowed only.
/////////////////////////////////////////////////////////////////////////////
#ifndef _INCLUDED_CLASS_THREAD_BASE_H_
#define _INCLUDED_CLASS_THREAD_BASE_H_

//===========================================================//
//= Include files.                                          =//
//===========================================================//
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <string>

//===========================================================//
//= Macro definition.                                       =//
//===========================================================//
/**
 * @brief 线程名称最大长度
 */
#define THREAD_NAME_LEN_MAX         (15)

#ifndef S_OK
#define S_OK                        (0)
#endif /* S_OK definition. */
//===========================================================//
//= Data type declare.                                      =//
//===========================================================//

/**
 * @enum mutex_err_t
 * @brief 互斥锁错误码枚举
 */
typedef enum _e_mutex_error_
{
    mutex_no_err = 0,               ///< 操作成功
    mutex_err_invalid,              ///< 互斥锁未初始化
    mutex_err_dead_lock,            ///< 死锁（已被当前线程锁定）
    mutex_err_busy,                 ///< 忙（已被其他线程锁定）
    mutex_err_unlocked,             ///< 尝试解锁未锁定的互斥锁
    mutex_err_timeout,              ///< 锁定超时
    mutex_err_misc                  ///< 其他错误
} mutex_err_t;

/**
 * @enum sema_err_t
 * @brief 信号量错误码枚举
 */
typedef enum _e_sema_error_
{
    sema_no_error = 0,              ///< 操作成功
    sema_err_invalid,               ///< 信号量未正确初始化
    sema_err_busy,                  ///< 信号量忙（TryWait失败）
    sema_err_timeout,               ///< 等待超时
    sema_err_overflow,              ///< 信号量溢出（Post超过最大值）
    sema_err_misc                   ///< 其他错误
} sema_err_t;

/**
 * @enum cond_err_t
 * @brief 条件变量错误码枚举
 */
typedef enum _e_condition_error_
{
    cond_no_error = 0,              ///< 操作成功
    cond_err_invalid,               ///< 条件变量无效
    cond_err_timeout,               ///< 等待超时
    cond_err_misc_error             ///< 其他错误
} cond_err_t;

/**
 * @enum thread_err_t
 * @brief 线程错误码枚举
 */
typedef enum _e_thread_error_
{
    thread_no_err = 0,              ///< 无错误
    thread_err_no_resource,         ///< 资源不足无法创建线程
    thread_err_running,             ///< 线程已在运行
    thread_err_not_running,         ///< 线程未运行
    thread_err_killed,              ///< 线程被终止
    thread_err_misc                 ///< 其他错误
} thread_err_t;

typedef enum _e_thread_sched_
{
    thread_sched_other = SCHED_OTHER, ///< 分时调度策略，普通任务，基于优先级和时间片的CFS调度。
    thread_sched_fifo = SCHED_FIFO,   ///< 实时调度策略, 先到先服务，高优先级线程会一直运行直到阻塞或主动让出。
    thread_sched_rr = SCHED_RR,       ///< 实时调度策略, 时间片轮转，时间片轮转，避免独占CPU。
}thread_sched_t;

extern const pthread_t INVALID_THREAD_HANDLE;

//===========================================================//
//= Class declare.                                          =//
//===========================================================//

/**
 * @class mutex
 * @brief 互斥锁类，封装pthread_mutex_t
 */
class mutex
{
private:
    pthread_mutex_t                 m_mutex;        ///< pthread互斥锁对象
#ifndef USE_PTHREAD_RECURSIVE_MUTEX_INIT
    pthread_mutexattr_t             m_attr;         ///< 互斥锁属性
#endif
    bool                            m_is_ok;        ///< 初始化状态标志

public:
    /**
     * @brief 构造函数
     * @param recursive 是否创建递归互斥锁，默认为true
     */
    explicit                        mutex(bool recursive = true);

    /**
     * @brief 析构函数
     */
    virtual                         ~mutex(void);

    /**
     * @brief 检查互斥锁是否初始化成功
     * @return true-成功，false-失败
     */
    bool                            is_ok(void) const { return m_is_ok; }

    /**
     * @brief 锁定互斥锁
     * @return 错误码
     */
    mutex_err_t                     lock(void);

    /**
     * @brief 尝试锁定互斥锁
     * @return 错误码
     */
    mutex_err_t                     try_lock(void);

    /**
     * @brief 解锁互斥锁
     * @return 错误码
     */
    mutex_err_t                     unlock(void);

    // 条件变量类需要使用互斥锁
    friend class condition;
};

/**
 * @class condition
 * @brief 条件变量类，封装pthread_cond_t
 */
class condition
{
private:
    mutex&                          m_mutex_instance;   ///< 关联的互斥锁引用
    pthread_condattr_t              m_attr;             ///< 条件变量属性
    pthread_cond_t                  m_cond;             ///< pthread条件变量对象
    bool                            m_is_ok;            ///< 初始化状态标志
    clockid_t                       m_clock_id;         ///< 时钟类型

public:
    /**
     * @brief 构造函数
     * @param mutex 关联的互斥锁对象
     */
    explicit                        condition(mutex& mutex);

    /**
     * @brief 析构函数
     */
    virtual                         ~condition(void);

    /**
     * @brief 检查条件变量是否初始化成功
     * @return true-成功，false-失败
     */
    bool                            is_ok(void) const { return m_is_ok; }

    /**
     * @brief 等待条件变量
     * @param timeout_ms 超时时间（毫秒），0表示无限等待
     * @return 错误码
     */
    cond_err_t                      wait(uint32_t timeout_ms = 0);

    /**
     * @brief 唤醒一个等待线程
     * @return 错误码
     */
    cond_err_t                      signal(void);

    /**
     * @brief 唤醒所有等待线程
     * @return 错误码
     */
    cond_err_t                      broadcast(void);
};

/**
 * @class mutex_locker
 * @brief 互斥锁RAII包装类，自动管理锁的生命周期
 */
class mutex_locker
{
private:
    mutex&                          m_mutex_obj;        ///< 管理的互斥锁对象
    bool                            m_ok;               ///< 锁定状态标志

public:
    /**
     * @brief 构造函数，自动锁定互斥锁
     * @param mutex 要管理的互斥锁
     */
    explicit                        mutex_locker(mutex& mutex);

    /**
     * @brief 析构函数，自动解锁互斥锁
     */
    virtual                         ~mutex_locker(void);

    /**
     * @brief 检查是否成功锁定
     * @return true-成功，false-失败
     */
    bool                            is_ok(void) const { return m_ok; }
};

/**
 * @class semaphore
 * @brief 信号量类，封装POSIX信号量
 */
class semaphore
{
private:
    sem_t                           m_handle;           ///< 信号量句柄
    bool                            m_ok;               ///< 初始化状态标志

public:
    /**
     * @brief 构造函数
     * @param init_val 信号量初始值
     */
    explicit                        semaphore(unsigned int init_val = 0);

    /**
     * @brief 析构函数
     */
    virtual                         ~semaphore(void);

    /**
     * @brief 等待信号量（阻塞）
     * @return 错误码
     */
    sema_err_t                      wait(void);

    /**
     * @brief 尝试等待信号量（非阻塞）
     * @return 错误码
     */
    sema_err_t                      try_wait(void);

    /**
     * @brief 释放信号量
     * @return 错误码
     */
    sema_err_t                      post(void);

    /**
     * @brief 检查信号量是否初始化成功
     * @return true-成功，false-失败
     */
    bool                            is_ok(void) const { return m_ok; }
};

/**
 * @class self_thread_handler
 * @brief 当前线程处理器类
 */
class self_thread_handler
{
private:
    pthread_t                       m_handle;           ///< 当前线程句柄

public:
    /**
     * @brief 构造函数
     */
    explicit                        self_thread_handler(void);

    /**
     * @brief 析构函数
     */
    virtual                         ~self_thread_handler(void);

    /**
     * @brief 获取线程ID
     * @return 线程ID
     */
    pthread_t                       get_id(void) const;

    /**
     * @brief 获取线程名称
     * @param buf 名称缓冲区
     * @param len 缓冲区长度
     */
    void                            get_name(char* buf, unsigned int len);

    /**
     * @brief 获取线程名称（使用默认缓冲区大小）
     * @param buf 名称缓冲区（必须不小于THREAD_NAME_LEN_MAX+1）
     */
    void                            get_name(char* buf);

    /**
     * @brief 设置线程名称
     * @param new_name 新名称
     */
    void                            set_name(const char* new_name);

    /**
     * @brief 设置当前线程的优先级
     * @param new_sched 调度策略类型
     * @param len 缓冲区长度
     */
    bool                            set_sched(thread_sched_t new_sched, int priority = 0);
};

/**
 * @class thread
 * @brief 线程基类
 */
class thread
{
public:
    /**
     * @enum thread_state_t
     * @brief 线程状态枚举
     */
    typedef enum _e_thread_state_
    {
        THREAD_STATE_IDLE = 0,      ///< 空闲
        THREAD_STATE_RUNNING,       ///< 运行中
        THREAD_STATE_CANCELED,      ///< 已取消
        THREAD_STATE_EXITED                      ///< 已退出
    } thread_state_t;

    typedef enum _e_thread_cancel_state_
    {
        THREAD_CANCEL_DISABLE = 0,
        THREAD_CANCEL_ENABLE,
    } thread_cancel_state_t;

    typedef enum _e_thread_cancel_type_
    {
        THREAD_CANCEL_TYPE_DEFERRED = 0,
        THREAD_CANCEL_TYPE_ASYNC,
    } thread_cancel_type_t;

protected:
    /**
     * @brief 调用线程入口函数
     * @return 线程返回值
     */
    virtual void*                   call_entry(void);

    /**
     * @brief 线程入口函数（纯虚函数，需要子类实现）
     * @return 线程返回值
     */
    virtual void*                   entry(void) = 0;

    /**
     * @brief 设置线程状态
     * @param new_state 新状态
     */
    void                            set_state(thread_state_t new_state);

    /**
     * @brief 设置线程是否可以被取消，仅对调用函数的线程自身有效。
     * @param state 线程是否可以被取消。
     * @param type 线程取消类型，state参数为不可取消时此参数无效。
     * @return 错误码
     */
    thread_err_t                    set_cancelable(thread_cancel_state_t state, thread_cancel_type_t type);
public:
    /**
     * @brief 构造函数
     * @param thread_name 线程名称，默认为nullptr
     */
    explicit                        thread(const char* thread_name = nullptr);

    /**
     * @brief 拷贝构造函数（禁用）
     */
    explicit                        thread(const thread&) = delete;

    /**
     * @brief 拷贝赋值运算符（禁用）
     */
    thread&                         operator=(const thread&) = delete;

    /**
     * @brief 析构函数
     */
    virtual                         ~thread(void);

    /**
     * @brief 启动线程
     * @return 错误码
     */
    thread_err_t                    start(void);

    /**
     * @brief 取消线程
     * @return 错误码
     */
    thread_err_t                    cancel(void);

    /**
     * @brief 测试取消点，程序中应尽可能频繁的调用此接口以使线程及时结束并释放资源。
     */
    static void                     test_cancel(void);

    /**
     * @brief 启用/禁用取消
     * @param enable true-启用，false-禁用
     */
    static void                     enable_cancel(bool enable);

    /**
     * @brief 当前线程让出CPU执行资源，进入等待状态。
     */
    static void                     yield(void);

    /**
     * @brief 获取线程状态
     * @return 线程状态
     */
    thread_state_t                  get_state(void);

    /**
     * @brief 获取当前线程ID
     * @return 线程ID
     */
    static unsigned long            current_thread_id(void);

    /**
     * @brief 当前线程休眠指定毫秒数
     * @param sleep_ms 休眠时间（毫秒）
     */
    static void                     sleep_ms(unsigned int sleep_ms);

    /**
     * @brief 等待线程结束
     * @param exit_code 返回的退出码指针，默认为nullptr
     * @return 0-成功，其他-错误码
     */
    int                             join(void** exit_code = nullptr);

    /**
     * @brief 获取线程名称
     * @return 线程名称
     */
    const char*                     get_name(void);

    /**
     * @brief 设置线程名称
     * @param new_name 新名称
     * @return 0-成功，其他-错误码
     */
    int                             set_name(const char* new_name);

    /**
     * @brief 设置当前线程的优先级
     * @param new_sched 调度策略类型
     * @param priority 调度优先级
     */
    bool                            set_sched(thread_sched_t new_sched, int priority = 0);
    
    /**
     * @brief 获取当前线程处理器
     * @return 当前线程处理器引用
     */
    static self_thread_handler&     self(void);

private:
    pthread_t                       m_handle;           ///< 线程句柄
    thread_state_t                  m_state;            ///< 线程状态
    mutable mutex                   m_mutex;            ///< 状态保护互斥锁
    char                            m_thread_name[THREAD_NAME_LEN_MAX+1];  ///< 线程名称
    bool                            m_is_joinable;      ///< 是否可连接

    /**
     * @brief 线程启动函数（静态）
     * @param ptr 线程对象指针
     * @return 线程返回值
     */
    static void*                    thread_start_proc(void *ptr);
};

#endif // _INCLUDED_CLASS_THREAD_BASE_H_
