/**
 * @file test_unicode_utils.c
 * @brief Unicode编码转换库测试程序
 * @details 测试UTF-8/UTF-16/Unicode之间的各种转换功能
 * 
 * @author 自动生成
 * @date 2023
 * @version 1.0
 */

#include "unicode_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief 测试系统字节序检测
 */
static void test_byte_order(void)
{
    printf("Testing byte order detection...\n");
    utf16_byte_order_t order = get_native_byte_order();
    
    if (order == UTF16_LE)
    {
        printf("  System byte order: Little Endian\n");
    }
    else if (order == UTF16_BE)
    {
        printf("  System byte order: Big Endian\n");
    }
    else
    {
        printf("  System byte order: Unknown\n");
    }
    printf("\n");
}

/**
 * @brief 测试UTF-8有效性检查
 */
static void test_utf8_validation(void)
{
    printf("Testing UTF-8 validation...\n");
    
    /* 有效的UTF-8字符串 */
    const uint8_t* valid_utf8 = (const uint8_t*)"Hello, 世界! 😊";
    bool is_valid = is_valid_utf8(valid_utf8, 0);
    printf("  Valid UTF-8 string: %s\n", is_valid ? "PASS" : "FAIL");
    
    /* 无效的UTF-8字符串 */
    const uint8_t invalid_utf8[] = {0xC0, 0x80, 0}; /* 过长的编码 */
    is_valid = is_valid_utf8(invalid_utf8, sizeof(invalid_utf8) - 1);
    printf("  Invalid UTF-8 string: %s\n", !is_valid ? "PASS" : "FAIL");
    
    /* 边界情况测试 */
    const uint8_t boundary_utf8[] = {0x7F, 0}; /* 单字节最大 */
    is_valid = is_valid_utf8(boundary_utf8, sizeof(boundary_utf8) - 1);
    printf("  Boundary UTF-8 (U+007F): %s\n", is_valid ? "PASS" : "FAIL");
    
    printf("\n");
}

/**
 * @brief 测试UTF-16有效性检查
 */
static void test_utf16_validation(void)
{
    printf("Testing UTF-16 validation...\n");
    
    /* 有效的UTF-16字符串 (LE) */
    const uint16_t valid_utf16_le[] = {0x0048, 0x0065, 0x006C, 0x006C, 0x006F, 0x0020, 0x4E16, 0x754C, 0x0021, 0};
    bool is_valid = is_valid_utf16(valid_utf16_le, 0, UTF16_LE);
    printf("  Valid UTF-16 LE string: %s\n", is_valid ? "PASS" : "FAIL");
    
    /* 有效的UTF-16字符串 (BE) */
    const uint16_t valid_utf16_be[] = {0x4800, 0x6500, 0x6C00, 0x6C00, 0x6F00, 0x2000, 0x164E, 0x4C75, 0x2100, 0};
    is_valid = is_valid_utf16(valid_utf16_be, 0, UTF16_BE);
    printf("  Valid UTF-16 BE string: %s\n", is_valid ? "PASS" : "FAIL");
    
    /* 无效的UTF-16字符串 (孤立的低代理项) */
    const uint16_t invalid_utf16[] = {0xDC00, 0}; /* 孤立的低代理项 */
    is_valid = is_valid_utf16(invalid_utf16, 0, UTF16_LE);
    printf("  Invalid UTF-16 (lone low surrogate): %s\n", !is_valid ? "PASS" : "FAIL");
    
    printf("\n");
}

/**
 * @brief 测试码点与UTF-8之间的转换
 */
