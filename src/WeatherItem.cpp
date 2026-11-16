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

const wchar_t* WeatherItem::GetItemId() const {
    return L"WeatherPlus_MainWidget";
}

const wchar_t* WeatherItem::GetItemLableText() const { return L""; }
const wchar_t* WeatherItem::GetItemValueText() const { return L""; }
const wchar_t* WeatherItem::GetItemValueSampleText() const { return L""; }

int WeatherItem::GetItemWidth() const {
    auto cfg = m_owner->GetConfig();
    switch (cfg.display_mode) {
    case DisplayMode::Compact:  return 64;
    case DisplayMode::Medium:   return 154; // 66 + 2*44
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
    if (type == MT_WHEEL_UP) {
        m_owner->StepScroll(-1);
        return 1;
    }
    if (type == MT_WHEEL_DOWN) {
        m_owner->StepScroll(1);
        return 1;
    }
    if (type == MT_LCLICKED) {
        m_owner->CycleOrResetScroll();
        return 1;
    }
    if (type == MT_DBCLICKED) {
        m_owner->TriggerRefresh();
        return 1;
    }
    return 0;
}

void WeatherItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) {
    if (!hDC || w <= 4 || h <= 4) return;
    m_owner->EnsureGdiPlus();

    HDC hdc = reinterpret_cast<HDC>(hDC);
    int saved = SaveDC(hdc);
    SetBkMode(hdc, TRANSPARENT);

    PluginConfig cfg = m_owner->GetConfig();
    WeatherSnapshot snap = m_owner->GetSnapshot();
    int scroll_idx = m_owner->GetScrollIndex();

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
        curW = std::min(w, (int)std::lround((float)w * 66.0f / (float)GetItemWidth()));
    }

    int totalSlots = (int)snap.slots.size();
    int maxStart = std::max(0, totalSlots - numForecast);
    int start = std::clamp(scroll_idx, 0, maxStart);

    wchar_t curTemp[16] = L"--\u00B0";
    if (snap.valid) FormatTemp(snap.current_temp_c, cfg.unit, curTemp, 16);
    WeatherKind curKind = snap.valid ? snap.current_kind : WeatherKind::Cloudy;
    std::wstring cityName = snap.valid ? snap.city_name : cfg.EffectiveCityName();

    float topY = (float)y + 1.0f;
    float boxH = (float)h - 2.0f;

    // 1. Tile backdrops (Sky, TopBar, BottomBar, LeftPill, Watermark, or optional subtle fill)
    for (int i = 0; i < totalTiles; ++i) {
        int tx = (i == 0) ? x : (x + curW + (int)std::lround((float)(w - curW) * (float)(i - 1) / (float)numForecast));
        int txNext = (i == 0) ? (x + curW) : (x + curW + (int)std::lround((float)(w - curW) * (float)i / (float)numForecast));
        int tw = std::max(1, txNext - tx);

        WeatherKind k = curKind;
        if (i > 0) {
            int idx = start + (i - 1);
            k = (snap.valid && idx < totalSlots) ? snap.slots[idx].kind : WeatherKind::Cloudy;
        }
        IconRenderer::DrawTileBackdrop(hdc, (float)tx, topY, (float)tw, boxH, k, dark_mode, cfg.icon_style, drawBg);
    }

    // 2. Tile foregrounds & text
    if (twoRowInside) {
        HFONT fTop = CreateUiFont(std::max(8, (int)std::lround(9.5f * s)), false);
        HFONT fVal = CreateUiFont(std::max(10, (int)std::lround(13.0f * s)), true);
        HFONT fFVal = CreateUiFont(std::max(8, (int)std::lround(10.0f * s)), true);
        HFONT oldF = (HFONT)SelectObject(hdc, fTop);

        for (int i = 0; i < totalTiles; ++i) {
            int tx = (i == 0) ? x : (x + curW + (int)std::lround((float)(w - curW) * (float)(i - 1) / (float)numForecast));
            int txNext = (i == 0) ? (x + curW) : (x + curW + (int)std::lround((float)(w - curW) * (float)i / (float)numForecast));
            int tw = std::max(1, txNext - tx);

            // Style 6: Variant F (16x16 icon top + "Y 9" bottom; left tile = 22px icon + city/temp)
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
                        if (snap.valid && snap.rain_expected) {
                            IconRenderer::DrawRainDrop(hdc, tx + 16.0f * s, y + 18.0f * s, 9.0f * s, dark_mode);
                        }
                        int txtX = tx + (int)std::lround(26.0f * s);
                        RECT rcTop{ txtX, y + (int)std::lround(2.0f * s), tx + tw - 2, y + (int)std::lround(14.0f * s) };
                        RECT rcBot{ txtX, y + (int)std::lround(13.5f * s), tx + tw - 2, y + h - 2 };
                        SelectObject(hdc, fTop);
                        DrawShadowText(hdc, cityName.c_str(), rcTop, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, colMuted, false);
                        SelectObject(hdc, fVal);
                        DrawShadowText(hdc, curTemp, rcBot, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX, colPrimary, false);
                    }
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

            // Style 3: Variant C (11px glyph + short label on top, large temp bottom)
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

                    if (i == 0 && snap.valid && snap.rain_expected) {
                        IconRenderer::DrawRainDrop(hdc, (float)(tx + tw) - 10.5f * s, (float)(y + h) - 12.0f * s, 8.5f * s, dark_mode);
                    }
                }
                continue;
            }

            // Styles 0 (SkyCards), 1 (TopBar), 2 (BottomBar), 4 (LeftPill), 5 (Watermark)
            int leftPad = (cfg.icon_style == IconStyle::LeftPill) ? (int)std::lround(5.5f * s) : 2;
            int topPad = (cfg.icon_style == IconStyle::TopBar) ? (int)std::lround(3.5f * s)
                       : (cfg.icon_style == IconStyle::BottomBar ? (int)std::lround(1.5f * s) : (int)std::lround(2.0f * s));
            RECT rcTop{ tx + leftPad, y + topPad, tx + tw - 2, y + topPad + (int)std::lround(11.0f * s) };
            RECT rcBot{ tx + leftPad, y + topPad + (int)std::lround(10.5f * s), tx + tw - 2, y + h - (cfg.icon_style == IconStyle::BottomBar ? (int)std::lround(3.5f * s) : 2) };

            if (i == 0) {
                SelectObject(hdc, fTop);
                DrawShadowText(hdc, cityName.c_str(), rcTop, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, colMuted, useShadow);
                SelectObject(hdc, fVal);
                DrawShadowText(hdc, curTemp, rcBot, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX, colPrimary, useShadow);
                if (snap.valid && snap.rain_expected) {
                    IconRenderer::DrawRainDrop(hdc, (float)(tx + tw) - 11.0f * s, (float)(y + h) - 12.5f * s, 9.0f * s, dark_mode || isSky);
                }
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
        // Fallback if h < 24 (16px single row)
        HFONT fInline = CreateUiFont(std::max(9, h - 5), true);
        HFONT oldF = (HFONT)SelectObject(hdc, fInline);
        for (int i = 0; i < totalTiles; ++i) {
            int tx = (i == 0) ? x : (x + curW + (int)std::lround((float)(w - curW) * (float)(i - 1) / (float)numForecast));
            int txNext = (i == 0) ? (x + curW) : (x + curW + (int)std::lround((float)(w - curW) * (float)i / (float)numForecast));
            int tw = std::max(1, txNext - tx);
            RECT rc{ tx + 2, y, tx + tw - 2, y + h };

            if (i == 0) {
                std::wstring lbl = (cfg.display_mode == DisplayMode::Compact) ? std::wstring(curTemp) : (cityName.substr(0, std::min<size_t>(4, cityName.size())) + L" " + curTemp);
                DrawShadowText(hdc, lbl.c_str(), rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX, colPrimary, useShadow);
                if (snap.valid && snap.rain_expected) {
                    IconRenderer::DrawRainDrop(hdc, (float)(tx + tw) - 9.0f, (float)y + 3.0f, 7.5f, dark_mode || isSky);
                }
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

    // 3. Optional dividers & outer border
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
