//===========================================================//
//= Include files.                                          =//
//===========================================================//
#include <log/misc_log.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

//===========================================================//
//= Macro definition.                                       =//
//===========================================================//

//===========================================================//
//= Static function definition.                             =//
//===========================================================//
static void misc_default_log_cb(uint32_t level, const char* format, ...);

//===========================================================//
//= Global variable declare.                                =//
//===========================================================//

//===========================================================//
//= Static variable.                                        =//
//===========================================================//
static misc_lib_log_cb_t *s_misc_log_cb = misc_default_log_cb;
static const char* s_log_level_label[MISC_LOG_LEVEL_OFF] = 
{
	"T",
	"D",
	"I",
	"W",
	"E",
	"F",
};
//===========================================================//
//= Function definition.                                    =//
//===========================================================//
static void misc_default_log_cb(uint32_t level, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    printf("[STDLOG-%s]", s_log_level_label[level]);
    vprintf(format, args);
    va_end(args);
}

void misc_lib_cb_set(misc_lib_log_cb_t *func)
{
	s_misc_log_cb = func;
}

misc_lib_log_cb_t* misc_lib_cb_get(void)
{
	return s_misc_log_cb;
}