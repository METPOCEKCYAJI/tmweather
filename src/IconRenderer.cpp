#include "IconRenderer.h"
#include "GdiPlusInclude.h"

using namespace Gdiplus;

namespace {
void AddCloudPath(GraphicsPath& path, float x, float y, float s) {
    // Full edge-to-edge cloud: spans [1.8..22.2] horizontally, [4.2..20.0] vertically
    path.AddEllipse(x + 1.8f * s, y + 8.6f * s, 10.6f * s, 10.4f * s);
    path.AddEllipse(x + 6.2f * s, y + 4.2f * s, 11.8f * s, 12.2f * s);
    path.AddEllipse(x + 12.8f * s, y + 9.2f * s, 9.4f * s, 9.8f * s);
    path.AddRectangle(RectF(x + 6.8f * s, y + 12.2f * s, 10.8f * s, 6.8f * s));
}

void DrawSolidCloud(Graphics& g, float x, float y, float size, Color cTop, Color cBot, bool useGrad) {
    float s = std::max(4.0f, size) / 24.0f;
    GraphicsPath path(FillModeWinding);
    AddCloudPath(path, x, y, s);
    if (useGrad) {
        LinearGradientBrush br(RectF(x, y + 4.0f * s, std::max(2.0f, size), std::max(2.0f, 16.0f * s)),
                               cTop, cBot, LinearGradientModeVertical);
        g.FillPath(&br, &path);
    } else {
        SolidBrush br(cTop);
        g.FillPath(&br, &path);
    }
}

void DrawOutlineCloud(Graphics& g, float x, float y, float size, Color col, float stroke_w) {
    float s = std::max(4.0f, size) / 24.0f;
    GraphicsPath p;
    p.AddArc(x + 2.2f * s, y + 9.5f * s, 9.0f * s, 9.0f * s, 90.0f, 155.0f);
    p.AddArc(x + 6.8f * s, y + 4.8f * s, 10.6f * s, 10.6f * s, 195.0f, 165.0f);
    p.AddArc(x + 13.0f * s, y + 9.8f * s, 8.6f * s, 8.6f * s, 275.0f, 175.0f);
    p.AddLine(x + 17.5f * s, y + 18.4f * s, x + 6.7f * s, y + 18.4f * s);
    p.CloseFigure();
    Pen pen(col, stroke_w);
    pen.SetLineJoin(LineJoinRound);
    g.DrawPath(&pen, &p);
}

void DrawSun(Graphics& g, float cx, float cy, float r, float ray_outer, Color cTop, Color cBot, float stroke_w, bool outlineOnly, bool useGrad) {
    if (outlineOnly) {
        Pen pen(cTop, stroke_w);
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        g.DrawEllipse(&pen, cx - r, cy - r, r * 2.0f, r * 2.0f);
        const float inner = r + stroke_w * 1.1f;
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
        return;
    }

    if (useGrad) {
        float gr = r * 1.38f;
        LinearGradientBrush br(RectF(cx - gr, cy - gr, std::max(2.0f, gr * 2.0f), std::max(2.0f, gr * 2.0f)),
                               cTop, cBot, LinearGradientModeForwardDiagonal);
        g.FillEllipse(&br, cx - gr, cy - gr, gr * 2.0f, gr * 2.0f);
        return;
    }

    SolidBrush br(cTop);
    g.FillEllipse(&br, cx - r, cy - r, r * 2.0f, r * 2.0f);
    Pen pen(cTop, stroke_w);
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

void DrawMoon(Graphics& g, float x, float y, float size, Color col, bool outlineOnly, float stroke_w) {
    float s = std::max(4.0f, size) / 24.0f;
    GraphicsPath outer, inner;
    outer.AddEllipse(x + 2.5f * s, y + 2.2f * s, 18.6f * s, 18.6f * s);
    inner.AddEllipse(x + 9.2f * s, y + 0.5f * s, 16.5f * s, 16.5f * s);
    Region reg(&outer);
    reg.Exclude(&inner);
    if (outlineOnly) {
        Pen pen(col, stroke_w);
        pen.SetLineJoin(LineJoinRound);
        g.DrawEllipse(&pen, x + 2.8f * s, y + 2.5f * s, 17.8f * s, 17.8f * s);
        SolidBrush mask(Color(220, 14, 16, 20));
        // Clean crescent outline via arc path
        GraphicsPath cp;
        cp.AddArc(x + 2.8f * s, y + 2.4f * s, 18.4f * s, 18.4f * s, 38.0f, 232.0f);
        cp.AddArc(x + 8.6f * s, y + 1.2f * s, 14.8f * s, 14.8f * s, 240.0f, -180.0f);
        cp.CloseFigure();
        g.DrawPath(&pen, &cp);
    } else {
        SolidBrush br(col);
        g.FillRegion(&br, &reg);
    }
}
} // namespace

void IconRenderer::DrawWeatherIcon(HDC hdc, float x, float y, float size, WeatherKind kind, bool dark_mode, IconStyle style) {
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    bool outline = (style == IconStyle::ThickOutline);
    bool soft3d  = (style == IconStyle::Soft3D);
    bool hcWhite = (style == IconStyle::HighContrastWhite);
    bool mono    = (style == IconStyle::Monochrome);

    Color fg = dark_mode ? Color(255, 255, 255) : Color(28, 32, 40);

    Color sunTop = (mono || hcWhite) ? fg : (soft3d ? Color(255, 242, 120) : Color(255, 212, 0));
    Color sunBot = (mono || hcWhite) ? fg : Color(255, 148, 0);
    Color moonCol = (mono || hcWhite) ? fg : (dark_mode ? Color(124, 196, 255) : Color(56, 128, 230));
    Color cloudTop = (mono || hcWhite || style == IconStyle::BoldVivid)
        ? (dark_mode ? Color(255, 255, 255) : Color(71, 85, 105))
        : (soft3d ? (dark_mode ? Color(255, 255, 255) : Color(148, 163, 184))
                  : (dark_mode ? Color(255, 255, 255) : Color(30, 41, 59)));
    Color cloudBot = soft3d
        ? (dark_mode ? Color(180, 198, 218) : Color(71, 85, 105))
        : cloudTop;

    Color rainCol = mono ? fg : (dark_mode ? Color(0, 186, 255) : Color(14, 130, 226));
    Color snowCol = mono ? fg : (dark_mode ? Color(145, 224, 255) : Color(38, 144, 235));
    Color boltCol = mono ? fg : Color(255, 212, 0);
    Color fogCol  = (mono || hcWhite) ? fg : (dark_mode ? Color(205, 214, 226) : Color(71, 85, 105));

    float s = std::max(4.0f, size) / 24.0f;
    float strokeW = std::max(1.6f, 2.2f * s);

    switch (kind) {
    case WeatherKind::ClearDay:
        DrawSun(g, x + 12.0f * s, y + 12.0f * s, 5.6f * s, 10.9f * s, sunTop, sunBot, std::max(1.5f, 2.3f * s), outline, soft3d);
        break;
    case WeatherKind::ClearNight:
        DrawMoon(g, x, y, size, moonCol, outline, strokeW);
        break;
    case WeatherKind::PartlyCloudyDay:
        DrawSun(g, x + 8.0f * s, y + 7.8f * s, 4.4f * s, 8.2f * s, sunTop, sunBot, std::max(1.3f, 1.9f * s), outline, soft3d);
        if (outline) {
            Color bg = dark_mode ? Color(255, 16, 18, 22) : Color(255, 240, 242, 245);
            GraphicsPath mask;
            AddCloudPath(mask, x + 1.5f * s, y + 2.2f * s, s * 0.92f);
            SolidBrush mb(bg);
            g.FillPath(&mb, &mask);
            DrawOutlineCloud(g, x + 1.5f * s, y + 2.2f * s, size * 0.92f, cloudTop, strokeW);
        } else {
            DrawSolidCloud(g, x + 1.5f * s, y + 2.2f * s, size * 0.92f, cloudTop, cloudBot, soft3d);
        }
        break;
    case WeatherKind::PartlyCloudyNight:
        DrawMoon(g, x - 2.2f * s, y - 1.5f * s, size * 0.72f, moonCol, outline, strokeW);
        if (outline) DrawOutlineCloud(g, x + 1.5f * s, y + 2.4f * s, size * 0.92f, cloudTop, strokeW);
        else DrawSolidCloud(g, x + 1.5f * s, y + 2.4f * s, size * 0.92f, cloudTop, cloudBot, soft3d);
        break;
    case WeatherKind::Cloudy:
    default:
        if (outline) DrawOutlineCloud(g, x, y + 0.5f * s, size, cloudTop, strokeW);
        else DrawSolidCloud(g, x, y + 0.5f * s, size, cloudTop, cloudBot, soft3d);
        break;
    case WeatherKind::Rain: {
        if (outline) DrawOutlineCloud(g, x, y - 3.0f * s, size * 0.95f, cloudTop, strokeW);
        else DrawSolidCloud(g, x, y - 3.0f * s, size * 0.95f, cloudTop, cloudBot, soft3d);
        Pen pen(rainCol, std::max(1.8f, 2.8f * s));
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        g.DrawLine(&pen, x + 8.6f * s, y + 16.4f * s, x + 6.8f * s, y + 21.8f * s);
        g.DrawLine(&pen, x + 15.6f * s, y + 16.4f * s, x + 13.8f * s, y + 21.8f * s);
        break;
    }
    case WeatherKind::Snow: {
        if (outline) DrawOutlineCloud(g, x, y - 3.0f * s, size * 0.95f, cloudTop, strokeW);
        else DrawSolidCloud(g, x, y - 3.0f * s, size * 0.95f, cloudTop, cloudBot, soft3d);
        SolidBrush br(snowCol);
        float r = std::max(1.4f, 1.9f * s);
        g.FillEllipse(&br, x + 7.2f * s - r, y + 18.8f * s - r, r * 2, r * 2);
        g.FillEllipse(&br, x + 12.2f * s - r, y + 20.6f * s - r, r * 2, r * 2);
        g.FillEllipse(&br, x + 17.2f * s - r, y + 18.8f * s - r, r * 2, r * 2);
        break;
    }
    case WeatherKind::Storm: {
        if (outline) DrawOutlineCloud(g, x, y - 3.0f * s, size * 0.95f, cloudTop, strokeW);
        else DrawSolidCloud(g, x, y - 3.0f * s, size * 0.95f, cloudTop, cloudBot, soft3d);
        PointF pts[6] = {
            {x + 12.8f * s, y + 13.4f * s},
            {x + 9.0f * s,  y + 18.4f * s},
            {x + 12.0f * s, y + 18.4f * s},
            {x + 10.4f * s, y + 23.0f * s},
            {x + 16.0f * s, y + 17.0f * s},
            {x + 12.8f * s, y + 17.0f * s}
        };
        SolidBrush br(boltCol);
        g.FillPolygon(&br, pts, 6);
        break;
    }
    case WeatherKind::Fog: {
        Pen pen(fogCol, std::max(1.8f, 2.5f * s));
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        g.DrawLine(&pen, x + 3.0f * s, y + 7.0f * s, x + 21.0f * s, y + 7.0f * s);
        g.DrawLine(&pen, x + 2.0f * s, y + 12.0f * s, x + 18.5f * s, y + 12.0f * s);
        g.DrawLine(&pen, x + 5.5f * s, y + 17.0f * s, x + 22.0f * s, y + 17.0f * s);
        break;
    }
    }
}

void IconRenderer::DrawRainDrop(HDC hdc, float x, float y, float size, bool dark_mode, IconStyle style) {
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    bool mono = (style == IconStyle::Monochrome);
    Color col = mono ? (dark_mode ? Color(255, 255, 255) : Color(28, 32, 40))
                     : (dark_mode ? Color(0, 186, 255) : Color(14, 130, 226));
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