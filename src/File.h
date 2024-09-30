// Copyright 2020 Benbuck Nason

#pragma once

// Windows
#include <Windows.h>

// Standard library
#include <string>

std::string fileRead(const std::string & fileName);
bool fileWrite(const std::string & fileName, const std::string & contents);
bool fileExists(const std::string & filename);
bool directoryExists(const std::string & directory);
std::string getExecutablePath(void);
std::string getAppDataPath(void);
std::string pathJoin(const std::string & path1, const std::string & path2);
