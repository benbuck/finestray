// Copyright 2020 Benbuck Nason

// windows
#include <Windows.h>

// standard library
#include <stdarg.h>
#include <stdio.h>

void debugPrintf(const char * fmt, ...)
{
    char buf[1024];

    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (len >= sizeof(buf)) {
        __debugbreak();
    }

    OutputDebugStringA(buf);
}
