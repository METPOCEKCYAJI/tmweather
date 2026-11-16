#include "IconRenderer.h"
#include "GdiPlusInclude.h"
#include <algorithm>

using namespace Gdiplus;

namespace {
struct BarPalette {
    Color sun, moon, cloud, rainBg, rainStr, stormEdge, stormBolt, snowBg, snowDot;
};

BarPalette GetBarPalette(int contrast, bool dark_mode) {
    BarPalette p;
    if (contrast == 2) {
        p.sun       = Color(255, 255, 240, 0);
        p.moon      = Color(255, 80, 165, 255);
        p.cloud     = dark_mode ? Color(255, 241, 245, 249) : Color(255, 51, 65, 85);
        p.rainBg    = Color(255, 29, 78, 216);
        p.rainStr   = Color(255, 0, 235, 255);
        p.stormEdge = Color(255, 139, 92, 246);
        p.stormBolt = Color(255, 255, 240, 0);
        p.snowBg    = Color(255, 0, 150, 230);
        p.snowDot   = Color(255, 255, 255, 255);
    } else if (contrast == 1) {
        p.sun       = Color(255, 255, 224, 24);
        p.moon      = Color(255, 96, 165, 250);
        p.cloud     = dark_mode ? Color(255, 203, 213, 225) : Color(255, 82, 98, 118);
        p.rainBg    = Color(255, 29, 78, 216);
        p.rainStr   = Color(255, 56, 204, 255);
        p.stormEdge = Color(255, 110, 108, 246);
        p.stormBolt = Color(255, 255, 224, 24);
        p.snowBg    = Color(255, 2, 132, 199);
        p.snowDot   = Color(255, 235, 248, 255);
    } else {
        p.sun       = Color(255, 245, 190, 24);
        p.moon      = Color(255, 59, 130, 246);
        p.cloud     = Color(255, 148, 163, 184);
        p.rainBg    = Color(255, 30, 64, 175);
        p.rainStr   = Color(255, 56, 189, 248);
        p.stormEdge = Color(255, 99, 102, 241);
        p.stormBolt = Color(255, 250, 204, 21);
        p.snowBg    = Color(255, 3, 105, 161);
        p.snowDot   = Color(255, 210, 236, 252);
    }
    return p;
}

void DrawHorizontalBar(Graphics& g, float x, float y, float w, float barH, WeatherKind kind, const BarPalette& pal) {
    switch (kind) {
    case WeatherKind::ClearDay: {
        SolidBrush b(pal.sun);
        g.FillRectangle(&b, x, y, w, barH);
        break;
    }
    case WeatherKind::ClearNight: {
        SolidBrush b(pal.moon);
        g.FillRectangle(&b, x, y, w, barH);
        break;
    }
    case WeatherKind::PartlyCloudyDay: {
        SolidBrush b1(pal.sun), b2(pal.cloud);
        g.FillRectangle(&b1, x, y, w * 0.45f, barH);
        g.FillRectangle(&b2, x + w * 0.45f, y, w * 0.55f, barH);
        break;
    }
    case WeatherKind::PartlyCloudyNight: {
        SolidBrush b1(pal.moon), b2(pal.cloud);
        g.FillRectangle(&b1, x, y, w * 0.45f, barH);
        g.FillRectangle(&b2, x + w * 0.45f, y, w * 0.55f, barH);
        break;
    }
    case WeatherKind::Cloudy:
    case WeatherKind::Fog:
    default: {
        SolidBrush b(pal.cloud);
        g.FillRectangle(&b, x, y, w, barH);
        break;
    }
    case WeatherKind::Rain: {
        SolidBrush bBg(pal.rainBg), bStr(pal.rainStr);
        g.FillRectangle(&bBg, x, y, w, barH);
        for (float px = x + 2.0f; px < x + w; px += 7.0f) {
            g.FillRectangle(&bStr, px, y, std::min(3.8f, x + w - px), barH);
        }
        break;
    }
    case WeatherKind::Storm: {
        SolidBrush b1(pal.stormEdge), b2(pal.stormBolt);
        g.FillRectangle(&b1, x, y, w, barH);
        g.FillRectangle(&b2, x + w * 0.32f, y, w * 0.36f, barH);
        break;
    }
    case WeatherKind::Snow: {
        SolidBrush bBg(pal.snowBg), bDot(pal.snowDot);
        g.FillRectangle(&bBg, x, y, w, barH);
        for (float px = x + 2.0f; px < x + w; px += 6.0f) {
            g.FillRectangle(&bDot, px, y, std::min(2.8f, x + w - px), barH);
        }
        break;
    }
    }
}

void DrawVerticalPill(Graphics& g, float x, float y, float w, float h, WeatherKind kind, const BarPalette& pal) {
    float pw = std::max(2.0f, w);
    float ph = std::max(6.0f, h);
    float d = pw;
    GraphicsPath p;
    p.AddArc(x, y, d, d, 180, 180);
    p.AddArc(x, y + ph - d, d, d, 0, 180);
    p.CloseFigure();

    Color c1 = pal.cloud, c2 = pal.cloud;
    switch (kind) {
    case WeatherKind::ClearDay:          c1 = c2 = pal.sun; break;
    case WeatherKind::ClearNight:        c1 = c2 = pal.moon; break;
    case WeatherKind::PartlyCloudyDay:   c1 = pal.sun; c2 = pal.cloud; break;
    case WeatherKind::PartlyCloudyNight: c1 = pal.moon; c2 = pal.cloud; break;
    case WeatherKind::Rain:              c1 = pal.rainStr; c2 = pal.rainBg; break;
    case WeatherKind::Storm:             c1 = pal.stormBolt; c2 = pal.stormEdge; break;
    case WeatherKind::Snow:              c1 = pal.snowDot; c2 = pal.snowBg; break;
    default: break;
    }
    LinearGradientBrush br(RectF(x, y, pw, ph), c1, c2, LinearGradientModeVertical);
    g.FillPath(&br, &p);
}

void DrawVignetteOverlay(Graphics& g, float x, float y, float w, float h, int level, Color baseTone) {
    if (level <= 0) return;
    BYTE alpha = 155;
    float focus = 0.20f;
    switch (level) {
    case 1: alpha = 165; focus = 0.22f; break;
    case 2: alpha = 205; focus = 0.34f; break;
    case 3: alpha = 232; focus = 0.44f; break;
    default: alpha = 248; focus = 0.56f; break;
    }
    GraphicsPath ell;
    ell.AddEllipse(x - w * 0.20f, y - h * 0.20f, w * 1.40f, h * 1.40f);
    PathGradientBrush vig(&ell);
    vig.SetCenterPoint(PointF(x + w * 0.5f, y + h * 0.52f));
    vig.SetCenterColor(Color(alpha, baseTone.GetR(), baseTone.GetG(), baseTone.GetB()));
    Color surr(0, baseTone.GetR(), baseTone.GetG(), baseTone.GetB());
    int cnt = 1;
    vig.SetSurroundColors(&surr, &cnt);
    vig.SetFocusScales(focus, focus * 0.85f);
    g.FillRectangle(&vig, RectF(x, y, w, h));
}

void DrawSnowflake(Graphics& g, float cx, float cy, float r, Color col, float stroke_w) {
    Pen pen(col, stroke_w);
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    float dx = r * 0.866f;
    float dy = r * 0.5f;
    g.DrawLine(&pen, cx, cy - r, cx, cy + r);
    g.DrawLine(&pen, cx - dx, cy - dy, cx + dx, cy + dy);
    g.DrawLine(&pen, cx - dx, cy + dy, cx + dx, cy - dy);
}

void AddCloudPath(GraphicsPath& path, float x, float y, float s) {
    path.AddEllipse(x + 1.8f * s, y + 8.6f * s, 10.6f * s, 10.4f * s);
    path.AddEllipse(x + 6.2f * s, y + 4.2f * s, 11.8f * s, 12.2f * s);
    path.AddEllipse(x + 12.8f * s, y + 9.2f * s, 9.4f * s, 9.8f * s);
    path.AddRectangle(RectF(x + 6.8f * s, y + 12.2f * s, 10.8f * s, 6.8f * s));
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
    float s = std::max(4.0f, size) / 24.0f;
    GraphicsPath outer, inner;
    outer.AddEllipse(x + 2.5f * s, y + 2.2f * s, 18.6f * s, 18.6f * s);
    inner.AddEllipse(x + 9.2f * s, y + 0.5f * s, 16.5f * s, 16.5f * s);
    Region reg(&outer);
    reg.Exclude(&inner);
    SolidBrush br(col);
    g.FillRegion(&br, &reg);
}

void DrawSolidCloud(Graphics& g, float x, float y, float size, Color col) {
    float s = std::max(4.0f, size) / 24.0f;
    GraphicsPath path(FillModeWinding);
    AddCloudPath(path, x, y, s);
    SolidBrush br(col);
    g.FillPath(&br, &path);
}
} // namespace

