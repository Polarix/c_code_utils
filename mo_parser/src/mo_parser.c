/**
 * @file mo_parser.c
 * @brief MO文件解析器实现 - 支持多种查找策略
 */

#include "mo_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* 平台相关的文件操作 */
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

/* 内部常量定义 */
#define MO_MAGIC 0x950412de
#define MO_MAGIC_REV 0xde120495
#define MO_MAX_STRING_LENGTH 4096
#define MO_CACHE_SIZE 64

/* 字符串表项结构（内存中） */
typedef struct {
    const char* original;
    const char* translation;
    size_t original_len;
    size_t translation_len;
} mo_string_pair_t;

/* 缓存项结构 */
typedef struct {
    const char* original;
    const char* translation;
    #ifdef MO_SEARCH_METHOD_HASH
    uint32_t hash;              /**< 哈希值缓存（仅哈希表模式） */
    #endif
} mo_cache_item_t;

/* 哈希表相关定义（仅哈希表模式） */
#ifdef MO_SEARCH_METHOD_HASH
#define MO_HASH_TABLE_LOAD_FACTOR 0.75f

/* 哈希表槽位状态 */
typedef enum {
    MO_HASH_SLOT_EMPTY = 0,      /**< 空槽位 */
    MO_HASH_SLOT_OCCUPIED = 1,   /**< 已占用槽位 */
    MO_HASH_SLOT_DELETED = 2     /**< 已删除槽位 */
} mo_hash_slot_state_t;

/* 哈希表槽位结构 */
typedef struct {
    const char* original;        /**< 原始字符串指针 */
    const char* translation;     /**< 翻译字符串指针 */
    uint32_t original_len;       /**< 原始字符串长度 */
    uint32_t translation_len;    /**< 翻译字符串长度 */
    uint32_t hash;               /**< 字符串哈希值 */
    mo_hash_slot_state_t state;  /**< 槽位状态 */
} mo_hash_slot_t;
#endif

/* MO文件上下文结构 */
struct mo_context {
    uint8_t* data;              /**< MO文件数据指针 */
    size_t size;                /**< 数据大小 */
    bool is_mapped;             /**< 是否为内存映射模式 */
    
    mo_header_t* header;        /**< 文件头部指针 */
    mo_string_entry_t* orig_table;  /**< 原始字符串表 */
    mo_string_entry_t* trans_table; /**< 翻译字符串表 */
    
    mo_string_pair_t* pairs;    /**< 字符串对数组（解析后） */
    uint32_t num_strings;       /**< 字符串数量 */
    
    /* 哈希表相关（仅哈希表模式） */
    #ifdef MO_SEARCH_METHOD_HASH
    mo_hash_slot_t* hash_table;   /**< 哈希表数组 */
    uint32_t hash_table_size;     /**< 哈希表大小（必须是2的幂次） */
    uint32_t hash_table_mask;     /**< 哈希表掩码（size-1） */
    uint32_t hash_table_count;    /**< 哈希表中已存储的项数 */
    #endif
    
    /* 缓存机制 */
    mo_cache_item_t cache[MO_CACHE_SIZE];
    uint32_t cache_index;
    
    bool logging_enabled;       /**< 日志开关 */
    
    /* 性能统计（可选） */
    #ifdef MO_ENABLE_STATS
    mo_stats_t stats;
    #endif
    const char* search_method;  /**< 查找方法名称 */
};

/* 查找策略方法名称 */
#ifdef MO_SEARCH_METHOD_LINEAR
static const char* s_search_method = "LINEAR";
#elif defined(MO_SEARCH_METHOD_BINARY)
static const char* s_search_method = "BINARY";
#elif defined(MO_SEARCH_METHOD_HASH)
static const char* s_search_method = "HASH";
#else
static const char* s_search_method = "UNKNOWN";
#endif

/* 内部函数声明 */
static uint32_t mo_swap_uint32(uint32_t val, bool swap);
static const char* mo_get_string(const mo_context_t* ctx, 
                                const mo_string_entry_t* table,
                                uint32_t index);