static void test_codepoint_utf8_conversion(void)
{
    printf("Testing codepoint <-> UTF-8 conversion...\n");
    
    /* 测试各种码点 */
    uint32_t test_codepoints[] = {
        0x41,        /* 'A' - ASCII */
        0x7F,        /* DEL - 单字节最大 */
        0x80,        /* 两字节最小 */
        0x7FF,       /* 两字节最大 */
        0x800,       /* 三字节最小 */
        0xFFFF,      /* 三字节最大 */
        0x10000,     /* 四字节最小 */
        0x10FFFF     /* 四字节最大 */
    };
    
    const char* test_names[] = {
        "ASCII 'A' (U+0041)",
        "DEL (U+007F)",
        "Two-byte min (U+0080)",
        "Two-byte max (U+07FF)",
        "Three-byte min (U+0800)",
        "Three-byte max (U+FFFF)",
        "Four-byte min (U+10000)",
        "Four-byte max (U+10FFFF)"
    };
    
    size_t num_tests = sizeof(test_codepoints) / sizeof(test_codepoints[0]);
    bool all_passed = true;
    
    for (size_t i = 0; i < num_tests; i++)
    {
        uint32_t original_codepoint = test_codepoints[i];
        uint8_t utf8_buffer[5] = {0};
        size_t utf8_len = 0;
        
        /* 码点 -> UTF-8 */
        conv_result_t result = codepoint_to_utf8(original_codepoint, utf8_buffer, &utf8_len);
        
        if (result != CONV_SUCCESS)
        {
            printf("  FAIL: %s -> UTF-8 conversion failed\n", test_names[i]);
            all_passed = false;
            continue;
        }
        
        /* UTF-8 -> 码点 */
        uint32_t decoded_codepoint = 0;
        size_t decoded_len = 0;
        result = utf8_to_codepoint(utf8_buffer, &decoded_codepoint, &decoded_len);
        
        if (result != CONV_SUCCESS)
        {
            printf("  FAIL: UTF-8 -> %s conversion failed\n", test_names[i]);
            all_passed = false;
            continue;
        }
        
        /* 检查是否一致 */
        if (original_codepoint != decoded_codepoint || utf8_len != decoded_len)
        {
            printf("  FAIL: %s mismatch (original: 0x%06X, decoded: 0x%06X)\n", 
                   test_names[i], original_codepoint, decoded_codepoint);
            all_passed = false;
        }
        else
        {
            printf("  PASS: %s (0x%06X -> %d bytes UTF-8)\n", 
                   test_names[i], original_codepoint, (int)utf8_len);
        }
    }
    
    printf("  Overall: %s\n\n", all_passed ? "PASS" : "FAIL");
}

/**
 * @brief 测试码点与UTF-16之间的转换
 */
static void test_codepoint_utf16_conversion(void)
{
    printf("Testing codepoint <-> UTF-16 conversion...\n");
    
    /* 测试各种码点 */
    uint32_t test_codepoints[] = {
        0x41,        /* 'A' - 基本多文种平面 */
        0x7F,        /* DEL */
        0x80,        /* 控制字符 */
        0x7FF,       /* 扩展拉丁 */
        0x800,       /* 三字节UTF-8 */
        0xFFFF,      /* 基本多文种平面最大 */
        0x10000,     /* 辅助平面最小 */
        0x10FFFF     /* Unicode最大码点 */
    };
    
    const char* test_names[] = {
        "ASCII 'A' (U+0041)",
        "DEL (U+007F)",
        "Control char (U+0080)",
        "Extended Latin (U+07FF)",
        "Three-byte char (U+0800)",
        "BMP max (U+FFFF)",
        "Supplementary min (U+10000)",
        "Unicode max (U+10FFFF)"
    };
    
    size_t num_tests = sizeof(test_codepoints) / sizeof(test_codepoints[0]);
    bool all_passed = true;
    utf16_byte_order_t test_orders[] = {UTF16_LE, UTF16_BE};
    const char* order_names[] = {"LE", "BE"};
    
    for (size_t order_idx = 0; order_idx < 2; order_idx++)
    {
        printf("  Byte order: %s\n", order_names[order_idx]);
        
        for (size_t i = 0; i < num_tests; i++)
        {
            uint32_t original_codepoint = test_codepoints[i];
            uint16_t utf16_buffer[3] = {0}; /* 最多需要2个单元 + null终止符 */
            size_t utf16_len = 0;
            
            /* 码点 -> UTF-16 */
            conv_result_t result = codepoint_to_utf16(original_codepoint, utf16_buffer, 
                                                  test_orders[order_idx], &utf16_len);
            
            if (result != CONV_SUCCESS)
            {
                printf("    FAIL: %s -> UTF-16 conversion failed\n", test_names[i]);
                all_passed = false;
                continue;
            }
            
            /* UTF-16 -> 码点 */
            uint32_t decoded_codepoint = 0;
            size_t decoded_len = 0;
            result = utf16_to_codepoint(utf16_buffer, &decoded_codepoint, 
                                       test_orders[order_idx], &decoded_len);
            
            if (result != CONV_SUCCESS)
            {
                printf("    FAIL: UTF-16 -> %s conversion failed\n", test_names[i]);
                all_passed = false;
                continue;
            }
            
            /* 检查是否一致 */
            if (original_codepoint != decoded_codepoint || utf16_len != decoded_len)
            {
                printf("    FAIL: %s mismatch (original: 0x%06X, decoded: 0x%06X)\n", 
                       test_names[i], original_codepoint, decoded_codepoint);
                all_passed = false;
            }
            else
            {
                printf("    PASS: %s (0x%06X -> %d UTF-16 units)\n", 
                       test_names[i], original_codepoint, (int)utf16_len);
            }
        }
    }
    
    printf("  Overall: %s\n\n", all_passed ? "PASS" : "FAIL");
}

