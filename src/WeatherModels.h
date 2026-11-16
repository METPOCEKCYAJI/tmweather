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
    Drizzle,
    Rain,
    Sleet,
    Snow,
    Hail,
    Storm,
    StormHail
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
    bool is_today = true;
    bool is_tomorrow = false;
    int day_num = 0;
    double temp_c = 0.0;
    WeatherKind kind = WeatherKind::Unknown;
};

struct AlertSegment {
    int hours_ahead = 0;
    int day_offset = 0;
    DayPart part = DayPart::Day;
    double temp_c = 0.0;
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
    std::vector<AlertSegment> alert_segments;
};
