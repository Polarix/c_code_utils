#ifndef _INCLUDE_MISC_LOG_H_
#define _INCLUDE_MISC_LOG_H_
//===========================================================//
//= Include files.                                          =//
//===========================================================//
#include <stdio.h>
#include <stdint.h>

//===========================================================//
//= Macro definition.                                       =//
//===========================================================//
#ifdef MISC_LOG_USE_CSTDOUT
 #define MISC_INF_LOG(fmt, ...)  printf("[MISC-I]" fmt "\n", ##__VA_ARGS__);
 #define MISC_DBG_LOG(fmt, ...)  printf("[MISC-D]" fmt "\n", ##__VA_ARGS__);
 #define MISC_WRN_LOG(fmt, ...)  printf("[MISC-W]" fmt "\n", ##__VA_ARGS__);
 #define MISC_ERR_LOG(fmt, ...)  printf("[MISC-E]" fmt "\n", ##__VA_ARGS__);
 #define MISC_TRC_LOG(fmt, ...)  printf("[MISC-T]" fmt "\n", ##__VA_ARGS__);
#else
 #define MISC_INF_LOG(fmt, ...)  (*misc_lib_cb_get())(MISC_LOG_LEVEL_INF, "[MISC]" fmt "\n", ##__VA_ARGS__);
 #define MISC_DBG_LOG(fmt, ...)  (*misc_lib_cb_get())(MISC_LOG_LEVEL_DBG, "[MISC]" fmt "\n", ##__VA_ARGS__);
 #define MISC_WRN_LOG(fmt, ...)  (*misc_lib_cb_get())(MISC_LOG_LEVEL_WRN, "[MISC]" fmt "\n", ##__VA_ARGS__);
 #define MISC_ERR_LOG(fmt, ...)  (*misc_lib_cb_get())(MISC_LOG_LEVEL_ERR, "[MISC]" fmt "\n", ##__VA_ARGS__);
 #define MISC_TRC_LOG(fmt, ...)  (*misc_lib_cb_get())(MISC_LOG_LEVEL_TRC, "[MISC]" fmt "\n", ##__VA_ARGS__);
#endif // MISC_LOG_USE_CSTDOUT

//===========================================================//
//= Data type declare.                                      =//
//===========================================================//
typedef void (misc_lib_log_cb_t)(uint32_t level, const char* format, ...);

enum misc_lib_log_level
{
	MISC_LOG_LEVEL_TRC = 0,
	MISC_LOG_LEVEL_DBG,
	MISC_LOG_LEVEL_INF,
	MISC_LOG_LEVEL_WRN,
	MISC_LOG_LEVEL_ERR,
	MISC_LOG_LEVEL_FAL,
	MISC_LOG_LEVEL_OFF
};

//===========================================================//
//= Function declare.                                       =//
//===========================================================//
#ifdef __cplusplus
extern "C" {
#endif

void misc_lib_cb_set(misc_lib_log_cb_t *func);
misc_lib_log_cb_t* misc_lib_cb_get(void);

#ifdef __cplusplus
}
#endif

#endif // _INCLUDE_MISC_LOG_H_