/**
 * @brief 测试UTF-8与UTF-16之间的转换
 */
static void test_utf8_utf16_conversion(void)
{
    printf("Testing UTF-8 <-> UTF-16 conversion...\n");
    
    /* 测试字符串包含ASCII、中文、表情符号 */
    const uint8_t* test_utf8 = (const uint8_t*)"Hello, 世界! 🌍";
    size_t utf8_len = strlen((const char*)test_utf8);
    
    printf("  Test string: %s\n", test_utf8);
    printf("  UTF-8 length: %zu bytes\n", utf8_len);
    
    /* 转换为UTF-16 LE */
    size_t max_utf16_size = utf16_from_utf8_max_size(test_utf8, 0);
    uint16_t* utf16_le_buffer = (uint16_t*)malloc(max_utf16_size * sizeof(uint16_t));
    size_t utf16_le_len = 0;
    
    conv_result_t result = utf8_to_utf16(test_utf8, utf16_le_buffer, max_utf16_size, 
                                      UTF16_LE, &utf16_le_len);
    
    if (result != CONV_SUCCESS)
    {
        printf("  FAIL: UTF-8 -> UTF-16 LE conversion failed\n");
        free(utf16_le_buffer);
        return;
    }
    
    printf("  UTF-16 LE length: %zu units\n", utf16_le_len);
    
    /* 转换回UTF-8 */
    size_t max_utf8_size = utf8_from_utf16_max_size(utf16_le_buffer, 0, UTF16_LE);
    uint8_t* utf8_buffer = (uint8_t*)malloc(max_utf8_size);
    size_t decoded_utf8_len = 0;
    
    result = utf16_to_utf8(utf16_le_buffer, utf8_buffer, max_utf8_size, 
                           UTF16_LE, &decoded_utf8_len);
    
    if (result != CONV_SUCCESS)
    {
        printf("  FAIL: UTF-16 LE -> UTF-8 conversion failed\n");
    }
    else
    {
        /* 检查是否一致 */
        if (utf8_len != decoded_utf8_len || 
            memcmp(test_utf8, utf8_buffer, utf8_len) != 0)
        {
            printf("  FAIL: UTF-8 -> UTF-16 LE -> UTF-8 conversion mismatch\n");
        }
        else
        {
            printf("  PASS: UTF-8 -> UTF-16 LE -> UTF-8\n");
        }
    }
    
    /* 测试UTF-16 BE */
    uint16_t* utf16_be_buffer = (uint16_t*)malloc(max_utf16_size * sizeof(uint16_t));
    size_t utf16_be_len = 0;
    
    result = utf8_to_utf16(test_utf8, utf16_be_buffer, max_utf16_size, 
                           UTF16_BE, &utf16_be_len);
    
    if (result != CONV_SUCCESS)
    {
        printf("  FAIL: UTF-8 -> UTF-16 BE conversion failed\n");
    }
    else
    {
        printf("  UTF-16 BE length: %zu units\n", utf16_be_len);
        
        /* 转换回UTF-8 */
        result = utf16_to_utf8(utf16_be_buffer, utf8_buffer, max_utf8_size, 
                               UTF16_BE, &decoded_utf8_len);
        
        if (result != CONV_SUCCESS)
        {
            printf("  FAIL: UTF-16 BE -> UTF-8 conversion failed\n");
        }
        else
        {
            /* 检查是否一致 */
            if (utf8_len != decoded_utf8_len || 
                memcmp(test_utf8, utf8_buffer, utf8_len) != 0)
            {
                printf("  FAIL: UTF-8 -> UTF-16 BE -> UTF-8 conversion mismatch\n");
            }
            else
            {
                printf("  PASS: UTF-8 -> UTF-16 BE -> UTF-8\n");
            }
        }
    }
    
    /* 测试字节序转换 */
    uint16_t* utf16_converted = (uint16_t*)malloc(max_utf16_size * sizeof(uint16_t));
    result = utf16_change_byte_order(utf16_le_buffer, utf16_converted, 0, 
                                     UTF16_LE, UTF16_BE);
    
    if (result != CONV_SUCCESS)
    {
        printf("  FAIL: UTF-16 LE -> UTF-16 BE conversion failed\n");
    }
    else
    {
        /* 检查转换后的字节序 */
        bool conversion_ok = true;
        for (size_t i = 0; i < utf16_le_len && conversion_ok; i++)
        {
            uint16_t le_unit = utf16_le_buffer[i];
            uint16_t expected_be_unit = ((le_unit & 0x00FF) << 8) | ((le_unit & 0xFF00) >> 8);
            
            if (utf16_converted[i] != expected_be_unit)
            {
                conversion_ok = false;
            }
        }
        
        if (conversion_ok)
        {
            printf("  PASS: UTF-16 LE -> UTF-16 BE byte order conversion\n");
        }
        else
        {
            printf("  FAIL: UTF-16 LE -> UTF-16 BE byte order conversion mismatch\n");
        }
    }
    
    /* 清理 */
    free(utf16_le_buffer);
    free(utf16_be_buffer);
    free(utf16_converted);
    free(utf8_buffer);
    
    printf("\n");
}

