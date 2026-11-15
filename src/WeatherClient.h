#pragma once
#include "Config.h"
#include "WeatherModels.h"
#include <vector>
#include <string>

class WeatherClient {
public:
    static bool TryParseCoordinatesOrGoogleLink(const std::wstring& input, double& out_lat, double& out_lon);
    static std::vector<GeoCandidate> SearchLocation(const std::wstring& query, const std::wstring& country, std::wstring& out_error);
    static bool ReverseGeocode(double lat, double lon, std::wstring& out_city, std::wstring& out_country);
    static WeatherSnapshot FetchWeather(const PluginConfig& cfg);
};