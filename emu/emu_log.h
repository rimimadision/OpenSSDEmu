#ifndef EMU_LOG
#define EMU_LOG

#include "emu_config.h"
#ifdef EMU
#include<stdarg.h>
#include "../lib/type.h"
typedef enum LOG_TYPE
{
	LOG,
	ERR,
	WARN,
	XIL,
	DEBUG,
} LOG_TYPE;

void emu_log_println(LOG_TYPE T, const char fmt[], ...);
static inline void dprint(const char fmt[], ...)
{
	va_list args = NULL;
	va_start(args, fmt);
	emu_log_println(DEBUG, fmt, args);
	va_end(args);
}
static inline void xil_printf(const char fmt[], ...)
{
	va_list args = NULL;
	va_start(args, fmt);
	emu_log_println(XIL, fmt, args);
	va_end(args);
}
#endif

#endif