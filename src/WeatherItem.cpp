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

bool WeatherItem::IsDoubleLineExclusive() const {
    return m_owner->GetConfig().display_mode != DisplayMode::Compact;
}

bool WeatherItem::IsDoubleLineExclusive() {
    return m_owner->GetConfig().display_mode != DisplayMode::Compact;
}

int WeatherItem::GetItemWidth() const {
    auto cfg = m_owner->GetConfig();
    switch (cfg.display_mode) {
    case DisplayMode::Compact:  return 62;
    case DisplayMode::Medium:   return 162;
    case DisplayMode::Extended: return (cfg.extended_slots == 3) ? 206 : 246;
    }
    return 162;
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

    COLORREF colPrimary = dark_mode ? RGB(244, 246, 250) : RGB(24, 28, 34);
    COLORREF colMuted   = dark_mode ? RGB(164, 174, 190) : RGB(92, 102, 118);

    if (h < 24) {
        float icoSize = (float)(h - 2);
        WeatherKind k = snap.valid ? snap.current_kind : WeatherKind::Cloudy;
        IconRenderer::DrawWeatherIcon(hdc, (float)x + 2.0f, (float)y + 1.0f, icoSize, k, dark_mode, cfg.icon_style);
        wchar_t tbuf[16] = L"--\u00B0";
        if (snap.valid) FormatTemp(snap.current_temp_c, cfg.unit, tbuf, 16);
        HFONT f = CreateUiFont(std::max(10, h - 4), true);
        HFONT oldF = (HFONT)SelectObject(hdc, f);
        SetTextColor(hdc, colPrimary);
        RECT rc{ x + (int)icoSize + 4, y, x + w - 2, y + h };
        DrawTextW(hdc, tbuf, -1, &rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        SelectObject(hdc, oldF);
        DeleteObject(f);
        RestoreDC(hdc, saved);
        return;
    }

    float s = (float)h / 32.0f;

    if (cfg.border_style != BorderStyle::Minimal) {
        Graphics g(hdc);
        g.SetSmoothingMode(SmoothingModeAntiAlias);

        float rx = (float)x + 0.5f;
        float ry = (float)y + 1.0f;
        float rw = (float)w - 1.5f;
        float rh = (float)h - 2.5f;
        float rad = 5.5f * s;

        GraphicsPath card;
        float d = rad * 2.0f;
        card.AddArc(rx, ry, d, d, 180, 90);
        card.AddArc(rx + rw - d, ry, d, d, 270, 90);
        card.AddArc(rx + rw - d, ry + rh - d, d, d, 0, 90);
        card.AddArc(rx, ry + rh - d, d, d, 90, 90);
        card.CloseFigure();

        Color fillCol = dark_mode ? Color(18, 255, 255, 255) : Color(165, 255, 255, 255);
        SolidBrush fillBr(fillCol);
        g.FillPath(&fillBr, &card);

        if (cfg.border_style == BorderStyle::BorderAndFill) {
            Color borderCol = dark_mode ? Color(48, 255, 255, 255) : Color(46, 0, 0, 0);
            Pen borderPen(borderCol, 1.0f);
            g.DrawPath(&borderPen, &card);
        }
    }

    wchar_t tempBuf[16] = L"--\u00B0";
    if (snap.valid) FormatTemp(snap.current_temp_c, cfg.unit, tempBuf, 16);
    WeatherKind curKind = snap.valid ? snap.current_kind : WeatherKind::Cloudy;

    if (cfg.display_mode == DisplayMode::Compact) {
        float icoSize = 22.0f * s;
        IconRenderer::DrawWeatherIcon(hdc, x + 5.0f * s, y + (h - icoSize) * 0.5f, icoSize, curKind, dark_mode, cfg.icon_style);

        HFONT fTemp = CreateUiFont(std::max(10, (int)std::lround(13.0f * s)), true);
        HFONT oldF = (HFONT)SelectObject(hdc, fTemp);
        SetTextColor(hdc, colPrimary);
        RECT rc{ x + (int)(28.0f * s), y, x + w - (int)(6.0f * s), y + h };
        DrawTextW(hdc, tempBuf, -1, &rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        SelectObject(hdc, oldF);
        DeleteObject(fTemp);
        RestoreDC(hdc, saved);
        return;
    }

    int curW = (int)std::lround((cfg.display_mode == DisplayMode::Medium ? 82.0f : 86.0f) * s);
    curW = std::min(curW, w);

    float heroSize = 24.0f * s;
    IconRenderer::DrawWeatherIcon(hdc, x + 5.0f * s, y + (h - heroSize) * 0.5f, heroSize, curKind, dark_mode, cfg.icon_style);

    std::wstring cityText = snap.valid ? snap.city_name : cfg.EffectiveCityName();
    HFONT fCity = CreateUiFont(std::max(8, (int)std::lround(10.0f * s)), false);
    HFONT fCurT = CreateUiFont(std::max(9, (int)std::lround(13.0f * s)), true);

    HFONT oldF = (HFONT)SelectObject(hdc, fCity);
    SetTextColor(hdc, colMuted);
    RECT rcCity{ x + (int)std::lround(33.0f * s), y + (int)std::lround(2.0f * s),
                 x + curW - (int)std::lround(4.0f * s), y + (int)std::lround(15.0f * s) };
    DrawTextW(hdc, cityText.c_str(), -1, &rcCity, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

    SelectObject(hdc, fCurT);
    SetTextColor(hdc, colPrimary);
    RECT rcCurT{ x + (int)std::lround(33.0f * s), y + (int)std::lround(14.5f * s),
                 x + curW - (int)std::lround(15.0f * s), y + h - (int)std::lround(2.0f * s) };
    DrawTextW(hdc, tempBuf, -1, &rcCurT, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    if (snap.valid && snap.rain_expected) {
        float dropSize = 10.5f * s;
        IconRenderer::DrawRainDrop(hdc, x + curW - 14.0f * s, y + 17.5f * s, dropSize, dark_mode, cfg.icon_style);
    }

    int numTiles = (cfg.display_mode == DisplayMode::Medium) ? 2 : cfg.extended_slots;
    int remW = w - curW;
    if (numTiles > 0 && remW > 10) {
        Graphics g(hdc);
        Color divCol = dark_mode ? Color(28, 255, 255, 255) : Color(28, 0, 0, 0);
        Pen divPen(divCol, 1.0f);
        g.DrawLine(&divPen, (float)(x + curW), (float)y + 3.0f * s, (float)(x + curW), (float)(y + h) - 3.0f * s);

        HFONT fTile = CreateUiFont(std::max(8, (int)std::lround(9.5f * s)), false);
        SelectObject(hdc, fTile);

        float tileW = (float)remW / (float)numTiles;
        float tileIcoSize = 16.0f * s;

        int totalSlots = (int)snap.slots.size();
        int maxStart = std::max(0, totalSlots - numTiles);
        int start = std::clamp(scroll_idx, 0, maxStart);

        for (int i = 0; i < numTiles; ++i) {
            int tx = x + curW + (int)std::lround(i * tileW);
            int tw = (int)std::lround((i + 1) * tileW) - (int)std::lround(i * tileW);

            if (i > 0) {
                g.DrawLine(&divPen, (float)tx, (float)y + 3.0f * s, (float)tx, (float)(y + h) - 3.0f * s);
            }

            int idx = start + i;
            if (snap.valid && idx < totalSlots) {
                const auto& slot = snap.slots[idx];
                IconRenderer::DrawWeatherIcon(hdc, tx + (tw - tileIcoSize) * 0.5f, y + 2.0f * s,
                                              tileIcoSize, slot.kind, dark_mode, cfg.icon_style);

                wchar_t tStr[12]{};
                FormatTemp(slot.temp_c, cfg.unit, tStr, 12);
                std::wstring lbl = std::wstring(L10n::DayPartShort(cfg.lang, slot.part)) + L" " + tStr;

                SetTextColor(hdc, colPrimary);
                RECT rcLbl{ tx + 1, y + (int)std::lround(17.5f * s), tx + tw - 1, y + h - (int)std::lround(1.5f * s) };
                DrawTextW(hdc, lbl.c_str(), -1, &rcLbl, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            }
        }
        DeleteObject(fTile);
    }

    SelectObject(hdc, oldF);
    DeleteObject(fCity);
    DeleteObject(fCurT);
    RestoreDC(hdc, saved);
}