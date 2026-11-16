#include "WeatherItem.h"
#include "WeatherPlugin.h"
#include "IconRenderer.h"
#include "Localization.h"
#include "GdiPlusInclude.h"
#include <algorithm>
#include <cmath>

using namespace Gdiplus;

namespace {
int FormatTemp(double temp_c, TempUnit unit, wchar_t* buf, size_t n) {
    double v = (unit == TempUnit::Fahrenheit) ? (temp_c * 9.0 / 5.0 + 32.0) : temp_c;
    int iv = (int)std::llround(v);
    return swprintf_s(buf, n, L"%d\u00B0", iv);
}

HFONT CreateUiFont(int height_px, bool bold) {
    return CreateFontW(-height_px, 0, 0, 0, bold ? FW_SEMIBOLD : FW_NORMAL, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
}

void DrawShadowText(HDC hdc, const wchar_t* text, RECT rc, UINT fmt, COLORREF fg, bool shadow) {
    if (shadow) {
        SetTextColor(hdc, RGB(10, 12, 16));
        RECT sh{ rc.left, rc.top + 1, rc.right, rc.bottom + 1 };
        DrawTextW(hdc, text, -1, &sh, fmt);
    }
    SetTextColor(hdc, fg);
    DrawTextW(hdc, text, -1, &rc, fmt);
}
} // namespace

WeatherItem::WeatherItem(WeatherPlugin* owner) : m_owner(owner) {}

const wchar_t* WeatherItem::GetItemName() const {
    m_nameBuf = L10n::ItemName(m_owner->GetConfig().lang);
    return m_nameBuf.c_str();
}

const wchar_t* WeatherItem::GetItemId() const { return L"WeatherPlus_MainWidget"; }
const wchar_t* WeatherItem::GetItemLableText() const { return L""; }
const wchar_t* WeatherItem::GetItemValueText() const { return L""; }
const wchar_t* WeatherItem::GetItemValueSampleText() const { return L""; }

int WeatherItem::GetItemWidth() const {
    auto cfg = m_owner->GetConfig();
    switch (cfg.display_mode) {
    case DisplayMode::Compact:  return 64;
    case DisplayMode::Medium:   return 154;
    case DisplayMode::Extended: return (cfg.extended_slots == 3) ? 198 : 242;
    }
    return 154;
}

int WeatherItem::GetItemWidthEx(void* hDC) const {
    int dpi = 96;
    if (hDC) {
        dpi = GetDeviceCaps(reinterpret_cast<HDC>(hDC), LOGPIXELSX);
    } else {
        HDC screen = GetDC(nullptr);
        dpi = GetDeviceCaps(screen, LOGPIXELSX);
        ReleaseDC(nullptr, screen);
    }
    return MulDiv(GetItemWidth(), dpi, 96);
}

int WeatherItem::OnMouseEvent(MouseEventType type, int x, int y, void* hWnd, int flag) {
    if (type == MT_WHEEL_UP)   { m_owner->StepScroll(-1); return 1; }
    if (type == MT_WHEEL_DOWN) { m_owner->StepScroll(1); return 1; }
    if (type == MT_LCLICKED)   { m_owner->CycleOrResetScroll(); return 1; }
    if (type == MT_DBCLICKED)  { m_owner->TriggerRefresh(); return 1; }
    return 0;
}

int WeatherItem::EvaluateTempAlert(const PluginConfig& cfg, const WeatherSnapshot& snap, std::wstring* out_desc) {
    if (cfg.alert_mode == AlertMode::Off || !snap.valid || snap.daily.size() < 2) return 0;

    double baseTemp = snap.daily[0].day_temp_c;
    double threshC = (cfg.unit == TempUnit::Fahrenheit) ? (cfg.alert_threshold * 5.0 / 9.0) : (double)cfg.alert_threshold;

    double bestDiff = 0.0;
    int bestOff = 0;
    double bestTarget = 0.0;

    for (const auto& d : snap.daily) {
        if (d.day_offset <= 0 || d.day_offset > cfg.alert_days) continue;
        double diff = d.day_temp_c - baseTemp;
        if (std::abs(diff) >= threshC && std::abs(diff) > std::abs(bestDiff)) {
            bestDiff = diff;
            bestOff = d.day_offset;
            bestTarget = d.day_temp_c;
        }
    }

    if (bestOff == 0) return 0;
    int dir = (bestDiff > 0) ? 1 : -1;

    if (out_desc) {
        double dispTarget = (cfg.unit == TempUnit::Fahrenheit) ? (bestTarget * 9.0 / 5.0 + 32.0) : bestTarget;
        double dispDiff   = (cfg.unit == TempUnit::Fahrenheit) ? (bestDiff * 9.0 / 5.0) : bestDiff;
        const wchar_t* unitStr = (cfg.unit == TempUnit::Fahrenheit) ? L"\u00B0F" : L"\u00B0C";
        wchar_t buf[128]{};
        if (cfg.lang == LangMode::RU) {
            swprintf_s(buf, L"%s \u0447\u0435\u0440\u0435\u0437 %d \u0434\u043D.: %d%s (%+d\u00B0)",
                       dir > 0 ? L"\u041F\u043E\u0442\u0435\u043F\u043B\u0435\u043D\u0438\u0435" : L"\u041F\u043E\u0445\u043E\u043B\u043E\u0434\u0430\u043D\u0438\u0435",
                       bestOff, (int)std::llround(dispTarget), unitStr, (int)std::llround(dispDiff));
        } else {
            swprintf_s(buf, L"%s in %dd: %d%s (%+d\u00B0)",
                       dir > 0 ? L"Warming" : L"Cooling",
                       bestOff, (int)std::llround(dispTarget), unitStr, (int)std::llround(dispDiff));
        }
        *out_desc = buf;
    }
    return dir;
}

void WeatherItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) {
    if (!hDC || w <= 4 || h <= 4) return;
    m_owner->EnsureGdiPlus();
    DrawBlock(reinterpret_cast<HDC>(hDC), x, y, w, h, dark_mode,
              m_owner->GetConfig(), m_owner->GetSnapshot(), m_owner->GetScrollIndex(), false);
}

