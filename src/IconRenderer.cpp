#include "IconRenderer.h"
#include "GdiPlusInclude.h"

using namespace Gdiplus;

namespace {
void AddCloudPath(GraphicsPath& path, float x, float y, float s) {
    path.AddEllipse(x + 3.4f * s, y + 9.2f * s, 8.6f * s, 8.2f * s);
    path.AddEllipse(x + 7.0f * s, y + 5.8f * s, 9.6f * s, 9.6f * s);
    path.AddEllipse(x + 12.6f * s, y + 9.8f * s, 7.8f * s, 7.6f * s);
    path.AddRectangle(RectF(x + 7.2f * s, y + 12.2f * s, 9.6f * s, 5.2f * s));
}

void DrawSun(Graphics& g, float cx, float cy, float r, float ray_outer, Color col, float stroke_w) {
    SolidBrush br(col);
    g.FillEllipse(&br, cx - r, cy - r, r * 2.0f, r * 2.0f);
    Pen pen(col, stroke_w);
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    const float inner = r + stroke_w * 0.85f;
    const float d1 = inner * 0.7071f;
    const float d2 = ray_outer * 0.7071f;
    g.DrawLine(&pen, cx, cy - inner, cx, cy - ray_outer);
    g.DrawLine(&pen, cx, cy + inner, cx, cy + ray_outer);
    g.DrawLine(&pen, cx - inner, cy, cx - ray_outer, cy);
    g.DrawLine(&pen, cx + inner, cy, cx + ray_outer, cy);
    g.DrawLine(&pen, cx - d1, cy - d1, cx - d2, cy - d2);
    g.DrawLine(&pen, cx + d1, cy + d1, cx + d2, cy + d2);
    g.DrawLine(&pen, cx + d1, cy - d1, cx + d2, cy - d2);
    g.DrawLine(&pen, cx - d1, cy + d1, cx - d2, cy + d2);
}

void DrawMoon(Graphics& g, float x, float y, float size, Color col) {
    float s = size / 24.0f;
    GraphicsPath outer, inner;
    outer.AddEllipse(x + 4.5f * s, y + 4.0f * s, 15.0f * s, 15.0f * s);
    inner.AddEllipse(x + 9.8f * s, y + 2.4f * s, 13.4f * s, 13.4f * s);
    Region reg(&outer);
    reg.Exclude(&inner);
    SolidBrush br(col);
    g.FillRegion(&br, &reg);
}

void DrawSolidCloud(Graphics& g, float x, float y, float size, Color col) {
    float s = size / 24.0f;
    GraphicsPath path(FillModeWinding);
    AddCloudPath(path, x, y, s);
    SolidBrush br(col);
    g.FillPath(&br, &path);
}
} // namespace

