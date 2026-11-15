#include "WeatherPlugin.h"
#include "WeatherClient.h"
#include "OptionsDialog.h"
#include "Localization.h"
#include "GdiPlusInclude.h"
#include <algorithm>
#include <cmath>

HINSTANCE g_hInstance = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        g_hInstance = hModule;
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}

extern "C" __declspec(dllexport) ITMPlugin* __cdecl TMPluginGetInstance() {
    return &WeatherPlugin::Instance();
}

WeatherPlugin& WeatherPlugin::Instance() {
    static WeatherPlugin inst;
    return inst;
}

WeatherPlugin::WeatherPlugin() : m_item(this) {}

WeatherPlugin::~WeatherPlugin() {
    StopWorker();
    if (m_gdiplusToken) {
        Gdiplus::GdiplusShutdown(m_gdiplusToken);
        m_gdiplusToken = 0;
    }
}

void WeatherPlugin::EnsureGdiPlus() {
    if (!m_gdiplusToken) {
        Gdiplus::GdiplusStartupInput gdiInput;
        Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiInput, nullptr);
    }
}

void WeatherPlugin::OnInitialize(ITrafficMonitor* pApp) {
    m_app = pApp;
    EnsureGdiPlus();
    if (m_app && m_configDir.empty()) {
        const wchar_t* dir = m_app->GetPluginConfigDir();
        if (dir) {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_configDir = dir;
            ConfigStore::Load(m_configDir, m_cfg);
        }
    }
    StartWorker();
}

void WeatherPlugin::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) {
    if (index == EI_CONFIG_DIR && data) {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_configDir = data;
        ConfigStore::Load(m_configDir, m_cfg);
    }
}

IPluginItem* WeatherPlugin::GetItem(int index) {
    return (index == 0) ? &m_item : nullptr;
}

void WeatherPlugin::StartWorker() {
    if (m_worker.joinable()) return;
    m_stop = false;
    m_fetchRequested = true;
    m_worker = std::thread(&WeatherPlugin::WorkerLoop, this);
}

void WeatherPlugin::StopWorker() {
    {
        std::lock_guard<std::mutex> lk(m_cvMutex);
        m_stop = true;
    }
    m_cv.notify_all();
    if (m_worker.joinable()) m_worker.join();
}

void WeatherPlugin::WorkerLoop() {
    while (true) {
        PluginConfig cfgCopy;
        {
            std::unique_lock<std::mutex> lk(m_cvMutex);
            m_cv.wait(lk, [this] { return m_stop || m_fetchRequested; });
            if (m_stop) break;
            m_fetchRequested = false;
        }
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            cfgCopy = m_cfg;
        }

        WeatherSnapshot fresh = WeatherClient::FetchWeather(cfgCopy);

        {
            std::lock_guard<std::mutex> lk(m_mutex);
            if (fresh.valid || !m_snap.valid) {
                m_snap = std::move(fresh);
            } else {
                m_snap.error_msg = fresh.error_msg;
            }
        }
    }
}

void WeatherPlugin::DataRequired() {
    if (!m_worker.joinable()) StartWorker();

    ULONGLONG now = GetTickCount64();
    PluginConfig cfgCopy = GetConfig();

    ULONGLONG intervalMs = (ULONGLONG)std::max(5, cfgCopy.refresh_minutes) * 60ULL * 1000ULL;
    if (m_lastFetchTick == 0 || (now - m_lastFetchTick) >= intervalMs) {
        m_lastFetchTick = now;
        {
            std::lock_guard<std::mutex> lk(m_cvMutex);
            m_fetchRequested = true;
        }
        m_cv.notify_one();
    }

    if (cfgCopy.display_mode != DisplayMode::Compact && cfgCopy.scroll_mode == ScrollBehavior::AutoCarousel) {
        if (m_lastCarouselTick == 0) m_lastCarouselTick = now;
        if (now - m_lastCarouselTick >= 4000ULL) {
            m_lastCarouselTick = now;
            CycleOrResetScroll();
        }
    }
}

ITMPlugin::OptionReturn WeatherPlugin::ShowOptionsDialog(void* hParent) {
    PluginConfig editable = GetConfig();
    if (OptionsDialog::Show(g_hInstance, reinterpret_cast<HWND>(hParent), editable)) {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_cfg = editable;
            ConfigStore::Save(m_configDir, m_cfg);
        }
        m_scrollIndex = 0;
        TriggerRefresh();
        return OR_OPTION_CHANGED;
    }
    return OR_OPTION_UNCHANGED;
}

const wchar_t* WeatherPlugin::GetInfo(PluginInfoIndex index) {
    LangMode lang = GetConfig().lang;
    switch (index) {
    case TMI_NAME:        return L10n::PluginName(lang);
    case TMI_DESCRIPTION: return L10n::PluginDesc(lang);
    case TMI_AUTHOR:      return L"Community";
    case TMI_COPYRIGHT:   return L"MIT License";
    case TMI_VERSION:     return L"1.0.0";
    case TMI_URL:         return L"https://github.com/zhongyang219/TrafficMonitor";
    default:              return L"";
    }
}

