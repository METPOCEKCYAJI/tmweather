#include "OptionsDialog.h"
#include "WeatherClient.h"
#include "WeatherPlugin.h"
#include "WeatherItem.h"
#include "resource.h"
#include <commctrl.h>
#include <algorithm>
#include <cmath>
#include <vector>

#ifndef EM_SETCUEBANNER
#define EM_SETCUEBANNER 0x1501
#endif

namespace {
struct DialogState {
    PluginConfig cfg;
    PluginConfig live_cfg;
    WeatherSnapshot preview_snap;
    std::vector<GeoCandidate> candidates;
    bool preview_dark = true;
    bool in_init = false;
};

WeatherSnapshot MakeSampleSnapshot(const PluginConfig& cfg) {
    WeatherSnapshot s;
    s.valid = true;
    s.city_name = cfg.EffectiveCityName();
    s.current_temp_c = 7.0;
    s.current_kind = WeatherKind::Rain;
    s.rain_expected = true;
    s.slots = {
        { DayPart::Morning, true,  false, 16, 9.0,  WeatherKind::ClearDay },
        { DayPart::Day,     true,  false, 16, 12.0, WeatherKind::PartlyCloudyDay },
        { DayPart::Evening, true,  false, 16, 8.0,  WeatherKind::Storm },
        { DayPart::Night,   true,  false, 16, 4.0,  WeatherKind::ClearNight },
        { DayPart::Morning, false, true,  17, -2.0, WeatherKind::Snow }
    };
    s.daily = {
        { 0, 16, 9.0 },
        { 1, 17, 1.0 }
    };
    return s;
}

void SetCue(HWND hDlg, int id, const wchar_t* hint) {
    SendDlgItemMessageW(hDlg, id, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(hint));
}

std::wstring GetEdit(HWND hDlg, int id) {
    wchar_t buf[512]{};
    GetDlgItemTextW(hDlg, id, buf, 512);
    return buf;
}

void ReadSettingsFromControls(HWND hDlg, DialogState* st, PluginConfig& dst) {
    dst.lang = (SendDlgItemMessageW(hDlg, IDC_COMBO_LANG, CB_GETCURSEL, 0, 0) == 1) ? LangMode::EN : LangMode::RU;
    dst.provider = (SendDlgItemMessageW(hDlg, IDC_COMBO_PROVIDER, CB_GETCURSEL, 0, 0) == 1) ? ProviderType::Yandex : ProviderType::OpenMeteo;
    dst.yandex_api_key = GetEdit(hDlg, IDC_EDIT_API_KEY);
    dst.search_query = GetEdit(hDlg, IDC_EDIT_QUERY);
    dst.search_country = GetEdit(hDlg, IDC_EDIT_COUNTRY);
    dst.custom_city_name = GetEdit(hDlg, IDC_EDIT_CUSTOM_CITY);

    int candIdx = (int)SendDlgItemMessageW(hDlg, IDC_COMBO_RESULTS, CB_GETCURSEL, 0, 0);
    if (candIdx >= 0 && candIdx < (int)st->candidates.size()) {
        dst.resolved_city = st->candidates[candIdx].name;
        dst.resolved_country = st->candidates[candIdx].country;
        dst.latitude = st->candidates[candIdx].lat;
        dst.longitude = st->candidates[candIdx].lon;
    }

    dst.unit = (SendDlgItemMessageW(hDlg, IDC_COMBO_UNITS, CB_GETCURSEL, 0, 0) == 1) ? TempUnit::Fahrenheit : TempUnit::Celsius;
    dst.display_mode = static_cast<DisplayMode>(std::clamp((int)SendDlgItemMessageW(hDlg, IDC_COMBO_MODE, CB_GETCURSEL, 0, 0), 0, 2));
    dst.extended_slots = (SendDlgItemMessageW(hDlg, IDC_COMBO_EXT_SLOTS, CB_GETCURSEL, 0, 0) == 0) ? 3 : 4;
    dst.scroll_mode = (SendDlgItemMessageW(hDlg, IDC_COMBO_SCROLL, CB_GETCURSEL, 0, 0) == 1) ? ScrollBehavior::AutoCarousel : ScrollBehavior::TimeAndWheel;
    dst.icon_style = static_cast<IconStyle>(std::clamp((int)SendDlgItemMessageW(hDlg, IDC_COMBO_ICON_STYLE, CB_GETCURSEL, 0, 0), 0, 6));
    dst.vignette_level = std::clamp((int)SendDlgItemMessageW(hDlg, IDC_COMBO_VIGNETTE, CB_GETCURSEL, 0, 0), 0, 4);
    dst.bar_thickness = std::clamp((int)SendDlgItemMessageW(hDlg, IDC_COMBO_BAR_THICK, CB_GETCURSEL, 0, 0), 0, 3);
    dst.bar_contrast = std::clamp((int)SendDlgItemMessageW(hDlg, IDC_COMBO_BAR_CONTRAST, CB_GETCURSEL, 0, 0), 0, 2);
    dst.border_style = static_cast<BorderStyle>(std::clamp((int)SendDlgItemMessageW(hDlg, IDC_COMBO_BORDER, CB_GETCURSEL, 0, 0), 0, 3));

    dst.alert_mode = static_cast<AlertMode>(std::clamp((int)SendDlgItemMessageW(hDlg, IDC_COMBO_ALERT, CB_GETCURSEL, 0, 0), 0, 2));
    dst.alert_pos = static_cast<AlertPos>(std::clamp((int)SendDlgItemMessageW(hDlg, IDC_COMBO_ALERT_POS, CB_GETCURSEL, 0, 0), 0, 5));

    BOOL okD = FALSE;
    int ad = (int)GetDlgItemInt(hDlg, IDC_EDIT_ALERT_DAYS, &okD, FALSE);
    dst.alert_days = okD ? std::clamp(ad, 1, 7) : 3;

    BOOL okT = FALSE;
    int at = (int)GetDlgItemInt(hDlg, IDC_EDIT_ALERT_DIFF, &okT, FALSE);
    dst.alert_threshold = okT ? std::clamp(at, 1, 40) : 5;

    dst.fill_tomorrow = (SendDlgItemMessageW(hDlg, IDC_CHK_FILL_TOMORROW, BM_GETCHECK, 0, 0) == BST_CHECKED);

    BOOL okR = FALSE;
    int mins = (int)GetDlgItemInt(hDlg, IDC_EDIT_REFRESH, &okR, FALSE);
    dst.refresh_minutes = okR ? std::clamp(mins, 5, 360) : 30;
}

void RefreshPreview(HWND hDlg, DialogState* st) {
    if (!st || st->in_init) return;
    ReadSettingsFromControls(hDlg, st, st->live_cfg);
    InvalidateRect(GetDlgItem(hDlg, IDC_PREVIEW_BOX), nullptr, FALSE);
}

void ApplyLanguageTexts(HWND hDlg, LangMode lang, DialogState* st) {
    st->in_init = true;
    bool ru = (lang == LangMode::RU);
    SetWindowTextW(hDlg, ru ? L"\u041D\u0430\u0441\u0442\u0440\u043E\u0439\u043A\u0438 \u041F\u043E\u0433\u043E\u0434\u0430 \u041F\u043B\u044E\u0441" : L"WeatherPlus Settings");
    SetDlgItemTextW(hDlg, IDC_BTN_PREVIEW_THEME, ru ? L"\u0422\u0451\u043C\u043D./\u0421\u0432\u0435\u0442\u043B." : L"Dark / Light");

    SetDlgItemTextW(hDlg, IDC_LBL_LANG, ru ? L"\u042F\u0437\u044B\u043A:" : L"Language:");
    SetDlgItemTextW(hDlg, IDC_LBL_PROVIDER, ru ? L"\u041F\u043E\u0441\u0442\u0430\u0432\u0449\u0438\u043A:" : L"Provider:");
    SetDlgItemTextW(hDlg, IDC_LBL_API_KEY, ru ? L"\u041A\u043B\u044E\u0447 \u042F\u043D\u0434\u0435\u043A\u0441\u0430:" : L"Yandex Key:");
    SetDlgItemTextW(hDlg, IDC_LBL_LOCATION, ru ? L"\u0413\u043E\u0440\u043E\u0434 / \u041A\u043E\u043E\u0440\u0434.:" : L"City / Coords:");
    SetDlgItemTextW(hDlg, IDC_BTN_SEARCH, ru ? L"\u041D\u0430\u0439\u0442\u0438" : L"Find");
    SetDlgItemTextW(hDlg, IDC_LBL_RESULTS, ru ? L"\u041D\u0430\u0439\u0434\u0435\u043D\u043E:" : L"Matched:");
    SetDlgItemTextW(hDlg, IDC_LBL_CUSTOM_CITY, ru ? L"\u0418\u043C\u044F \u043D\u0430 \u043F\u0430\u043D\u0435\u043B\u0438:" : L"Display City:");
    SetDlgItemTextW(hDlg, IDC_LBL_UNITS, ru ? L"\u0415\u0434\u0438\u043D\u0438\u0446\u044B / \u0420\u0435\u0436\u0438\u043C:" : L"Units / Mode:");
    SetDlgItemTextW(hDlg, IDC_LBL_EXT_SLOTS, ru ? L"\u041F\u043B\u0438\u0442\u043A\u0438 / \u0421\u043A\u0440\u043E\u043B\u043B:" : L"Slots / Scroll:");
    SetDlgItemTextW(hDlg, IDC_LBL_ICON_STYLE, ru ? L"\u0421\u0442\u0438\u043B\u044C:" : L"Style:");
    SetDlgItemTextW(hDlg, IDC_LBL_VIGNETTE, ru ? L"\u0412\u0438\u043D\u044C\u0435\u0442\u043A\u0430:" : L"Vignette:");
    SetDlgItemTextW(hDlg, IDC_LBL_BAR_OPT, ru ? L"\u041F\u043E\u043B\u043E\u0441\u044B (\u0422\u043E\u043B\u0449. / \u0426\u0432\u0435\u0442):" : L"Bars (Size / Color):");
    SetDlgItemTextW(hDlg, IDC_LBL_BORDER, ru ? L"\u0420\u0430\u043C\u043A\u0430 \u0438 \u0444\u043E\u043D:" : L"Border & Bg:");
    SetDlgItemTextW(hDlg, IDC_LBL_ALERT, ru ? L"\u041F\u0435\u0440\u0435\u043F\u0430\u0434 \u0442\u0435\u043C\u043F.:" : L"Temp Alert:");
    SetDlgItemTextW(hDlg, IDC_LBL_ALERT_DAYS, ru ? L"\u0434\u043D." : L"d.");
    SetDlgItemTextW(hDlg, IDC_LBL_ALERT_DEG, L"\u00B0");
    SetDlgItemTextW(hDlg, IDC_LBL_ALERT_POS, ru ? L"\u0423\u0433\u043E\u043B \u043F\u0435\u0440\u0435\u043F\u0430\u0434\u0430:" : L"Alert Position:");
    SetDlgItemTextW(hDlg, IDC_CHK_FILL_TOMORROW, ru ? L"\u0414\u043E\u043F\u043E\u043B\u043D\u044F\u0442\u044C \u0437\u0430\u0432\u0442\u0440\u0430\u0448\u043D\u0438\u043C\u0438 \u0438\u043D\u0442\u0435\u0440\u0432\u0430\u043B\u0430\u043C\u0438" : L"Fill with tomorrow slots");
    SetDlgItemTextW(hDlg, IDC_LBL_REFRESH, ru ? L"\u041E\u0431\u043D\u043E\u0432\u043B\u0435\u043D\u0438\u0435 (\u043C\u0438\u043D):" : L"Refresh (min):");
    SetDlgItemTextW(hDlg, IDOK, L"OK");
    SetDlgItemTextW(hDlg, IDCANCEL, ru ? L"\u041E\u0442\u043C\u0435\u043D\u0430" : L"Cancel");

    SetCue(hDlg, IDC_EDIT_API_KEY, ru ? L"\u0412\u0441\u0442\u0430\u0432\u044C\u0442\u0435 X-Yandex-Weather-Key (\u0434\u043B\u044F Open-Meteo \u043D\u0435 \u043D\u0443\u0436\u0435\u043D)" : L"Paste X-Yandex-Weather-Key (not needed for Open-Meteo)");
    SetCue(hDlg, IDC_EDIT_QUERY, ru ? L"\u041D\u0430\u043F\u0440.: Berlin  \u0438\u043B\u0438  52.5200, 13.4050  \u0438\u043B\u0438 \u0441\u0441\u044B\u043B\u043A\u0430 Google Maps" : L"e.g. Berlin  or  52.5200, 13.4050  or Google Maps URL");
    SetCue(hDlg, IDC_EDIT_COUNTRY, ru ? L"\u0421\u0442\u0440\u0430\u043D\u0430 (\u043D\u0435\u043E\u0431\u044F\u0437\u0430\u0442\u0435\u043B\u044C\u043D\u043E), \u043D\u0430\u043F\u0440.: Germany \u0438\u043B\u0438 DE" : L"Country (optional), e.g. Germany or DE");
    SetCue(hDlg, IDC_EDIT_CUSTOM_CITY, ru ? L"\u041E\u0441\u0442\u0430\u0432\u044C\u0442\u0435 \u043F\u0443\u0441\u0442\u044B\u043C \u0434\u043B\u044F \u0430\u0432\u0442\u043E-\u0438\u043C\u0435\u043D\u0438" : L"Leave empty for auto name");
    SetCue(hDlg, IDC_EDIT_ALERT_DAYS, L"3");
    SetCue(hDlg, IDC_EDIT_ALERT_DIFF, L"5");
    SetCue(hDlg, IDC_EDIT_REFRESH, L"30");

    auto refillCombo = [&](int id, const std::vector<const wchar_t*>& items, int fallbackSel) {
        HWND h = GetDlgItem(hDlg, id);
        int cur = (int)SendMessageW(h, CB_GETCURSEL, 0, 0);
        if (cur < 0) cur = fallbackSel;
        SendMessageW(h, CB_RESETCONTENT, 0, 0);
        for (const auto* s : items) SendMessageW(h, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s));
        SendMessageW(h, CB_SETCURSEL, std::clamp(cur, 0, (int)items.size() - 1), 0);
    };