static void mo_log(const mo_context_t* ctx, const char* fmt, ...);

/* 查找函数声明 */
#ifdef MO_SEARCH_METHOD_LINEAR
static uint32_t mo_find_string_linear(const mo_context_t* ctx, 
                                     const char* str, size_t len);
#endif

#ifdef MO_SEARCH_METHOD_BINARY
static uint32_t mo_find_string_binary(const mo_context_t* ctx, 
                                     const char* str, size_t len);
static int mo_compare_pairs(const void* a, const void* b);
#endif

#ifdef MO_SEARCH_METHOD_HASH
static uint32_t mo_find_string_hash(const mo_context_t* ctx, 
                                   const char* str, size_t len,
                                   uint32_t hash);
static uint32_t mo_hash_string(const char* str, size_t len);
static uint32_t mo_next_power_of_two(uint32_t n);
static bool mo_build_hash_table(mo_context_t* ctx);
#endif

/* 全局变量 */
static bool g_logging_enabled = false;

/**
 * @brief 交换32位整数字节序
 */
static uint32_t mo_swap_uint32(uint32_t val, bool swap)
{
    if (!swap)
    {
        return val;
    }
    
    return ((val >> 24) & 0x000000FF) |
           ((val >> 8)  & 0x0000FF00) |
           ((val << 8)  & 0x00FF0000) |
           ((val << 24) & 0xFF000000);
}

/**
 * @brief 记录日志信息
 */
