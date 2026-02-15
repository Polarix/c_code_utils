/**
 * @file unicode_utils.c
 * @brief UTF-8/UTF-16/Unicode编码转换库实现
 * @details 实现了UTF-8、UTF-16和Unicode码点之间的相互转换功能，
 *          支持UTF-16的小端(LE)和大端(BE)字节序。
 *          遵循C11标准，不依赖外部库。
 * 
 * @author 自动生成
 * @date 2023
 * @version 1.0
 */

#include "unicode_utils.h"
#include <string.h>

/**
 * @brief 获取系统原生字节序
 * 
 * @return utf16_byte_order_t 系统原生字节序
 */
utf16_byte_order_t get_native_byte_order(void)
{
    union
    {
        uint16_t value;
        uint8_t bytes[2];
    } test = {0x1234};
    
    utf16_byte_order_t result = UTF16_NATIVE;
    
    if (test.bytes[0] == 0x34 && test.bytes[1] == 0x12)
    {
        result = UTF16_LE;
    }
    else if (test.bytes[0] == 0x12 && test.bytes[1] == 0x34)
    {
        result = UTF16_BE;
    }
    
    return result;
}

/**
 * @brief 检查UTF-8序列的有效性
 * 
 * @param utf8_str UTF-8字符串
 * @param length 字符串长度（字节数），如果为0则自动计算
 * @return true UTF-8序列有效
 * @return false UTF-8序列无效
 */
bool is_valid_utf8(const uint8_t* utf8_str, size_t length)
{
    bool valid = true;
    size_t i = 0;
    
    if (utf8_str == NULL)
    {
        valid = false;
    }
    else
    {
        /* 如果length为0，则自动计算长度（直到遇到null字符） */
        if (length == 0)
        {
            while (utf8_str[i] != 0)
            {
                i++;
            }
            length = i;
            i = 0;
        }
        
        while (i < length && valid)
        {
            uint8_t c = utf8_str[i];
            
            /* 单字节字符 (0xxxxxxx) */
            if ((c & 0x80) == 0x00)
            {
                i++;
            }
            /* 两字节字符 (110xxxxx 10xxxxxx) */
            else if ((c & 0xE0) == 0xC0)
            {
                /* 检查是否有足够的字节 */
                if (i + 1 >= length)
                {
                    valid = false;
                }
                else
                {
                    /* 检查后续字节是否以10开头 */
                    if ((utf8_str[i + 1] & 0xC0) != 0x80)
                    {
                        valid = false;
                    }
                    /* 检查编码是否过短（例如，不应该编码为11000000 10000000） */
                    else if ((c & 0xFE) == 0xC0)
                    {
                        valid = false;
                    }
                    else
                    {
                        i += 2;
                    }
                }
            }
            /* 三字节字符 (1110xxxx 10xxxxxx 10xxxxxx) */
            else if ((c & 0xF0) == 0xE0)
            {
                /* 检查是否有足够的字节 */
                if (i + 2 >= length)
                {
                    valid = false;
                }
                else
                {
                    /* 检查后续字节是否以10开头 */
                    if ((utf8_str[i + 1] & 0xC0) != 0x80 || 
                        (utf8_str[i + 2] & 0xC0) != 0x80)
                    {
                        valid = false;
                    }
                    /* 检查编码是否过短 */
                    else if (c == 0xE0 && (utf8_str[i + 1] & 0xE0) == 0x80)
                    {
                        valid = false;
                    }
                    else
                    {
                        i += 3;
                    }
                }
            }
            /* 四字节字符 (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx) */
            else if ((c & 0xF8) == 0xF0)
            {
                /* 检查是否有足够的字节 */
                if (i + 3 >= length)
                {
                    valid = false;
                }
                else
                {
                    /* 检查后续字节是否以10开头 */
                    if ((utf8_str[i + 1] & 0xC0) != 0x80 || 
                        (utf8_str[i + 2] & 0xC0) != 0x80 || 
                        (utf8_str[i + 3] & 0xC0) != 0x80)
                    {
                        valid = false;
                    }
                    /* 检查编码是否过短 */
                    else if (c == 0xF0 && (utf8_str[i + 1] & 0xF0) == 0x80)
                    {
                        valid = false;
                    }
                    /* 检查码点是否超过U+10FFFF */
                    else if (c == 0xF4 && utf8_str[i + 1] > 0x8F)
                    {
                        valid = false;
                    }
                    else
                    {
                        i += 4;
                    }
                }
            }
            /* 无效的UTF-8起始字节 */
            else
            {
                valid = false;
            }
        }
    }
    
    return valid;
}

