/**
 * @file log_utils_demo.c
 * @brief 完整演示代码：测试不同输出回调、等级过滤、大量日志
 */

#include <log_utils.h>
#include <stdio.h>
#include <time.h>

/**
 * @brief 普通输出函数（直接打印）
 */
int normal_output(const char *format, ...)
{
    int ret;
    va_list args;
    va_start(args, format);
    ret = vprintf(format, args);
    va_end(args);
    return ret;
}

/**
 * @brief 带时间戳的输出函数
 */
int timestamp_output(const char *format, ...)
{
    int ret;
    va_list args;
    va_start(args, format);
    /* 模拟时间戳，实际可用 time() + localtime() */
    printf("[2025-03-04 10:00:00] ");
    ret = vprintf(format, args);
    va_end(args);
    return ret;
}

/**
 * @brief 空输出函数（什么也不做）
 */
int null_output(const char *format, ...)
{
    (void)format;   /* 避免未使用参数警告 */
    return 0;
}

int main(void)
{
    /* 1. 测试不同输出回调 */
    printf("=== Test different output callbacks ===\n");
    logger_t *logger_normal   = logger_create(LOG_LEVEL_I, normal_output);
    logger_t *logger_timestamp = logger_create(LOG_LEVEL_I, timestamp_output);
    logger_t *logger_null     = logger_create(LOG_LEVEL_I, null_output);

    LOG_I(logger_normal,   "This is normal output");
    LOG_I(logger_timestamp, "This is timestamp output");
    LOG_I(logger_null,     "This should NOT appear (null output)");

    /* 2. 测试同一批日志在不同等级下的输出 */
    printf("\n=== Test log level filtering ===\n");
    logger_t *logger_level_test = logger_create(LOG_LEVEL_W, normal_output);
    LOG_E(logger_level_test, "Error message (should appear)");
    LOG_W(logger_level_test, "Warning message (should appear)");
    LOG_I(logger_level_test, "Info message (should NOT appear)");
    LOG_D(logger_level_test, "Debug message (should NOT appear)");
    LOG_T(logger_level_test, "Trace message (should NOT appear)");

    printf("--- After setting level to DEBUG ---\n");
    logger_set_level(logger_level_test, LOG_LEVEL_D);
    LOG_E(logger_level_test, "Error message (appears)");
    LOG_W(logger_level_test, "Warning message (appears)");
    LOG_I(logger_level_test, "Info message (appears)");
    LOG_D(logger_level_test, "Debug message (appears)");
    LOG_T(logger_level_test, "Trace message (should NOT appear, DEBUG < TRACE)");

    /* 3. 大量日志输出（使用空输出函数，避免刷屏） */
    printf("\n=== Massive log output (10000 logs to null output) ===\n");
    clock_t start = clock();
    for (int i = 0; i < 10000; i++)
    {
        LOG_I(logger_null, "This is log message number %d", i);
    }
    clock_t end = clock();
    printf("Finished 10000 logs to null output in %.3f seconds.\n",
           (double)(end - start) / CLOCKS_PER_SEC);

    /* 清理 */
    logger_destroy(logger_normal);
    logger_destroy(logger_timestamp);
    logger_destroy(logger_null);
    logger_destroy(logger_level_test);

    return 0;
}
