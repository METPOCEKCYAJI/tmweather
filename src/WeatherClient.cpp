#include "WeatherClient.h"
#include <windows.h>
#include <winhttp.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <regex>
#include <sstream>

using json = nlohmann::json;

std::wstring GeoCandidate::DisplayLabel() const {
    std::wstring res = name;
    if (!admin1.empty() && admin1 != name) res += L", " + admin1;
    if (!country.empty()) res += L", " + country;
    wchar_t coords[64]{};
    swprintf_s(coords, L" (%.3f, %.3f)", lat, lon);
    res += coords;
    return res;
}

namespace {
std::string ToUtf8(const std::wstring& ws) {
    if (ws.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), nullptr, 0, nullptr, nullptr);
    std::string out(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), out.data(), len, nullptr, nullptr);
    return out;
}

std::wstring FromUtf8(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring out(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), out.data(), len);
    return out;
}

std::wstring UrlEncode(const std::wstring& ws) {
    std::string utf8 = ToUtf8(ws);
    std::ostringstream oss;
    for (unsigned char c : utf8) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            oss << c;
        } else if (c == ' ') {
            oss << "%20";
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", c);
            oss << buf;
        }
    }
    return FromUtf8(oss.str());
}

bool HttpRequest(const std::wstring& host, INTERNET_PORT port, const std::wstring& path,
                 const std::wstring& method, const std::wstring& headers,
                 const std::string& body, std::string& out_response, DWORD& out_status) {
    out_response.clear();
    out_status = 0;

    DWORD accessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
#ifdef WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY
    accessType = WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY;
#endif
    HINTERNET hSession = WinHttpOpen(L"TrafficMonitor-WeatherPlus/1.0", accessType, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    WinHttpSetTimeouts(hSession, 5000, 8000, 8000, 10000);
    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, method.c_str(), path.c_str(), nullptr,
                                            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    const wchar_t* hdrs = headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str();
    DWORD hdrLen = headers.empty() ? 0 : (DWORD)headers.size();
    LPVOID bodyPtr = body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(body.data());
    DWORD bodyLen = (DWORD)body.size();

    BOOL ok = WinHttpSendRequest(hRequest, hdrs, hdrLen, bodyPtr, bodyLen, bodyLen, 0);
    if (ok) ok = WinHttpReceiveResponse(hRequest, nullptr);

    if (ok) {
        DWORD statusSize = sizeof(out_status);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &out_status, &statusSize, WINHTTP_NO_HEADER_INDEX);

        DWORD avail = 0;
        while (WinHttpQueryDataAvailable(hRequest, &avail) && avail > 0) {
            std::string chunk(avail, '\0');
            DWORD read = 0;
            if (WinHttpReadData(hRequest, chunk.data(), avail, &read) && read > 0) {
                out_response.append(chunk.data(), read);
            } else {
                break;
            }
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return ok && out_status >= 200 && out_status < 300;
}

WeatherKind MapWmoCode(int code, bool is_day) {
    if (code == 0) return is_day ? WeatherKind::ClearDay : WeatherKind::ClearNight;
    if (code == 1 || code == 2) return is_day ? WeatherKind::PartlyCloudyDay : WeatherKind::PartlyCloudyNight;
    if (code == 3) return WeatherKind::Cloudy;
    if (code == 45 || code == 48) return WeatherKind::Fog;
    if (code == 51 || code == 53 || code == 55 || code == 56 || code == 57) return WeatherKind::Drizzle;
    if (code == 66 || code == 67) return WeatherKind::Sleet;
    if ((code >= 61 && code <= 65) || (code >= 80 && code <= 82)) return WeatherKind::Rain;
    if (code == 77) return WeatherKind::Hail;
    if ((code >= 71 && code <= 75) || code == 85 || code == 86) return WeatherKind::Snow;
    if (code == 96 || code == 99) return WeatherKind::StormHail;
    if (code == 95) return WeatherKind::Storm;
    return WeatherKind::Cloudy;
}

bool IsWmoPrecip(int code) { return (code >= 51 && code <= 99); }

WeatherKind MapYandexCondition(const std::string& cond, bool is_day) {
    if (cond == "CLEAR") return is_day ? WeatherKind::ClearDay : WeatherKind::ClearNight;
    if (cond == "PARTLY_CLOUDY") return is_day ? WeatherKind::PartlyCloudyDay : WeatherKind::PartlyCloudyNight;
    if (cond == "CLOUDY" || cond == "OVERCAST") return WeatherKind::Cloudy;
    if (cond == "FOG" || cond == "MIST") return WeatherKind::Fog;
    if (cond == "THUNDERSTORM_WITH_HAIL") return WeatherKind::StormHail;
    if (cond == "THUNDERSTORM" || cond == "THUNDERSTORM_WITH_RAIN") return WeatherKind::Storm;
    if (cond == "HAIL") return WeatherKind::Hail;
    if (cond == "WET_SNOW" || cond == "SLEET") return WeatherKind::Sleet;
    if (cond == "SNOW" || cond == "LIGHT_SNOW" || cond == "SNOW_SHOWERS" || cond == "SNOWFALL") return WeatherKind::Snow;
    if (cond == "LIGHT_RAIN" || cond == "DRIZZLE") return WeatherKind::Drizzle;
    if (cond.find("RAIN") != std::string::npos || cond == "SHOWERS") return WeatherKind::Rain;
    return WeatherKind::Cloudy;
}

bool IsYandexPrecip(const std::string& cond) {
    return cond.find("RAIN") != std::string::npos ||
           cond.find("SNOW") != std::string::npos ||
           cond.find("THUNDER") != std::string::npos ||
           cond == "DRIZZLE" || cond == "SHOWERS" || cond == "HAIL" || cond == "SLEET";
}

bool ParseIsoDateTime(const std::string& s, int& y, int& m, int& d, int& h) {
    if (s.size() < 13) return false;
    return sscanf_s(s.c_str(), "%d-%d-%dT%d", &y, &m, &d, &h) == 4;
}

time_t ToEpochHour(int y, int m, int d, int h) {
    std::tm t{};
    t.tm_year = y - 1900;
    t.tm_mon = m - 1;
    t.tm_mday = d;
    t.tm_hour = h;
    return _mkgmtime(&t);
}

DayPart HourToDayPart(int h) {
    if (h >= 6 && h < 12) return DayPart::Morning;
    if (h >= 12 && h < 18) return DayPart::Day;
    if (h >= 18 && h < 23) return DayPart::Evening;
    return DayPart::Night;
}

void AddDays(int y, int m, int d, int delta_days, int& oy, int& om, int& od) {
    std::tm t{};
    t.tm_year = y - 1900;
    t.tm_mon = m - 1;
    t.tm_mday = d + delta_days;
    t.tm_hour = 12;
    time_t tt = _mkgmtime(&t);
    std::tm out{};
    gmtime_s(&out, &tt);
    oy = out.tm_year + 1900;
    om = out.tm_mon + 1;
    od = out.tm_mday;
}

const HourlyPoint* FindTargetHour(const std::vector<HourlyPoint>& hours, int y, int m, int d, int target_h) {
    const HourlyPoint* best = nullptr;
    int best_diff = 99;
    for (const auto& p : hours) {
        if (p.year == y && p.month == m && p.day == d) {
            int diff = std::abs(p.hour - target_h);
            if (diff < best_diff) {
                best_diff = diff;
                best = &p;
            }
        }
    }
    return best;
}

void BuildSlotsFromHourly(const std::vector<HourlyPoint>& hours, int cur_y, int cur_m, int cur_d, int cur_h,
                          bool fill_tomorrow, WeatherSnapshot& snap) {
    snap.slots.clear();
    snap.alert_segments.clear();
    snap.local_hour = cur_h;

    time_t curEpoch = ToEpochHour(cur_y, cur_m, cur_d, cur_h);

    for (const auto& p : hours) {
        int hAhead = (int)((ToEpochHour(p.year, p.month, p.day, p.hour) - curEpoch) / 3600);
        if (hAhead >= 0 && hAhead <= 6 && p.is_precip) {
            snap.rain_expected = true;
        }
        if (hAhead > 0 && hAhead <= 7 * 24) {
            AlertSegment seg;
            seg.hours_ahead = hAhead;
            seg.day_offset = (hAhead - 1) / 24 + 1;
            seg.part = HourToDayPart(p.hour);
            seg.temp_c = p.temp_c;
            snap.alert_segments.push_back(seg);
        }
    }

    auto try_add = [&](int dy, int dm, int dd, int rep_hour, DayPart part, bool is_tomorrow) {
        if (const HourlyPoint* hp = FindTargetHour(hours, dy, dm, dd, rep_hour)) {
            ForecastSlot slot;
            slot.part = part;
            slot.is_today = !is_tomorrow;
            slot.is_tomorrow = is_tomorrow;
            slot.day_num = dd;
            slot.temp_c = hp->temp_c;
            slot.kind = hp->kind;
            snap.slots.push_back(slot);
        }
    };

    int t1y, t1m, t1d, t2y, t2m, t2d;
    AddDays(cur_y, cur_m, cur_d, 1, t1y, t1m, t1d);
    AddDays(cur_y, cur_m, cur_d, 2, t2y, t2m, t2d);

    if (cur_h < 6) {
        try_add(cur_y, cur_m, cur_d, 2, DayPart::Night, false);
        try_add(cur_y, cur_m, cur_d, 9, DayPart::Morning, false);
        try_add(cur_y, cur_m, cur_d, 15, DayPart::Day, false);
        try_add(cur_y, cur_m, cur_d, 20, DayPart::Evening, false);
    } else {
        if (cur_h < 12) try_add(cur_y, cur_m, cur_d, 9, DayPart::Morning, false);
        if (cur_h < 18) try_add(cur_y, cur_m, cur_d, 15, DayPart::Day, false);
        if (cur_h < 23) try_add(cur_y, cur_m, cur_d, 20, DayPart::Evening, false);
        try_add(t1y, t1m, t1d, 2, DayPart::Night, false);
    }

    if (fill_tomorrow) {
        if (cur_h < 6) {
            try_add(t1y, t1m, t1d, 2, DayPart::Night, true);
        }
        try_add(t1y, t1m, t1d, 9, DayPart::Morning, true);
        try_add(t1y, t1m, t1d, 15, DayPart::Day, true);
        try_add(t1y, t1m, t1d, 20, DayPart::Evening, true);
        try_add(t2y, t2m, t2d, 2, DayPart::Night, true);
        try_add(t2y, t2m, t2d, 9, DayPart::Morning, true);
    }
}

void StampLocalFetchTime(WeatherSnapshot& snap) {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t buf[16]{};
    swprintf_s(buf, L"%02d:%02d", st.wHour, st.wMinute);
    snap.updated_time = buf;
}
} // namespace

bool WeatherClient::TryParseCoordinatesOrGoogleLink(const std::wstring& input, double& out_lat, double& out_lon) {
    std::string s = ToUtf8(input);
    std::regex re_3d4d(R"(!3d(-?\d+(?:\.\d+)?)!4d(-?\d+(?:\.\d+)?))");
    std::smatch m;
    if (std::regex_search(s, m, re_3d4d)) {
        double lat = std::stod(m[1].str());
        double lon = std::stod(m[2].str());
        if (lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0) {
            out_lat = lat; out_lon = lon; return true;
        }
    }

    std::regex re_pair(R"((-?\d{1,2}\.\d+)\s*[,;\s]\s*(-?\d{1,3}\.\d+))");
    if (std::regex_search(s, m, re_pair)) {
        double lat = std::stod(m[1].str());
        double lon = std::stod(m[2].str());
        if (lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0) {
            out_lat = lat; out_lon = lon; return true;
        }
    }
    return false;
}

bool WeatherClient::ReverseGeocode(double lat, double lon, std::wstring& out_city, std::wstring& out_country) {
    wchar_t path[256]{};
    swprintf_s(path, L"/reverse?format=jsonv2&lat=%.6f&lon=%.6f&zoom=10", lat, lon);
    std::string resp;
    DWORD status = 0;
    if (!HttpRequest(L"nominatim.openstreetmap.org", INTERNET_DEFAULT_HTTPS_PORT, path, L"GET", L"", "", resp, status)) {
        return false;
    }
    try {
        auto j = json::parse(resp);
        if (j.contains("address")) {
            const auto& a = j["address"];
            std::string city;
            if (a.contains("city")) city = a["city"].get<std::string>();
            else if (a.contains("town")) city = a["town"].get<std::string>();
            else if (a.contains("village")) city = a["village"].get<std::string>();
            else if (a.contains("municipality")) city = a["municipality"].get<std::string>();
            else if (a.contains("state")) city = a["state"].get<std::string>();
            if (!city.empty()) out_city = FromUtf8(city);
            if (a.contains("country")) out_country = FromUtf8(a["country"].get<std::string>());
            return !out_city.empty();
        }
    } catch (...) {}
    return false;
}

std::vector<GeoCandidate> WeatherClient::SearchLocation(const std::wstring& query, const std::wstring& country, std::wstring& out_error) {
    std::vector<GeoCandidate> list;
    out_error.clear();

    double lat = 0.0, lon = 0.0;
    if (TryParseCoordinatesOrGoogleLink(query, lat, lon)) {
        GeoCandidate c;
        c.lat = lat;
        c.lon = lon;
        wchar_t fallback[48]{};
        swprintf_s(fallback, L"%.2f, %.2f", lat, lon);
        c.name = fallback;
        ReverseGeocode(lat, lon, c.name, c.country);
        list.push_back(c);
        return list;
    }

    if (query.empty()) return list;

    std::wstring path = L"/v1/search?name=" + UrlEncode(query) + L"&count=10&language=" +
                        (query[0] >= 0x0400 ? std::wstring(L"ru") : std::wstring(L"en")) + L"&format=json";

    std::string resp;
    DWORD status = 0;
    if (!HttpRequest(L"geocoding-api.open-meteo.com", INTERNET_DEFAULT_HTTPS_PORT, path, L"GET", L"", "", resp, status)) {
        out_error = L"Network error";
        return list;
    }

    try {
        auto j = json::parse(resp);
        if (!j.contains("results") || !j["results"].is_array()) return list;

        std::wstring country_lower = country;
        std::transform(country_lower.begin(), country_lower.end(), country_lower.begin(), ::towlower);

        for (const auto& item : j["results"]) {
            GeoCandidate c;
            c.name = FromUtf8(item.value("name", ""));
            c.country = FromUtf8(item.value("country", ""));
            std::wstring cc = FromUtf8(item.value("country_code", ""));
            c.admin1 = FromUtf8(item.value("admin1", ""));
            c.lat = item.value("latitude", 0.0);
            c.lon = item.value("longitude", 0.0);

            if (!country_lower.empty()) {
                std::wstring cl = c.country;
                std::wstring ccl = cc;
                std::transform(cl.begin(), cl.end(), cl.begin(), ::towlower);
                std::transform(ccl.begin(), ccl.end(), ccl.begin(), ::towlower);
                if (cl.find(country_lower) == std::wstring::npos && ccl != country_lower) continue;
            }
            list.push_back(c);
        }

        if (list.empty() && !country_lower.empty()) {
            for (const auto& item : j["results"]) {
                GeoCandidate c;
                c.name = FromUtf8(item.value("name", ""));
                c.country = FromUtf8(item.value("country", ""));
                c.admin1 = FromUtf8(item.value("admin1", ""));
                c.lat = item.value("latitude", 0.0);
                c.lon = item.value("longitude", 0.0);
                list.push_back(c);
            }
        }
    } catch (...) {
        out_error = L"Parse error";
    }
    return list;
}

WeatherSnapshot WeatherClient::FetchWeather(const PluginConfig& cfg) {
    WeatherSnapshot snap;
    snap.city_name = cfg.EffectiveCityName();
    snap.country_name = cfg.resolved_country;

    if (cfg.provider == ProviderType::Yandex && !cfg.yandex_api_key.empty()) {
        json reqBody = {
            {"query",
             "query Weather($lat: Float!, $lon: Float!) { "
             "weatherByPoint(request: { lat: $lat, lon: $lon }) { "
             "now { temperature feelsLike condition daytime humidity windSpeed } "
             "forecast { days(limit: 7) { time hours { time temperature condition } } } "
             "} }"},
            {"variables", {{"lat", cfg.latitude}, {"lon", cfg.longitude}}}
        };

        std::wstring hdr = L"Content-Type: application/json\r\nX-Yandex-Weather-Key: " + cfg.yandex_api_key + L"\r\n";
        std::string resp;
        DWORD status = 0;
        if (HttpRequest(L"api.weather.yandex.ru", INTERNET_DEFAULT_HTTPS_PORT, L"/graphql/query",
                        L"POST", hdr, reqBody.dump(), resp, status)) {
            try {
                auto j = json::parse(resp);
                if (j.contains("data") && j["data"].contains("weatherByPoint") && !j["data"]["weatherByPoint"].is_null()) {
                    const auto& wp = j["data"]["weatherByPoint"];
                    const auto& now = wp["now"];
                    std::string cond = now.value("condition", "CLOUDY");
                    bool is_day = (now.value("daytime", "DAY") == "DAY");

                    snap.current_temp_c = now.value("temperature", 0.0);
                    snap.feels_like_c = now.value("feelsLike", snap.current_temp_c);
                    snap.humidity = now.value("humidity", 0);
                    snap.wind_ms = now.value("windSpeed", 0.0);
                    snap.current_kind = MapYandexCondition(cond, is_day);
                    snap.rain_expected = IsYandexPrecip(cond);

                    std::vector<HourlyPoint> hours;
                    int cur_y = 0, cur_m = 0, cur_d = 0, cur_h = 12;
                    if (wp.contains("forecast") && wp["forecast"].contains("days")) {
                        const auto& days = wp["forecast"]["days"];
                        if (!days.empty()) {
                            std::string d0 = days[0].value("time", "");
                            ParseIsoDateTime(d0, cur_y, cur_m, cur_d, cur_h);
                            if (days[0].contains("hours") && !days[0]["hours"].empty()) {
                                int hy, hm, hd, hh;
                                if (ParseIsoDateTime(days[0]["hours"][0].value("time", ""), hy, hm, hd, hh)) {
                                    cur_y = hy; cur_m = hm; cur_d = hd; cur_h = hh;
                                }
                            }
                        }
                        for (const auto& dayObj : days) {
                            if (!dayObj.contains("hours")) continue;
                            for (const auto& hr : dayObj["hours"]) {
                                HourlyPoint hp;
                                if (!ParseIsoDateTime(hr.value("time", ""), hp.year, hp.month, hp.day, hp.hour)) continue;
                                hp.temp_c = hr.value("temperature", 0.0);
                                std::string hc = hr.value("condition", "CLOUDY");
                                bool hd_flag = (hp.hour >= 6 && hp.hour < 21);
                                hp.kind = MapYandexCondition(hc, hd_flag);
                                hp.is_precip = IsYandexPrecip(hc);
                                hours.push_back(hp);
                            }
                        }
                    }

                    BuildSlotsFromHourly(hours, cur_y, cur_m, cur_d, cur_h, cfg.fill_tomorrow, snap);
                    StampLocalFetchTime(snap);
                    snap.valid = true;
                    return snap;
                }
            } catch (...) {
                snap.error_msg = L"Yandex JSON error";
                return snap;
            }
        }
        snap.error_msg = (status == 401 || status == 403) ? L"Invalid Yandex API key" : L"Yandex network error";
        return snap;
    }

    wchar_t path[512]{};
    swprintf_s(path,
               L"/v1/forecast?latitude=%.5f&longitude=%.5f"
               L"&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,weather_code,wind_speed_10m"
               L"&hourly=temperature_2m,weather_code,is_day"
               L"&wind_speed_unit=ms&timezone=auto&forecast_days=7",
               cfg.latitude, cfg.longitude);

    std::string resp;
    DWORD status = 0;
    if (!HttpRequest(L"api.open-meteo.com", INTERNET_DEFAULT_HTTPS_PORT, path, L"GET", L"", "", resp, status)) {
        snap.error_msg = L"Open-Meteo network error";
        return snap;
    }

    try {
        auto j = json::parse(resp);
        const auto& cur = j.at("current");
        int cur_wmo = cur.value("weather_code", 0);
        bool cur_day = cur.value("is_day", 1) != 0;

        snap.current_temp_c = cur.value("temperature_2m", 0.0);
        snap.feels_like_c = cur.value("apparent_temperature", snap.current_temp_c);
        snap.humidity = cur.value("relative_humidity_2m", 0);
        snap.wind_ms = cur.value("wind_speed_10m", 0.0);
        snap.current_kind = MapWmoCode(cur_wmo, cur_day);
        snap.rain_expected = IsWmoPrecip(cur_wmo);

        int cur_y = 2026, cur_m = 1, cur_d = 1, cur_h = 12;
        ParseIsoDateTime(cur.value("time", ""), cur_y, cur_m, cur_d, cur_h);

        std::vector<HourlyPoint> hours;
        const auto& hr = j.at("hourly");
        const auto& times = hr.at("time");
        const auto& temps = hr.at("temperature_2m");
        const auto& codes = hr.at("weather_code");
        const auto& is_days = hr.value("is_day", json::array());

        size_t n = std::min({times.size(), temps.size(), codes.size()});
        hours.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            HourlyPoint hp;
            if (!ParseIsoDateTime(times[i].get<std::string>(), hp.year, hp.month, hp.day, hp.hour)) continue;
            hp.temp_c = temps[i].is_number() ? temps[i].get<double>() : 0.0;
            int wmo = codes[i].is_number() ? codes[i].get<int>() : 0;
            bool hd = (i < is_days.size() && is_days[i].is_number()) ? (is_days[i].get<int>() != 0) : (hp.hour >= 6 && hp.hour < 21);
            hp.kind = MapWmoCode(wmo, hd);
            hp.is_precip = IsWmoPrecip(wmo);
            hours.push_back(hp);
        }

        BuildSlotsFromHourly(hours, cur_y, cur_m, cur_d, cur_h, cfg.fill_tomorrow, snap);
        StampLocalFetchTime(snap);
        snap.valid = true;
    } catch (...) {
        snap.error_msg = L"Open-Meteo parse error";
    }
    return snap;
}
