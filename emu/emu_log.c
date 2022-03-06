#include"emu_config.h"
#include"emu_log.h"
#include <stdio.h>
#include<stdarg.h>

void emu_log_println(LOG_TYPE T, const char fmt[], ...)
{
#ifdef EMU
	va_list args = NULL;
	va_start(args, fmt);
	switch (T)
	{
	case LOG:
		printf("[EMU LOG]");
		break;
	case WARN:
		printf("[EMU WARN]");
		break;
	case ERR:
		printf("[EMU ERR]");
		break;
	case DEBUG:
		printf("[EMU DEBUG]");
		break;
	case XIL:
		printf("[XIL]");
		vprintf(fmt, args);
		va_end(args);
		return;
	default:
		printf("Log not identified\n");
		return;
	}
	
	vprintf(fmt, args);
	printf("\n");
	va_end(args);
#endif
}