#pragma once
#include <windows.h>
#include "PluginInterface.h"
#include "Config.h"
#include "WeatherModels.h"
#include "WeatherItem.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

class WeatherPlugin : public ITMPlugin {
public:
    static WeatherPlugin& Instance();

    IPluginItem* GetItem(int index) override;
    void DataRequired() override;
    OptionReturn ShowOptionsDialog(void* hParent) override;
    const wchar_t* GetInfo(PluginInfoIndex index) override;
    const wchar_t* GetTooltipInfo() override;
    void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) override;
    void OnInitialize(ITrafficMonitor* pApp) override;

    int GetCommandCount() override;
    const wchar_t* GetCommandName(int command_index) override;
    void OnPluginCommand(int command_index, void* hWnd, void* para) override;

    PluginConfig GetConfig() const;
    WeatherSnapshot GetSnapshot() const;
    int GetScrollIndex() const;

    void EnsureGdiPlus();
    void StepScroll(int delta);
    void CycleOrResetScroll();
    void TriggerRefresh();

private:
    WeatherPlugin();
    ~WeatherPlugin();

    void StartWorker();
    void StopWorker();
    void WorkerLoop();

    ITrafficMonitor* m_app = nullptr;
    WeatherItem m_item;

    mutable std::mutex m_mutex;
    PluginConfig m_cfg;
    WeatherSnapshot m_snap;
    std::wstring m_configDir;
    std::wstring m_tooltipCache;

    std::atomic<int> m_scrollIndex{ 0 };
    ULONGLONG m_lastCarouselTick = 0;

    std::thread m_worker;
    std::mutex m_cvMutex;
    std::condition_variable m_cv;
    bool m_stop = false;
    bool m_fetchRequested = true;
    ULONGLONG m_lastFetchTick = 0;

    ULONG_PTR m_gdiplusToken = 0;
};
