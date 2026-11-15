#pragma once
#include "PluginInterface.h"
#include "Config.h"
#include "WeatherModels.h"
#include <string>

class WeatherPlugin;

class WeatherItem : public IPluginItem {
public:
    WeatherItem(WeatherPlugin* owner, bool is_top);

    const wchar_t* GetItemName() const override;
    const wchar_t* GetItemId() const override;
    const wchar_t* GetItemLableText() const override;
    const wchar_t* GetItemValueText() const override;
    const wchar_t* GetItemValueSampleText() const override;

    bool IsCustomDraw() const override { return true; }
    int GetItemWidth() const override;
    void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;
    int GetItemWidthEx(void* hDC) const override;
    int OnMouseEvent(MouseEventType type, int x, int y, void* hWnd, int flag) override;

private:
    WeatherPlugin* m_owner = nullptr;
    bool m_isTop = true;
    mutable std::wstring m_nameBuf;
};