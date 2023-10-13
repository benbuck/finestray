// Copyright 2020 Benbuck Nason

#pragma once

namespace CommandLine
{

bool getArgs(int * argc, char *** argv);
void freeArgs(int argc, char ** argv);

} // namespace CommandLine