    refillCombo(IDC_COMBO_PROVIDER,
        ru ? std::vector<const wchar_t*>{L"Open-Meteo (\u0431\u0435\u0441\u043F\u043B\u0430\u0442\u043D\u043E, \u0431\u0435\u0437 \u043A\u043B\u044E\u0447\u0430)", L"\u042F\u043D\u0434\u0435\u043A\u0441.\u041F\u043E\u0433\u043E\u0434\u0430 (\u043D\u0443\u0436\u0435\u043D API-\u043A\u043B\u044E\u0447)"}
           : std::vector<const wchar_t*>{L"Open-Meteo (free, no key)", L"Yandex Weather (API key)"},
        (int)st->cfg.provider);

    refillCombo(IDC_COMBO_UNITS, {L"Celsius (\u00B0C)", L"Fahrenheit (\u00B0F)"}, (int)st->cfg.unit);

    refillCombo(IDC_COMBO_MODE,
        ru ? std::vector<const wchar_t*>{L"\u041A\u0440\u0430\u0442\u043A\u0438\u0439", L"\u0421\u0440\u0435\u0434\u043D\u0438\u0439 (2)", L"\u0420\u0430\u0441\u0448\u0438\u0440\u0435\u043D\u043D\u044B\u0439"}
           : std::vector<const wchar_t*>{L"Compact", L"Medium (2)", L"Extended"},
        (int)st->cfg.display_mode);

