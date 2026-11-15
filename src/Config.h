#pragma once
#include <string>

enum class LangMode { RU = 0, EN = 1 };
enum class ProviderType { OpenMeteo = 0, Yandex = 1 };
enum class TempUnit { Celsius = 0, Fahrenheit = 1 };
enum class DisplayMode { Compact = 0, Medium = 1, Extended = 2 };
enum class ScrollBehavior { TimeAndWheel = 0, AutoCarousel = 1 };
enum class IconStyle { BoldVivid = 0, ThickOutline = 1, Soft3D = 2, HighContrastWhite = 3, Monochrome = 4 };
enum class BorderStyle { BorderAndFill = 0, FillOnly = 1, Minimal = 2 };

struct PluginConfig {
    LangMode lang = LangMode::RU;
    ProviderType provider = ProviderType::OpenMeteo;
    std::wstring yandex_api_key;

    std::wstring search_query;
    std::wstring search_country;
    std::wstring custom_city_name;
    std::wstring resolved_city = L"Berlin";
    std::wstring resolved_country = L"Germany";
    double latitude = 52.5200;
    double longitude = 13.4050;

    TempUnit unit = TempUnit::Celsius;
    DisplayMode display_mode = DisplayMode::Extended;
    int extended_slots = 4;
    ScrollBehavior scroll_mode = ScrollBehavior::TimeAndWheel;
    IconStyle icon_style = IconStyle::BoldVivid;
    BorderStyle border_style = BorderStyle::BorderAndFill;
    bool fill_tomorrow = true;
    int refresh_minutes = 30;

    std::wstring EffectiveCityName() const {
        if (!custom_city_name.empty()) return custom_city_name;
        if (!resolved_city.empty()) return resolved_city;
        return L"Weather";
    }
};

class ConfigStore {
public:
    static void Load(const std::wstring& config_dir, PluginConfig& cfg);
    static void Save(const std::wstring& config_dir, const PluginConfig& cfg);
private:
    static std::wstring GetIniPath(const std::wstring& config_dir);
};