const wchar_t* WeatherPlugin::GetTooltipInfo() {
    PluginConfig cfg = GetConfig();
    WeatherSnapshot snap = GetSnapshot();
    bool ru = (cfg.lang == LangMode::RU);

    if (!snap.valid) {
        m_tooltipCache = snap.error_msg.empty()
            ? (ru ? L"\u0417\u0430\u0433\u0440\u0443\u0437\u043A\u0430 \u0434\u0430\u043D\u043D\u044B\u0445 \u043E \u043F\u043E\u0433\u043E\u0434\u0435..." : L"Loading weather data...")
            : ((ru ? L"\u041E\u0448\u0438\u0431\u043A\u0430: " : L"Error: ") + snap.error_msg);
        return m_tooltipCache.c_str();
    }

    auto fmtT = [&](double tc) {
        double v = (cfg.unit == TempUnit::Fahrenheit) ? (tc * 9.0 / 5.0 + 32.0) : tc;
        wchar_t b[24]{};
        swprintf_s(b, L"%d%s", (int)std::llround(v), (cfg.unit == TempUnit::Fahrenheit ? L"\u00B0F" : L"\u00B0C"));
        return std::wstring(b);
    };

    std::wstring tip = snap.city_name;
    if (!snap.country_name.empty()) tip += L", " + snap.country_name;
    tip += L"\n" + std::wstring(L10n::WeatherName(cfg.lang, snap.current_kind)) + L": " + fmtT(snap.current_temp_c);
    tip += (ru ? L" (\u043E\u0449\u0443\u0449\u0430\u0435\u0442\u0441\u044F " : L" (feels like ") + fmtT(snap.feels_like_c) + L")";

    wchar_t extra[128]{};
    swprintf_s(extra, ru ? L"\n\u0412\u0435\u0442\u0435\u0440: %.1f \u043C/\u0441  |  \u0412\u043B\u0430\u0436\u043D\u043E\u0441\u0442\u044C: %d%%" : L"\nWind: %.1f m/s  |  Humidity: %d%%",
               snap.wind_ms, snap.humidity);
    tip += extra;

    if (snap.rain_expected) {
        tip += ru ? L"\n\u041E\u0436\u0438\u0434\u0430\u044E\u0442\u0441\u044F \u043E\u0441\u0430\u0434\u043A\u0438 \u0432 \u0431\u043B\u0438\u0436\u0430\u0439\u0448\u0438\u0435 \u0447\u0430\u0441\u044B" : L"\nPrecipitation expected soon";
    }

    if (!snap.slots.empty()) {
        tip += L"\n";
        int shown = 0;
        for (const auto& sl : snap.slots) {
            if (shown >= 6) break;
            tip += L"\n";
            if (sl.is_tomorrow) tip += ru ? L"\u0417\u0430\u0432\u0442\u0440\u0430 \u00B7 " : L"Tomorrow \u00B7 ";
            tip += L10n::DayPartFull(cfg.lang, sl.part);
            tip += L": " + fmtT(sl.temp_c) + L" (" + L10n::WeatherName(cfg.lang, sl.kind) + L")";
            ++shown;
        }
    }

    if (!snap.updated_time.empty()) {
        tip += (ru ? L"\n\n\u041E\u0431\u043D\u043E\u0432\u043B\u0435\u043D\u043E: " : L"\n\nUpdated: ") + snap.updated_time;
    }

    m_tooltipCache = std::move(tip);
    return m_tooltipCache.c_str();
}

int WeatherPlugin::GetCommandCount() { return 1; }

const wchar_t* WeatherPlugin::GetCommandName(int command_index) {
    if (command_index == 0) {
        return GetConfig().lang == LangMode::RU ? L"\u041E\u0431\u043D\u043E\u0432\u0438\u0442\u044C \u043F\u043E\u0433\u043E\u0434\u0443 \u0441\u0435\u0439\u0447\u0430\u0441" : L"Refresh Weather Now";
    }
    return L"";
}

void WeatherPlugin::OnPluginCommand(int command_index, void* hWnd, void* para) {
    if (command_index == 0) TriggerRefresh();
}

PluginConfig WeatherPlugin::GetConfig() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_cfg;
}

WeatherSnapshot WeatherPlugin::GetSnapshot() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_snap;
}

int WeatherPlugin::GetScrollIndex() const {
    return m_scrollIndex.load();
}

void WeatherPlugin::StepScroll(int delta) {
    PluginConfig cfg = GetConfig();
    WeatherSnapshot snap = GetSnapshot();
    int visible = (cfg.display_mode == DisplayMode::Medium) ? 2 : cfg.extended_slots;
    int maxStart = std::max(0, (int)snap.slots.size() - visible);
    int cur = m_scrollIndex.load();
    m_scrollIndex = std::clamp(cur + delta, 0, maxStart);
}

void WeatherPlugin::CycleOrResetScroll() {
    PluginConfig cfg = GetConfig();
    WeatherSnapshot snap = GetSnapshot();
    int visible = (cfg.display_mode == DisplayMode::Medium) ? 2 : cfg.extended_slots;
    int maxStart = std::max(0, (int)snap.slots.size() - visible);
    if (maxStart <= 0) {
        m_scrollIndex = 0;
        return;
    }
    int next = m_scrollIndex.load() + 1;
    m_scrollIndex = (next > maxStart) ? 0 : next;
}

void WeatherPlugin::TriggerRefresh() {
    m_scrollIndex = 0;
    m_lastFetchTick = GetTickCount64();
    {
        std::lock_guard<std::mutex> lk(m_cvMutex);
        m_fetchRequested = true;
    }
    m_cv.notify_one();
}