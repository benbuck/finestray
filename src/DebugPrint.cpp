// Copyright 2020 Benbuck Nason

#include <Windows.h>
#include <stdio.h>
#include <stdarg.h>

void debugPrintf(char const * fmt, ...)
{
    char buf[1024];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    OutputDebugStringA(buf);
}