void IconRenderer::DrawWeatherGlyph(HDC hdc, float x, float y, float size, WeatherKind kind, bool dark_mode, BYTE alpha) {
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    Color sunCol(alpha, 255, 212, 0);
    Color moonCol = dark_mode ? Color(alpha, 124, 196, 255) : Color(alpha, 46, 118, 224);
    Color cloudCol = dark_mode ? Color(alpha, 245, 248, 252) : Color(alpha, 82, 98, 118);
    Color rainCol = dark_mode ? Color(alpha, 56, 189, 248) : Color(alpha, 14, 130, 226);
    Color snowCol = dark_mode ? Color(alpha, 190, 238, 255) : Color(alpha, 38, 144, 235);
    Color fogCol = dark_mode ? Color(alpha, 190, 202, 218) : Color(alpha, 82, 98, 118);

    float s = std::max(4.0f, size) / 24.0f;

    switch (kind) {
    case WeatherKind::ClearDay:
        DrawSun(g, x + 12.0f * s, y + 12.0f * s, 5.5f * s, 10.6f * s, sunCol, std::max(1.1f, 2.2f * s));
        break;
    case WeatherKind::ClearNight:
        DrawMoon(g, x, y, size, moonCol);
        break;
    case WeatherKind::PartlyCloudyDay:
        DrawSun(g, x + 8.2f * s, y + 8.0f * s, 4.3f * s, 8.0f * s, sunCol, std::max(1.0f, 1.8f * s));
        DrawSolidCloud(g, x + 1.5f * s, y + 2.2f * s, size * 0.92f, cloudCol);
        break;
    case WeatherKind::PartlyCloudyNight:
        DrawMoon(g, x - 2.0f * s, y - 1.5f * s, size * 0.72f, moonCol);
        DrawSolidCloud(g, x + 1.5f * s, y + 2.4f * s, size * 0.92f, cloudCol);
        break;
    case WeatherKind::Cloudy:
    default:
        DrawSolidCloud(g, x, y + 0.5f * s, size, cloudCol);
        break;
    case WeatherKind::Rain: {
        DrawSolidCloud(g, x, y - 2.8f * s, size * 0.95f, cloudCol);
        Pen pen(rainCol, std::max(1.3f, 2.7f * s));
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        g.DrawLine(&pen, x + 8.6f * s, y + 16.4f * s, x + 6.8f * s, y + 21.8f * s);
        g.DrawLine(&pen, x + 15.6f * s, y + 16.4f * s, x + 13.8f * s, y + 21.8f * s);
        break;
    }
    case WeatherKind::Snow: {
        DrawSolidCloud(g, x, y - 2.8f * s, size * 0.95f, cloudCol);
        float sr = std::max(1.6f, 2.4f * s);
        float sw = std::max(1.0f, 1.3f * s);
        DrawSnowflake(g, x + 7.2f * s, y + 19.2f * s, sr, snowCol, sw);
        DrawSnowflake(g, x + 12.2f * s, y + 20.6f * s, sr, snowCol, sw);
        DrawSnowflake(g, x + 17.0f * s, y + 19.0f * s, sr, snowCol, sw);
        break;
    }
    case WeatherKind::Storm: {
        DrawSolidCloud(g, x, y - 2.8f * s, size * 0.95f, cloudCol);
        PointF pts[6] = {
            {x + 12.8f * s, y + 13.4f * s},
            {x + 9.0f * s,  y + 18.4f * s},
            {x + 12.0f * s, y + 18.4f * s},
            {x + 10.4f * s, y + 23.0f * s},
            {x + 16.0f * s, y + 17.0f * s},
            {x + 12.8f * s, y + 17.0f * s}
        };
        SolidBrush br(sunCol);
        g.FillPolygon(&br, pts, 6);
        break;
    }
    case WeatherKind::Fog: {
        Pen pen(fogCol, std::max(1.2f, 2.4f * s));
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        g.DrawLine(&pen, x + 3.0f * s, y + 7.0f * s, x + 21.0f * s, y + 7.0f * s);
        g.DrawLine(&pen, x + 2.0f * s, y + 12.0f * s, x + 18.5f * s, y + 12.0f * s);
        g.DrawLine(&pen, x + 5.5f * s, y + 17.0f * s, x + 22.0f * s, y + 17.0f * s);
        break;
    }
    }
}

