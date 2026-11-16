#include "IconRenderer.h"
#include "GdiPlusInclude.h"
#include <algorithm>

using namespace Gdiplus;

namespace {
struct BarPalette {
    Color sun1, sun2, moon1, moon2, cl1, cl2, rain, snow, hailBg, hailDot, bolt, skyMid;
};

BarPalette GetBarPalette(int chroma, bool dark_mode) {
    BarPalette p;
    if (chroma == 2) {
        p.sun1    = Color(255, 255, 240, 0);
        p.sun2    = Color(255, 255, 107, 0);
        p.moon1   = Color(255, 96, 165, 250);
        p.moon2   = Color(255, 224, 242, 254);
        p.cl1     = dark_mode ? Color(255, 255, 255, 255) : Color(255, 100, 116, 139);
        p.cl2     = dark_mode ? Color(255, 71, 85, 105)   : Color(255, 51, 65, 85);
        p.rain    = Color(255, 0, 234, 255);
        p.snow    = dark_mode ? Color(255, 255, 255, 255) : Color(255, 2, 132, 199);
        p.hailBg  = Color(115, 56, 189, 248);
        p.hailDot = dark_mode ? Color(255, 255, 255, 255) : Color(255, 15, 23, 42);
        p.bolt    = Color(255, 255, 240, 0);
        p.skyMid  = Color(255, 125, 211, 252);
    } else if (chroma == 1) {
        p.sun1    = Color(255, 255, 225, 26);
        p.sun2    = Color(255, 245, 158, 11);
        p.moon1   = Color(255, 59, 130, 246);
        p.moon2   = Color(255, 147, 197, 253);
        p.cl1     = dark_mode ? Color(255, 203, 213, 225) : Color(255, 100, 116, 139);
        p.cl2     = dark_mode ? Color(255, 71, 85, 105)   : Color(255, 51, 65, 85);
        p.rain    = Color(255, 56, 189, 248);
        p.snow    = dark_mode ? Color(255, 255, 255, 255) : Color(255, 71, 85, 105);
        p.hailBg  = Color(100, 56, 189, 248);
        p.hailDot = dark_mode ? Color(255, 255, 255, 255) : Color(255, 15, 23, 42);
        p.bolt    = Color(255, 250, 204, 21);
        p.skyMid  = p.cl1;
    } else {
        p.sun1    = Color(255, 234, 179, 8);
        p.sun2    = Color(255, 202, 138, 4);
        p.moon1   = Color(255, 37, 99, 235);
        p.moon2   = Color(255, 37, 99, 235);
        p.cl1     = Color(255, 148, 163, 184);
        p.cl2     = Color(255, 148, 163, 184);
        p.rain    = Color(255, 37, 99, 235);
        p.snow    = dark_mode ? Color(255, 226, 232, 240) : Color(255, 71, 85, 105);
        p.hailBg  = Color(85, 100, 116, 139);
        p.hailDot = dark_mode ? Color(255, 255, 255, 255) : Color(255, 15, 23, 42);
        p.bolt    = Color(255, 250, 204, 21);
        p.skyMid  = p.cl1;
    }
    return p;
}

void FillGrad3(Graphics& g, float x, float y, float w, float h, bool vert, Color c1, Color c2, Color c3) {
    RectF rc(x, y, std::max(2.0f, w), std::max(2.0f, h));
    LinearGradientBrush br(rc, c1, c3, vert ? LinearGradientModeVertical : LinearGradientModeHorizontal);
    Color cols[3] = { c1, c2, c3 };
    REAL pos[3] = { 0.0f, 0.5f, 1.0f };
    br.SetInterpolationColors(cols, pos, 3);
    g.FillRectangle(&br, rc);
}

void DrawWeatherBar(Graphics& g, float x, float y, float w, float h, bool vert, WeatherKind kind, int chroma, bool dark_mode) {
    BarPalette pal = GetBarPalette(chroma, dark_mode);
    float len = vert ? h : w;
    float thick = vert ? w : h;
    float s = thick / 3.0f;

    switch (kind) {
    case WeatherKind::ClearDay:
        FillGrad3(g, x, y, w, h, vert, pal.sun1, pal.sun1, pal.sun2);
        break;
    case WeatherKind::ClearNight:
        FillGrad3(g, x, y, w, h, vert, pal.moon1, pal.moon2, pal.moon1);
        break;
    case WeatherKind::PartlyCloudyDay:
        FillGrad3(g, x, y, w, h, vert, pal.sun1, pal.skyMid, pal.cl2);
        break;
    case WeatherKind::PartlyCloudyNight:
        FillGrad3(g, x, y, w, h, vert, pal.moon1, pal.moon2, pal.cl2);
        break;
    case WeatherKind::Cloudy:
    default:
        FillGrad3(g, x, y, w, h, vert, pal.cl1, pal.cl1, pal.cl2);
        break;
    case WeatherKind::Fog: {
        float t1 = std::max(1.0f, thick * 0.36f);
        SolidBrush b1(Color(215, pal.cl1.GetR(), pal.cl1.GetG(), pal.cl1.GetB()));
        SolidBrush b2(Color(215, pal.cl2.GetR(), pal.cl2.GetG(), pal.cl2.GetB()));
        if (vert) {
            g.FillRectangle(&b1, x, y, t1, h);
            g.FillRectangle(&b2, x + w - t1, y + 2.0f, t1, std::max(2.0f, h - 4.0f));
        } else {
            g.FillRectangle(&b1, x, y, w, t1);
            g.FillRectangle(&b2, x + 2.0f, y + h - t1, std::max(2.0f, w - 4.0f), t1);
        }
        break;
    }
    case WeatherKind::Drizzle: {
        SolidBrush br(pal.rain);
        float r = std::max(1.0f, thick * 0.35f);
        float step = std::max(5.5f, 7.5f * s);
        for (float p = 3.0f; p < len - 2.0f; p += step) {
            if (vert) g.FillEllipse(&br, x + w * 0.5f - r, y + p - r, r * 2.0f, r * 2.0f);
            else      g.FillEllipse(&br, x + p - r, y + h * 0.5f - r, r * 2.0f, r * 2.0f);
        }
        break;
    }
    case WeatherKind::Rain:
    case WeatherKind::Snow: {
        SolidBrush br(kind == WeatherKind::Rain ? pal.rain : pal.snow);
        float seg = std::max(3.5f, 5.0f * s);
        float step = seg + std::max(2.5f, 3.0f * s);
        for (float p = 0.0f; p < len - 1.5f; p += step) {
            float cur = std::min(seg, len - p);
            if (vert) g.FillRectangle(&br, x, y + p, w, cur);
            else      g.FillRectangle(&br, x + p, y, cur, h);
        }
        break;
    }
    case WeatherKind::Sleet: {
        SolidBrush brR(pal.rain), brS(pal.snow);
        float seg = std::max(3.2f, 4.5f * s);
        float step = seg + std::max(2.4f, 3.0f * s);
        int idx = 0;
        for (float p = 0.0f; p < len - 1.5f; p += step, ++idx) {
            float cur = std::min(seg, len - p);
            SolidBrush& br = (idx % 2 == 0) ? brR : brS;
            if (vert) g.FillRectangle(&br, x, y + p, w, cur);
            else      g.FillRectangle(&br, x + p, y, cur, h);
        }
        break;
    }
    case WeatherKind::Hail: {
        SolidBrush bg(pal.hailBg), dot(pal.hailDot);
        g.FillRectangle(&bg, x, y, w, h);
        float r = std::max(1.1f, thick * 0.42f);
        float step = std::max(5.5f, 7.5f * s);
        for (float p = 4.0f; p < len - 2.0f; p += step) {
            if (vert) g.FillEllipse(&dot, x + w * 0.5f - r, y + p - r, r * 2.0f, r * 2.0f);
            else      g.FillEllipse(&dot, x + p - r, y + h * 0.5f - r, r * 2.0f, r * 2.0f);
        }
        break;
    }
    case WeatherKind::Storm: {
        Pen pen(pal.bolt, std::max(1.3f, thick * 0.38f));
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        pen.SetLineJoin(LineJoinRound);
        float zw = std::max(4.0f, 5.0f * s);
        float step = zw + std::max(3.0f, 4.0f * s);
        for (float p = 1.0f; p < len - zw; p += step) {
            PointF pts[3];
            if (vert) {
                pts[0] = {x + 0.6f, y + p};
                pts[1] = {x + w - 0.6f, y + p + zw * 0.5f};
                pts[2] = {x + 0.6f, y + p + zw};
            } else {
                pts[0] = {x + p, y + h - 0.6f};
                pts[1] = {x + p + zw * 0.5f, y + 0.6f};
                pts[2] = {x + p + zw, y + h - 0.6f};
            }
            g.DrawLines(&pen, pts, 3);
        }
        break;
    }
    case WeatherKind::StormHail: {
        Pen pen(pal.bolt, std::max(1.2f, thick * 0.36f));
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        pen.SetLineJoin(LineJoinRound);
        SolidBrush dot(pal.hailDot);
        float zw = std::max(3.8f, 4.4f * s);
        float step = zw + std::max(3.0f, 3.6f * s);
        float r = std::max(1.1f, thick * 0.38f);
        int idx = 0;
        for (float p = 1.0f; p < len - 3.5f; p += step, ++idx) {
            if (idx % 2 == 0) {
                PointF pts[3];
                if (vert) {
                    pts[0] = {x + 0.6f, y + p};
                    pts[1] = {x + w - 0.6f, y + p + zw * 0.5f};
                    pts[2] = {x + 0.6f, y + p + zw};
                } else {
                    pts[0] = {x + p, y + h - 0.6f};
                    pts[1] = {x + p + zw * 0.5f, y + 0.6f};
                    pts[2] = {x + p + zw, y + h - 0.6f};
                }
                g.DrawLines(&pen, pts, 3);
            } else {
                if (vert) g.FillEllipse(&dot, x + w * 0.5f - r, y + p + 2.0f - r, r * 2.0f, r * 2.0f);
                else      g.FillEllipse(&dot, x + p + 2.0f - r, y + h * 0.5f - r, r * 2.0f, r * 2.0f);
            }
        }
        break;
    }
    }
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
    Color iceCol = dark_mode ? Color(alpha, 224, 242, 254) : Color(alpha, 2, 132, 199);
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
    case WeatherKind::Fog: {
        Pen pen(fogCol, std::max(1.2f, 2.4f * s));
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        g.DrawLine(&pen, x + 3.0f * s, y + 7.0f * s, x + 21.0f * s, y + 7.0f * s);
        g.DrawLine(&pen, x + 2.0f * s, y + 12.0f * s, x + 18.5f * s, y + 12.0f * s);
        g.DrawLine(&pen, x + 5.5f * s, y + 17.0f * s, x + 22.0f * s, y + 17.0f * s);
        break;
    }
    case WeatherKind::Drizzle: {
        DrawSolidCloud(g, x, y - 2.8f * s, size * 0.95f, cloudCol);
        SolidBrush br(rainCol);
        float r = std::max(1.1f, 1.5f * s);
        g.FillEllipse(&br, x + 8.0f * s - r, y + 18.2f * s - r, r * 2, r * 2);
        g.FillEllipse(&br, x + 15.0f * s - r, y + 18.2f * s - r, r * 2, r * 2);
        g.FillEllipse(&br, x + 11.5f * s - r, y + 21.2f * s - r, r * 2, r * 2);
        break;
    }
    case WeatherKind::Rain: {
        DrawSolidCloud(g, x, y - 2.8f * s, size * 0.95f, cloudCol);
        Pen pen(rainCol, std::max(1.3f, 2.7f * s));
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        g.DrawLine(&pen, x + 8.6f * s, y + 16.4f * s, x + 6.8f * s, y + 21.8f * s);
        g.DrawLine(&pen, x + 15.6f * s, y + 16.4f * s, x + 13.8f * s, y + 21.8f * s);
        break;
    }
    case WeatherKind::Sleet: {
        DrawSolidCloud(g, x, y - 2.8f * s, size * 0.95f, cloudCol);
        Pen pen(rainCol, std::max(1.3f, 2.6f * s));
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        g.DrawLine(&pen, x + 8.2f * s, y + 16.5f * s, x + 6.5f * s, y + 21.8f * s);
        DrawSnowflake(g, x + 16.0f * s, y + 19.3f * s, std::max(1.6f, 2.4f * s), iceCol, std::max(1.0f, 1.3f * s));
        break;
    }
    case WeatherKind::Snow: {
        DrawSolidCloud(g, x, y - 2.8f * s, size * 0.95f, cloudCol);
        float sr = std::max(1.6f, 2.4f * s);
        float sw = std::max(1.0f, 1.3f * s);
        DrawSnowflake(g, x + 7.5f * s, y + 19.2f * s, sr, iceCol, sw);
        DrawSnowflake(g, x + 16.5f * s, y + 19.2f * s, sr, iceCol, sw);
        break;
    }
    case WeatherKind::Hail: {
        DrawSolidCloud(g, x, y - 2.8f * s, size * 0.95f, cloudCol);
        SolidBrush brI(iceCol), brR(rainCol);
        float r = std::max(1.4f, 2.1f * s);
        g.FillEllipse(&brI, x + 7.5f * s - r, y + 19.0f * s - r, r * 2, r * 2);
        g.FillEllipse(&brI, x + 16.5f * s - r, y + 19.0f * s - r, r * 2, r * 2);
        g.FillEllipse(&brR, x + 12.0f * s - r * 0.85f, y + 21.2f * s - r * 0.85f, r * 1.7f, r * 1.7f);
        break;
    }
    case WeatherKind::Storm: {
        DrawSolidCloud(g, x, y - 2.8f * s, size * 0.95f, cloudCol);
        PointF pts[6] = {
            {x + 12.8f * s, y + 13.4f * s}, {x + 9.0f * s, y + 18.4f * s},
            {x + 12.0f * s, y + 18.4f * s}, {x + 10.4f * s, y + 23.0f * s},
            {x + 16.0f * s, y + 17.0f * s}, {x + 12.8f * s, y + 17.0f * s}
        };
        SolidBrush br(sunCol);
        g.FillPolygon(&br, pts, 6);
        Pen pen(rainCol, std::max(1.2f, 2.1f * s));
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        g.DrawLine(&pen, x + 6.8f * s, y + 17.0f * s, x + 5.6f * s, y + 20.6f * s);
        break;
    }
    case WeatherKind::StormHail: {
        DrawSolidCloud(g, x, y - 2.8f * s, size * 0.95f, cloudCol);
        PointF pts[6] = {
            {x + 11.5f * s, y + 13.4f * s}, {x + 8.0f * s, y + 18.4f * s},
            {x + 10.8f * s, y + 18.4f * s}, {x + 9.4f * s, y + 23.0f * s},
            {x + 14.4f * s, y + 17.0f * s}, {x + 11.5f * s, y + 17.0f * s}
        };
        SolidBrush br(sunCol), brI(iceCol);
        g.FillPolygon(&br, pts, 6);
        float r = std::max(1.4f, 2.1f * s);
        g.FillEllipse(&brI, x + 17.2f * s - r, y + 19.2f * s - r, r * 2, r * 2);
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
    float barPx = (float)(2 + std::clamp(cfg.bar_thickness, 0, 3)) * s;

    bool isSkyOld = (cfg.icon_style == IconStyle::SkyCards);
    bool isSkyNew = (cfg.icon_style == IconStyle::SkyTransparent);

    if (!isSkyOld && !isSkyNew) {
        if (draw_bg) {
            Color bg = dark_mode ? Color(18, 255, 255, 255) : Color(180, 255, 255, 255);
            SolidBrush bgBr(bg);
            g.FillRectangle(&bgBr, rc);
        }

        if (cfg.icon_style == IconStyle::TopBar) {
            float padX = 4.0f * s;
            DrawWeatherBar(g, x + padX, y, std::max(4.0f, w - padX * 2.0f), barPx, false, kind, cfg.bar_contrast, dark_mode);
        } else if (cfg.icon_style == IconStyle::BottomBar) {
            float padX = 4.0f * s;
            DrawWeatherBar(g, x + padX, y + h - barPx, std::max(4.0f, w - padX * 2.0f), barPx, false, kind, cfg.bar_contrast, dark_mode);
        } else if (cfg.icon_style == IconStyle::LeftPill) {
            float padY = std::max(2.0f, 4.0f * s);
            DrawWeatherBar(g, x + 2.5f * s, y + padY, barPx, std::max(6.0f, h - padY * 2.0f), true, kind, cfg.bar_contrast, dark_mode);
        } else if (cfg.icon_style == IconStyle::Watermark) {
            float gSize = std::min(w, h) * 0.90f;
            DrawWeatherGlyph(hdc, x + (w - gSize) * 0.5f, y + (h - gSize) * 0.5f, gSize, kind, dark_mode, 185);
            Color base = dark_mode ? Color(255, 12, 14, 18) : Color(255, 238, 242, 246);
            DrawVignetteOverlay(g, x, y, w, h, cfg.vignette_level, base);
        }

        g.ResetClip();
        return;
    }

    if (isSkyOld) {
        Color c1(255, 71, 85, 105), c2(255, 30, 41, 59);
        switch (kind) {
        case WeatherKind::ClearDay:          c1 = Color(255, 204, 98, 10); c2 = Color(255, 78, 30, 5); break;
        case WeatherKind::ClearNight:        c1 = Color(255, 30, 27, 75);  c2 = Color(255, 9, 13, 24); break;
        case WeatherKind::PartlyCloudyDay:   c1 = Color(255, 168, 76, 14); c2 = Color(255, 30, 41, 59); break;
        case WeatherKind::PartlyCloudyNight: c1 = Color(255, 30, 41, 82);  c2 = Color(255, 30, 41, 59); break;
        case WeatherKind::Cloudy:
        case WeatherKind::Fog:
        default:                             c1 = Color(255, 75, 88, 106); c2 = Color(255, 30, 41, 59); break;
        case WeatherKind::Drizzle:           c1 = Color(255, 30, 64, 175); c2 = Color(255, 30, 41, 59); break;
        case WeatherKind::Rain:              c1 = Color(255, 29, 78, 216); c2 = Color(255, 15, 23, 42); break;
        case WeatherKind::Sleet:             c1 = Color(255, 29, 78, 216); c2 = Color(255, 3, 105, 161); break;
        case WeatherKind::Snow:              c1 = Color(255, 2, 132, 199); c2 = Color(255, 30, 41, 59); break;
        case WeatherKind::Hail:              c1 = Color(255, 15, 118, 110);c2 = Color(255, 30, 41, 59); break;
        case WeatherKind::Storm:             c1 = Color(255, 62, 52, 175); c2 = Color(255, 15, 23, 42); break;
        case WeatherKind::StormHail:         c1 = Color(255, 76, 29, 149); c2 = Color(255, 15, 23, 42); break;
        }
        LinearGradientBrush bgBr(rc, c1, c2, LinearGradientModeForwardDiagonal);
        g.FillRectangle(&bgBr, rc);
    } else if (draw_bg) {
        Color bg = dark_mode ? Color(18, 255, 255, 255) : Color(180, 255, 255, 255);
        SolidBrush bgBr(bg);
        g.FillRectangle(&bgBr, rc);
    }

    BYTE cloudA = isSkyNew ? 140 : 155;
    Color clCol = (isSkyNew && !dark_mode) ? Color(cloudA, 100, 116, 139) : Color(cloudA, 226, 232, 240);
    Color iceCol = (isSkyNew && !dark_mode) ? Color(235, 2, 132, 199) : Color(235, 224, 242, 254);
    Color moonCol = (isSkyNew && !dark_mode) ? Color(220, 37, 99, 235) : Color(220, 147, 197, 253);

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
        SolidBrush moonBr(moonCol);
        g.FillRegion(&moonBr, &reg);
        break;
    }
    case WeatherKind::PartlyCloudyDay: {
        SolidBrush sunBr(Color(215, 251, 191, 36));
        float r = 8.2f * s;
        g.FillEllipse(&sunBr, x - r * 0.32f, y - r * 0.42f, r * 2.0f, r * 2.0f);
        SolidBrush clBr(clCol);
        g.FillEllipse(&clBr, x + w * 0.36f, y + h * 0.44f, w * 0.46f, h * 0.72f);
        g.FillEllipse(&clBr, x + w * 0.58f, y + h * 0.34f, w * 0.54f, h * 0.85f);
        break;
    }
    case WeatherKind::PartlyCloudyNight: {
        GraphicsPath outer, inner;
        outer.AddEllipse(x + 1.5f * s, y + 1.0f * s, 9.5f * s, 9.5f * s);
        inner.AddEllipse(x + 4.8f * s, y + 0.2f * s, 8.2f * s, 8.2f * s);
        Region reg(&outer);
        reg.Exclude(&inner);
        SolidBrush moonBr(moonCol);
        g.FillRegion(&moonBr, &reg);
        SolidBrush clBr(clCol);
        g.FillEllipse(&clBr, x + w * 0.40f, y + h * 0.46f, w * 0.45f, h * 0.72f);
        g.FillEllipse(&clBr, x + w * 0.62f, y + h * 0.36f, w * 0.50f, h * 0.85f);
        break;
    }
    case WeatherKind::Cloudy: {
        SolidBrush clBr(clCol);
        g.FillEllipse(&clBr, x - 3.0f * s, y + h * 0.52f, w * 0.55f, h * 0.72f);
        g.FillEllipse(&clBr, x + w * 0.34f, y + h * 0.38f, w * 0.74f, h * 0.86f);
        break;
    }
    case WeatherKind::Fog: {
        Pen fogPen(clCol, std::max(1.4f, 2.2f * s));
        fogPen.SetStartCap(LineCapRound);
        fogPen.SetEndCap(LineCapRound);
        g.DrawLine(&fogPen, x + 3.0f * s, y + 4.0f * s, x + w - 8.0f * s, y + 4.0f * s);
        g.DrawLine(&fogPen, x + 6.0f * s, y + h - 4.0f * s, x + w - 3.0f * s, y + h - 4.0f * s);
        break;
    }
    case WeatherKind::Drizzle: {
        SolidBrush dBr(Color(215, 56, 189, 248));
        float r = std::max(1.3f, 1.8f * s);
        g.FillEllipse(&dBr, x + 6.0f * s, y + 4.0f * s, r * 2, r * 2);
        g.FillEllipse(&dBr, x + w - 8.5f * s, y + 5.0f * s, r * 2, r * 2);
        g.FillEllipse(&dBr, x + 7.5f * s, y + h - 7.0f * s, r * 2, r * 2);
        g.FillEllipse(&dBr, x + w - 9.0f * s, y + h - 7.0f * s, r * 2, r * 2);
        break;
    }
    case WeatherKind::Rain: {
        Pen rainPen(Color(215, 56, 189, 248), std::max(1.5f, 2.0f * s));
        rainPen.SetStartCap(LineCapRound);
        rainPen.SetEndCap(LineCapRound);
        g.DrawLine(&rainPen, x + 9.0f * s, y + 2.0f * s, x + 4.0f * s, y + 12.0f * s);
        g.DrawLine(&rainPen, x + w - 5.0f * s, y + 3.0f * s, x + w - 10.0f * s, y + 14.0f * s);
        g.DrawLine(&rainPen, x + w - 4.0f * s, y + h - 10.0f * s, x + w - 8.0f * s, y + h - 2.0f * s);
        break;
    }
    case WeatherKind::Sleet: {
        Pen rainPen(Color(215, 56, 189, 248), std::max(1.5f, 2.0f * s));
        rainPen.SetStartCap(LineCapRound);
        rainPen.SetEndCap(LineCapRound);
        g.DrawLine(&rainPen, x + 8.5f * s, y + 2.0f * s, x + 4.0f * s, y + 11.5f * s);
        g.DrawLine(&rainPen, x + 10.5f * s, y + h - 10.0f * s, x + 6.5f * s, y + h - 2.0f * s);
        float sr = std::max(2.0f, 2.8f * s);
        DrawSnowflake(g, x + w - 7.5f * s, y + 5.8f * s, sr, iceCol, std::max(1.0f, 1.2f * s));
        DrawSnowflake(g, x + w - 8.0f * s, y + h - 5.8f * s, sr * 0.9f, iceCol, std::max(1.0f, 1.2f * s));
        break;
    }
    case WeatherKind::Snow: {
        float r = std::max(2.0f, 3.0f * s);
        float sw = std::max(1.0f, 1.2f * s);
        DrawSnowflake(g, x + 6.5f * s, y + 5.5f * s, r, iceCol, sw);
        DrawSnowflake(g, x + w - 7.5f * s, y + 6.0f * s, r, iceCol, sw);
        DrawSnowflake(g, x + 7.5f * s, y + h - 6.0f * s, r * 0.85f, iceCol, sw);
        DrawSnowflake(g, x + w - 8.0f * s, y + h - 6.0f * s, r * 0.85f, iceCol, sw);
        break;
    }
    case WeatherKind::Hail: {
        SolidBrush hBr(iceCol);
        float r = std::max(1.7f, 2.3f * s);
        g.FillEllipse(&hBr, x + 4.5f * s, y + 3.5f * s, r * 2, r * 2);
        g.FillEllipse(&hBr, x + w - 8.5f * s, y + 3.5f * s, r * 2, r * 2);
        g.FillEllipse(&hBr, x + 5.5f * s, y + h - 7.5f * s, r * 1.9f, r * 1.9f);
        g.FillEllipse(&hBr, x + w - 9.0f * s, y + h - 7.5f * s, r * 1.9f, r * 1.9f);
        break;
    }
    case WeatherKind::Storm: {
        PointF pts[6] = {
            {x + w - 9.0f * s, y + 1.5f * s}, {x + w - 14.0f * s, y + 13.0f * s},
            {x + w - 9.0f * s, y + 13.0f * s}, {x + w - 12.0f * s, y + 24.5f * s},
            {x + w - 3.0f * s, y + 10.5f * s}, {x + w - 8.0f * s, y + 10.5f * s}
        };
        SolidBrush boltBr(Color(230, 250, 204, 21));
        g.FillPolygon(&boltBr, pts, 6);
        Pen rainPen(Color(200, 56, 189, 248), std::max(1.4f, 1.8f * s));
        rainPen.SetStartCap(LineCapRound);
        rainPen.SetEndCap(LineCapRound);
        g.DrawLine(&rainPen, x + 8.0f * s, y + 2.5f * s, x + 4.5f * s, y + 10.5f * s);
        break;
    }
    case WeatherKind::StormHail: {
        PointF pts[6] = {
            {x + w - 9.0f * s, y + 1.5f * s}, {x + w - 14.0f * s, y + 13.0f * s},
            {x + w - 9.0f * s, y + 13.0f * s}, {x + w - 12.0f * s, y + 24.5f * s},
            {x + w - 3.0f * s, y + 10.5f * s}, {x + w - 8.0f * s, y + 10.5f * s}
        };
        SolidBrush boltBr(Color(230, 250, 204, 21)), hBr(iceCol);
        g.FillPolygon(&boltBr, pts, 6);
        float r = std::max(1.6f, 2.2f * s);
        g.FillEllipse(&hBr, x + 4.5f * s, y + 4.0f * s, r * 2, r * 2);
        g.FillEllipse(&hBr, x + 5.5f * s, y + h - 7.5f * s, r * 1.9f, r * 1.9f);
        break;
    }
    }

    Color vigBase = isSkyOld ? Color(255, 11, 14, 19)
                             : (dark_mode ? Color(255, 12, 13, 15) : Color(255, 238, 242, 246));
    DrawVignetteOverlay(g, x, y, w, h, cfg.vignette_level, vigBase);
    g.ResetClip();
}

void IconRenderer::DrawRainDrop(HDC hdc, float x, float y, float size, bool dark_mode) {
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    Color col = dark_mode ? Color(255, 56, 189, 248) : Color(255, 14, 130, 226);
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
