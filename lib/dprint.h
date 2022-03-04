#ifndef D_PRINT_H
#define D_PRINT_H
#include"emu_config.h"
#ifndef EMU
#include"xil_printf.h"

#define DEBUG_TEST1
#ifdef DEBUG_TEST
#define DEBUG_PRINTF xil_printf
#else
#define DEBUG_PRINTF //
#endif

//void debug_printf(const char8 *ctrl1, ...);
void debug_printf(const char8* fmt, ...);
#else
#include"emu_log.h"
#include<stdio.h>
#include<stdarg.h>
void DEBUG_PRINTF(const char* fmt, ...);
inline void DEBUG_PRINTF(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    emu_log_println(DEBUG, fmt, args);
    va_end(args);
}
#endif 
#endif