void IconRenderer::DrawTileBackdrop(HDC hdc, float x, float y, float w, float h, WeatherKind kind, bool dark_mode, const PluginConfig& cfg, bool draw_bg) {
    if (w < 2.0f || h < 2.0f) return;
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    RectF rc(x, y, w, h);
    g.SetClip(rc);

    float s = h / 30.0f;
    BarPalette pal = GetBarPalette(cfg.bar_contrast, dark_mode);
    float barPx = (float)(2 + std::clamp(cfg.bar_thickness, 0, 3)) * s;

    if (cfg.icon_style != IconStyle::SkyCards) {
        if (draw_bg) {
            Color bg = dark_mode ? Color(18, 255, 255, 255) : Color(180, 255, 255, 255);
            SolidBrush bgBr(bg);
            g.FillRectangle(&bgBr, rc);
        }

        if (cfg.icon_style == IconStyle::TopBar) {
            float padX = 4.0f * s;
            DrawHorizontalBar(g, x + padX, y, std::max(2.0f, w - padX * 2.0f), barPx, kind, pal);
        } else if (cfg.icon_style == IconStyle::BottomBar) {
            float padX = 4.0f * s;
            DrawHorizontalBar(g, x + padX, y + h - barPx, std::max(2.0f, w - padX * 2.0f), barPx, kind, pal);
        } else if (cfg.icon_style == IconStyle::LeftPill) {
            float padY = std::max(2.0f, 4.5f * s);
            DrawVerticalPill(g, x + 2.5f * s, y + padY, barPx, std::max(6.0f, h - padY * 2.0f), kind, pal);
        } else if (cfg.icon_style == IconStyle::Watermark) {
            float gSize = std::min(w, h) * 0.90f;
            DrawWeatherGlyph(hdc, x + (w - gSize) * 0.5f, y + (h - gSize) * 0.5f, gSize, kind, dark_mode, 175);
            Color base = dark_mode ? Color(255, 12, 14, 18) : Color(255, 238, 242, 246);
            DrawVignetteOverlay(g, x, y, w, h, cfg.vignette_level, base);
        }

        g.ResetClip();
        return;
    }

    // Style 0: Sky Atmosphere Cards + adjustable radial vignette
    Color c1(255, 71, 85, 105), c2(255, 30, 41, 59);
    switch (kind) {
    case WeatherKind::ClearDay:          c1 = Color(255, 204, 98, 10); c2 = Color(255, 78, 30, 5); break;
    case WeatherKind::ClearNight:        c1 = Color(255, 30, 27, 75);  c2 = Color(255, 9, 13, 24); break;
    case WeatherKind::PartlyCloudyDay:   c1 = Color(255, 168, 76, 14); c2 = Color(255, 30, 41, 59); break;
    case WeatherKind::PartlyCloudyNight: c1 = Color(255, 30, 41, 82);  c2 = Color(255, 30, 41, 59); break;
    case WeatherKind::Cloudy:
    case WeatherKind::Fog:
    default:                             c1 = Color(255, 75, 88, 106); c2 = Color(255, 30, 41, 59); break;
    case WeatherKind::Rain:              c1 = Color(255, 29, 78, 216); c2 = Color(255, 15, 23, 42); break;
    case WeatherKind::Storm:             c1 = Color(255, 62, 52, 175); c2 = Color(255, 15, 23, 42); break;
    case WeatherKind::Snow:              c1 = Color(255, 2, 132, 199); c2 = Color(255, 30, 41, 59); break;
    }

    LinearGradientBrush bgBr(rc, c1, c2, LinearGradientModeForwardDiagonal);
    g.FillRectangle(&bgBr, rc);

    switch (kind) {
    case WeatherKind::ClearDay: {
        SolidBrush sunBr(Color(215, 251, 191, 36));
        float r = 11.0f * s;
        g.FillEllipse(&sunBr, x + w - r * 1.12f, y - r * 0.62f, r * 2.0f, r * 2.0f);
        break;
    }
    case WeatherKind::ClearNight: {
        GraphicsPath outer, inner;
        float cx = x + w - 13.0f * s, cy = y + 1.0f * s;
        outer.AddEllipse(cx, cy, 13.0f * s, 13.0f * s);
        inner.AddEllipse(cx + 4.5f * s, cy - 1.2f * s, 11.5f * s, 11.5f * s);
        Region reg(&outer);
        reg.Exclude(&inner);
        SolidBrush moonBr(Color(210, 147, 197, 253));
        g.FillRegion(&moonBr, &reg);
        break;
    }
    case WeatherKind::PartlyCloudyDay: {
        SolidBrush sunBr(Color(215, 251, 191, 36));
        float r = 8.2f * s;
        g.FillEllipse(&sunBr, x - r * 0.32f, y - r * 0.42f, r * 2.0f, r * 2.0f);
        SolidBrush clBr(Color(145, 226, 232, 240));
        g.FillEllipse(&clBr, x + w * 0.36f, y + h * 0.44f, w * 0.46f, h * 0.72f);
        g.FillEllipse(&clBr, x + w * 0.58f, y + h * 0.34f, w * 0.54f, h * 0.85f);
        break;
    }
    case WeatherKind::PartlyCloudyNight: {
        SolidBrush moonBr(Color(195, 147, 197, 253));
        g.FillEllipse(&moonBr, x + 2.0f * s, y + 1.5f * s, 7.5f * s, 7.5f * s);
        SolidBrush clBr(Color(140, 203, 213, 225));
        g.FillEllipse(&clBr, x + w * 0.42f, y + h * 0.46f, w * 0.45f, h * 0.72f);
        g.FillEllipse(&clBr, x + w * 0.62f, y + h * 0.36f, w * 0.50f, h * 0.85f);
        break;
    }
    case WeatherKind::Cloudy: {
        SolidBrush clBr(Color(125, 180, 195, 212));
        g.FillEllipse(&clBr, x - 3.0f * s, y + h * 0.52f, w * 0.55f, h * 0.72f);
        g.FillEllipse(&clBr, x + w * 0.34f, y + h * 0.38f, w * 0.74f, h * 0.86f);
        break;
    }
    case WeatherKind::Fog: {
        Pen fogPen(Color(145, 226, 232, 240), std::max(1.4f, 2.2f * s));
        fogPen.SetStartCap(LineCapRound);
        fogPen.SetEndCap(LineCapRound);
        g.DrawLine(&fogPen, x + 3.0f * s, y + 4.0f * s, x + w - 8.0f * s, y + 4.0f * s);
        g.DrawLine(&fogPen, x + 6.0f * s, y + h - 4.0f * s, x + w - 3.0f * s, y + h - 4.0f * s);
        break;
    }
    case WeatherKind::Rain: {
        Pen rainPen(Color(205, 96, 185, 255), std::max(1.5f, 2.0f * s));
        rainPen.SetStartCap(LineCapRound);
        rainPen.SetEndCap(LineCapRound);
        g.DrawLine(&rainPen, x + 9.0f * s, y + 2.0f * s, x + 4.0f * s, y + 12.0f * s);
        g.DrawLine(&rainPen, x + w - 5.0f * s, y + 3.0f * s, x + w - 10.0f * s, y + 14.0f * s);
        g.DrawLine(&rainPen, x + w - 4.0f * s, y + h - 10.0f * s, x + w - 8.0f * s, y + h - 2.0f * s);
        break;
    }
    case WeatherKind::Storm: {
        PointF pts[6] = {
            {x + w - 9.0f * s,  y + 1.5f * s},
            {x + w - 14.0f * s, y + 13.0f * s},
            {x + w - 9.0f * s,  y + 13.0f * s},
            {x + w - 12.0f * s, y + 24.5f * s},
            {x + w - 3.0f * s,  y + 10.5f * s},
            {x + w - 8.0f * s,  y + 10.5f * s}
        };
        SolidBrush boltBr(Color(230, 250, 204, 21));
        g.FillPolygon(&boltBr, pts, 6);
        break;
    }
    case WeatherKind::Snow: {
        Color snCol(235, 230, 248, 255);
        float r = std::max(2.0f, 3.0f * s);
        float sw = std::max(1.0f, 1.2f * s);
        DrawSnowflake(g, x + 6.5f * s, y + 5.5f * s, r, snCol, sw);
        DrawSnowflake(g, x + w - 7.5f * s, y + 6.0f * s, r, snCol, sw);
        DrawSnowflake(g, x + 7.5f * s, y + h - 6.0f * s, r * 0.85f, snCol, sw);
        DrawSnowflake(g, x + w - 8.0f * s, y + h - 6.0f * s, r * 0.85f, snCol, sw);
        break;
    }
    }

    DrawVignetteOverlay(g, x, y, w, h, cfg.vignette_level, Color(255, 11, 14, 19));
    g.ResetClip();
}

