#pragma once
#include "WeatherModels.h"
#include "Config.h"
#include <windows.h>

class IconRenderer {
public:
    static void DrawWeatherIcon(HDC hdc, float x, float y, float size, WeatherKind kind, bool dark_mode, IconStyle style);
    static void DrawRainDrop(HDC hdc, float x, float y, float size, bool dark_mode, IconStyle style);
};