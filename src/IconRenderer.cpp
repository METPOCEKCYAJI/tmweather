#include "IconRenderer.h"
#include "GdiPlusInclude.h"

using namespace Gdiplus;

void IconRenderer::DrawTileBackdrop(HDC hdc, float x, float y, float w, float h, WeatherKind kind, bool dark_mode, IconStyle style) {
    if (w < 2.0f || h < 2.0f) return;
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    RectF rc(x, y, w, h);
    g.SetClip(rc);

    if (style == IconStyle::TopBar) {
        // Concept 3: Clean card background + distinct top 3px weather bar
        Color bg = dark_mode ? Color(18, 255, 255, 255) : Color(185, 255, 255, 255);
        SolidBrush bgBr(bg);
        g.FillRectangle(&bgBr, rc);

        float barH = std::max(2.5f, h * 0.11f);
        switch (kind) {
        case WeatherKind::ClearDay: {
            SolidBrush b(Color(255, 250, 204, 21));
            g.FillRectangle(&b, x, y, w, barH);
            break;
        }
        case WeatherKind::ClearNight: {
            SolidBrush b(Color(255, 59, 130, 246));
            g.FillRectangle(&b, x, y, w, barH);
            break;
        }
        case WeatherKind::PartlyCloudyDay: {
            SolidBrush b1(Color(255, 250, 204, 21));
            SolidBrush b2(Color(255, 148, 163, 184));
            g.FillRectangle(&b1, x, y, w * 0.45f, barH);
            g.FillRectangle(&b2, x + w * 0.45f, y, w * 0.55f, barH);
            break;
        }
        case WeatherKind::PartlyCloudyNight: {
            SolidBrush b1(Color(255, 96, 165, 250));
            SolidBrush b2(Color(255, 148, 163, 184));
            g.FillRectangle(&b1, x, y, w * 0.45f, barH);
            g.FillRectangle(&b2, x + w * 0.45f, y, w * 0.55f, barH);
            break;
        }
        case WeatherKind::Cloudy:
        case WeatherKind::Fog:
        default: {
            SolidBrush b(Color(255, 148, 163, 184));
            g.FillRectangle(&b, x, y, w, barH);
            break;
        }
        case WeatherKind::Rain: {
            SolidBrush bBg(Color(255, 29, 78, 216));
            SolidBrush bStr(Color(255, 56, 189, 248));
            g.FillRectangle(&bBg, x, y, w, barH);
            for (float px = x + 2.0f; px < x + w; px += 7.0f) {
                g.FillRectangle(&bStr, px, y, 3.8f, barH);
            }
            break;
        }
        case WeatherKind::Storm: {
            SolidBrush b1(Color(255, 99, 102, 241));
            SolidBrush b2(Color(255, 250, 204, 21));
            g.FillRectangle(&b1, x, y, w, barH);
            g.FillRectangle(&b2, x + w * 0.32f, y, w * 0.36f, barH);
            break;
        }
        case WeatherKind::Snow: {
            SolidBrush bBg(Color(255, 2, 132, 199));
            SolidBrush bDot(Color(255, 224, 242, 254));
            g.FillRectangle(&bBg, x, y, w, barH);
            for (float px = x + 2.0f; px < x + w; px += 6.0f) {
                g.FillRectangle(&bDot, px, y, 2.8f, barH);
            }
            break;
        }
        }
        g.ResetClip();
        return;
    }

    // Concept 1A: Sky Atmosphere Cards
    Color c1(255, 71, 85, 105), c2(255, 30, 41, 59);
    switch (kind) {
    case WeatherKind::ClearDay:
        c1 = Color(255, 194, 92, 10);  c2 = Color(255, 78, 30, 5);
        break;
    case WeatherKind::ClearNight:
        c1 = Color(255, 30, 27, 75);   c2 = Color(255, 9, 13, 24);
        break;
    case WeatherKind::PartlyCloudyDay:
        c1 = Color(255, 154, 72, 14);  c2 = Color(255, 30, 41, 59);
        break;
    case WeatherKind::PartlyCloudyNight:
        c1 = Color(255, 30, 41, 82);   c2 = Color(255, 30, 41, 59);
        break;
    case WeatherKind::Cloudy:
    case WeatherKind::Fog:
    default:
        c1 = Color(255, 75, 88, 106);  c2 = Color(255, 30, 41, 59);
        break;
    case WeatherKind::Rain:
        c1 = Color(255, 29, 78, 216);  c2 = Color(255, 15, 23, 42);
        break;
    case WeatherKind::Storm:
        c1 = Color(255, 55, 48, 163);  c2 = Color(255, 15, 23, 42);
        break;
    case WeatherKind::Snow:
        c1 = Color(255, 2, 132, 199);  c2 = Color(255, 30, 41, 59);
        break;
    }

    LinearGradientBrush bgBr(rc, c1, c2, LinearGradientModeForwardDiagonal);
    g.FillRectangle(&bgBr, rc);

    float s = h / 30.0f;
    switch (kind) {
    case WeatherKind::ClearDay: {
        SolidBrush sunBr(Color(165, 251, 191, 36));
        float r = 10.5f * s;
        g.FillEllipse(&sunBr, x + w - r * 1.15f, y - r * 0.65f, r * 2.0f, r * 2.0f);
        break;
    }
    case WeatherKind::ClearNight: {
        GraphicsPath outer, inner;
        float cx = x + w - 13.0f * s, cy = y + 1.0f * s;
        outer.AddEllipse(cx, cy, 13.0f * s, 13.0f * s);
        inner.AddEllipse(cx + 4.5f * s, cy - 1.2f * s, 11.5f * s, 11.5f * s);
        Region reg(&outer);
        reg.Exclude(&inner);
        SolidBrush moonBr(Color(185, 147, 197, 253));
        g.FillRegion(&moonBr, &reg);
        break;
    }
    case WeatherKind::PartlyCloudyDay: {
        SolidBrush sunBr(Color(180, 251, 191, 36));
        float r = 7.8f * s;
        g.FillEllipse(&sunBr, x - r * 0.35f, y - r * 0.45f, r * 2.0f, r * 2.0f);
        SolidBrush clBr(Color(105, 203, 213, 225));
        g.FillEllipse(&clBr, x + w * 0.38f, y + h * 0.46f, w * 0.45f, h * 0.72f);
        g.FillEllipse(&clBr, x + w * 0.60f, y + h * 0.36f, w * 0.52f, h * 0.85f);
        break;
    }
    case WeatherKind::PartlyCloudyNight: {
        SolidBrush moonBr(Color(160, 147, 197, 253));
        g.FillEllipse(&moonBr, x + 2.0f * s, y + 1.5f * s, 7.5f * s, 7.5f * s);
        SolidBrush clBr(Color(105, 203, 213, 225));
        g.FillEllipse(&clBr, x + w * 0.42f, y + h * 0.46f, w * 0.45f, h * 0.72f);
        g.FillEllipse(&clBr, x + w * 0.62f, y + h * 0.36f, w * 0.50f, h * 0.85f);
        break;
    }
    case WeatherKind::Cloudy: {
        SolidBrush clBr(Color(90, 148, 163, 184));
        g.FillEllipse(&clBr, x - 3.0f * s, y + h * 0.52f, w * 0.55f, h * 0.72f);
        g.FillEllipse(&clBr, x + w * 0.34f, y + h * 0.38f, w * 0.74f, h * 0.86f);
        break;
    }
    case WeatherKind::Fog: {
        Pen fogPen(Color(115, 226, 232, 240), std::max(1.4f, 2.2f * s));
        fogPen.SetStartCap(LineCapRound);
        fogPen.SetEndCap(LineCapRound);
        g.DrawLine(&fogPen, x + 3.0f * s, y + 4.0f * s, x + w - 8.0f * s, y + 4.0f * s);
        g.DrawLine(&fogPen, x + 6.0f * s, y + h - 4.0f * s, x + w - 3.0f * s, y + h - 4.0f * s);
        break;
    }
    case WeatherKind::Rain: {
        Pen rainPen(Color(165, 96, 165, 250), std::max(1.5f, 2.0f * s));
        rainPen.SetStartCap(LineCapRound);
        rainPen.SetEndCap(LineCapRound);
        g.DrawLine(&rainPen, x + 9.0f * s, y + 2.0f * s, x + 4.0f * s, y + 12.0f * s);
        g.DrawLine(&rainPen, x + w - 5.0f * s, y + 4.0f * s, x + w - 10.0f * s, y + 15.0f * s);
        g.DrawLine(&rainPen, x + w - 4.0f * s, y + h - 10.0f * s, x + w - 8.0f * s, y + h - 2.0f * s);
        break;
    }
    case WeatherKind::Storm: {
        PointF pts[6] = {
            {x + w - 9.0f * s,  y + 2.0f * s},
            {x + w - 14.0f * s, y + 13.0f * s},
            {x + w - 9.0f * s,  y + 13.0f * s},
            {x + w - 12.0f * s, y + 24.0f * s},
            {x + w - 3.0f * s,  y + 11.0f * s},
            {x + w - 8.0f * s,  y + 11.0f * s}
        };
        SolidBrush boltBr(Color(210, 250, 204, 21));
        g.FillPolygon(&boltBr, pts, 6);
        break;
    }
    case WeatherKind::Snow: {
        SolidBrush snBr(Color(190, 224, 242, 254));
        float r = std::max(1.3f, 1.9f * s);
        g.FillEllipse(&snBr, x + 5.0f * s, y + 4.0f * s, r * 2, r * 2);
        g.FillEllipse(&snBr, x + w - 9.0f * s, y + 6.0f * s, r * 2, r * 2);
        g.FillEllipse(&snBr, x + 6.0f * s, y + h - 8.0f * s, r * 2, r * 2);
        g.FillEllipse(&snBr, x + w - 10.0f * s, y + h - 7.0f * s, r * 2, r * 2);
        break;
    }
    }

    g.ResetClip();
}

void IconRenderer::DrawRainDrop(HDC hdc, float x, float y, float size, bool dark_mode) {
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    Color col = dark_mode ? Color(255, 96, 165, 250) : Color(255, 14, 130, 226);
    float s = std::max(4.0f, size) / 24.0f;
    GraphicsPath path;
    path.AddBezier(x + 12.0f * s, y + 2.0f * s,
                   x + 12.0f * s, y + 2.0f * s,
                   x + 19.0f * s, y + 10.5f * s,
                   x + 19.0f * s, y + 15.0f * s);
    path.AddArc(x + 5.0f * s, y + 8.0f * s, 14.0f * s, 14.0f * s, 0.0f, 180.0f);
    path.AddBezier(x + 5.0f * s, y + 15.0f * s,
                   x + 5.0f * s, y + 10.5f * s,
                   x + 12.0f * s, y + 2.0f * s,
                   x + 12.0f * s, y + 2.0f * s);
    SolidBrush br(col);
    g.FillPath(&br, &path);
}
