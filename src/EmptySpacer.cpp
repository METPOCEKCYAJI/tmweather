#include "PluginInterface.h"

class EmptySpacerItem : public IPluginItem {
public:
    const wchar_t* GetItemName() const override { return L"Empty Spacer (1px)"; }
    const wchar_t* GetItemId() const override { return L"EmptySpacer_Slot"; }
    const wchar_t* GetItemLableText() const override { return L""; }
    const wchar_t* GetItemValueText() const override { return L""; }
    const wchar_t* GetItemValueSampleText() const override { return L""; }
    bool IsCustomDraw() const override { return true; }
    int GetItemWidth() const override { return 1; }
    int GetItemWidthEx(void*) const override { return 1; }
    void DrawItem(void*, int, int, int, int, bool) override {}
};

class EmptySpacerPlugin : public ITMPlugin {
public:
    IPluginItem* GetItem(int index) override { return index == 0 ? &m_item : nullptr; }
    void DataRequired() override {}
    const wchar_t* GetInfo(PluginInfoIndex index) override {
        switch (index) {
        case TMI_NAME:        return L"Empty Spacer";
        case TMI_DESCRIPTION: return L"Transparent 1px spacer slot for TrafficMonitor";
        case TMI_AUTHOR:      return L"Community";
        case TMI_VERSION:     return L"1.0.0";
        default:              return L"";
        }
    }
private:
    EmptySpacerItem m_item;
};

extern "C" __declspec(dllexport) ITMPlugin* __cdecl TMPluginGetInstance() {
    static EmptySpacerPlugin inst;
    return &inst;
}
