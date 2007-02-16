#include "stdarg.h"
#include "stdio.h"
#include "crtdbg.h"


void trace(char *format, ...)
{
    char msg[256];
    va_list args;
    va_start(args, format);
    _vsnprintf(msg, 256, format, args);
    va_end(args);
#ifdef _DEBUG
    _CrtDbgReport(_CRT_WARN, NULL, NULL, NULL, msg);
#else
    printf(msg);
#endif
}
