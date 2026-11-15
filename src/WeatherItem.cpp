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

void DrawHalfCard(HDC hdc, int x, int y, int w, int h, float s, bool is_top, bool dark_mode, BorderStyle style) {
    if (style == BorderStyle::Minimal) return;
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    Color fillCol = dark_mode ? Color(18, 255, 255, 255) : Color(165, 255, 255, 255);
    SolidBrush fillBr(fillCol);

    float rad = 4.5f * s;
    float d = rad * 2.0f;
    float rx = (float)x + 0.5f;
    float rw = (float)w - 1.5f;
    float ry = is_top ? ((float)y + 0.5f) : (float)y;
    float rh = is_top ? ((float)h - 0.5f) : ((float)h - 1.0f);

    GraphicsPath p;
    if (is_top) {
        p.AddArc(rx, ry, d, d, 180, 90);
        p.AddArc(rx + rw - d, ry, d, d, 270, 90);
        p.AddLine(rx + rw, ry + rh, rx, ry + rh);
        p.CloseFigure();
        g.FillPath(&fillBr, &p);
        if (style == BorderStyle::BorderAndFill) {
            Color borderCol = dark_mode ? Color(52, 255, 255, 255) : Color(48, 0, 0, 0);
            Pen pen(borderCol, 1.0f);
            GraphicsPath b;
            b.AddLine(rx, ry + rh, rx, ry + rad);
            b.AddArc(rx, ry, d, d, 180, 90);
            b.AddArc(rx + rw - d, ry, d, d, 270, 90);
            b.AddLine(rx + rw, ry + rad, rx + rw, ry + rh);
            g.DrawPath(&pen, &b);
        }
    } else {
        p.AddLine(rx, ry, rx + rw, ry);
        p.AddArc(rx + rw - d, ry + rh - d, d, d, 0, 90);
        p.AddArc(rx, ry + rh - d, d, d, 90, 90);
        p.CloseFigure();
        g.FillPath(&fillBr, &p);
        if (style == BorderStyle::BorderAndFill) {
            Color borderCol = dark_mode ? Color(52, 255, 255, 255) : Color(48, 0, 0, 0);
            Pen pen(borderCol, 1.0f);
            GraphicsPath b;
            b.AddLine(rx + rw, ry, rx + rw, ry + rh - rad);
            b.AddArc(rx + rw - d, ry + rh - d, d, d, 0, 90);
            b.AddArc(rx, ry + rh - d, d, d, 90, 90);
            b.AddLine(rx, ry + rh - rad, rx, ry);
            g.DrawPath(&pen, &b);
        }
    }
}
} // namespace

WeatherItem::WeatherItem(WeatherPlugin* owner, bool is_top) : m_owner(owner), m_isTop(is_top) {}

const wchar_t* WeatherItem::GetItemName() const {
    LangMode l = m_owner->GetConfig().lang;
    m_nameBuf = m_isTop ? L10n::TopItemName(l) : L10n::BottomItemName(l);
    return m_nameBuf.c_str();
}

const wchar_t* WeatherItem::GetItemId() const {
    return m_isTop ? L"WeatherPlus_Top" : L"WeatherPlus_Bottom";
}

const wchar_t* WeatherItem::GetItemLableText() const { return L""; }
const wchar_t* WeatherItem::GetItemValueText() const { return L""; }
const wchar_t* WeatherItem::GetItemValueSampleText() const { return L""; }

