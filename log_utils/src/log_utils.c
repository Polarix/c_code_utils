/**
 * @file log_utils.c
 * @brief 独立编译模式下的库实现（仅需定义宏并包含头文件）
 */

/* 表示正在构建库（用于符号导出） */
#define LOG_UTILS_BUILD

/* 触发头文件中的函数定义 */
#define LOG_UTILS_IMPLEMENTATION

#include "log_utils.h"