static void mo_log(const mo_context_t* ctx, const char* fmt, ...)
{
    if (!g_logging_enabled && (!ctx || !ctx->logging_enabled))
    {
        return;
    }
    
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[MO] ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

/**
 * @brief 获取错误描述字符串
 */
const char* mo_error_string(mo_error_t error)
{
    switch (error)
    {
        case MO_SUCCESS:
            return "Success";
        case MO_ERROR_FILE_NOT_FOUND:
            return "File not found";
        case MO_ERROR_INVALID_FORMAT:
            return "Invalid MO file format";
        case MO_ERROR_MEMORY:
            return "Memory allocation failed";
        case MO_ERROR_INVALID_CONTEXT:
            return "Invalid context handle";
        case MO_ERROR_IO:
            return "I/O error";
        case MO_ERROR_NOT_INITIALIZED:
            return "Parser not initialized";
        default:
            return "Unknown error";
    }
}

#ifdef MO_SEARCH_METHOD_LINEAR
/**
 * @brief 线性查找字符串索引
 */
static uint32_t mo_find_string_linear(const mo_context_t* ctx, 
                                     const char* str, size_t len)
{
    if (!ctx || !ctx->pairs || ctx->num_strings == 0)
    {
        return 0xFFFFFFFF;
    }
    
    /* 顺序遍历查找 */
    for (uint32_t i = 0; i < ctx->num_strings; i++)
    {
        const mo_string_pair_t* pair = &ctx->pairs[i];
        
        #ifdef MO_ENABLE_STATS
        ((mo_context_t*)ctx)->stats.comparisons++;
        #endif
        
        /* 先比较长度 */
        if (pair->original_len != len)
        {
            continue;
        }
        
        /* 再比较内容 */
        if (memcmp(pair->original, str, len) == 0)
        {
            return i;
        }
    }
    
    return 0xFFFFFFFF;
}
#endif

#ifdef MO_SEARCH_METHOD_BINARY
/**
 * @brief 比较字符串对（用于排序）
 */
static int mo_compare_pairs(const void* a, const void* b)
{
    const mo_string_pair_t* pair1 = (const mo_string_pair_t*)a;
    const mo_string_pair_t* pair2 = (const mo_string_pair_t*)b;
    
    /* 先比较长度 */
    if (pair1->original_len < pair2->original_len)
        return -1;
    else if (pair1->original_len > pair2->original_len)
        return 1;
    
    /* 长度相等时，比较内容 */
    return memcmp(pair1->original, pair2->original, pair1->original_len);
}

/**
 * @brief 二分查找字符串索引
 */
static uint32_t mo_find_string_binary(const mo_context_t* ctx, 
                                     const char* str, size_t len)
{
    uint32_t left = 0;
    uint32_t right = 0;
    
    if (!ctx || !ctx->pairs || ctx->num_strings == 0)
    {
        return 0xFFFFFFFF;
    }
    
    right = ctx->num_strings - 1;
    
    while (left <= right)
    {
        uint32_t mid = left + (right - left) / 2;
        const mo_string_pair_t* pair = &ctx->pairs[mid];
        
        #ifdef MO_ENABLE_STATS
        ((mo_context_t*)ctx)->stats.comparisons++;
        #endif
        
        /* 比较长度 */
        if (pair->original_len < len)
        {
            left = mid + 1;
        }
        else if (pair->original_len > len)
        {
            if (mid == 0) break;
            right = mid - 1;
        }
        else
        {
            /* 长度相等，比较内容 */
            int cmp = memcmp(pair->original, str, len);
            if (cmp == 0)
            {
                return mid;
            }
            else if (cmp < 0)
            {
                left = mid + 1;
            }
            else
            {
                if (mid == 0) break;
                right = mid - 1;
            }
        }
    }
    
    return 0xFFFFFFFF;
}
#endif

#ifdef MO_SEARCH_METHOD_HASH
/**
 * @brief 计算字符串的哈希值（djb2算法）
 */
static uint32_t mo_hash_string(const char* str, size_t len)
{
    uint32_t hash = 5381;
    for (size_t i = 0; i < len; i++)
    {
        hash = ((hash << 5) + hash) + (uint8_t)str[i]; /* hash * 33 + c */
    }
    return hash;
}

/**
 * @brief 获取大于等于n的最小2的幂次
 */
static uint32_t mo_next_power_of_two(uint32_t n)
{
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

/**
 * @brief 构建哈希表
 */
static bool mo_build_hash_table(mo_context_t* ctx)
{
    if (!ctx || !ctx->pairs || ctx->num_strings == 0)
    {
        return false;
    }
    
    /* 计算哈希表初始大小（2的幂次） */
    uint32_t target_size = (uint32_t)(ctx->num_strings / MO_HASH_TABLE_LOAD_FACTOR) + 1;
    ctx->hash_table_size = mo_next_power_of_two(target_size);
    ctx->hash_table_mask = ctx->hash_table_size - 1;
    ctx->hash_table_count = 0;
    
    /* 分配哈希表内存 */
    ctx->hash_table = (mo_hash_slot_t*)calloc(ctx->hash_table_size, sizeof(mo_hash_slot_t));
    if (!ctx->hash_table)
    {
        return false;
    }
    
    /* 初始化所有槽位为空 */
    for (uint32_t i = 0; i < ctx->hash_table_size; i++)
    {
        ctx->hash_table[i].state = MO_HASH_SLOT_EMPTY;
    }
    
    /* 插入所有字符串对到哈希表 */
    for (uint32_t i = 0; i < ctx->num_strings; i++)
    {
        const mo_string_pair_t* pair = &ctx->pairs[i];
        uint32_t hash = mo_hash_string(pair->original, pair->original_len);
        uint32_t index = hash & ctx->hash_table_mask;
        
        /* 开放寻址处理冲突 */
        while (ctx->hash_table[index].state == MO_HASH_SLOT_OCCUPIED)
        {
            index = (index + 1) & ctx->hash_table_mask;
        }
        
        /* 填充槽位 */
        ctx->hash_table[index].original = pair->original;
        ctx->hash_table[index].translation = pair->translation;
        ctx->hash_table[index].original_len = pair->original_len;
        ctx->hash_table[index].translation_len = pair->translation_len;
        ctx->hash_table[index].hash = hash;
        ctx->hash_table[index].state = MO_HASH_SLOT_OCCUPIED;
        ctx->hash_table_count++;
    }
    
    mo_log(ctx, "Hash table built: size=%u, items=%u, load=%.2f", 
           ctx->hash_table_size, ctx->hash_table_count,
           (float)ctx->hash_table_count / ctx->hash_table_size);
    
    return true;
}

/**
 * @brief 使用哈希表查找字符串索引
 */
static uint32_t mo_find_string_hash(const mo_context_t* ctx, 
                                   const char* str, size_t len,
                                   uint32_t hash)
{
    if (!ctx || !ctx->hash_table || ctx->hash_table_size == 0 || !str)
    {
        return 0xFFFFFFFF;
    }
    
    uint32_t index = hash & ctx->hash_table_mask;
    uint32_t start_index = index;
    
    /* 开放寻址查找 */
    while (1)
    {
        mo_hash_slot_t* slot = &ctx->hash_table[index];
        
        if (slot->state == MO_HASH_SLOT_EMPTY)
        {
            /* 找到空槽位，说明字符串不存在 */
            return 0xFFFFFFFF;
        }
        
        if (slot->state == MO_HASH_SLOT_OCCUPIED)
        {
            /* 检查哈希值是否匹配（快速过滤） */
            if (slot->hash == hash)
            {
                /* 哈希值匹配，进一步检查字符串内容 */
                if (slot->original_len == len && 
                    memcmp(slot->original, str, len) == 0)
                {
                    return index; /* 返回哈希表索引 */
                }
            }
            
            /* 记录哈希冲突 */
            #ifdef MO_ENABLE_STATS
            if (ctx->hash_table_count < ctx->hash_table_size)
            {
                ((mo_context_t*)ctx)->stats.hash_collisions++;
            }
            #endif
        }
        
        /* 线性探测下一个槽位 */
        index = (index + 1) & ctx->hash_table_mask;
        
        /* 如果回到起点，说明表已满且未找到 */
        if (index == start_index)
        {
            return 0xFFFFFFFF;
        }
    }
}
#endif

/**
 * @brief 从文件加载MO文件
 */
mo_error_t mo_context_create(const char* filename, mo_context_t** context)
{
    mo_error_t result = MO_SUCCESS;
    mo_context_t* ctx = NULL;
    FILE* file = NULL;
    uint8_t* data = NULL;
    size_t file_size = 0;
    
    /* 参数检查 */
    if (!filename || !context)
    {
        return MO_ERROR_INVALID_CONTEXT;
    }
    
    /* 分配上下文结构 */
    ctx = (mo_context_t*)calloc(1, sizeof(mo_context_t));
    if (!ctx)
    {
        result = MO_ERROR_MEMORY;
        goto cleanup;
    }
    
    /* 打开文件 */
    file = fopen(filename, "rb");
    if (!file)
    {
        result = MO_ERROR_FILE_NOT_FOUND;
        goto cleanup;
    }
    
    /* 获取文件大小 */
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size < sizeof(mo_header_t))
    {
        result = MO_ERROR_INVALID_FORMAT;
        goto cleanup;
    }
    
    /* 分配内存并读取文件 */
    data = (uint8_t*)malloc(file_size);
    if (!data)
    {
        result = MO_ERROR_MEMORY;
        goto cleanup;
    }
    
    if (fread(data, 1, file_size, file) != file_size)
    {
        result = MO_ERROR_IO;
        goto cleanup;
    }
    
    /* 初始化上下文 */
    ctx->data = data;
    ctx->size = file_size;
    ctx->is_mapped = false;
    
    /* 解析MO文件 */
    result = mo_context_create_from_memory(data, file_size, &ctx);
    if (result != MO_SUCCESS)
    {
        goto cleanup;
    }
    
    /* 转移数据所有权 */
    data = NULL;
    *context = ctx;
    ctx = NULL;
    
cleanup:
    if (file)
    {
        fclose(file);
    }
    
    if (data)
    {
        free(data);
    }
    
    if (ctx)
    {
        mo_context_free(ctx);
    }
    
    return result;
}

/**
 * @brief 从内存数据创建MO解析器
 */
mo_error_t mo_context_create_from_memory(const uint8_t* data, size_t size, 
                                        mo_context_t** context)
{
    mo_error_t result = MO_SUCCESS;
    mo_context_t* ctx = NULL;
    mo_header_t* header = NULL;
    bool need_swap = false;
    uint32_t i;
    
    /* 参数检查 */
    if (!data || size < sizeof(mo_header_t) || !context)
    {
        return MO_ERROR_INVALID_CONTEXT;
    }
    
    /* 分配上下文结构 */
    ctx = (mo_context_t*)calloc(1, sizeof(mo_context_t));
    if (!ctx)
    {
        result = MO_ERROR_MEMORY;
        goto cleanup;
    }
    
    #ifdef MO_ENABLE_STATS
    /* 初始化统计信息 */
    memset(&ctx->stats, 0, sizeof(mo_stats_t));
    #endif
    
    /* 复制数据 */
    ctx->data = (uint8_t*)malloc(size);
    if (!ctx->data)
    {
        result = MO_ERROR_MEMORY;
        goto cleanup;
    }
    
    memcpy(ctx->data, data, size);
    ctx->size = size;
    ctx->is_mapped = false;
    
    /* 设置头部指针 */
    header = (mo_header_t*)ctx->data;
    ctx->header = header;
    
    /* 检查魔数，确定字节序 */
    if (header->magic == MO_MAGIC)
    {
        need_swap = false;
    }
    else if (header->magic == MO_MAGIC_REV)
    {
        need_swap = true;
    }
    else
    {
        result = MO_ERROR_INVALID_FORMAT;
        goto cleanup;
    }
    
    /* 转换头部字段字节序 */
    header->revision = mo_swap_uint32(header->revision, need_swap);
    header->num_strings = mo_swap_uint32(header->num_strings, need_swap);
    header->orig_table_offset = mo_swap_uint32(header->orig_table_offset, need_swap);
    header->trans_table_offset = mo_swap_uint32(header->trans_table_offset, need_swap);
    header->hash_table_size = mo_swap_uint32(header->hash_table_size, need_swap);
    header->hash_table_offset = mo_swap_uint32(header->hash_table_offset, need_swap);
    
    /* 验证偏移量 */
    if (header->orig_table_offset + header->num_strings * sizeof(mo_string_entry_t) > size ||
        header->trans_table_offset + header->num_strings * sizeof(mo_string_entry_t) > size)
    {
        result = MO_ERROR_INVALID_FORMAT;
        goto cleanup;
    }
    
    /* 设置字符串表指针 */
    ctx->orig_table = (mo_string_entry_t*)(ctx->data + header->orig_table_offset);
    ctx->trans_table = (mo_string_entry_t*)(ctx->data + header->trans_table_offset);
    ctx->num_strings = header->num_strings;
    
    /* 分配字符串对数组 */
    ctx->pairs = (mo_string_pair_t*)calloc(header->num_strings, sizeof(mo_string_pair_t));
    if (!ctx->pairs)
    {
        result = MO_ERROR_MEMORY;
        goto cleanup;
    }
    
    /* 初始化字符串对 */
    for (i = 0; i < header->num_strings; i++)
    {
        uint32_t orig_len = mo_swap_uint32(ctx->orig_table[i].length, need_swap);
        uint32_t orig_offset = mo_swap_uint32(ctx->orig_table[i].offset, need_swap);
        uint32_t trans_len = mo_swap_uint32(ctx->trans_table[i].length, need_swap);
        uint32_t trans_offset = mo_swap_uint32(ctx->trans_table[i].offset, need_swap);
        
        /* 验证偏移量 */
        if (orig_offset + orig_len + 1 > size || trans_offset + trans_len + 1 > size)
        {
            result = MO_ERROR_INVALID_FORMAT;
            goto cleanup;
        }
        
        ctx->pairs[i].original = (const char*)(ctx->data + orig_offset);
        ctx->pairs[i].translation = (const char*)(ctx->data + trans_offset);
        ctx->pairs[i].original_len = orig_len;
        ctx->pairs[i].translation_len = trans_len;
    }
    
    /* 根据查找策略进行初始化 */
    #ifdef MO_SEARCH_METHOD_BINARY
    /* 二分查找：需要对数组进行排序 */
    qsort(ctx->pairs, header->num_strings, sizeof(mo_string_pair_t), mo_compare_pairs);
    mo_log(ctx, "Sorted %u string pairs for binary search", header->num_strings);
    #endif
    
    #ifdef MO_SEARCH_METHOD_HASH
    /* 哈希查找：构建哈希表 */
    if (!mo_build_hash_table(ctx))
    {
        result = MO_ERROR_MEMORY;
        goto cleanup;
    }
    #endif
    
    /* 初始化缓存 */
    ctx->cache_index = 0;
    ctx->logging_enabled = g_logging_enabled;
    
    ctx->search_method = s_search_method;
    mo_log(ctx, "MO context created successfully: %u strings, method=%s", 
           header->num_strings, ctx->search_method);

    *context = ctx;
    return MO_SUCCESS;
    
cleanup:
    if (ctx)
    {
        mo_context_free(ctx);
    }
    
    return result;
}

/**
 * @brief 释放MO解析上下文
 */
void mo_context_free(mo_context_t* context)
{
    if (!context)
    {
        return;
    }
    
    #ifdef MO_ENABLE_STATS
    mo_log(context, "Freeing MO context: total_lookups=%u, cache_hits=%u, cache_misses=%u", 
           context->stats.total_lookups, context->stats.cache_hits, context->stats.cache_misses);
    #ifdef MO_SEARCH_METHOD_HASH
    mo_log(context, "  hash_collisions=%u", context->stats.hash_collisions);
    #endif
    #if defined(MO_SEARCH_METHOD_LINEAR) || defined(MO_SEARCH_METHOD_BINARY)
    mo_log(context, "  comparisons=%u", context->stats.comparisons);
    #endif
    #else
    mo_log(context, "Freeing MO context");
    #endif
    
    if (context->data && !context->is_mapped)
    {
        free(context->data);
    }
    
    if (context->pairs)
    {
        free(context->pairs);
    }
    
    #ifdef MO_SEARCH_METHOD_HASH
    if (context->hash_table)
    {
        free(context->hash_table);
    }
    #endif
    
    free(context);
}

/**
 * @brief 获取指定索引的字符串
 */
static const char* mo_get_string(const mo_context_t* ctx, 
                                const mo_string_entry_t* table,
                                uint32_t index)
{
    if (!ctx || !table || index >= ctx->num_strings)
    {
        return NULL;
    }
    
    bool need_swap = (ctx->header->magic == MO_MAGIC_REV);
    uint32_t offset = mo_swap_uint32(table[index].offset, need_swap);
    
    if (offset >= ctx->size)
    {
        return NULL;
    }
    
    return (const char*)(ctx->data + offset);
}

/**
 * @brief 翻译字符串（带长度参数）
 */
const char* mo_translate_n(mo_context_t* context, 
                          const char* original, size_t original_len)
{
    uint32_t index = 0xFFFFFFFF;
    const char* result = original;
    
    /* 参数检查 */
    if (!context || !context->pairs || !original)
    {
        return original;
    }
    
    #ifdef MO_ENABLE_STATS
    context->stats.total_lookups++;
    #endif
    
    /* 检查缓存 */
    #ifdef MO_SEARCH_METHOD_HASH
    /* 哈希表模式：使用哈希值作为缓存键的一部分 */
    uint32_t hash = mo_hash_string(original, original_len);
    uint32_t cache_slot = hash & (MO_CACHE_SIZE - 1);
    if (context->cache[cache_slot].original == original &&
        context->cache[cache_slot].hash == hash)
    {
        #ifdef MO_ENABLE_STATS
        context->stats.cache_hits++;
        #endif
        return context->cache[cache_slot].translation ? 
               context->cache[cache_slot].translation : original;
    }
    #else
    /* 线性/二分模式：直接比较指针 */
    uint32_t cache_slot = (uintptr_t)original & (MO_CACHE_SIZE - 1);
    if (context->cache[cache_slot].original == original)
    {
        #ifdef MO_ENABLE_STATS
        context->stats.cache_hits++;
        #endif
        return context->cache[cache_slot].translation ? 
               context->cache[cache_slot].translation : original;
    }
    #endif
    
    #ifdef MO_ENABLE_STATS
    context->stats.cache_misses++;
    #endif
    
    /* 根据编译时选择的查找策略进行查找 */
    #if defined(MO_SEARCH_METHOD_LINEAR)
    index = mo_find_string_linear(context, original, original_len);
    #elif defined(MO_SEARCH_METHOD_BINARY)
    index = mo_find_string_binary(context, original, original_len);
    #elif defined(MO_SEARCH_METHOD_HASH)
    index = mo_find_string_hash(context, original, original_len, hash);
    #else
    #error "No search method defined! Use -DMO_SEARCH_METHOD_LINEAR, -DMO_SEARCH_METHOD_BINARY, or -DMO_SEARCH_METHOD_HASH"
    #endif
    
    if (index != 0xFFFFFFFF)
    {
        #if defined(MO_SEARCH_METHOD_HASH)
        /* 哈希表模式：索引指向哈希表槽位 */
        result = context->hash_table[index].translation;
        #else
        /* 线性/二分模式：索引指向pairs数组 */
        result = context->pairs[index].translation;
        #endif
        
        /* 更新缓存 */
        context->cache[cache_slot].original = original;
        context->cache[cache_slot].translation = result;
        #ifdef MO_SEARCH_METHOD_HASH
        context->cache[cache_slot].hash = hash;
        #endif
    }
    
    return result;
}

/**
 * @brief 翻译字符串
 */
const char* mo_translate(mo_context_t* context, const char* original)
{
    return mo_translate_n(context, original, original ? strlen(original) : 0);
}

/**
 * @brief 翻译字符串（带上下文和复数形式）
 */
const char* mo_translate_cp(mo_context_t* context, 
                           const char* context_str,
                           const char* singular, 
                           const char* plural,
                           unsigned long n)
{
    char key[MO_MAX_STRING_LENGTH];
    const char* result = singular;
    size_t key_len;
    
    if (!context || !singular)
    {
        return singular;
    }
    
    /* 构建带上下文的键 */
    if (context_str)
    {
        key_len = snprintf(key, sizeof(key), "%s\004%s", context_str, singular);
    }
    else
    {
        key_len = snprintf(key, sizeof(key), "%s", singular);
    }
    
    if (key_len >= sizeof(key))
    {
        return singular;
    }
    
    /* 查找翻译 */
    result = mo_translate_n(context, key, key_len);
    
    /* 如果找不到带上下文的，尝试查找不带上下文的 */
    if (result == key && context_str)
    {
        result = mo_translate(context, singular);
    }
    
    /* 处理复数形式 */
    if (plural && n != 1)
    {
        /* 查找复数形式翻译 */
        const char* plural_translation = NULL;
        
        if (context_str)
        {
            key_len = snprintf(key, sizeof(key), "%s\004%s", context_str, plural);
            if (key_len < sizeof(key))
            {
                plural_translation = mo_translate_n(context, key, key_len);
            }
        }
        
        if (!plural_translation || plural_translation == key)
        {
            plural_translation = mo_translate(context, plural);
        }
        
        result = plural_translation;
    }
    
    return result;
}

/**
 * @brief 获取字符串数量
 */
uint32_t mo_get_string_count(const mo_context_t* context)
{
    return context ? context->num_strings : 0;
}

/**
 * @brief 获取性能统计信息
 */
bool mo_get_stats(const mo_context_t* context, mo_stats_t* stats)
{
    #ifdef MO_ENABLE_STATS
    if (!context || !stats)
    {
        return false;
    }
    
    *stats = context->stats;
    return true;
    #else
    (void)context;
    (void)stats;
    return false;
    #endif
}

/**
 * @brief 启用/禁用日志
 */
void mo_enable_logging(bool enable)
{
    g_logging_enabled = enable;
}

/**
 * @brief 获取当前使用的查找方法
 */
const char* mo_get_search_method(const mo_context_t* context)
{
    return context ? context->search_method : "INVALID_CONTEXT";
}
