/**
 * @file search_comparison.c
 * @brief 不同查找策略性能比较测试
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mo_parser.h"

/* 生成随机字符串用于测试 */
static char* generate_random_string(int len)
{
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char* str = malloc(len + 1);
    
    for (int i = 0; i < len; i++)
    {
        int key = rand() % (int)(sizeof(charset) - 1);
        str[i] = charset[key];
    }
    str[len] = '\0';
    
    return str;
}

/* 从MO文件中提取所有原始字符串用于测试 */
static char** get_all_originals(const mo_context_t* ctx, uint32_t* count)
{
    *count = mo_get_string_count(ctx);
    char** strings = malloc(*count * sizeof(char*));
    
    /* 注意：这里需要访问内部结构，实际实现可能需要调整 */
    for (uint32_t i = 0; i < *count; i++)
    {
        /* 需要根据实际数据结构获取字符串 */
        /* 这是一个示例实现，可能需要修改 */
        strings[i] = strdup("test string"); /* 替换为实际字符串 */
    }
    
    return strings;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <mo_file>\n", argv[0]);
        return 1;
    }
    
    mo_context_t* ctx = NULL;
    mo_error_t err = mo_context_create(argv[1], &ctx);
    
    if (err != MO_SUCCESS)
    {
        fprintf(stderr, "Failed to load MO file: %s\n", mo_error_string(err));
        return 1;
    }
    
    printf("Performance Test for MO Parser\n");
    printf("=============================\n");
    printf("MO file: %s\n", argv[1]);
    printf("String count: %u\n", mo_get_string_count(ctx));
    printf("Search method: %s\n", mo_get_search_method(ctx));
    
    /* 性能测试 */
    const int num_tests = 10000;
    clock_t start, end;
    double total_time = 0;
    
    /* 测试已知字符串查找 */
    const char* test_strings[] = {
        "Close",
        "Frequency",
        "Duty-cycle",
        "Title",
        "New screen",
        "Button",
        "Help",
        "Save",
        "Open",
        "Exit",
        "Frequency1",
        "1Frequency",
        "1Frequency1",
        "Welcome",
        "Error",
        "Success",
        "Loading...",
        "Please wait",
        "Cancel",
        "OK",
        "Yes",
        "No",
        "This is a test string for MO parser",
        "Translation test",
        "Multi-language support",
        "Resource limited system"
    };
    
    printf("\nTesting known strings...\n");
    start = clock();
    
    for (int i = 0; i < num_tests; i++)
    {
        for (int j = 0; j < sizeof(test_strings)/sizeof(test_strings[0]); j++)
        {
            const char* translated = mo_translate(ctx, test_strings[j]);
            (void)translated; /* 避免编译器警告 */
        }
    }
    
    end = clock();
    total_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Time for %d lookups: %.3f seconds\n", 
           num_tests * sizeof(test_strings)/sizeof(test_strings[0]), total_time);
    
    /* 测试随机字符串查找（模拟未命中情况） */
    printf("\nTesting random strings (mostly misses)...\n");
    srand(time(NULL));
    start = clock();
    
    for (int i = 0; i < num_tests; i++)
    {
        char* random_str = generate_random_string(10 + rand() % 20);
        const char* translated = mo_translate(ctx, random_str);
        (void)translated; /* 避免编译器警告 */
        free(random_str);
    }
    
    end = clock();
    total_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Time for %d random lookups: %.3f seconds\n", num_tests, total_time);
    
    #ifdef MO_ENABLE_STATS
    /* 输出统计信息 */
    mo_stats_t stats;
    if (mo_get_stats(ctx, &stats))
    {
        printf("\nPerformance Statistics:\n");
        printf("  Total lookups: %u\n", stats.total_lookups);
        printf("  Cache hits: %u (%.1f%%)\n", 
               stats.cache_hits,
               stats.total_lookups > 0 ? 
               (float)stats.cache_hits / stats.total_lookups * 100.0f : 0.0f);
        printf("  Cache misses: %u\n", stats.cache_misses);
        #ifdef MO_SEARCH_METHOD_HASH
        printf("  Hash collisions: %u\n", stats.hash_collisions);
        #endif
        #if defined(MO_SEARCH_METHOD_LINEAR) || defined(MO_SEARCH_METHOD_BINARY)
        printf("  Comparisons: %u (avg: %.1f per lookup)\n", 
               stats.comparisons,
               stats.total_lookups > 0 ? 
               (float)stats.comparisons / stats.total_lookups : 0.0f);
        #endif
    }
    #endif
    
    mo_context_free(ctx);
    return 0;
}
