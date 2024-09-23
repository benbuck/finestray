// Copyright 2020 Benbuck Nason

#pragma once

// Windows
#include <Windows.h>

// Standard library
#include <string>

std::string fileRead(const std::string & fileName);
bool fileWrite(const std::string & fileName, const std::string & contents);
std::string getExecutablePath(void);
