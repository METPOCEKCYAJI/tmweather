#include "Config.h"
#include <windows.h>
#include <algorithm>
#include <cwchar>

namespace {
std::wstring ReadStr(const wchar_t* ini, const wchar_t* key, const wchar_t* defVal) {
    wchar_t buf[1024]{};
    GetPrivateProfileStringW(L"WeatherPlus", key, defVal, buf, 1024, ini);
    return buf;
}

int ReadInt(const wchar_t* ini, const wchar_t* key, int defVal) {
    return GetPrivateProfileIntW(L"WeatherPlus", key, defVal, ini);
}

double ReadDouble(const wchar_t* ini, const wchar_t* key, double defVal) {
    wchar_t buf[64]{};
    GetPrivateProfileStringW(L"WeatherPlus", key, L"", buf, 64, ini);
    if (buf[0] == L'\0') return defVal;
    return wcstod(buf, nullptr);
}

void WriteStr(const wchar_t* ini, const wchar_t* key, const std::wstring& val) {
    WritePrivateProfileStringW(L"WeatherPlus", key, val.c_str(), ini);
}

void WriteInt(const wchar_t* ini, const wchar_t* key, int val) {
    WriteStr(ini, key, std::to_wstring(val));
}
} // namespace

std::wstring ConfigStore::GetIniPath(const std::wstring& config_dir) {
    if (config_dir.empty()) return L"WeatherPlus.ini";
    std::wstring path = config_dir;
    if (path.back() != L'\\' && path.back() != L'/') path += L'\\';
    path += L"WeatherPlus.ini";
    return path;
}

void ConfigStore::Load(const std::wstring& config_dir, PluginConfig& cfg) {
    std::wstring ini = GetIniPath(config_dir);
    const wchar_t* p = ini.c_str();

    cfg.lang = (ReadInt(p, L"lang", 0) == 1) ? LangMode::EN : LangMode::RU;
    cfg.provider = (ReadInt(p, L"provider", 0) == 1) ? ProviderType::Yandex : ProviderType::OpenMeteo;
    cfg.yandex_api_key = ReadStr(p, L"yandex_api_key", L"");

    cfg.search_query = ReadStr(p, L"search_query", L"");
    cfg.search_country = ReadStr(p, L"search_country", L"");
    cfg.custom_city_name = ReadStr(p, L"custom_city_name", L"");
    cfg.resolved_city = ReadStr(p, L"resolved_city", L"Berlin");
    cfg.resolved_country = ReadStr(p, L"resolved_country", L"Germany");
    cfg.latitude = ReadDouble(p, L"latitude", 52.5200);
    cfg.longitude = ReadDouble(p, L"longitude", 13.4050);

    cfg.unit = (ReadInt(p, L"unit", 0) == 1) ? TempUnit::Fahrenheit : TempUnit::Celsius;
    int dm = ReadInt(p, L"display_mode", 2);
    cfg.display_mode = static_cast<DisplayMode>(std::clamp(dm, 0, 2));
    cfg.extended_slots = (ReadInt(p, L"extended_slots", 4) == 3) ? 3 : 4;
    cfg.scroll_mode = (ReadInt(p, L"scroll_mode", 0) == 1) ? ScrollBehavior::AutoCarousel : ScrollBehavior::TimeAndWheel;
    int ist = ReadInt(p, L"icon_style", 1);
    cfg.icon_style = static_cast<IconStyle>(std::clamp(ist, 0, 6));
    int bs = ReadInt(p, L"border_style", 3);
    cfg.border_style = static_cast<BorderStyle>(std::clamp(bs, 0, 3));
    cfg.fill_tomorrow = (ReadInt(p, L"fill_tomorrow", 1) != 0);
    cfg.refresh_minutes = std::clamp(ReadInt(p, L"refresh_minutes", 30), 5, 360);
}

void ConfigStore::Save(const std::wstring& config_dir, const PluginConfig& cfg) {
    std::wstring ini = GetIniPath(config_dir);
    const wchar_t* p = ini.c_str();

    WriteInt(p, L"lang", static_cast<int>(cfg.lang));
    WriteInt(p, L"provider", static_cast<int>(cfg.provider));
    WriteStr(p, L"yandex_api_key", cfg.yandex_api_key);

    WriteStr(p, L"search_query", cfg.search_query);
    WriteStr(p, L"search_country", cfg.search_country);
    WriteStr(p, L"custom_city_name", cfg.custom_city_name);
    WriteStr(p, L"resolved_city", cfg.resolved_city);
    WriteStr(p, L"resolved_country", cfg.resolved_country);

    wchar_t coord[64]{};
    swprintf_s(coord, L"%.6f", cfg.latitude);
    WriteStr(p, L"latitude", coord);
    swprintf_s(coord, L"%.6f", cfg.longitude);
    WriteStr(p, L"longitude", coord);

    WriteInt(p, L"unit", static_cast<int>(cfg.unit));
    WriteInt(p, L"display_mode", static_cast<int>(cfg.display_mode));
    WriteInt(p, L"extended_slots", cfg.extended_slots);
    WriteInt(p, L"scroll_mode", static_cast<int>(cfg.scroll_mode));
    WriteInt(p, L"icon_style", static_cast<int>(cfg.icon_style));
    WriteInt(p, L"border_style", static_cast<int>(cfg.border_style));
    WriteInt(p, L"fill_tomorrow", cfg.fill_tomorrow ? 1 : 0);
    WriteInt(p, L"refresh_minutes", cfg.refresh_minutes);
}