/**
 * @brief 检查UTF-16序列的有效性
 * 
 * @param utf16_str UTF-16字符串
 * @param length 字符串长度（代码单元数），如果为0则自动计算
 * @param byte_order 字节序
 * @return true UTF-16序列有效
 * @return false UTF-16序列无效
 */
bool is_valid_utf16(const uint16_t* utf16_str, size_t length, utf16_byte_order_t byte_order)
{
    bool valid = true;
    size_t i = 0;
    
    if (utf16_str == NULL)
    {
        valid = false;
    }
    else if (byte_order != UTF16_LE && byte_order != UTF16_BE && byte_order != UTF16_NATIVE)
    {
        valid = false;
    }
    else
    {
        utf16_byte_order_t effective_order = byte_order;
        
        /* 如果指定了UTF16_NATIVE，使用系统原生字节序 */
        if (effective_order == UTF16_NATIVE)
        {
            effective_order = get_native_byte_order();
        }
        
        /* 如果length为0，则自动计算长度（直到遇到null字符） */
        if (length == 0)
        {
            while (utf16_str[i] != 0)
            {
                i++;
            }
            length = i;
            i = 0;
        }
        
        while (i < length && valid)
        {
            uint16_t unit = utf16_str[i];
            
            /* 处理字节序 */
            if (effective_order == UTF16_BE)
            {
                unit = ((unit & 0x00FF) << 8) | ((unit & 0xFF00) >> 8);
            }
            
            /* 基本多文种平面 (U+0000 to U+D7FF and U+E000 to U+FFFF) */
            if ((unit < 0xD800) || (unit > 0xDFFF))
            {
                i++;
            }
            /* 高代理项 (0xD800-0xDBFF) */
            else if (unit >= 0xD800 && unit <= 0xDBFF)
            {
                /* 检查是否有足够的代码单元 */
                if (i + 1 >= length)
                {
                    valid = false;
                }
                else
                {
                    uint16_t next_unit = utf16_str[i + 1];
                    
                    /* 处理字节序 */
                    if (effective_order == UTF16_BE)
                    {
                        next_unit = ((next_unit & 0x00FF) << 8) | ((next_unit & 0xFF00) >> 8);
                    }
                    
                    /* 检查下一个单元是否是低代理项 */
                    if (next_unit >= 0xDC00 && next_unit <= 0xDFFF)
                    {
                        i += 2;
                    }
                    else
                    {
                        valid = false;
                    }
                }
            }
            /* 孤立的低代理项 (0xDC00-0xDFFF) - 无效 */
            else
            {
                valid = false;
            }
        }
    }
    
    return valid;
}

/**
 * @brief 计算UTF-8字符串的字节长度
 * 
 * @param utf8_str UTF-8字符串
 * @return size_t UTF-8字符串的字节长度（不包含结尾的null字符）
 */
size_t utf8_strlen(const uint8_t* utf8_str)
{
    size_t length = 0;
    
    if (utf8_str != NULL)
    {
        while (utf8_str[length] != 0)
        {
            length++;
        }
    }
    
    return length;
}

/**
 * @brief 计算UTF-16字符串的代码单元数量
 * 
 * @param utf16_str UTF-16字符串
 * @param byte_order 字节序
 * @return size_t UTF-16字符串的代码单元数量（不包含结尾的null字符）
 */
size_t utf16_strlen(const uint16_t* utf16_str, utf16_byte_order_t byte_order)
{
    size_t length = 0;
    
    if (utf16_str != NULL)
    {
        utf16_byte_order_t effective_order = byte_order;
        
        /* 如果指定了UTF16_NATIVE，使用系统原生字节序 */
        if (effective_order == UTF16_NATIVE)
        {
            effective_order = get_native_byte_order();
        }
        
        while (utf16_str[length] != 0)
        {
            length++;
        }
    }
    
    return length;
}

