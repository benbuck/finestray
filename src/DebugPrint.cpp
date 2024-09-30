// Copyright 2020 Benbuck Nason

// App
#include "DebugPrint.h"

// Windows
#include <Windows.h>

// Standard library
#include <stdarg.h>
#include <stdio.h>

void debugPrintf(const char * fmt, ...)
{
    char buf[1024];

    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (len >= (int)sizeof(buf)) {
        __debugbreak();
    }

    OutputDebugStringA(buf);
}