    refillCombo(IDC_COMBO_EXT_SLOTS,
        ru ? std::vector<const wchar_t*>{L"3 \u043F\u043B\u0438\u0442\u043A\u0438 (\u0443\u0436\u0435)", L"4 \u043F\u043B\u0438\u0442\u043A\u0438"}
           : std::vector<const wchar_t*>{L"3 tiles", L"4 tiles"},
        st->cfg.extended_slots == 3 ? 0 : 1);

    refillCombo(IDC_COMBO_SCROLL,
        ru ? std::vector<const wchar_t*>{L"\u041F\u043E \u0432\u0440\u0435\u043C\u0435\u043D\u0438 + \u043A\u043E\u043B\u0451\u0441\u0438\u043A\u043E", L"\u0410\u0432\u0442\u043E-\u043A\u0430\u0440\u0443\u0441\u0435\u043B\u044C (4\u0441)"}
           : std::vector<const wchar_t*>{L"Time + wheel", L"Auto-carousel"},
        (int)st->cfg.scroll_mode);

    refillCombo(IDC_COMBO_ICON_STYLE,
        ru ? std::vector<const wchar_t*>{
                L"\u0410\u0442\u043C\u043E\u0441\u0444\u0435\u0440\u043D\u044B\u0435 \u043A\u0430\u0440\u0442\u043E\u0447\u043A\u0438 (1\u0410)",
                L"\u041F\u043E\u043B\u043E\u0441\u0430 \u043F\u043E\u0433\u043E\u0434\u044B \u0441\u0432\u0435\u0440\u0445\u0443 (A)",
                L"\u041F\u043E\u043B\u043E\u0441\u0430 \u043F\u043E\u0433\u043E\u0434\u044B \u0441\u043D\u0438\u0437\u0443 (B)",
                L"\u0417\u043D\u0430\u0447\u043E\u043A + \u0432\u0440\u0435\u043C\u044F \u0441\u0432\u0435\u0440\u0445\u0443 (C)",
                L"\u041C\u0435\u0442\u043A\u0430 \u0441\u043B\u0435\u0432\u0430 \u043E\u0442 \u043A\u043E\u043B\u043E\u043D\u043A\u0438 (D)",
                L"\u041A\u0440\u0443\u043F\u043D\u044B\u0439 \u0437\u043D\u0430\u0447\u043E\u043A \u043D\u0430 \u0444\u043E\u043D\u0435 (E)",
                L"\u0417\u043D\u0430\u0447\u043E\u043A 16x16 \u0441\u0432\u0435\u0440\u0445\u0443 (F)"}
           : std::vector<const wchar_t*>{
                L"Sky Atmosphere Cards (1A)",
                L"Top Weather Bar (A)",
                L"Bottom Weather Bar (B)",
                L"Glyph + Time on Top (C)",
                L"Left Weather Pill (D)",
                L"Large Watermark Icon (E)",
                L"16x16 Icon Top + Temp (F)"},
        (int)st->cfg.icon_style);