int WeatherItem::GetItemWidth() const {
    auto cfg = m_owner->GetConfig();
    switch (cfg.display_mode) {
    case DisplayMode::Compact:  return 54;
    case DisplayMode::Medium:   return 160; // 72 + 2*44
    case DisplayMode::Extended: return (cfg.extended_slots == 3) ? 208 : 252; // 76 + 3*44 or 4*44
    }
    return 160;
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

    float s = (float)h / 16.0f;

    COLORREF colPrimary  = dark_mode ? RGB(246, 248, 252) : RGB(22, 26, 32);
    COLORREF colMuted    = dark_mode ? RGB(172, 182, 198) : RGB(84, 94, 110);
    COLORREF colTomorrow = dark_mode ? RGB(110, 224, 255) : RGB(3, 105, 161);

    DrawHalfCard(hdc, x, y, w, h, s, m_isTop, dark_mode, cfg.border_style);

    wchar_t tempBuf[16] = L"--\u00B0";
    if (snap.valid) FormatTemp(snap.current_temp_c, cfg.unit, tempBuf, 16);
    WeatherKind curKind = snap.valid ? snap.current_kind : WeatherKind::Cloudy;

    // 1. Compact mode: Top = full 15x15 icon centered (+ rain badge), Bottom = centered bold temp
    if (cfg.display_mode == DisplayMode::Compact) {
        if (m_isTop) {
            float icoSize = (float)(h - 1);
            IconRenderer::DrawWeatherIcon(hdc, x + (w - icoSize) * 0.5f, (float)y + 0.5f, icoSize, curKind, dark_mode, cfg.icon_style);
            if (snap.valid && snap.rain_expected) {
                IconRenderer::DrawRainDrop(hdc, (float)(x + w) - 10.5f * s, (float)y + 2.0f * s, 8.5f * s, dark_mode, cfg.icon_style);
            }
        } else {
            HFONT fTemp = CreateUiFont(std::max(10, (int)std::lround(13.0f * s)), true);
            HFONT oldF = (HFONT)SelectObject(hdc, fTemp);
            SetTextColor(hdc, colPrimary);
            RECT rc{ x + 2, y, x + w - 2, y + h };
            DrawTextW(hdc, tempBuf, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            SelectObject(hdc, oldF);
            DeleteObject(fTemp);
        }
        RestoreDC(hdc, saved);
        return;
    }

    // 2. Medium & Extended modes
    int curW = (int)std::lround((cfg.display_mode == DisplayMode::Medium ? 72.0f : 76.0f) * s);
    curW = std::min(curW, w);

    Graphics g(hdc);
    Color divCol = dark_mode ? Color(34, 255, 255, 255) : Color(34, 0, 0, 0);
    Pen divPen(divCol, 1.0f);

    if (m_isTop) {
        // Left zone: 15x15 hero icon on left + City centered in [x + 21*s .. x + curW - 3*s]
        float icoSize = (float)(h - 1);
        IconRenderer::DrawWeatherIcon(hdc, (float)x + 4.0f * s, (float)y + 0.5f, icoSize, curKind, dark_mode, cfg.icon_style);

        std::wstring cityText = snap.valid ? snap.city_name : cfg.EffectiveCityName();
        HFONT fCity = CreateUiFont(std::max(9, (int)std::lround(11.0f * s)), false);
        HFONT oldF = (HFONT)SelectObject(hdc, fCity);
        SetTextColor(hdc, colMuted);
        RECT rcCity{ x + (int)std::lround(21.0f * s), y, x + curW - (int)std::lround(3.0f * s), y + h };
        DrawTextW(hdc, cityText.c_str(), -1, &rcCity, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
        SelectObject(hdc, oldF);
        DeleteObject(fCity);
    } else {
        // Bottom left zone: Rain drop under hero icon (when raining) + Temp CENTERED under city!
        if (snap.valid && snap.rain_expected) {
            float dropSize = 11.0f * s;
            IconRenderer::DrawRainDrop(hdc, (float)x + 6.5f * s, (float)y + (h - dropSize) * 0.5f, dropSize, dark_mode, cfg.icon_style);
        }
        HFONT fCurT = CreateUiFont(std::max(10, (int)std::lround(13.0f * s)), true);
        HFONT oldF = (HFONT)SelectObject(hdc, fCurT);
        SetTextColor(hdc, colPrimary);
        RECT rcCurT{ x + (int)std::lround(21.0f * s), y, x + curW - (int)std::lround(3.0f * s), y + h };
        DrawTextW(hdc, tempBuf, -1, &rcCurT, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        SelectObject(hdc, oldF);
        DeleteObject(fCurT);
    }

    int numTiles = (cfg.display_mode == DisplayMode::Medium) ? 2 : cfg.extended_slots;
    int remW = w - curW;
    if (numTiles > 0 && remW > 10) {
        float divTop = m_isTop ? ((float)y + 2.0f * s) : (float)y;
        float divBot = m_isTop ? (float)(y + h) : ((float)(y + h) - 2.0f * s);
        g.DrawLine(&divPen, (float)(x + curW), divTop, (float)(x + curW), divBot);

        float tileW = (float)remW / (float)numTiles;
        int totalSlots = (int)snap.slots.size();
        int maxStart = std::max(0, totalSlots - numTiles);
        int start = std::clamp(scroll_idx, 0, maxStart);

        HFONT fTile = CreateUiFont(std::max(8, (int)std::lround(10.5f * s)), false);
        HFONT oldF = (HFONT)SelectObject(hdc, fTile);

        for (int i = 0; i < numTiles; ++i) {
            int tx = x + curW + (int)std::lround(i * tileW);
            int tw = (int)std::lround((i + 1) * tileW) - (int)std::lround(i * tileW);

            if (i > 0) {
                g.DrawLine(&divPen, (float)tx, divTop, (float)tx, divBot);
            }

            int idx = start + i;
            if (snap.valid && idx < totalSlots) {
                const auto& slot = snap.slots[idx];
                if (m_isTop) {
                    float tileIco = (float)(h - 1);
                    IconRenderer::DrawWeatherIcon(hdc, tx + (tw - tileIco) * 0.5f, (float)y + 0.5f,
                                                  tileIco, slot.kind, dark_mode, cfg.icon_style);
                } else {
                    wchar_t tStr[12]{};
                    FormatTemp(slot.temp_c, cfg.unit, tStr, 12);
                    std::wstring prefix;
                    if (!slot.is_today && slot.day_num > 0) {
                        prefix = std::to_wstring(slot.day_num);
                    }
                    prefix += L10n::DayPartShort(cfg.lang, slot.part);
                    std::wstring lbl = prefix + L" " + tStr;

                    SetTextColor(hdc, slot.is_today ? colPrimary : colTomorrow);
                    RECT rcLbl{ tx + 1, y, tx + tw - 1, y + h };
                    DrawTextW(hdc, lbl.c_str(), -1, &rcLbl, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
                }
            }
        }
        SelectObject(hdc, oldF);
        DeleteObject(fTile);
    }

    RestoreDC(hdc, saved);
}