// Copyright 2020 Benbuck Nason

#include "Settings.h"

namespace SettingsDialog
{

typedef void (*CompletionCallback)(bool success, const Settings & settings);
HWND show(HWND hwnd, const Settings & settings, CompletionCallback completionCallback);

} // namespace SettingsDialog
