/**
 * @file log_utils.c
 * @brief 独立库模式下的函数实现
 */

/* 独立库模式下，库在构建时会定义LOG_UTILS_BUILD宏 */
// #define LOG_UTILS_BUILD

/* 触发头文件中的函数定义 */
#define LOG_UTILS_IMPLEMENTATION

#include "log_utils.h"