/**
 * @brief 测试边界情况和错误处理
 */
static void test_edge_cases_and_errors(void)
{
    printf("Testing edge cases and error handling...\n");
    
    /* 测试无效的码点 */
    uint8_t utf8_buffer[5] = {0};
    size_t utf8_len = 0;
    conv_result_t result = codepoint_to_utf8(0x110000, utf8_buffer, &utf8_len);
    printf("  Invalid codepoint (0x110000): %s\n", 
           result == CONV_ERROR_INVALID_PARAM? "PASS" : "FAIL");
    
    /* 测试代理项码点 */
    result = codepoint_to_utf8(0xD800, utf8_buffer, &utf8_len);
    printf("  Surrogate codepoint (0xD800): %s\n", 
           result == CONV_ERROR_INVALID_PARAM ? "PASS" : "FAIL");
    
    /* 测试缓冲区太小 */
    uint16_t small_buffer[1] = {0};
    const uint8_t* utf8_str = (const uint8_t*)"Hello";
    result = utf8_to_utf16(utf8_str, small_buffer, 1, UTF16_LE, NULL);
    printf("  Buffer too small: %s\n", 
           result == CONV_ERROR_OUT_OF_BUFFER ? "PASS" : "FAIL");
    
    /* 测试无效的UTF-8序列 */
    uint32_t codepoint = 0;
    const uint8_t invalid_utf8[] = {0xFF, 0}; /* 无效起始字节 */
    result = utf8_to_codepoint(invalid_utf8, &codepoint, NULL);
    printf("  Invalid UTF-8 start byte: %s\n", 
           result == CONV_ERROR_INVALID_PARAM ? "PASS" : "FAIL");
    
    /* 测试不完整的UTF-8序列 */
    const uint8_t incomplete_utf8[] = {0xE0, 0x80}; /* 缺少第三个字节 */
    result = utf8_to_codepoint(incomplete_utf8, &codepoint, NULL);
    printf("  Incomplete UTF-8 sequence: %s\n", 
           result == CONV_ERROR_INVALID_PARAM ? "PASS" : "FAIL");
    
    /* 测试过长的UTF-8编码 */
    const uint8_t overlong_utf8[] = {0xC0, 0x80}; /* 过长的U+0000编码 */
    result = utf8_to_codepoint(overlong_utf8, &codepoint, NULL);
    printf("  Overlong UTF-8 encoding: %s\n", 
           result == CONV_ERROR_INVALID_PARAM ? "PASS" : "FAIL");
    
    printf("\n");
}

/**
 * @brief 测试 utf8_strclen 函数
 */
