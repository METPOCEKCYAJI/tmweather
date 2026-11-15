#pragma once
#include <string>
#include <vector>

enum class WeatherKind {
    Unknown = 0,
    ClearDay,
    ClearNight,
    PartlyCloudyDay,
    PartlyCloudyNight,
    Cloudy,
    Fog,
    Rain,
    Snow,
    Storm
};

enum class DayPart {
    Morning = 0,
    Day,
    Evening,
    Night
};

struct HourlyPoint {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    double temp_c = 0.0;
    WeatherKind kind = WeatherKind::Unknown;
    bool is_precip = false;
};

struct ForecastSlot {
    DayPart part = DayPart::Day;
    bool is_tomorrow = false;
    double temp_c = 0.0;
    WeatherKind kind = WeatherKind::Unknown;
};

struct GeoCandidate {
    std::wstring name;
    std::wstring country;
    std::wstring admin1;
    double lat = 0.0;
    double lon = 0.0;

    std::wstring DisplayLabel() const;
};

struct WeatherSnapshot {
    bool valid = false;
    bool loading = false;
    std::wstring error_msg;

    std::wstring city_name;
    std::wstring country_name;
    double current_temp_c = 0.0;
    double feels_like_c = 0.0;
    double wind_ms = 0.0;
    int humidity = 0;
    WeatherKind current_kind = WeatherKind::Unknown;
    bool rain_expected = false;
    int local_hour = 12;
    std::wstring updated_time;

    std::vector<ForecastSlot> slots;
};