/**
 * @brief 计算存储UTF-8字符串所需的最大字节数
 * 
 * @param utf16_str UTF-16字符串
 * @param length UTF-16代码单元数量（如果为0则自动计算）
 * @param byte_order 字节序
 * @return size_t 所需的最大字节数（包含结尾的null字符）
 */
size_t utf8_from_utf16_max_size(const uint16_t* utf16_str, size_t length, utf16_byte_order_t byte_order)
{
    size_t max_size = 1; /* 包含结尾的null字符 */
    
    if (utf16_str != NULL)
    {
        utf16_byte_order_t effective_order = byte_order;
        
        /* 如果指定了UTF16_NATIVE，使用系统原生字节序 */
        if (effective_order == UTF16_NATIVE)
        {
            effective_order = get_native_byte_order();
        }
        
        /* 如果length为0，则自动计算长度 */
        if (length == 0)
        {
            length = utf16_strlen(utf16_str, effective_order);
        }
        
        /* 最坏情况：每个UTF-16代码单元转换为4字节UTF-8 */
        max_size += length * 4;
    }
    
    return max_size;
}

/**
 * @brief 计算存储UTF-16字符串所需的最大代码单元数
 * 
 * @param utf8_str UTF-8字符串
 * @param length UTF-8字节数（如果为0则自动计算）
 * @return size_t 所需的最大代码单元数（包含结尾的null字符）
 */
