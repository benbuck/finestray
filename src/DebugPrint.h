// Copyright 2020 Benbuck Nason

#pragma once

#if defined(NDEBUG)
#define DEBUG_PRINTF(fmt, ...)
#else
void debugPrintf(char const * fmt, ...);
#define DEBUG_PRINTF(fmt, ...) debugPrintf(fmt, ##__VA_ARGS__)
#endif
