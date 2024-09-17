// Copyright 2020 Benbuck Nason

#pragma once

// standard library
#include <string>
#include <vector>

class CommandLine
{
public:
    CommandLine();

    bool parse();

    inline int getArgc() const { return static_cast<int>(args_.size()); }

    inline const char ** getArgv() const { return const_cast<const char **>(argv_.data()); }

private:
    std::vector<std::string> args_;
    std::vector<const char *> argv_;
};
