#ifndef LOG_UTILS_H
#define LOG_UTILS_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>   /* for malloc/free */

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 跨平台符号导出宏（仅独立编译模式且构建 DLL 时有效）========== */
#if !defined(LOG_UTILS_HEAD_ONLY)
    #if defined(_WIN32) && defined(LOG_UTILS_EXPORTS)
        #define LOG_UTILS_API __declspec(dllexport)
    #elif defined(__GNUC__) && defined(LOG_UTILS_EXPORTS)
        #define LOG_UTILS_API __attribute__((visibility("default")))
    #else
        #define LOG_UTILS_API
    #endif
#else
    /* 头文件模式下所有函数均为 static inline */
    #define LOG_UTILS_API static inline
#endif

/**
 * @enum _e_log_level_
 * @brief 日志等级枚举（数值越小越严重）
 */
typedef enum _e_log_level_
{
    LOG_LEVEL_E = 0,   /*!< 错误等级 */
    LOG_LEVEL_W,       /*!< 警告等级 */
    LOG_LEVEL_I,       /*!< 信息等级（默认阈值） */
    LOG_LEVEL_D,       /*!< 调试等级 */
    LOG_LEVEL_T        /*!< 跟踪等级 */
} log_level_t;

/**
 * @typedef log_output_func_t
 * @brief 日志输出函数指针类型（与 printf 兼容）
 */
typedef int (*log_output_func_t)(const char *format, ...);

/**
 * @struct s_st_logger_config
 * @brief  日志器结构体（用户可见，可独立控制）
 */
typedef struct s_st_logger_config
{
    log_level_t        level;       /*!< 当前日志等级阈值 */
    log_output_func_t  output_func; /*!< 输出函数指针 */
} logger_t;

/* ========== 日志器生命周期管理 ========== */

/**
 * @brief 创建一个新的日志器实例
 * @param level 初始日志等级阈值
 * @param func  初始输出函数指针，若为 NULL 则使用默认 printf
 * @return      成功返回指针，失败返回 NULL
 */
LOG_UTILS_API logger_t* logger_create(log_level_t level, log_output_func_t func);

/**
 * @brief 销毁日志器实例
 * @param logger 要销毁的日志器指针
 */
LOG_UTILS_API void logger_destroy(logger_t *logger);

/* ========== 日志器配置 ========== */

/**
 * @brief 设置日志等级阈值
 * @param logger 日志器指针
 * @param level  新的日志等级阈值
 */
LOG_UTILS_API void logger_set_level(logger_t *logger, log_level_t level);

/**
 * @brief 获取当前日志等级阈值
 * @param logger 日志器指针
 * @return       当前日志等级阈值（若 logger 为 NULL 返回 LOG_LEVEL_I）
 */
LOG_UTILS_API log_level_t logger_get_level(const logger_t *logger);

/**
 * @brief 设置输出函数
 * @param logger 日志器指针
 * @param func   新的输出函数指针，若为 NULL 则恢复为默认 printf
 */
LOG_UTILS_API void logger_set_output(logger_t *logger, log_output_func_t func);

/* ========== 日志记录 ========== */

/**
 * @brief 核心日志记录函数（va_list 版本）
 * @param logger 日志器指针
 * @param level  本条日志的等级
 * @param format 格式化字符串
 * @param args   va_list 可变参数列表
 */
LOG_UTILS_API void logger_logv(logger_t *logger, log_level_t level, const char *format, va_list args);

/**
 * @brief 核心日志记录函数
 * @param logger 日志器指针
 * @param level  本条日志的等级
 * @param format 格式化字符串
 * @param ...    可变参数
 */
LOG_UTILS_API void logger_log(logger_t *logger, log_level_t level, const char *format, ...);

/* ========== 便捷宏 ========== */
#define LOG_E(logger, ...) logger_log(logger, LOG_LEVEL_E, __VA_ARGS__)
#define LOG_W(logger, ...) logger_log(logger, LOG_LEVEL_W, __VA_ARGS__)
#define LOG_I(logger, ...) logger_log(logger, LOG_LEVEL_I, __VA_ARGS__)
#define LOG_D(logger, ...) logger_log(logger, LOG_LEVEL_D, __VA_ARGS__)
#define LOG_T(logger, ...) logger_log(logger, LOG_LEVEL_T, __VA_ARGS__)

/* ========== Header‑only 模式下的函数实现 ========== */
#ifdef LOG_UTILS_HEAD_ONLY

LOG_UTILS_API logger_t* logger_create(log_level_t level, log_output_func_t func)
{
    logger_t *logger = (logger_t*)malloc(sizeof(logger_t));
    logger_t *result = NULL;

    if (logger != NULL)
    {
        logger->level = level;
        logger->output_func = (func != NULL) ? func : printf;
        result = logger;
    }

    return result;
}

LOG_UTILS_API void logger_destroy(logger_t *logger)
{
    if (logger != NULL)
    {
        free(logger);
    }
}

LOG_UTILS_API void logger_set_level(logger_t *logger, log_level_t level)
{
    if (logger != NULL)
    {
        logger->level = level;
    }
}

LOG_UTILS_API log_level_t logger_get_level(const logger_t *logger)
{
    log_level_t result = LOG_LEVEL_I;

    if (logger != NULL)
    {
        result = logger->level;
    }

    return result;
}

LOG_UTILS_API void logger_set_output(logger_t *logger, log_output_func_t func)
{
    if (logger != NULL)
    {
        logger->output_func = (func != NULL) ? func : printf;
    }
}

LOG_UTILS_API void logger_logv(logger_t *logger, log_level_t level, const char *format, va_list args)
{
    if (logger != NULL && level <= logger->level)
    {
        char level_char = '?';

        switch (level)
        {
            case LOG_LEVEL_E: level_char = 'E'; break;
            case LOG_LEVEL_W: level_char = 'W'; break;
            case LOG_LEVEL_I: level_char = 'I'; break;
            case LOG_LEVEL_D: level_char = 'D'; break;
            case LOG_LEVEL_T: level_char = 'T'; break;
        }

        char user_buffer[1024];
        int len = vsnprintf(user_buffer, sizeof(user_buffer), format, args);

        if (len < 0)
        {
            user_buffer[0] = '\0';
        }
        else
        {
            if (len > 0 && len < (int)sizeof(user_buffer) - 1 && user_buffer[len - 1] != '\n')
            {
                user_buffer[len] = '\n';
                user_buffer[len + 1] = '\0';
            }
        }

        logger->output_func("[%c] %s", level_char, user_buffer);
    }
}

LOG_UTILS_API void logger_log(logger_t *logger, log_level_t level, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    logger_logv(logger, level, format, args);
    va_end(args);
}

#endif /* LOG_UTILS_HEAD_ONLY */

#ifdef __cplusplus
}
#endif

#endif /* LOG_UTILS_H */