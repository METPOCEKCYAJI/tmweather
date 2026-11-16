#pragma once
#include "WeatherModels.h"
#include "Config.h"
#include <windows.h>

class IconRenderer {
public:
    static void DrawTileBackdrop(HDC hdc, float x, float y, float w, float h, WeatherKind kind, bool dark_mode, IconStyle style);
    static void DrawRainDrop(HDC hdc, float x, float y, float size, bool dark_mode);
};