    refillCombo(IDC_COMBO_VIGNETTE,
        ru ? std::vector<const wchar_t*>{L"\u0412\u044B\u043A\u043B", L"\u0421\u043B\u0430\u0431\u0430\u044F", L"\u0421\u0440\u0435\u0434\u043D\u044F\u044F", L"\u0421\u0438\u043B\u044C\u043D\u0430\u044F (\u0440\u0435\u043A.)", L"\u041C\u0430\u043A\u0441\u0438\u043C\u0430\u043B\u044C\u043D\u0430\u044F"}
           : std::vector<const wchar_t*>{L"Off", L"Weak", L"Medium", L"Strong (rec.)", L"Maximum"},
        st->cfg.vignette_level);

    refillCombo(IDC_COMBO_BAR_THICK,
        ru ? std::vector<const wchar_t*>{L"2 px (\u043C\u0438\u043D.)", L"3 px (+1)", L"4 px (+2)", L"5 px (+3)"}
           : std::vector<const wchar_t*>{L"2 px (min)", L"3 px (+1)", L"4 px (+2)", L"5 px (+3)"},
        st->cfg.bar_thickness);

    refillCombo(IDC_COMBO_BAR_CONTRAST,
        ru ? std::vector<const wchar_t*>{L"\u041E\u0431\u044B\u0447\u043D\u0430\u044F", L"\u042F\u0440\u043A\u0430\u044F", L"\u0423\u043B\u044C\u0442\u0440\u0430"}
           : std::vector<const wchar_t*>{L"Normal", L"Bright", L"Ultra"},
        st->cfg.bar_contrast);

