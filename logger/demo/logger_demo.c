/**
 * @file logutils_demo.c
 * @brief 演示多实例日志器的独立性
 */

#include <logger.h>
#include <stdio.h>

/**
 * @brief 自定义输出函数（带时间戳示例）
 */
int timestamp_output(const char *format, ...)
{
    int ret;
    va_list args;
    va_start(args, format);
    /* 简单模拟时间戳，实际可用 time() */
    printf("[2025-01-01 12:00:00] ");
    ret = vprintf(format, args);
    va_end(args);
    return ret;
}

int main(void)
{
    /* 创建两个独立的日志器实例，以网络和数据库为例 */
    logger_t *net_logger = logger_create(LOG_LEVEL_I, NULL); /* 网络模块，默认 INFO */
    logger_t *db_logger  = logger_create(LOG_LEVEL_W, timestamp_output); /* 数据库模块，默认 WARNING，带时间戳 */

    if (net_logger == NULL || db_logger == NULL)
    {
        fprintf(stderr, "Failed to create loggers.\n");
        return 1;
    }

    /* 网络模块输出测试 */
    LOG_E(net_logger, "Network error: connection lost\n");
    LOG_W(net_logger, "Network warning: high latency\n");
    LOG_I(net_logger, "Network info: connected\n");
    LOG_D(net_logger, "Network debug: packet sent\n");   /* 等级 INFO 时不显示 */

    /* 数据库模块输出测试 */
    LOG_E(db_logger, "Database error: query failed\n");
    LOG_W(db_logger, "Database warning: slow query\n");
    LOG_I(db_logger, "Database info: connected\n");      /* 等级 WARNING 时不显示 */
    LOG_D(db_logger, "Database debug: rows=10\n");       /* 不显示 */

    printf("\n--- Change network logger to DEBUG ---\n\n");
    logger_set_level(net_logger, LOG_LEVEL_D);
    LOG_D(net_logger, "Network debug: now appears\n");

    printf("\n--- Change database logger to INFO ---\n\n");
    logger_set_level(db_logger, LOG_LEVEL_I);
    LOG_I(db_logger, "Database info: now appears\n");

    /* 清理 */
    logger_destroy(net_logger);
    logger_destroy(db_logger);

    return 0;
}
