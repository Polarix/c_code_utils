/**
 * @file mo_parser.h
 * @brief GNU gettext MO文件解析引擎
 * @copyright MIT License
 * 
 * 这是一个轻量级的MO文件解析器，用于在资源受限的嵌入式系统中
 * 高效处理多语言翻译文件。支持标准MO文件格式，内存占用小，检索速度快。
 */

#ifndef MO_PARSER_H
#define MO_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief MO文件解析器上下文句柄 */
typedef struct mo_context mo_context_t;

/**
 * @brief 错误代码定义
 */
typedef enum {
    MO_SUCCESS = 0,          /**< 操作成功 */
    MO_ERROR_FILE_NOT_FOUND, /**< 文件未找到 */
    MO_ERROR_INVALID_FORMAT, /**< 无效的MO文件格式 */
    MO_ERROR_MEMORY,         /**< 内存分配失败 */
    MO_ERROR_INVALID_CONTEXT,/**< 无效的上下文句柄 */
    MO_ERROR_IO,             /**< 文件IO错误 */
    MO_ERROR_NOT_INITIALIZED /**< 解析器未初始化 */
} mo_error_t;

/**
 * @brief MO文件头部结构
 * @note 严格遵循MO文件格式规范，使用packed属性确保内存布局
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;             /**< 魔数标识：0x950412de (LE) 或 0xde120495 (BE) */
    uint32_t revision;          /**< 格式版本：0表示标准格式 */
    uint32_t num_strings;       /**< 字符串对数量 */
    uint32_t orig_table_offset; /**< 原始字符串表偏移 */
    uint32_t trans_table_offset;/**< 翻译字符串表偏移 */
    uint32_t hash_table_size;   /**< 哈希表大小 */
    uint32_t hash_table_offset; /**< 哈希表偏移 */
} mo_header_t;

/**
 * @brief 字符串条目结构（在文件中的表示）
 */
typedef struct __attribute__((packed)) {
    uint32_t length;    /**< 字符串长度（字节数） */
    uint32_t offset;    /**< 字符串在文件中的偏移 */
} mo_string_entry_t;

/**
 * @brief 性能统计结构（可选）
 */
typedef struct {
    uint32_t total_lookups;      /**< 总查找次数 */
    uint32_t cache_hits;         /**< 缓存命中次数 */
    uint32_t cache_misses;       /**< 缓存未命中次数 */
    uint32_t hash_collisions;    /**< 哈希冲突次数（仅哈希表模式） */
    uint32_t comparisons;        /**< 比较次数（仅线性和二分模式） */
} mo_stats_t;

/**
 * @brief 从文件加载MO文件并创建解析上下文
 * 
 * @param[in] filename MO文件路径
 * @param[out] context 输出的上下文句柄指针
 * @return mo_error_t 错误代码
 * 
 * @note 此函数会将整个MO文件映射到内存中，以提高访问速度。
 *       使用完成后必须调用mo_context_free释放资源。
 */
mo_error_t mo_context_create(const char* filename, mo_context_t** context);

/**
 * @brief 从内存数据创建MO解析上下文
 * 
 * @param[in] data MO文件数据指针
 * @param[in] size 数据大小（字节）
 * @param[out] context 输出的上下文句柄指针
 * @return mo_error_t 错误代码
 * 
 * @note 此函数会复制数据到内部缓冲区，调用者可以释放原始数据。
 */
mo_error_t mo_context_create_from_memory(const uint8_t* data, size_t size, 
                                        mo_context_t** context);

/**
 * @brief 释放MO解析上下文
 * 
 * @param[in] context MO上下文句柄
 */
void mo_context_free(mo_context_t* context);

/**
 * @brief 获取翻译字符串
 * 
 * @param[in] context MO上下文句柄
 * @param[in] original 原始字符串（需要翻译的字符串）
 * @return const char* 翻译后的字符串，未找到时返回原始字符串
 * 
 * @note 根据编译时选择的查找策略（LINEAR/BINARY/HASH）使用不同的查找算法。
 */
const char* mo_translate(mo_context_t* context, const char* original);

/**
 * @brief 获取翻译字符串（带长度参数）
 * 
 * @param[in] context MO上下文句柄
 * @param[in] original 原始字符串
 * @param[in] original_len 原始字符串长度
 * @return const char* 翻译后的字符串
 */
const char* mo_translate_n(mo_context_t* context, 
                          const char* original, size_t original_len);

/**
 * @brief 获取翻译字符串（带上下文和复数形式）
 * 
 * @param[in] context MO上下文句柄
 * @param[in] context_str 上下文字符串（可为NULL）
 * @param[in] singular 单数形式字符串
 * @param[in] plural 复数形式字符串（可为NULL）
 * @param[in] n 数量值
 * @return const char* 翻译后的字符串
 * 
 * @note 上下文格式为"context\004original"，复数形式使用gettext标准规则。
 */
const char* mo_translate_cp(mo_context_t* context, 
                           const char* context_str,
                           const char* singular, 
                           const char* plural,
                           unsigned long n);

/**
 * @brief 获取MO文件中的字符串数量
 * 
 * @param[in] context MO上下文句柄
 * @return uint32_t 字符串对数量
 */
uint32_t mo_get_string_count(const mo_context_t* context);

/**
 * @brief 获取错误描述信息
 * 
 * @param[in] error 错误代码
 * @return const char* 错误描述字符串
 */
const char* mo_error_string(mo_error_t error);

/**
 * @brief 启用/禁用详细日志输出
 * 
 * @param[in] enable true启用日志，false禁用
 */
void mo_enable_logging(bool enable);

/**
 * @brief 获取性能统计信息（仅当启用MO_ENABLE_STATS时可用）
 * 
 * @param[in] context MO上下文句柄
 * @param[out] stats 统计信息结构体指针
 * @return bool 成功返回true，失败返回false
 */
bool mo_get_stats(const mo_context_t* context, mo_stats_t* stats);

/**
 * @brief 获取当前使用的查找方法
 * 
 * @param[in] context MO上下文句柄
 * @return const char* 查找方法字符串
 */
const char* mo_get_search_method(const mo_context_t* context);

#ifdef __cplusplus
}
#endif

#endif /* MO_PARSER_H */
