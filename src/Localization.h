#pragma once
#include "Config.h"
#include "WeatherModels.h"
#include <string>

struct L10n {
    static inline const wchar_t* PluginName(LangMode l) {
        return l == LangMode::RU ? L"\u041F\u043E\u0433\u043E\u0434\u0430 \u041F\u043B\u044E\u0441" : L"Weather Plus";
    }
    static inline const wchar_t* PluginDesc(LangMode l) {
        return l == LangMode::RU
            ? L"\u041F\u043E\u0433\u043E\u0434\u0430 \u0432 \u043F\u0430\u043D\u0435\u043B\u0438 \u0437\u0430\u0434\u0430\u0447 (Open-Meteo \u0438 \u042F\u043D\u0434\u0435\u043A\u0441.\u041F\u043E\u0433\u043E\u0434\u0430)"
            : L"Taskbar weather widget (Open-Meteo & Yandex Weather)";
    }
    static inline const wchar_t* ItemName(LangMode l) {
        return l == LangMode::RU ? L"\u041F\u043E\u0433\u043E\u0434\u0430 (\u0431\u043B\u043E\u043A)" : L"Weather Widget";
    }
    static inline const wchar_t* DayPartShort(LangMode l, DayPart p) {
        if (l == LangMode::RU) {
            switch (p) {
            case DayPart::Morning: return L"\u0423";
            case DayPart::Day:     return L"\u0414";
            case DayPart::Evening: return L"\u0412";
            case DayPart::Night:   return L"\u041D";
            }
        } else {
            switch (p) {
            case DayPart::Morning: return L"M";
            case DayPart::Day:     return L"D";
            case DayPart::Evening: return L"E";
            case DayPart::Night:   return L"N";
            }
        }
        return L"";
    }
    static inline const wchar_t* DayPartFull(LangMode l, DayPart p) {
        if (l == LangMode::RU) {
            switch (p) {
            case DayPart::Morning: return L"\u0423\u0442\u0440\u043E";
            case DayPart::Day:     return L"\u0414\u0435\u043D\u044C";
            case DayPart::Evening: return L"\u0412\u0435\u0447\u0435\u0440";
            case DayPart::Night:   return L"\u041D\u043E\u0447\u044C";
            }
        } else {
            switch (p) {
            case DayPart::Morning: return L"Morn";
            case DayPart::Day:     return L"Day";
            case DayPart::Evening: return L"Eve";
            case DayPart::Night:   return L"Night";
            }
        }
        return L"";
    }
    static inline const wchar_t* WeatherName(LangMode l, WeatherKind k) {
        if (l == LangMode::RU) {
            switch (k) {
            case WeatherKind::ClearDay:
            case WeatherKind::ClearNight:        return L"\u042F\u0441\u043D\u043E";
            case WeatherKind::PartlyCloudyDay:
            case WeatherKind::PartlyCloudyNight: return L"\u041C\u0430\u043B\u043E\u043E\u0431\u043B\u0430\u0447\u043D\u043E";
            case WeatherKind::Cloudy:            return L"\u041E\u0431\u043B\u0430\u0447\u043D\u043E";
            case WeatherKind::Fog:               return L"\u0422\u0443\u043C\u0430\u043D";
            case WeatherKind::Drizzle:           return L"\u041C\u043E\u0440\u043E\u0441\u044C";
            case WeatherKind::Rain:              return L"\u0414\u043E\u0436\u0434\u044C";
            case WeatherKind::Sleet:             return L"\u0414\u043E\u0436\u0434\u044C \u0441\u043E \u0441\u043D\u0435\u0433\u043E\u043C";
            case WeatherKind::Snow:              return L"\u0421\u043D\u0435\u0433";
            case WeatherKind::Hail:              return L"\u0413\u0440\u0430\u0434";
            case WeatherKind::Storm:             return L"\u0413\u0440\u043E\u0437\u0430";
            case WeatherKind::StormHail:         return L"\u0413\u0440\u043E\u0437\u0430 \u0441 \u0433\u0440\u0430\u0434\u043E\u043C";
            default:                             return L"\u041D\u0435\u0442 \u0434\u0430\u043D\u043D\u044B\u0445";
            }
        } else {
            switch (k) {
            case WeatherKind::ClearDay:
            case WeatherKind::ClearNight:        return L"Clear";
            case WeatherKind::PartlyCloudyDay:
            case WeatherKind::PartlyCloudyNight: return L"Partly cloudy";
            case WeatherKind::Cloudy:            return L"Cloudy";
            case WeatherKind::Fog:               return L"Fog";
            case WeatherKind::Drizzle:           return L"Drizzle";
            case WeatherKind::Rain:              return L"Rain";
            case WeatherKind::Sleet:             return L"Sleet";
            case WeatherKind::Snow:              return L"Snow";
            case WeatherKind::Hail:              return L"Hail";
            case WeatherKind::Storm:             return L"Thunderstorm";
            case WeatherKind::StormHail:         return L"Storm with hail";
            default:                             return L"N/A";
            }
        }
    }
};
