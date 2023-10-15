// Copyright 2020 Benbuck Nason

#pragma once

#if defined(NDEBUG)
#define DEBUG_PRINTF(fmt, ...)
#else
void debugPrintf(const char * fmt, ...);
#define DEBUG_PRINTF(fmt, ...) debugPrintf(fmt, ##__VA_ARGS__)
#endif