size_t utf16_from_utf8_max_size(const uint8_t* utf8_str, size_t length)
{
    size_t max_size = 1; /* 包含结尾的null字符 */
    
    if (utf8_str != NULL)
    {
        /* 如果length为0，则自动计算长度 */
        if (length == 0)
        {
            length = utf8_strlen(utf8_str);
        }
        
        /* 最坏情况：每个UTF-8字节转换为1个UTF-16代码单元 */
        max_size += length;
    }
    
    return max_size;
}

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
                        size_t* out_length)
{
    conv_result_t result = CONV_SUCCESS;
    size_t i = 0;
    size_t j = 0;
    uint32_t codepoint = 0;
    size_t codepoint_len = 0;
    
    /* 检查输入参数 */
    if (utf8_str == NULL || utf16_buffer == NULL || buffer_size == 0)
    {
        /* 无效的输入参数 */
        result = CONV_ERROR_INVALID_PARAM;
    }
    else if (byte_order != UTF16_LE && byte_order != UTF16_BE && byte_order != UTF16_NATIVE)
    {
        /* 无效的字节序参数 */
        result = CONV_ERROR_INVALID_PARAM;
    }
    else
    {
        utf16_byte_order_t effective_order = byte_order;
        
        /* 如果指定了UTF16_NATIVE，使用系统原生字节序 */
        if (effective_order == UTF16_NATIVE)
        {
            effective_order = get_native_byte_order();
        }
        
        /* 转换每个字符 */
        while (utf8_str[i] != 0 && result == CONV_SUCCESS)
        {
            /* 将UTF-8转换为码点 */
            result = utf8_to_codepoint(&utf8_str[i], &codepoint, &codepoint_len);
            
            if (result == CONV_SUCCESS)
            {
                /* 检查是否有足够的空间 */
                size_t needed_units = (codepoint <= 0xFFFF) ? 1 : 2;
                
                if (j + needed_units + 1 > buffer_size) /* +1 for null terminator */
                {
                    result = CONV_ERROR_OUT_OF_BUFFER;
                }
                else
                {
                    /* 将码点转换为UTF-16 */
                    size_t utf16_len = 0;
                    uint16_t utf16_units[2] = {0};
                    
                    result = codepoint_to_utf16(codepoint, utf16_units, effective_order, &utf16_len);
                    
                    if (result == CONV_SUCCESS)
                    {
                        /* 复制到输出缓冲区 */
                        for (size_t k = 0; k < utf16_len; k++)
                        {
                            utf16_buffer[j++] = utf16_units[k];
                        }
                        
                        /* 前进到下一个UTF-8字符 */
                        i += codepoint_len;
                    }
                }
            }
        }
        
        /* 添加null终止符 */
        if (result == CONV_SUCCESS)
        {
            if (j < buffer_size)
            {
                utf16_buffer[j] = 0;
                
                /* 设置输出长度 */
                if (out_length != NULL)
                {
                    *out_length = j;
                }
            }
            else
            {
                result = CONV_ERROR_OUT_OF_BUFFER;
            }
        }
    }
    
    return result;
}

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
                        size_t* out_length)
{
    conv_result_t result = CONV_SUCCESS;
    size_t i = 0;
    size_t j = 0;
    uint32_t codepoint = 0;
    size_t codepoint_len = 0;
    
    /* 检查输入参数 */
    if (utf16_str == NULL || utf8_buffer == NULL || buffer_size == 0)
    {
        /* 无效的输入参数 */
        result = CONV_ERROR_INVALID_PARAM;
    }
    else if (byte_order != UTF16_LE && byte_order != UTF16_BE && byte_order != UTF16_NATIVE)
    {
        /* 无效的字节序参数 */
        result = CONV_ERROR_INVALID_PARAM;
    }
    else
    {
        utf16_byte_order_t effective_order = byte_order;
        
        /* 如果指定了UTF16_NATIVE，使用系统原生字节序 */
        if (effective_order == UTF16_NATIVE)
        {
            effective_order = get_native_byte_order();
        }
        
        /* 转换每个字符 */
        while (utf16_str[i] != 0 && result == CONV_SUCCESS)
        {
            /* 将UTF-16转换为码点 */
            result = utf16_to_codepoint(&utf16_str[i], &codepoint, effective_order, &codepoint_len);
            
            if (result == CONV_SUCCESS)
            {
                /* 检查是否有足够的空间 */
                uint8_t temp_buffer[5] = {0};
                size_t utf8_len = 0;
                
                /* 先将码点转换为UTF-8到临时缓冲区 */
                result = codepoint_to_utf8(codepoint, temp_buffer, &utf8_len);
                
                if (result == CONV_SUCCESS)
                {
                    if (j + utf8_len + 1 > buffer_size) /* +1 for null terminator */
                    {
                        result = CONV_ERROR_OUT_OF_BUFFER;
                    }
                    else
                    {
                        /* 复制到输出缓冲区 */
                        for (size_t k = 0; k < utf8_len; k++)
                        {
                            utf8_buffer[j++] = temp_buffer[k];
                        }
                        
                        /* 前进到下一个UTF-16字符 */
                        i += codepoint_len;
                    }
                }
            }
        }
        
        /* 添加null终止符 */
        if (result == CONV_SUCCESS)
        {
            if (j < buffer_size)
            {
                utf8_buffer[j] = 0;
                
                /* 设置输出长度 */
                if (out_length != NULL)
                {
                    *out_length = j;
                }
            }
            else
            {
                result = CONV_ERROR_OUT_OF_BUFFER;
            }
        }
    }
    
    return result;
}

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
                                  utf16_byte_order_t dst_order)
{
    conv_result_t result = CONV_SUCCESS;
    
    /* 检查输入参数 */
    if (src == NULL || dst == NULL)
    {
        /* 无效的输入参数 */
        result = CONV_ERROR_INVALID_PARAM;
    }
    else if (src_order != UTF16_LE && src_order != UTF16_BE && src_order != UTF16_NATIVE)
    {
        /* 无效的源字节序(理论上不存在) */
        result = CONV_ERROR_INVALID_PARAM;
    }
    else if (dst_order != UTF16_LE && dst_order != UTF16_BE && dst_order != UTF16_NATIVE)
    {
        /* 无效的目标字节序(理论上不存在) */
        result = CONV_ERROR_INVALID_PARAM;
    }
    else
    {
        utf16_byte_order_t effective_src_order = src_order;
        utf16_byte_order_t effective_dst_order = dst_order;
        
        /* 如果指定了UTF16_NATIVE，使用系统原生字节序 */
        if (effective_src_order == UTF16_NATIVE)
        {
            effective_src_order = get_native_byte_order();
        }
        
        if (effective_dst_order == UTF16_NATIVE)
        {
            effective_dst_order = get_native_byte_order();
        }
        
        /* 如果源和目标字节序相同，直接复制 */
        if (effective_src_order == effective_dst_order)
        {
            /* 如果length为0，则自动计算长度 */
            if (length == 0)
            {
                while (src[length] != 0)
                {
                    dst[length] = src[length];
                    length++;
                }
                dst[length] = 0;
            }
            else
            {
                for (size_t i = 0; i < length; i++)
                {
                    dst[i] = src[i];
                }
            }
        }
        /* 如果length为0，则自动计算长度（直到遇到null字符） */
        else if (length == 0)
        {
            size_t i = 0;
            
            while (src[i] != 0)
            {
                uint16_t unit = src[i];
                
                /* 交换字节序 */
                dst[i] = ((unit & 0x00FF) << 8) | ((unit & 0xFF00) >> 8);
                i++;
            }
            dst[i] = 0;
        }
        /* 转换指定长度的字符串 */
        else
        {
            for (size_t i = 0; i < length; i++)
            {
                uint16_t unit = src[i];
                
                /* 交换字节序 */
                dst[i] = ((unit & 0x00FF) << 8) | ((unit & 0xFF00) >> 8);
            }
        }
    }
    
    return result;
}

