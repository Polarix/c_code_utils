/**
 * @file unicode_utils.h
 * @brief UTF-8/UTF-16/Unicode编码转换库
 * @details 该库提供了UTF-8、UTF-16和Unicode码点之间的相互转换功能，
 *          支持UTF-16的小端(LE)和大端(BE)字节序。
 *          遵循C11标准，不依赖外部库。
 * 
 * @author 自动生成
 * @date 2023
 * @version 1.0
 */

#ifndef UNICODE_UTILS_H
#define UNICODE_UTILS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 字节序类型枚举
 */
typedef enum _e_u16_byte_order
{
    UTF16_LE,   /**< UTF-16小端字节序 (Little Endian) */
    UTF16_BE,   /**< UTF-16大端字节序 (Big Endian) */
    UTF16_NATIVE /**< 系统原生字节序 */
} utf16_byte_order_t;

/**
 * @brief 转换结果状态码
 */
typedef enum _e_conv_result
{
    CONV_SUCCESS = 0,               /**< 转换成功 */
    CONV_ERROR_INVALID_PARAM = -1,  /**< 错误或者无效的参数 */
    CONV_ERROR_INVALID_DATA = -2,   /**< 无效的数据 */
    CONV_ERROR_OUT_OF_BUFFER = -3, /**< 缓冲区太小 */
} conv_result_t;

/**
 * @brief 获取系统原生字节序
 * 
 * @return Utf16ByteOrder 系统原生字节序
 */
utf16_byte_order_t get_native_byte_order(void);

/**
 * @brief 检查UTF-8序列的有效性
 * 
 * @param utf8_str UTF-8字符串
 * @param length 字符串长度（字节数），如果为0则自动计算
 * @return true UTF-8序列有效
 * @return false UTF-8序列无效
 */
bool is_valid_utf8(const uint8_t* utf8_str, size_t length);

/**
 * @brief 检查UTF-16序列的有效性
 * 
 * @param utf16_str UTF-16字符串
 * @param length 字符串长度（代码单元数），如果为0则自动计算
 * @param byte_order 字节序
 * @return true UTF-16序列有效
 * @return false UTF-16序列无效
 */
bool is_valid_utf16(const uint16_t* utf16_str, size_t length, utf16_byte_order_t byte_order);

/**
 * @brief 计算UTF-8字符串的字节长度
 * 
 * @param utf8_str UTF-8字符串
 * @return size_t UTF-8字符串的字节长度（不包含结尾的null字符）
 */
size_t utf8_strlen(const uint8_t* utf8_str);

/**
 * @brief 计算UTF-16字符串的代码单元数量
 * 
 * @param utf16_str UTF-16字符串
 * @param byte_order 字节序
 * @return size_t UTF-16字符串的代码单元数量（不包含结尾的null字符）
 */
size_t utf16_strlen(const uint16_t* utf16_str, utf16_byte_order_t byte_order);

/**
 * @brief 计算存储UTF-8字符串所需的最大字节数
 * 
 * @param utf16_str UTF-16字符串
 * @param length UTF-16代码单元数量（如果为0则自动计算）
 * @param byte_order 字节序
 * @return size_t 所需的最大字节数（包含结尾的null字符）
 */
size_t utf8_from_utf16_max_size(const uint16_t* utf16_str, size_t length, utf16_byte_order_t byte_order);

/**
 * @brief 计算存储UTF-16字符串所需的最大代码单元数
 * 
 * @param utf8_str UTF-8字符串
 * @param length UTF-8字节数（如果为0则自动计算）
 * @return size_t 所需的最大代码单元数（包含结尾的null字符）
 */
size_t utf16_from_utf8_max_size(const uint8_t* utf8_str, size_t length);

/**
 * @brief 将UTF-8转换为UTF-16
 * 
 * @param utf8_str 输入UTF-8字符串
 * @param utf16_buffer 输出UTF-16缓冲区
 * @param buffer_size 缓冲区大小（代码单元数）
 * @param byte_order 输出的UTF-16字节序
 * @param out_length 实际输出的代码单元数（可选，可为NULL）
 * @return conv_result_t 转换结果状态码
 */
conv_result_t utf8_to_utf16(const uint8_t* utf8_str, uint16_t* utf16_buffer, 
                        size_t buffer_size, utf16_byte_order_t byte_order, 
                        size_t* out_length);

/**
 * @brief 将UTF-16转换为UTF-8
 * 
 * @param utf16_str 输入UTF-16字符串
 * @param utf8_buffer 输出UTF-8缓冲区
 * @param buffer_size 缓冲区大小（字节数）
 * @param byte_order 输入的UTF-16字节序
 * @param out_length 实际输出的字节数（可选，可为NULL）
 * @return conv_result_t 转换结果状态码
 */
conv_result_t utf16_to_utf8(const uint16_t* utf16_str, uint8_t* utf8_buffer, 
                        size_t buffer_size, utf16_byte_order_t byte_order, 
                        size_t* out_length);

/**
 * @brief 将UTF-16从一种字节序转换为另一种字节序
 * 
 * @param src 源UTF-16字符串
 * @param dst 目标UTF-16字符串（可与src相同）
 * @param length 代码单元数量（如果为0则自动计算）
 * @param src_order 源字节序
 * @param dst_order 目标字节序
 * @return conv_result_t 转换结果状态码
 */
conv_result_t utf16_change_byte_order(const uint16_t* src, uint16_t* dst, 
                                  size_t length, utf16_byte_order_t src_order, 
                                  utf16_byte_order_t dst_order);

/**
 * @brief 将Unicode码点转换为UTF-8
 * 
 * @param codepoint Unicode码点
 * @param utf8_buffer 输出UTF-8缓冲区（至少5字节）
 * @param out_length 实际输出的字节数（可选，可为NULL）
 * @return conv_result_t 转换结果状态码
 */
conv_result_t codepoint_to_utf8(uint32_t codepoint, uint8_t* utf8_buffer, size_t* out_length);

/**
 * @brief 将UTF-8转换为Unicode码点
 * 
 * @param utf8_str UTF-8序列
 * @param codepoint 输出的Unicode码点
 * @param out_length 实际读取的字节数（可选，可为NULL）
 * @return conv_result_t 转换结果状态码
 */
conv_result_t utf8_to_codepoint(const uint8_t* utf8_str, uint32_t* codepoint, size_t* out_length);

/**
 * @brief 将Unicode码点转换为UTF-16
 * 
 * @param codepoint Unicode码点
 * @param utf16_buffer 输出UTF-16缓冲区（至少2个代码单元）
 * @param byte_order 输出的UTF-16字节序
 * @param out_length 实际输出的代码单元数（可选，可为NULL）
 * @return conv_result_t 转换结果状态码
 */
conv_result_t codepoint_to_utf16(uint32_t codepoint, uint16_t* utf16_buffer, 
                             utf16_byte_order_t byte_order, size_t* out_length);

/**
 * @brief 将UTF-16转换为Unicode码点
 * 
 * @param utf16_str UTF-16序列
 * @param codepoint 输出的Unicode码点
 * @param byte_order 输入的UTF-16字节序
 * @param out_length 实际读取的代码单元数（可选，可为NULL）
 * @return conv_result_t 转换结果状态码
 */
conv_result_t utf16_to_codepoint(const uint16_t* utf16_str, uint32_t* codepoint, 
                             utf16_byte_order_t byte_order, size_t* out_length);

#ifdef __cplusplus
}
#endif

#endif /* UNICODE_UTILS_H */