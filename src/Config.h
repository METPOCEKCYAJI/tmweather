#pragma once
#include <string>

enum class LangMode { RU = 0, EN = 1 };
enum class ProviderType { OpenMeteo = 0, Yandex = 1 };
enum class TempUnit { Celsius = 0, Fahrenheit = 1 };
enum class DisplayMode { Compact = 0, Medium = 1, Extended = 2 };
enum class ScrollBehavior { TimeAndWheel = 0, AutoCarousel = 1 };
enum class IconStyle {
    SkyCards = 0,
    TopBar = 1,
    BottomBar = 2,
    GlyphTop = 3,
    LeftPill = 4,
    Watermark = 5,
    IconTopLabelBottom = 6
};
enum class BorderStyle {
    BorderAndFill = 0,
    FillOnly = 1,
    DividersOnly = 2,
    TransparentClean = 3
};
enum class AlertMode {
    Off = 0,
    Arrow = 1,
    Thermometer = 2
};
enum class AlertPos {
    LeftTop = 0,
    LeftCenter = 1,
    LeftBottom = 2,
    RightTop = 3,
    RightCenter = 4,
    RightBottom = 5
};

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
    IconStyle icon_style = IconStyle::SkyCards;
    int vignette_level = 3; // 0=Off, 1=Weak, 2=Medium, 3=Strong, 4=Max
    int bar_thickness = 1;  // 0=2px, 1=3px, 2=4px, 3=5px
    int bar_contrast = 1;   // 0=Normal, 1=Bright, 2=Ultra
    BorderStyle border_style = BorderStyle::TransparentClean;

    AlertMode alert_mode = AlertMode::Thermometer;
    AlertPos alert_pos = AlertPos::LeftTop;
    int alert_days = 3;
    int alert_threshold = 5;

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