    refillCombo(IDC_COMBO_BORDER,
        ru ? std::vector<const wchar_t*>{
                L"\u0420\u0430\u043C\u043A\u0430, \u0444\u043E\u043D \u0438 \u0440\u0430\u0437\u0434\u0435\u043B\u0438\u0442\u0435\u043B\u0438",
                L"\u0424\u043E\u043D \u0438 \u0440\u0430\u0437\u0434\u0435\u043B\u0438\u0442\u0435\u043B\u0438 (\u0431\u0435\u0437 \u0440\u0430\u043C\u043A\u0438)",
                L"\u0422\u043E\u043B\u044C\u043A\u043E \u0440\u0430\u0437\u0434\u0435\u043B\u0438\u0442\u0435\u043B\u0438 (\u0431\u0435\u0437 \u0444\u043E\u043D\u0430)",
                L"\u0411\u0435\u0437 \u0440\u0430\u043C\u043A\u0438, \u0444\u043E\u043D\u0430 \u0438 \u0440\u0430\u0437\u0434\u0435\u043B\u0438\u0442\u0435\u043B\u0435\u0439"}
           : std::vector<const wchar_t*>{
                L"Border, fill & dividers",
                L"Fill & dividers (no border)",
                L"Dividers only (no fill)",
                L"Clean (no border, fill or dividers)"},
        (int)st->cfg.border_style);

