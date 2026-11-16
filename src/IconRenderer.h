#pragma once
#include "WeatherModels.h"
#include "Config.h"
#include <windows.h>

class IconRenderer {
public:
    static void DrawTileBackdrop(HDC hdc, float x, float y, float w, float h, WeatherKind kind, bool dark_mode, const PluginConfig& cfg, bool draw_bg);
    static void DrawWeatherGlyph(HDC hdc, float x, float y, float size, WeatherKind kind, bool dark_mode, BYTE alpha);
    static void DrawRainDrop(HDC hdc, float x, float y, float size, bool dark_mode);
    static void DrawTempAlert(HDC hdc, float x, float y, float size, int dir, AlertMode mode, bool dark_mode);
};
