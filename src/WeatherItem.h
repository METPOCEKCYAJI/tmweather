#pragma once
#include "PluginInterface.h"
#include "Config.h"
#include "WeatherModels.h"
#include <string>

class WeatherPlugin;

class WeatherItem : public IPluginItem {
public:
    explicit WeatherItem(WeatherPlugin* owner);

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
    int IsDoubleLineExclusive() const override { return 1; }

private:
    WeatherPlugin* m_owner = nullptr;
    mutable std::wstring m_nameBuf;
};