/**
 * @brief 将Unicode码点转换为UTF-8
 * 
 * @param codepoint Unicode码点
 * @param utf8_buffer 输出UTF-8缓冲区（至少5字节）
 * @param out_length 实际输出的字节数（可选，可为NULL）
 * @return conv_result_t 转换结果状态码
 */
conv_result_t codepoint_to_utf8(uint32_t codepoint, uint8_t* utf8_buffer, size_t* out_length)
{
    conv_result_t result = CONV_SUCCESS;
    
    /* 检查输入参数 */
    if (utf8_buffer == NULL)
    {
        result = CONV_ERROR_INVALID_PARAM;
    }
    else if (codepoint > 0x10FFFF)
    {
        result = CONV_ERROR_INVALID_PARAM;
    }
    else if (codepoint >= 0xD800 && codepoint <= 0xDFFF)
    {
        /* UTF-16代理项区间无效 */
        result = CONV_ERROR_INVALID_PARAM;
    }
    else
    {
        /* 单字节字符 (U+0000 to U+007F) */
        if (codepoint <= 0x7F)
        {
            utf8_buffer[0] = (uint8_t)codepoint;
            
            if (out_length != NULL)
            {
                *out_length = 1;
            }
        }
        /* 两字节字符 (U+0080 to U+07FF) */
        else if (codepoint <= 0x7FF)
        {
            utf8_buffer[0] = (uint8_t)(0xC0 | (codepoint >> 6));
            utf8_buffer[1] = (uint8_t)(0x80 | (codepoint & 0x3F));
            
            if (out_length != NULL)
            {
                *out_length = 2;
            }
        }
        /* 三字节字符 (U+0800 to U+FFFF) */
        else if (codepoint <= 0xFFFF)
        {
            utf8_buffer[0] = (uint8_t)(0xE0 | (codepoint >> 12));
            utf8_buffer[1] = (uint8_t)(0x80 | ((codepoint >> 6) & 0x3F));
            utf8_buffer[2] = (uint8_t)(0x80 | (codepoint & 0x3F));
            
            if (out_length != NULL)
            {
                *out_length = 3;
            }
        }
        /* 四字节字符 (U+10000 to U+10FFFF) */
        else
        {
            utf8_buffer[0] = (uint8_t)(0xF0 | (codepoint >> 18));
            utf8_buffer[1] = (uint8_t)(0x80 | ((codepoint >> 12) & 0x3F));
            utf8_buffer[2] = (uint8_t)(0x80 | ((codepoint >> 6) & 0x3F));
            utf8_buffer[3] = (uint8_t)(0x80 | (codepoint & 0x3F));
            
            if (out_length != NULL)
            {
                *out_length = 4;
            }
        }
    }
    
    return result;
}

/**
 * @brief 将UTF-8转换为Unicode码点
 * 
 * @param utf8_str UTF-8序列
 * @param codepoint 输出的Unicode码点
 * @param out_length 实际读取的字节数（可选，可为NULL）
 * @return conv_result_t 转换结果状态码
 */
