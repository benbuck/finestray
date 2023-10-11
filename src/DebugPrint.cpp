// Copyright 2020 Benbuck Nason

#include <Windows.h>
#include <stdio.h>
#include <stdarg.h>

void debugPrintf(char const * fmt, ...)
{
    char buf[65536];

    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (len >= sizeof(buf)) {
        __debugbreak();
    }

    OutputDebugStringA(buf);
}
