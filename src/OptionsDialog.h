#pragma once
#include "Config.h"
#include <windows.h>

class OptionsDialog {
public:
    static bool Show(HINSTANCE hInst, HWND parent, PluginConfig& cfg);
};