void WeatherItem::DrawBlock(HDC hdc, int x, int y, int w, int h, bool dark_mode, const PluginConfig& cfg,
                            const WeatherSnapshot& snap, int scroll_idx, bool force_preview_alert) {
    int saved = SaveDC(hdc);
    SetBkMode(hdc, TRANSPARENT);

    bool isSky = (cfg.icon_style == IconStyle::SkyCards);
    bool isWm  = (cfg.icon_style == IconStyle::Watermark);
    bool useShadow = isSky || (isWm && dark_mode);

    bool drawBg = (cfg.border_style == BorderStyle::BorderAndFill || cfg.border_style == BorderStyle::FillOnly);
    bool drawDividers = (cfg.border_style != BorderStyle::TransparentClean);
    bool drawBorder = (cfg.border_style == BorderStyle::BorderAndFill);

    bool twoRowInside = (h >= 24);
    float s = twoRowInside ? ((float)h / 30.0f) : ((float)h / 18.0f);

    COLORREF colPrimary = isSky ? RGB(255, 255, 255) : (dark_mode ? RGB(246, 248, 252) : RGB(22, 26, 32));
    COLORREF colMuted   = isSky ? RGB(230, 238, 248) : (dark_mode ? RGB(156, 168, 186) : RGB(84, 94, 110));
    COLORREF colTmw     = isSky ? RGB(147, 232, 255) : (dark_mode ? RGB(105, 215, 255) : RGB(3, 105, 161));

    int numForecast = (cfg.display_mode == DisplayMode::Compact) ? 0
                    : ((cfg.display_mode == DisplayMode::Medium) ? 2 : cfg.extended_slots);
    int totalTiles = 1 + numForecast;

    int curW = (cfg.display_mode == DisplayMode::Compact) ? w : std::min(w, (int)std::lround(66.0f * (twoRowInside ? s : 1.0f)));
    if (!twoRowInside && cfg.display_mode != DisplayMode::Compact) {
        int baseFull = (cfg.display_mode == DisplayMode::Medium) ? 154 : (cfg.extended_slots == 3 ? 198 : 242);
        curW = std::min(w, (int)std::lround((float)w * 66.0f / (float)baseFull));
    }

    int totalSlots = (int)snap.slots.size();
    int maxStart = std::max(0, totalSlots - numForecast);
    int start = std::clamp(scroll_idx, 0, maxStart);

    wchar_t curTemp[16] = L"--\u00B0";
    if (snap.valid) FormatTemp(snap.current_temp_c, cfg.unit, curTemp, 16);
    WeatherKind curKind = snap.valid ? snap.current_kind : WeatherKind::Cloudy;
    std::wstring cityName = snap.valid ? snap.city_name : cfg.EffectiveCityName();

    int alertDir = EvaluateTempAlert(cfg, snap);
    if (alertDir == 0 && force_preview_alert && cfg.alert_mode != AlertMode::Off) alertDir = 1;

    float topY = (float)y + 1.0f;
    float boxH = (float)h - 2.0f;

    for (int i = 0; i < totalTiles; ++i) {
        int tx = (i == 0) ? x : (x + curW + (int)std::lround((float)(w - curW) * (float)(i - 1) / (float)numForecast));
        int txNext = (i == 0) ? (x + curW) : (x + curW + (int)std::lround((float)(w - curW) * (float)i / (float)numForecast));
        int tw = std::max(1, txNext - tx);

        WeatherKind k = curKind;
        if (i > 0) {
            int idx = start + (i - 1);
            k = (snap.valid && idx < totalSlots) ? snap.slots[idx].kind : WeatherKind::Cloudy;
        }
        IconRenderer::DrawTileBackdrop(hdc, (float)tx, topY, (float)tw, boxH, k, dark_mode, cfg, drawBg);
    }

    auto drawAlertAndRain = [&](int tx, int tw) {
        bool hasRain = (snap.valid && snap.rain_expected);
        float aSize = 10.5f * s;
        float leftOff = (cfg.icon_style == IconStyle::LeftPill) ? (5.5f * s) : (2.0f * s);
        if (alertDir != 0) {
            float ax = (cfg.alert_pos <= AlertPos::LeftBottom) ? ((float)tx + leftOff)
                                                               : ((float)(tx + tw) - aSize - 2.0f * s);
            float ay = (float)y + 2.0f * s;
            if (cfg.alert_pos == AlertPos::LeftCenter || cfg.alert_pos == AlertPos::RightCenter) {
                ay = (float)y + ((float)h - aSize) * 0.5f;
            } else if (cfg.alert_pos == AlertPos::LeftBottom || cfg.alert_pos == AlertPos::RightBottom) {
                ay = (float)(y + h) - aSize - 2.0f * s;
            }
            IconRenderer::DrawTempAlert(hdc, ax, ay, aSize, alertDir, cfg.alert_mode, dark_mode || isSky);
        }
        if (hasRain) {
            float rShift = (alertDir != 0 && cfg.alert_pos == AlertPos::RightBottom) ? 21.5f * s : 11.0f * s;
            IconRenderer::DrawRainDrop(hdc, (float)(tx + tw) - rShift, (float)(y + h) - 12.5f * s, 9.0f * s, dark_mode || isSky);
        }
    };

    if (twoRowInside) {
        HFONT fTop = CreateUiFont(std::max(8, (int)std::lround(9.5f * s)), false);
        HFONT fVal = CreateUiFont(std::max(10, (int)std::lround(13.0f * s)), true);
        HFONT fFVal = CreateUiFont(std::max(8, (int)std::lround(10.0f * s)), true);
        HFONT oldF = (HFONT)SelectObject(hdc, fTop);

        for (int i = 0; i < totalTiles; ++i) {
            int tx = (i == 0) ? x : (x + curW + (int)std::lround((float)(w - curW) * (float)(i - 1) / (float)numForecast));
            int txNext = (i == 0) ? (x + curW) : (x + curW + (int)std::lround((float)(w - curW) * (float)i / (float)numForecast));
            int tw = std::max(1, txNext - tx);

            if (cfg.icon_style == IconStyle::IconTopLabelBottom) {
                if (i == 0) {
                    if (cfg.display_mode == DisplayMode::Compact) {
                        float ico = 22.0f * s;
                        IconRenderer::DrawWeatherGlyph(hdc, tx + 4.0f * s, y + (h - ico) * 0.5f, ico, curKind, dark_mode, 255);
                        SelectObject(hdc, fVal);
                        RECT rc{ tx + (int)std::lround(28.0f * s), y, tx + tw - 3, y + h };
                        DrawShadowText(hdc, curTemp, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX, colPrimary, false);
                    } else {
                        float ico = 22.0f * s;
                        IconRenderer::DrawWeatherGlyph(hdc, tx + 2.0f * s, y + (h - ico) * 0.5f, ico, curKind, dark_mode, 255);
                        int txtX = tx + (int)std::lround(26.0f * s);
                        RECT rcTop{ txtX, y + (int)std::lround(2.0f * s), tx + tw - 2, y + (int)std::lround(14.0f * s) };
                        RECT rcBot{ txtX, y + (int)std::lround(13.5f * s), tx + tw - 2, y + h - 2 };
                        SelectObject(hdc, fTop);
                        DrawShadowText(hdc, cityName.c_str(), rcTop, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, colMuted, false);
                        SelectObject(hdc, fVal);
                        DrawShadowText(hdc, curTemp, rcBot, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX, colPrimary, false);
                    }
                    drawAlertAndRain(tx, tw);
                } else {
                    int idx = start + (i - 1);
                    if (snap.valid && idx < totalSlots) {
                        const auto& slot = snap.slots[idx];
                        float tileIco = 16.0f * s;
                        IconRenderer::DrawWeatherGlyph(hdc, tx + (tw - tileIco) * 0.5f, y + 1.5f * s, tileIco, slot.kind, dark_mode, 255);

                        wchar_t tStr[12]{};
                        FormatTemp(slot.temp_c, cfg.unit, tStr, 12);
                        std::wstring prefix = (!slot.is_today && slot.day_num > 0) ? std::to_wstring(slot.day_num) : std::wstring();
                        prefix += L10n::DayPartShort(cfg.lang, slot.part);
                        std::wstring lbl = prefix + L" " + tStr;

                        SelectObject(hdc, fFVal);
                        RECT rcLbl{ tx + 1, y + (int)std::lround(17.5f * s), tx + tw - 1, y + h - 1 };
                        DrawShadowText(hdc, lbl.c_str(), rcLbl, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX,
                                       slot.is_today ? colPrimary : colTmw, false);
                    }
                }
                continue;
            }

            if (cfg.icon_style == IconStyle::GlyphTop) {
                float gSize = 11.0f * s;
                float gap = 2.5f * s;
                std::wstring topText;
                WeatherKind tk = curKind;
                COLORREF topCol = colMuted;
                const wchar_t* botText = curTemp;
                bool validTile = true;

                wchar_t tStr[12]{};
                if (i == 0) {
                    topText = cityName;
                } else {
                    int idx = start + (i - 1);
                    if (snap.valid && idx < totalSlots) {
                        const auto& slot = snap.slots[idx];
                        tk = slot.kind;
                        if (!slot.is_today && slot.day_num > 0) topText = std::to_wstring(slot.day_num);
                        topText += L10n::DayPartShort(cfg.lang, slot.part);
                        topCol = slot.is_today ? colMuted : colTmw;
                        FormatTemp(slot.temp_c, cfg.unit, tStr, 12);
                        botText = tStr;
                    } else {
                        validTile = false;
                    }
                }

                if (validTile) {
                    SelectObject(hdc, fTop);
                    RECT calcRc{ 0, 0, 0, 0 };
                    DrawTextW(hdc, topText.c_str(), -1, &calcRc, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
                    int maxTxtW = std::max(8, tw - (int)std::lround(gSize + gap + 4.0f * s));
                    int txtW = std::min((int)(calcRc.right - calcRc.left), maxTxtW);
                    float totalTopW = gSize + gap + (float)txtW;
                    float startX = (float)tx + ((float)tw - totalTopW) * 0.5f;

                    IconRenderer::DrawWeatherGlyph(hdc, startX, (float)y + 2.5f * s, gSize, tk, dark_mode, 255);
                    RECT rcTop{ (int)std::lround(startX + gSize + gap), y + (int)std::lround(1.5f * s),
                                tx + tw - 1, y + (int)std::lround(14.0f * s) };
                    DrawShadowText(hdc, topText.c_str(), rcTop, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, topCol, false);

                    SelectObject(hdc, fVal);
                    RECT rcBot{ tx + 2, y + (int)std::lround(13.0f * s), tx + tw - 2, y + h - 2 };
                    DrawShadowText(hdc, botText, rcBot, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX, colPrimary, false);

                    if (i == 0) drawAlertAndRain(tx, tw);
                }
                continue;
            }

            int leftPad = (cfg.icon_style == IconStyle::LeftPill) ? (int)std::lround(6.0f * s) : 2;
            int topPad = (cfg.icon_style == IconStyle::TopBar) ? (int)std::lround((2 + cfg.bar_thickness) * s + 1.0f)
                       : (cfg.icon_style == IconStyle::BottomBar ? (int)std::lround(1.5f * s) : (int)std::lround(2.0f * s));
            int botSub = (cfg.icon_style == IconStyle::BottomBar) ? (int)std::lround((2 + cfg.bar_thickness) * s) : 2;
            RECT rcTop{ tx + leftPad, y + topPad, tx + tw - 2, y + topPad + (int)std::lround(10.5f * s) };
            RECT rcBot{ tx + leftPad, y + topPad + (int)std::lround(9.5f * s), tx + tw - 2, y + h - botSub };

            if (i == 0) {
                SelectObject(hdc, fTop);
                DrawShadowText(hdc, cityName.c_str(), rcTop, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, colMuted, useShadow);
                SelectObject(hdc, fVal);
                DrawShadowText(hdc, curTemp, rcBot, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX, colPrimary, useShadow);
                drawAlertAndRain(tx, tw);
            } else {
                int idx = start + (i - 1);
                if (snap.valid && idx < totalSlots) {
                    const auto& slot = snap.slots[idx];
                    std::wstring topLbl;
                    if (slot.is_today) {
                        topLbl = L10n::DayPartFull(cfg.lang, slot.part);
                    } else {
                        if (slot.day_num > 0) topLbl = std::to_wstring(slot.day_num) + L"\u00B7";
                        topLbl += L10n::DayPartShort(cfg.lang, slot.part);
                    }
                    SelectObject(hdc, fTop);
                    DrawShadowText(hdc, topLbl.c_str(), rcTop, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX,
                                   slot.is_today ? colMuted : colTmw, useShadow);

                    wchar_t tStr[12]{};
                    FormatTemp(slot.temp_c, cfg.unit, tStr, 12);
                    SelectObject(hdc, fVal);
                    DrawShadowText(hdc, tStr, rcBot, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX, colPrimary, useShadow);
                }
            }
        }
        SelectObject(hdc, oldF);
        DeleteObject(fTop);
        DeleteObject(fVal);
        DeleteObject(fFVal);
    } else {
        HFONT fInline = CreateUiFont(std::max(9, h - 5), true);
        HFONT oldF = (HFONT)SelectObject(hdc, fInline);
        for (int i = 0; i < totalTiles; ++i) {
            int tx = (i == 0) ? x : (x + curW + (int)std::lround((float)(w - curW) * (float)(i - 1) / (float)numForecast));
            int txNext = (i == 0) ? (x + curW) : (x + curW + (int)std::lround((float)(w - curW) * (float)i / (float)numForecast));
            int tw = std::max(1, txNext - tx);
            RECT rc{ tx + 2, y, tx + tw - 2, y + h };

            if (i == 0) {
                std::wstring lbl = (cfg.display_mode == DisplayMode::Compact) ? std::wstring(curTemp)
                                 : (cityName.substr(0, std::min<size_t>(4, cityName.size())) + L" " + curTemp);
                DrawShadowText(hdc, lbl.c_str(), rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX, colPrimary, useShadow);
                drawAlertAndRain(tx, tw);
            } else {
                int idx = start + (i - 1);
                if (snap.valid && idx < totalSlots) {
                    const auto& slot = snap.slots[idx];
                    wchar_t tStr[12]{};
                    FormatTemp(slot.temp_c, cfg.unit, tStr, 12);
                    std::wstring prefix = (!slot.is_today && slot.day_num > 0) ? std::to_wstring(slot.day_num) : std::wstring();
                    prefix += L10n::DayPartShort(cfg.lang, slot.part);
                    std::wstring lbl = prefix + L" " + tStr;
                    DrawShadowText(hdc, lbl.c_str(), rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX,
                                   slot.is_today ? colPrimary : colTmw, useShadow);
                }
            }
        }
        SelectObject(hdc, oldF);
        DeleteObject(fInline);
    }

    if (drawDividers || drawBorder) {
        Graphics g(hdc);
        g.SetSmoothingMode(SmoothingModeAntiAlias);

        if (drawDividers) {
            Color divCol = dark_mode ? Color(48, 255, 255, 255) : Color(44, 0, 0, 0);
            Pen divPen(divCol, 1.0f);
            for (int i = 1; i < totalTiles; ++i) {
                int tx = x + curW + (int)std::lround((float)(w - curW) * (float)(i - 1) / (float)numForecast);
                g.DrawLine(&divPen, (float)tx, topY + 1.0f, (float)tx, topY + boxH - 1.0f);
            }
        }

        if (drawBorder) {
            Color borderCol = dark_mode ? Color(65, 255, 255, 255) : Color(58, 0, 0, 0);
            Pen borderPen(borderCol, 1.0f);
            g.DrawRectangle(&borderPen, (float)x + 0.5f, topY, (float)w - 1.5f, boxH);
        }
    }

    RestoreDC(hdc, saved);
}