    refillCombo(IDC_COMBO_ALERT,
        ru ? std::vector<const wchar_t*>{L"\u041E\u0442\u043A\u043B", L"\u0421\u0442\u0440\u0435\u043B\u043A\u0430", L"\u0413\u0440\u0430\u0434\u0443\u0441\u043D\u0438\u043A"}
           : std::vector<const wchar_t*>{L"Off", L"Arrow", L"Thermometer"},
        (int)st->cfg.alert_mode);

    refillCombo(IDC_COMBO_ALERT_POS,
        ru ? std::vector<const wchar_t*>{
                L"\u041B\u0435\u0432\u043E \u0432\u0435\u0440\u0445",
                L"\u041B\u0435\u0432\u043E \u0446\u0435\u043D\u0442\u0440",
                L"\u041B\u0435\u0432\u043E \u043D\u0438\u0437",
                L"\u041F\u0440\u0430\u0432\u043E \u0432\u0435\u0440\u0445",
                L"\u041F\u0440\u0430\u0432\u043E \u0446\u0435\u043D\u0442\u0440",
                L"\u041F\u0440\u0430\u0432\u043E \u043D\u0438\u0437"}
           : std::vector<const wchar_t*>{
                L"Left Top", L"Left Center", L"Left Bottom",
                L"Right Top", L"Right Center", L"Right Bottom"},
        (int)st->cfg.alert_pos);

    st->in_init = false;
}

void UpdateControlStates(HWND hDlg) {
    int prov = (int)SendDlgItemMessageW(hDlg, IDC_COMBO_PROVIDER, CB_GETCURSEL, 0, 0);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_API_KEY), prov == 1);

    int mode = (int)SendDlgItemMessageW(hDlg, IDC_COMBO_MODE, CB_GETCURSEL, 0, 0);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_EXT_SLOTS), mode == 2);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_SCROLL), mode != 0);
    EnableWindow(GetDlgItem(hDlg, IDC_CHK_FILL_TOMORROW), mode != 0);

    int sty = (int)SendDlgItemMessageW(hDlg, IDC_COMBO_ICON_STYLE, CB_GETCURSEL, 0, 0);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_VIGNETTE), sty == 0 || sty == 5);
    bool isBar = (sty == 1 || sty == 2 || sty == 4);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_BAR_THICK), isBar);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_BAR_CONTRAST), isBar);

    int al = (int)SendDlgItemMessageW(hDlg, IDC_COMBO_ALERT, CB_GETCURSEL, 0, 0);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_ALERT_DAYS), al != 0);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_ALERT_DIFF), al != 0);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_ALERT_POS), al != 0);
}

