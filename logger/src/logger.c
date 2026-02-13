/**
 * @file logger.c
 * @brief 多实例日志器实现
 */

#include "logger.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

logger_t* logger_create(log_level_t level, log_output_func_t func)
{
    logger_t *logger = (logger_t*)malloc(sizeof(logger_t));
    if (logger == NULL)
    {
        return NULL;  /* 唯一出口1：内存分配失败 */
    }

    logger->level = level;
    logger->output_func = (func != NULL) ? func : printf;

    return logger;  /* 唯一出口2：成功 */
}

void logger_destroy(logger_t *logger)
{
    if (logger != NULL)
    {
        free(logger);
    }
    return;  /* 唯一出口 */
}

void logger_set_level(logger_t *logger, log_level_t level)
{
    if (logger != NULL)
    {
        logger->level = level;
    }
    return;  /* 唯一出口 */
}

log_level_t logger_get_level(const logger_t *logger)
{
    if (logger == NULL)
    {
        return LOG_LEVEL_I;  /* 唯一出口1：空指针返回默认 */
    }
    return logger->level;    /* 唯一出口2：正常返回 */
}

void logger_set_output(logger_t *logger, log_output_func_t func)
{
    if (logger != NULL)
    {
        logger->output_func = (func != NULL) ? func : printf;
    }
    return;  /* 唯一出口 */
}

void logger_log(logger_t *logger, log_level_t level, const char *format, ...)
{
    if (logger == NULL || level > logger->level)
    {
        return;  /* 唯一出口1：无效 logger 或等级不足 */
    }

    char level_char = '?';
    switch (level)
    {
        case LOG_LEVEL_E: level_char = 'E'; break;
        case LOG_LEVEL_W: level_char = 'W'; break;
        case LOG_LEVEL_I: level_char = 'I'; break;
        case LOG_LEVEL_D: level_char = 'D'; break;
        case LOG_LEVEL_T: level_char = 'T'; break;
        /* default: 保持 '?' */
        default: level_char = '?';
    }

    va_list args;
    va_start(args, format);
    char user_buffer[1024];
    int len = vsnprintf(user_buffer, sizeof(user_buffer), format , args);
    va_end(args);

    /* 自动添加换行符（如果末尾不是换行且缓冲区足够） */
    if (len > 0 && len < (int)sizeof(user_buffer) - 1 && user_buffer[len - 1] != '\n')
    {
        user_buffer[len] = '\n';
        user_buffer[len + 1] = '\0';
    }

    logger->output_func("[%c] %s", level_char, user_buffer);

    return;  /* 唯一出口2：正常输出 */
}