void IconRenderer::DrawWeatherIcon(HDC hdc, float x, float y, float size, WeatherKind kind, bool dark_mode, IconStyle style) {
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    bool mono = (style == IconStyle::Monochrome);
    Color fg = dark_mode ? Color(245, 246, 250) : Color(32, 37, 45);
    Color sunCol = mono ? fg : Color(255, 202, 44);
    Color moonCol = mono ? fg : (dark_mode ? Color(138, 194, 255) : Color(72, 132, 224));
    Color cloudCol = mono ? fg : (dark_mode ? Color(230, 237, 245) : Color(100, 116, 139));
    Color rainCol = mono ? fg : (dark_mode ? Color(64, 186, 255) : Color(14, 136, 230));
    Color snowCol = mono ? fg : (dark_mode ? Color(165, 226, 255) : Color(56, 152, 236));
    Color fogCol = mono ? fg : (dark_mode ? Color(176, 188, 204) : Color(100, 116, 139));

    float s = size / 24.0f;

    switch (kind) {
    case WeatherKind::ClearDay:
        DrawSun(g, x + 12.0f * s, y + 12.0f * s, 5.1f * s, 9.8f * s, sunCol, std::max(1.4f, 2.0f * s));
        break;
    case WeatherKind::ClearNight:
        DrawMoon(g, x, y, size, moonCol);
        break;
    case WeatherKind::PartlyCloudyDay:
        DrawSun(g, x + 8.6f * s, y + 8.4f * s, 4.0f * s, 7.2f * s, sunCol, std::max(1.2f, 1.7f * s));
        DrawSolidCloud(g, x + 1.8f * s, y + 2.4f * s, size * 0.90f, cloudCol);
        break;
    case WeatherKind::PartlyCloudyNight:
        DrawMoon(g, x - 2.0f * s, y - 1.5f * s, size * 0.72f, moonCol);
        DrawSolidCloud(g, x + 1.8f * s, y + 2.6f * s, size * 0.90f, cloudCol);
        break;
    case WeatherKind::Cloudy:
    default:
        DrawSolidCloud(g, x, y + 0.5f * s, size, cloudCol);
        break;
    case WeatherKind::Rain: {
        DrawSolidCloud(g, x, y - 2.2f * s, size, cloudCol);
        Pen pen(rainCol, std::max(1.5f, 2.5f * s));
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        g.DrawLine(&pen, x + 9.0f * s, y + 16.5f * s, x + 7.4f * s, y + 20.8f * s);
        g.DrawLine(&pen, x + 15.2f * s, y + 16.5f * s, x + 13.6f * s, y + 20.8f * s);
        break;
    }
    case WeatherKind::Snow: {
        DrawSolidCloud(g, x, y - 2.2f * s, size, cloudCol);
        SolidBrush br(snowCol);
        float r = std::max(1.2f, 1.7f * s);
        g.FillEllipse(&br, x + 7.5f * s - r, y + 18.4f * s - r, r * 2, r * 2);
        g.FillEllipse(&br, x + 12.2f * s - r, y + 20.0f * s - r, r * 2, r * 2);
        g.FillEllipse(&br, x + 16.6f * s - r, y + 18.0f * s - r, r * 2, r * 2);
        break;
    }
    case WeatherKind::Storm: {
        DrawSolidCloud(g, x, y - 2.2f * s, size, cloudCol);
        PointF pts[6] = {
            {x + 12.6f * s, y + 13.8f * s},
            {x + 9.4f * s,  y + 18.2f * s},
            {x + 12.0f * s, y + 18.2f * s},
            {x + 10.6f * s, y + 22.4f * s},
            {x + 15.2f * s, y + 17.0f * s},
            {x + 12.4f * s, y + 17.0f * s}
        };
        SolidBrush br(sunCol);
        g.FillPolygon(&br, pts, 6);
        break;
    }
    case WeatherKind::Fog: {
        Pen pen(fogCol, std::max(1.6f, 2.2f * s));
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        g.DrawLine(&pen, x + 4.5f * s, y + 7.5f * s, x + 19.5f * s, y + 7.5f * s);
        g.DrawLine(&pen, x + 3.0f * s, y + 12.0f * s, x + 17.5f * s, y + 12.0f * s);
        g.DrawLine(&pen, x + 6.0f * s, y + 16.5f * s, x + 20.5f * s, y + 16.5f * s);
        break;
    }
    }
}

void IconRenderer::DrawRainDrop(HDC hdc, float x, float y, float size, bool dark_mode, IconStyle style) {
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    bool mono = (style == IconStyle::Monochrome);
    Color col = mono ? (dark_mode ? Color(245, 246, 250) : Color(32, 37, 45))
                     : (dark_mode ? Color(64, 186, 255) : Color(14, 136, 230));
    float s = size / 24.0f;
    GraphicsPath path;
    path.AddBezier(x + 12.0f * s, y + 2.5f * s,
                   x + 12.0f * s, y + 2.5f * s,
                   x + 18.5f * s, y + 10.5f * s,
                   x + 18.5f * s, y + 14.8f * s);
    path.AddArc(x + 5.5f * s, y + 8.3f * s, 13.0f * s, 13.0f * s, 0.0f, 180.0f);
    path.AddBezier(x + 5.5f * s, y + 14.8f * s,
                   x + 5.5f * s, y + 10.5f * s,
                   x + 12.0f * s, y + 2.5f * s,
                   x + 12.0f * s, y + 2.5f * s);
    SolidBrush br(col);
    g.FillPath(&br, &path);
}