void PopulateResults(HWND hDlg, DialogState* st) {
    HWND hCombo = GetDlgItem(hDlg, IDC_COMBO_RESULTS);
    SendMessageW(hCombo, CB_RESETCONTENT, 0, 0);
    for (const auto& c : st->candidates) {
        std::wstring lbl = c.DisplayLabel();
        SendMessageW(hCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(lbl.c_str()));
    }
    if (!st->candidates.empty()) SendMessageW(hCombo, CB_SETCURSEL, 0, 0);
}

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    DialogState* st = reinterpret_cast<DialogState*>(GetWindowLongPtrW(hDlg, DWLP_USER));

    switch (msg) {
    case WM_INITDIALOG: {
        st = reinterpret_cast<DialogState*>(lParam);
        SetWindowLongPtrW(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(st));
        st->in_init = true;

        HWND hLang = GetDlgItem(hDlg, IDC_COMBO_LANG);
        SendMessageW(hLang, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"\u0420\u0443\u0441\u0441\u043A\u0438\u0439"));
        SendMessageW(hLang, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"English"));
        SendMessageW(hLang, CB_SETCURSEL, (int)st->cfg.lang, 0);

        SetDlgItemTextW(hDlg, IDC_EDIT_API_KEY, st->cfg.yandex_api_key.c_str());
        SetDlgItemTextW(hDlg, IDC_EDIT_QUERY, st->cfg.search_query.c_str());
        SetDlgItemTextW(hDlg, IDC_EDIT_COUNTRY, st->cfg.search_country.c_str());
        SetDlgItemTextW(hDlg, IDC_EDIT_CUSTOM_CITY, st->cfg.custom_city_name.c_str());
        SetDlgItemInt(hDlg, IDC_EDIT_ALERT_DAYS, st->cfg.alert_days, FALSE);
        SetDlgItemInt(hDlg, IDC_EDIT_ALERT_DIFF, st->cfg.alert_threshold, FALSE);
        SendDlgItemMessageW(hDlg, IDC_CHK_FILL_TOMORROW, BM_SETCHECK, st->cfg.fill_tomorrow ? BST_CHECKED : BST_UNCHECKED, 0);
        SetDlgItemInt(hDlg, IDC_EDIT_REFRESH, st->cfg.refresh_minutes, FALSE);

        GeoCandidate cur;
        cur.name = st->cfg.resolved_city;
        cur.country = st->cfg.resolved_country;
        cur.lat = st->cfg.latitude;
        cur.lon = st->cfg.longitude;
        st->candidates = {cur};
        PopulateResults(hDlg, st);

        ApplyLanguageTexts(hDlg, st->cfg.lang, st);
        UpdateControlStates(hDlg);
        RefreshPreview(hDlg, st);
        return TRUE;
    }

    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (st && dis && dis->CtlID == IDC_PREVIEW_BOX) {
            int cw = dis->rcItem.right - dis->rcItem.left;
            int ch = dis->rcItem.bottom - dis->rcItem.top;
            if (cw <= 0 || ch <= 0) return TRUE;

            HDC memDC = CreateCompatibleDC(dis->hDC);
            HBITMAP bmp = CreateCompatibleBitmap(dis->hDC, cw, ch);
            HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, bmp);

            COLORREF bgCol = st->preview_dark ? RGB(12, 13, 15) : RGB(238, 242, 246);
            HBRUSH br = CreateSolidBrush(bgCol);
            RECT full{ 0, 0, cw, ch };
            FillRect(memDC, &full, br);
            DeleteObject(br);

            int dpi = GetDeviceCaps(dis->hDC, LOGPIXELSX);
            int targetH = MulDiv(30, dpi, 96);
            targetH = std::min(targetH, ch - 4);
            int logicalW = 154;
            if (st->live_cfg.display_mode == DisplayMode::Compact) logicalW = 64;
            else if (st->live_cfg.display_mode == DisplayMode::Extended) logicalW = (st->live_cfg.extended_slots == 3) ? 198 : 242;
            int targetW = std::min(cw - 8, MulDiv(logicalW, dpi, 96));

            int px = (cw - targetW) / 2;
            int py = (ch - targetH) / 2;
            WeatherItem::DrawBlock(memDC, px, py, targetW, targetH, st->preview_dark, st->live_cfg, st->preview_snap, 0, true);

            BitBlt(dis->hDC, dis->rcItem.left, dis->rcItem.top, cw, ch, memDC, 0, 0, SRCCOPY);
            SelectObject(memDC, oldBmp);
            DeleteObject(bmp);
            DeleteDC(memDC);
            return TRUE;
        }
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);
        if (!st) break;

        if (id == IDC_BTN_PREVIEW_THEME && code == BN_CLICKED) {
            st->preview_dark = !st->preview_dark;
            InvalidateRect(GetDlgItem(hDlg, IDC_PREVIEW_BOX), nullptr, FALSE);
            return TRUE;
        }
        if (id == IDC_COMBO_LANG && code == CBN_SELCHANGE) {
            int sel = (int)SendDlgItemMessageW(hDlg, IDC_COMBO_LANG, CB_GETCURSEL, 0, 0);
            ApplyLanguageTexts(hDlg, sel == 1 ? LangMode::EN : LangMode::RU, st);
            UpdateControlStates(hDlg);
            RefreshPreview(hDlg, st);
            return TRUE;
        }
        if (!st->in_init && (code == CBN_SELCHANGE || code == EN_CHANGE || (id == IDC_CHK_FILL_TOMORROW && code == BN_CLICKED))) {
            if (id == IDC_COMBO_PROVIDER || id == IDC_COMBO_MODE || id == IDC_COMBO_ICON_STYLE || id == IDC_COMBO_ALERT) {
                UpdateControlStates(hDlg);
            }
            if (id == IDC_EDIT_CUSTOM_CITY && !st->preview_snap.valid) {
                st->preview_snap.city_name = st->live_cfg.EffectiveCityName();
            }
            RefreshPreview(hDlg, st);
        }
        if (id == IDC_BTN_SEARCH && code == BN_CLICKED) {
            std::wstring q = GetEdit(hDlg, IDC_EDIT_QUERY);
            std::wstring c = GetEdit(hDlg, IDC_EDIT_COUNTRY);
            int langSel = (int)SendDlgItemMessageW(hDlg, IDC_COMBO_LANG, CB_GETCURSEL, 0, 0);
            bool ru = (langSel == 0);
            if (q.empty()) {
                SetDlgItemTextW(hDlg, IDC_LBL_STATUS, ru ? L"\u0412\u0432\u0435\u0434\u0438\u0442\u0435 \u0433\u043E\u0440\u043E\u0434 \u0438\u043B\u0438 \u043A\u043E\u043E\u0440\u0434." : L"Enter city or coords");
                return TRUE;
            }
            SetDlgItemTextW(hDlg, IDC_LBL_STATUS, ru ? L"\u041F\u043E\u0438\u0441\u043A..." : L"Searching...");
            std::wstring err;
            auto res = WeatherClient::SearchLocation(q, c, err);
            if (res.empty()) {
                SetDlgItemTextW(hDlg, IDC_LBL_STATUS, err.empty() ? (ru ? L"\u041D\u0435 \u043D\u0430\u0439\u0434\u0435\u043D\u043E" : L"Not found") : err.c_str());
            } else {
                st->candidates = std::move(res);
                PopulateResults(hDlg, st);
                wchar_t msgBuf[64]{};
                swprintf_s(msgBuf, ru ? L"\u041D\u0430\u0439\u0434\u0435\u043D\u043E: %zu" : L"Found: %zu", st->candidates.size());
                SetDlgItemTextW(hDlg, IDC_LBL_STATUS, msgBuf);
                RefreshPreview(hDlg, st);
            }
            return TRUE;
        }
        if (id == IDOK) {
            ReadSettingsFromControls(hDlg, st, st->cfg);

            double lat = 0.0, lon = 0.0;
            bool parsedCoords = !st->cfg.search_query.empty() && WeatherClient::TryParseCoordinatesOrGoogleLink(st->cfg.search_query, lat, lon);
            int candIdx = (int)SendDlgItemMessageW(hDlg, IDC_COMBO_RESULTS, CB_GETCURSEL, 0, 0);

            if (parsedCoords && (st->candidates.empty() || candIdx < 0 ||
                std::abs(st->candidates[candIdx].lat - lat) > 0.0001 || std::abs(st->candidates[candIdx].lon - lon) > 0.0001)) {
                st->cfg.latitude = lat;
                st->cfg.longitude = lon;
                std::wstring revCity, revCountry;
                if (WeatherClient::ReverseGeocode(lat, lon, revCity, revCountry)) {
                    st->cfg.resolved_city = revCity;
                    st->cfg.resolved_country = revCountry;
                } else {
                    wchar_t buf[48]{};
                    swprintf_s(buf, L"%.2f, %.2f", lat, lon);
                    st->cfg.resolved_city = buf;
                    st->cfg.resolved_country.clear();
                }
            }
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        if (id == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}
} // namespace

bool OptionsDialog::Show(HINSTANCE hInst, HWND parent, PluginConfig& cfg) {
    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    DialogState st;
    st.cfg = cfg;
    st.live_cfg = cfg;
    st.preview_snap = WeatherPlugin::Instance().GetSnapshot();
    if (!st.preview_snap.valid) st.preview_snap = MakeSampleSnapshot(cfg);

    INT_PTR res = DialogBoxParamW(hInst, MAKEINTRESOURCEW(IDD_OPTIONS_DIALOG), parent, DlgProc, reinterpret_cast<LPARAM>(&st));
    if (res == IDOK) {
        cfg = st.cfg;
        return true;
    }
    return false;
}