static void test_utf8_strclen(void)
{
    printf("Testing utf8_strclen...\n");
    
    const uint8_t* test_strings[] = {
        (const uint8_t*)"",                    /* 空字符串 */
        (const uint8_t*)"Hello",                /* ASCII */
        (const uint8_t*)"Hello, 世界!",         /* 混合 ASCII 和中文 */
        (const uint8_t*)"🌍",                   /* 4字节UTF-8 (U+1F30D) */
        (const uint8_t*)"Hello, 🌍",            /* 混合 */
        NULL
    };
    /* 修正：第三个字符串应为10个字符 */
    size_t expected_counts[] = {0, 5, 10, 1, 8};
    
    for (int i = 0; test_strings[i] != NULL; i++)
    {
        size_t count = utf8_strclen(test_strings[i]);
        printf("  String %d: \"%s\" -> %zu characters (expected %zu) %s\n", 
               i+1, test_strings[i], count, expected_counts[i],
               (count == expected_counts[i]) ? "PASS" : "FAIL");
    }
    
    /* 测试无效UTF-8序列的计数行为 */
    const uint8_t invalid_utf8[] = {0xC0, 0x80, 0x41, 0}; /* 过长的0x00编码, 然后是'A' */
    size_t count_invalid = utf8_strclen(invalid_utf8);
    printf("  Invalid UTF-8 (overlong null + 'A'): %zu characters (should stop at first invalid, so 0) %s\n",
           count_invalid, (count_invalid == 0) ? "PASS" : "FAIL");
    
    printf("\n");
}

/**
 * @brief 测试 utf16_strclen 函数
 */
static void test_utf16_strclen(void)
{
    printf("Testing utf16_strclen...\n");
    
    /* 测试字符串: "Hello" (ASCII) 在UTF-16 LE */
    const uint16_t hello_le[] = {0x0048, 0x0065, 0x006C, 0x006C, 0x006F, 0};
    size_t count_hello = utf16_strclen(hello_le, UTF16_LE);
    printf("  UTF-16 LE \"Hello\": %zu characters (expected 5) %s\n",
           count_hello, (count_hello == 5) ? "PASS" : "FAIL");
    
    /* 测试字符串: "Hello, 世界!" 在UTF-16 LE */
    const uint16_t hello_world_le[] = {0x0048, 0x0065, 0x006C, 0x006C, 0x006F, 0x002C, 0x0020, 0x4E16, 0x754C, 0x0021, 0};
    size_t count_world = utf16_strclen(hello_world_le, UTF16_LE);
    /* 修正：应为10个字符 */
    printf("  UTF-16 LE \"Hello, 世界!\": %zu characters (expected 10) %s\n",
           count_world, (count_world == 10) ? "PASS" : "FAIL");
    
    /* 测试字符串: 包含代理对 (U+1F30D) 在UTF-16 LE: 0xD83C 0xDF0D */
    const uint16_t earth_le[] = {0xD83C, 0xDF0D, 0};
    size_t count_earth = utf16_strclen(earth_le, UTF16_LE);
    printf("  UTF-16 LE \"🌍\": %zu characters (expected 1) %s\n",
           count_earth, (count_earth == 1) ? "PASS" : "FAIL");
    
    /* 测试空字符串 */
    const uint16_t empty[] = {0};
    size_t count_empty = utf16_strclen(empty, UTF16_LE);
    printf("  Empty string: %zu characters (expected 0) %s\n",
           count_empty, (count_empty == 0) ? "PASS" : "FAIL");
    
    /* 测试无效UTF-16序列 (孤立的低代理项) */
    const uint16_t invalid_utf16[] = {0xDC00, 0x0041, 0}; /* 孤立的低代理项，然后是'A' */
    size_t count_invalid = utf16_strclen(invalid_utf16, UTF16_LE);
    printf("  Invalid UTF-16 (lone low surrogate + 'A'): %zu characters (should stop at invalid, so 0) %s\n",
           count_invalid, (count_invalid == 0) ? "PASS" : "FAIL");
    
    /* 测试UTF16_NATIVE */
    size_t count_native = utf16_strclen(hello_le, UTF16_NATIVE);
    printf("  UTF16_NATIVE for same string: %zu (should equal 5) %s\n",
           count_native, (count_native == 5) ? "PASS" : "FAIL");
    
    printf("\n");
}

/**
 * @brief 主函数
 * 
 * @return int 程序退出码
 */
int main(void)
{
    printf("========================================\n");
    printf("Unicode Encoding Conversion Library Test\n");
    printf("========================================\n\n");
    
    /* 运行所有测试 */
    test_byte_order();
    test_utf8_validation();
    test_utf16_validation();
    test_codepoint_utf8_conversion();
    test_codepoint_utf16_conversion();
    test_utf8_utf16_conversion();
    test_edge_cases_and_errors();
    test_utf8_strclen();    /* 新增测试 */
    test_utf16_strclen();   /* 新增测试 */
    
    printf("========================================\n");
    printf("All tests completed\n");
    printf("========================================\n");
    
    return 0;
}