void IconRenderer::DrawRainDrop(HDC hdc, float x, float y, float size, bool dark_mode) {
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    Color col = dark_mode ? Color(255, 96, 185, 255) : Color(255, 14, 130, 226);
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

void IconRenderer::DrawTempAlert(HDC hdc, float x, float y, float size, int dir, AlertMode mode, bool dark_mode) {
    if (dir == 0 || mode == AlertMode::Off) return;
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    bool hot = (dir > 0);
    Color col = hot ? Color(255, 255, 77, 79) : Color(255, 56, 189, 248);
    Color rim = dark_mode ? Color(230, 225, 232, 242) : Color(230, 45, 55, 72);
    float s = std::max(6.0f, size) / 12.0f;

    if (mode == AlertMode::Arrow) {
        PointF pts[7];
        if (hot) {
            pts[0] = {x + 6.0f * s, y + 1.0f * s};
            pts[1] = {x + 11.0f * s, y + 6.2f * s};
            pts[2] = {x + 7.8f * s, y + 6.2f * s};
            pts[3] = {x + 7.8f * s, y + 11.0f * s};
            pts[4] = {x + 4.2f * s, y + 11.0f * s};
            pts[5] = {x + 4.2f * s, y + 6.2f * s};
            pts[6] = {x + 1.0f * s, y + 6.2f * s};
        } else {
            pts[0] = {x + 6.0f * s, y + 11.0f * s};
            pts[1] = {x + 1.0f * s, y + 5.8f * s};
            pts[2] = {x + 4.2f * s, y + 5.8f * s};
            pts[3] = {x + 4.2f * s, y + 1.0f * s};
            pts[4] = {x + 7.8f * s, y + 1.0f * s};
            pts[5] = {x + 7.8f * s, y + 5.8f * s};
            pts[6] = {x + 11.0f * s, y + 5.8f * s};
        }
        SolidBrush br(col);
        g.FillPolygon(&br, pts, 7);
        return;
    }

    // Thermometer
    SolidBrush bgBr(dark_mode ? Color(210, 20, 24, 30) : Color(230, 255, 255, 255));
    g.FillEllipse(&bgBr, x + 3.0f * s, y + 6.0f * s, 6.0f * s, 5.6f * s);
    g.FillRectangle(&bgBr, x + 4.2f * s, y + 1.0f * s, 3.6f * s, 6.5f * s);

    Pen rimPen(rim, std::max(0.9f, 1.0f * s));
    g.DrawEllipse(&rimPen, x + 3.0f * s, y + 6.0f * s, 6.0f * s, 5.6f * s);
    g.DrawRectangle(&rimPen, x + 4.2f * s, y + 1.0f * s, 3.6f * s, 5.6f * s);

    SolidBrush merc(col);
    g.FillEllipse(&merc, x + 4.0f * s, y + 6.9f * s, 4.0f * s, 3.8f * s);
    float topBar = hot ? 2.0f : 4.8f;
    g.FillRectangle(&merc, x + 5.0f * s, y + topBar * s, 2.0f * s, (7.6f - topBar) * s);
}
