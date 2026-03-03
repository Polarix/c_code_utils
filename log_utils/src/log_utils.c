/**
 * @file log_utils.c
 * @brief 独立编译模式下的日志框架实现
 */

#include "log_utils.h"

#if !defined(LOG_UTILS_HEAD_ONLY)

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

#endif /* !LOG_UTILS_HEAD_ONLY */