conv_result_t utf8_to_codepoint(const uint8_t* utf8_str, uint32_t* codepoint, size_t* out_length)
{
    conv_result_t result = CONV_SUCCESS;
    
    /* 检查输入参数 */
    if (utf8_str == NULL || codepoint == NULL)
    {
        result = CONV_ERROR_INVALID_PARAM;
    }
    else
    {
        uint8_t first_byte = utf8_str[0];
        
        /* 单字节字符 (0xxxxxxx) */
        if ((first_byte & 0x80) == 0x00)
        {
            *codepoint = first_byte;
            
            if (out_length != NULL)
            {
                *out_length = 1;
            }
        }
        /* 两字节字符 (110xxxxx 10xxxxxx) */
        else if ((first_byte & 0xE0) == 0xC0)
        {
            /* 检查是否有足够的字节 */
            if ((utf8_str[1] & 0xC0) != 0x80)
            {
                result = CONV_ERROR_INVALID_PARAM;
            }
            else
            {
                *codepoint = ((first_byte & 0x1F) << 6) | (utf8_str[1] & 0x3F);
                
                /* 检查编码是否过短 */
                if (*codepoint < 0x80)
                {
                    result = CONV_ERROR_INVALID_PARAM;
                }
                else if (out_length != NULL)
                {
                    *out_length = 2;
                }
            }
        }
        /* 三字节字符 (1110xxxx 10xxxxxx 10xxxxxx) */
        else if ((first_byte & 0xF0) == 0xE0)
        {
            /* 检查是否有足够的字节 */
            if ((utf8_str[1] & 0xC0) != 0x80 || (utf8_str[2] & 0xC0) != 0x80)
            {
                result = CONV_ERROR_INVALID_PARAM;
            }
            else
            {
                *codepoint = ((first_byte & 0x0F) << 12) | 
                            ((utf8_str[1] & 0x3F) << 6) | 
                            (utf8_str[2] & 0x3F);
                
                /* 检查编码是否过短 */
                if (*codepoint < 0x800)
                {
                    result = CONV_ERROR_INVALID_PARAM;
                }
                else if (out_length != NULL)
                {
                    *out_length = 3;
                }
            }
        }
        /* 四字节字符 (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx) */
        else if ((first_byte & 0xF8) == 0xF0)
        {
            /* 检查是否有足够的字节 */
            if ((utf8_str[1] & 0xC0) != 0x80 || 
                (utf8_str[2] & 0xC0) != 0x80 || 
                (utf8_str[3] & 0xC0) != 0x80)
            {
                result = CONV_ERROR_INVALID_PARAM;
            }
            else
            {
                *codepoint = ((first_byte & 0x07) << 18) | 
                            ((utf8_str[1] & 0x3F) << 12) | 
                            ((utf8_str[2] & 0x3F) << 6) | 
                            (utf8_str[3] & 0x3F);
                
                /* 检查编码是否过短 */
                if (*codepoint < 0x10000)
                {
                    result = CONV_ERROR_INVALID_PARAM;
                }
                /* 检查码点是否超过U+10FFFF */
                else if (*codepoint > 0x10FFFF)
                {
                    result = CONV_ERROR_INVALID_DATA;
                }
                else if (out_length != NULL)
                {
                    *out_length = 4;
                }
            }
        }
        /* 无效的UTF-8起始字节 */
        else
        {
            result = CONV_ERROR_INVALID_PARAM;
        }
    }
    
    return result;
}

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
                             utf16_byte_order_t byte_order, size_t* out_length)
{
    conv_result_t result = CONV_SUCCESS;
    
    /* 检查输入参数 */
    if (utf16_buffer == NULL)
    {
        result = CONV_ERROR_INVALID_PARAM;
    }
    else if (codepoint > 0x10FFFF)
    {
        result = CONV_ERROR_INVALID_PARAM;
    }
    else if (codepoint >= 0xD800 && codepoint <= 0xDFFF)
    {
        /* UTF-16代理项区间无效 */
        result = CONV_ERROR_INVALID_PARAM;
    }
    else if (byte_order != UTF16_LE && byte_order != UTF16_BE && byte_order != UTF16_NATIVE)
    {
        result = CONV_ERROR_INVALID_PARAM;
    }
    else
    {
        utf16_byte_order_t effective_order = byte_order;
        
        /* 如果指定了UTF16_NATIVE，使用系统原生字节序 */
        if (effective_order == UTF16_NATIVE)
        {
            effective_order = get_native_byte_order();
        }
        
        /* 基本多文种平面 (U+0000 to U+FFFF, 除了代理项) */
        if (codepoint <= 0xFFFF)
        {
            utf16_buffer[0] = (uint16_t)codepoint;
            
            /* 处理字节序 */
            if (effective_order == UTF16_BE)
            {
                utf16_buffer[0] = ((utf16_buffer[0] & 0x00FF) << 8) | 
                                  ((utf16_buffer[0] & 0xFF00) >> 8);
            }
            
            if (out_length != NULL)
            {
                *out_length = 1;
            }
        }
        /* 辅助平面 (U+10000 to U+10FFFF) */
        else
        {
            /* 计算代理对 */
            codepoint -= 0x10000;
            uint16_t high_surrogate = (uint16_t)(0xD800 | (codepoint >> 10));
            uint16_t low_surrogate = (uint16_t)(0xDC00 | (codepoint & 0x3FF));
            
            /* 处理字节序 */
            if (effective_order == UTF16_BE)
            {
                high_surrogate = ((high_surrogate & 0x00FF) << 8) | 
                                 ((high_surrogate & 0xFF00) >> 8);
                low_surrogate = ((low_surrogate & 0x00FF) << 8) | 
                                ((low_surrogate & 0xFF00) >> 8);
            }
            
            utf16_buffer[0] = high_surrogate;
            utf16_buffer[1] = low_surrogate;
            
            if (out_length != NULL)
            {
                *out_length = 2;
            }
        }
    }
    
    return result;
}

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
                             utf16_byte_order_t byte_order, size_t* out_length)
{
    conv_result_t result = CONV_SUCCESS;
    
    /* 检查输入参数 */
    if (utf16_str == NULL || codepoint == NULL)
    {
        result = CONV_ERROR_INVALID_PARAM;
    }
    else if (byte_order != UTF16_LE && byte_order != UTF16_BE && byte_order != UTF16_NATIVE)
    {
        result = CONV_ERROR_INVALID_PARAM;
    }
    else
    {
        utf16_byte_order_t effective_order = byte_order;
        
        /* 如果指定了UTF16_NATIVE，使用系统原生字节序 */
        if (effective_order == UTF16_NATIVE)
        {
            effective_order = get_native_byte_order();
        }
        
        uint16_t first_unit = utf16_str[0];
        
        /* 处理字节序 */
        if (effective_order == UTF16_BE)
        {
            first_unit = ((first_unit & 0x00FF) << 8) | ((first_unit & 0xFF00) >> 8);
        }
        
        /* 基本多文种平面 (U+0000 to U+D7FF and U+E000 to U+FFFF) */
        if ((first_unit < 0xD800) || (first_unit > 0xDFFF))
        {
            *codepoint = first_unit;
            
            if (out_length != NULL)
            {
                *out_length = 1;
            }
        }
        /* 高代理项 (0xD800-0xDBFF) */
        else if (first_unit >= 0xD800 && first_unit <= 0xDBFF)
        {
            /* 检查是否有足够的代码单元 */
            uint16_t second_unit = utf16_str[1];
            
            if (second_unit == 0)
            {
                result = CONV_ERROR_INVALID_PARAM;
            }
            else
            {
                /* 处理字节序 */
                if (effective_order == UTF16_BE)
                {
                    second_unit = ((second_unit & 0x00FF) << 8) | 
                                  ((second_unit & 0xFF00) >> 8);
                }
                
                /* 检查下一个单元是否是低代理项 */
                if (second_unit >= 0xDC00 && second_unit <= 0xDFFF)
                {
                    /* 计算码点 */
                    *codepoint = 0x10000 + 
                                (((first_unit & 0x3FF) << 10) | 
                                 (second_unit & 0x3FF));
                    
                    if (out_length != NULL)
                    {
                        *out_length = 2;
                    }
                }
                else
                {
                    result = CONV_ERROR_INVALID_PARAM;
                }
            }
        }
        /* 孤立的低代理项 (0xDC00-0xDFFF) - 无效 */
        else
        {
            result = CONV_ERROR_INVALID_PARAM;
        }
    }
    
    return result;
}