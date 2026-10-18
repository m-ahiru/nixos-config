// Every standard header up front: the access hack below must never see them.
#if __has_include(<bits/stdc++.h>)
#include <bits/stdc++.h>
#include <dlfcn.h>
#else
#include <algorithm>
#include <any>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <deque>
#include <expected>
#include <format>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>
#endif

#include <hyprland/src/includes.hpp>

// We need two private members: the render pass element list (to slot our elements in
// where the software cursor is) and the texture of a CTexPassElement (to recognise
// which element *is* the cursor). Same trick the official plugins use. It has to wrap
// every Hyprland header that (transitively) pulls in Pass.hpp.
#define private public
#define protected public
#include <hyprland/src/render/pass/Pass.hpp>
#include <hyprland/src/render/pass/TexPassElement.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/Workspace.hpp>
#include "compat.hpp"
#include <hyprland/src/managers/eventLoop/EventLoopManager.hpp>
#include <hyprland/src/config/ConfigValue.hpp>
#undef protected
#undef private

using Render::GL::g_pHyprOpenGL;

#include "Trail.hpp"
#include "TrailPassElement.hpp"
#include "Shaders.hpp"
#include "globals.hpp"

// ───────────────────────────── tuning constants ─────────────────────────────

// the trail reaches full width this many (full) half-widths behind the cursor
static constexpr double HEAD_RAMP_LENGTH = 1.6;

static constexpr float  SPEED_SMOOTHING_S    = 0.035F; // EMA time constant for velocity
static constexpr float  SPEED_REFERENCE      = 1000.F; // px/s that adds 1x intensity
static constexpr float  SPEED_SCALE_CAP      = 8.F;    // max multiples of intensity
static constexpr double TELEPORT_DISTANCE    = 600.0;  // a single jump this big = warp, break the trail
static constexpr double MIN_SAMPLE_DISTANCE  = 0.35;   // ignore sub-pixel jitter
static constexpr double SUBDIVIDE_STEP       = 2.5;    // spline sample spacing (logical px)
static constexpr int    SUBDIVIDE_MAX        = 32;
static constexpr float  WIDTH_TAPER_EXPONENT = 0.5F;   // how pointy the tail gets
static constexpr size_t MAX_SAMPLES          = 4096;
static constexpr float  CLICK_ANIM_S         = 0.045F; // time constant of the click/hover expand
// anti-jitter: real mice report whole-pixel steps with noise → thin out + smooth the control points
static constexpr double CTRL_MIN_SPACING     = 4.0; // logical px between kept control points
static constexpr int    CTRL_SMOOTH_PASSES   = 2;   // [1 2 1]/4 passes over interior control points


// uniform slots
enum : uint8_t { TU_PROJ = 0, TU_FULLSIZE, TU_PTS, TU_COUNT, TU_HEAD, TU_TAIL, TU_OPACITY, TU_GLOW, TU_CORER, TU_HOT, TU_ADDITIVE };
enum : uint8_t { CU_PROJ = 0, CU_FULLSIZE, CU_COLOR, CU_CENTERCOLOR, CU_OUTER, CU_RADIUS, CU_RING, CU_GLOW, CU_GLOWSIZE, CU_OUTLINE, CU_SHADOW, CU_SHADOWSIZE, CU_STYLE };
enum : uint8_t { XU_PROJ = 0, XU_FULLSIZE, XU_OUTER, XU_RADIUS, XU_CONTRAST, XU_RING, XU_STYLE };
enum : uint8_t { IU_PROJ = 0, IU_FULLSIZE, IU_COLOR, IU_CENTERCOLOR, IU_OUTER, IU_BAR, IU_SERIF, IU_RING, IU_GLOW, IU_GLOWSIZE, IU_SHADOW, IU_SHADOWSIZE, IU_OUTLINE };

// ─────────────────────────────── helpers ────────────────────────────────────

static float secondsBetween(TrailClock::time_point a, TrailClock::time_point b) {
    return std::chrono::duration<float>(b - a).count();
}

static CBox unionBox(const CBox& a, const CBox& b) {
    if (a.empty())
        return b;
    if (b.empty())
        return a;
    const double x1 = std::min(a.x, b.x), y1 = std::min(a.y, b.y);
    const double x2 = std::max(a.x + a.w, b.x + b.w), y2 = std::max(a.y + a.h, b.y + b.h);
    return CBox{x1, y1, x2 - x1, y2 - y1};
}

static bool intersects(const CBox& a, const CBox& b) {
    return !a.empty() && !b.empty() && !a.intersection(b).empty();
}

static float lifetimeSeconds() {
    return std::max<float>(10.F, sc<float>(g_config.lifetime->value())) / 1000.F;
}

static float fval(const SP<Config::Values::CFloatValue>& v) {
    return sc<float>(v->value());
}

// centripetal Catmull-Rom (alpha = 0.5): no cusps or self-loops on sharp flicks
static Vector2D catmullRom(const Vector2D& p0, const Vector2D& p1, const Vector2D& p2, const Vector2D& p3, double u) {
    auto knot = [](double t, const Vector2D& a, const Vector2D& b) { return t + std::max(std::sqrt(a.distance(b)), 1e-4); };

    const double t0 = 0.0;
    const double t1 = knot(t0, p0, p1);
    const double t2 = knot(t1, p1, p2);
    const double t3 = knot(t2, p2, p3);
    const double t  = t1 + (t2 - t1) * u;

    const auto   a1 = p0 * ((t1 - t) / (t1 - t0)) + p1 * ((t - t0) / (t1 - t0));
    const auto   a2 = p1 * ((t2 - t) / (t2 - t1)) + p2 * ((t - t1) / (t2 - t1));
    const auto   a3 = p2 * ((t3 - t) / (t3 - t2)) + p3 * ((t - t2) / (t3 - t2));
    const auto   b1 = a1 * ((t2 - t) / (t2 - t0)) + a2 * ((t - t0) / (t2 - t0));
    const auto   b2 = a2 * ((t3 - t) / (t3 - t1)) + a3 * ((t - t1) / (t3 - t1));
    return b1 * ((t2 - t) / (t2 - t1)) + b2 * ((t - t1) / (t2 - t1));
}

// Raise saturation in HSV while keeping hue; also lifts very dark colors a little so a
// saturated-but-dark tone doesn't vanish. amount 0 = untouched.
static std::array<float, 3> boostSaturation(float r, float g, float b, float amount) {
    if (amount <= 0.F)
        return {r, g, b};

    const float mx = std::max({r, g, b}), mn = std::min({r, g, b});
    const float d  = mx - mn;
    if (mx <= 0.F || d < 1e-4F)
        return {r, g, b}; // grey: no hue to boost

    float h;
    if (mx == r)
        h = std::fmod((g - b) / d, 6.F);
    else if (mx == g)
        h = (b - r) / d + 2.F;
    else
        h = (r - g) / d + 4.F;
    if (h < 0.F)
        h += 6.F;

    const float s = std::clamp(d / mx + (1.F - d / mx) * amount, 0.F, 1.F);
    const float v = std::max(mx, 0.55F * amount + mx * (1.F - amount));

    const float c = v * s, x = c * (1.F - std::abs(std::fmod(h, 2.F) - 1.F)), m = v - c;
    std::array<float, 3> out;
    switch (sc<int>(h)) {
        case 0: out = {c, x, 0}; break;
        case 1: out = {x, c, 0}; break;
        case 2: out = {0, c, x}; break;
        case 3: out = {0, x, c}; break;
        case 4: out = {x, 0, c}; break;
        default: out = {c, 0, x}; break;
    }
    return {out[0] + m, out[1] + m, out[2] + m};
}

static std::array<float, 3> toRenderRGB(Config::INTEGER hex, bool boost = false) {
    const CHyprColor col{sc<uint64_t>(hex)};
    const auto       rgb       = boostSaturation(sc<float>(col.r), sc<float>(col.g), sc<float>(col.b), boost ? fval(g_config.colorBoost) : 0.F);
    const auto       converted = g_pHyprRenderer->getConvertedColor(CHyprColor{rgb[0], rgb[1], rgb[2], 1.F});
    return {sc<float>(converted.r), sc<float>(converted.g), sc<float>(converted.b)};
}

// ─────────────────────────────── input ──────────────────────────────────────

void CCursorTrail::onPointerMoved() {
    if (!CTCompat::pointer())
        return;

    const auto now = TrailClock::now();
    const auto pos = CTCompat::pointer()->position();

    updateCursorBox();

    // cursor hidden (in-game with a locked/hidden pointer, inactive timeout, ...): no trail, no circle,
    // and no frames scheduled for it, so games aren't forced to composite on every mouse move.
    if (cursorHidden()) {
        const bool dirty = !m_samples.empty() || !m_lastDamaged.empty();
        clear();
        m_trailBox  = {};
        m_cursorBox = {};
        if (dirty)
            queueDamage(); // one last frame to wipe what's left
        return;
    }

    // over a fullscreen game/video: no trail. The circle stays unless cursor_in_fullscreen = false,
    // in which case we also stop scheduling frames so direct scanout / game performance stay untouched.
    if (g_config.hideInFullscreen->value() && g_pCompositor) {
        const auto mon = CTCompat::monitorAt(pos);
        if (fullscreenOn(mon)) {
            const bool circle = g_config.cursorEnabled->value() && g_config.cursorInFullscreen->value();
            const bool dirty  = !m_samples.empty() || !m_lastDamaged.empty();
            clear();
            m_trailBox = {};
            if (!circle)
                m_cursorBox = {};
            if (circle || dirty)
                queueDamage();
            return;
        }
    }

    if (!g_config.enabled->value()) {
        if (!m_samples.empty())
            clear();
        queueDamage(); // circle still needs its (larger than the cursor) area redrawn
        return;
    }

    // over text (after the dwell delay) or a kept shape: stop recording, let what's there fade out
    if (textMode() || m_shape == SHAPE_KEEP) {
        queueDamage();
        return;
    }

    // resuming after a pause (text, hidden cursor, ...): start a fresh trail instead of
    // connecting to the old one with a straight line
    if (!m_samples.empty() && secondsBetween(m_samples.back().time, now) > 0.1F)
        clear();

    if (!m_samples.empty()) {
        const auto& last = m_samples.back();
        const auto  dist = last.pos.distance(pos);

        if (dist < MIN_SAMPLE_DISTANCE)
            return;

        // warps (workspace/monitor focus, cursor:warp_on_change etc.) should not draw a streak
        if (dist > TELEPORT_DISTANCE) {
            m_samples.clear();
            m_smoothedSpeed = 0.F;
        } else {
            const float dt    = std::max(secondsBetween(last.time, now), 0.0005F);
            const float speed = sc<float>(dist) / dt;
            const float k     = 1.F - std::exp(-dt / SPEED_SMOOTHING_S);
            m_smoothedSpeed += (speed - m_smoothedSpeed) * k;
        }
    }

    m_samples.push_back({pos, now, m_smoothedSpeed});
    if (m_samples.size() > MAX_SAMPLES)
        m_samples.pop_front();

    buildPoints(now);
    queueDamage();
}

void CCursorTrail::onPointerButton(bool pressed) {
    m_buttonsDown = std::max(0, m_buttonsDown + (pressed ? 1 : -1));
    if (g_config.cursorEnabled->value())
        queueDamage();
}

void CCursorTrail::onCursorShape(const std::string& shape) {
    static constexpr std::array<std::string_view, 4> TEXT  = {"text", "xterm", "ibeam", "vertical-text"};
    static constexpr std::array<std::string_view, 5> LINKS = {"pointer", "hand", "hand1", "hand2", "pointing_hand"};

    m_appHidCursor = false; // a named shape is always visible
    m_clientCursor = false; // ... and comes from us, not as an image from the app

    const eShape prev = m_shape;
    if (const auto kept = keptShapes(); std::ranges::find(kept, shape) != kept.end()) {
        m_shape = SHAPE_KEEP;
        m_keptTex.reset(); // our own theme cursor: grabbed on the next frame (see onRenderLastMoment)
    }
    else if (std::ranges::find(TEXT, shape) != TEXT.end())
        m_shape = SHAPE_TEXT;
    else if (std::ranges::find(LINKS, shape) != LINKS.end())
        m_shape = SHAPE_LINK;
    else
        m_shape = SHAPE_NORMAL;

    if (m_shape == SHAPE_TEXT && prev != SHAPE_TEXT)
        m_textSince = TrailClock::now(); // text mode kicks in after cursor_text_delay over text
    if (prev == SHAPE_KEEP && m_shape != SHAPE_KEEP)
        leaveKeep();
    queueDamage();
}

void CCursorTrail::onCursorClientSurface(bool visible) {
    // app draws its own cursor image: we can't know the shape → treat as normal, the circle replaces it.
    // No image at all = the app hides the cursor (games while playing): no trail then.
    // we can't know the shape from the name; the next frame compares the image with the
    // cursor_keep shapes (slurp sends its crosshair like this). Keep a kept shape until then.
    if (m_shape != SHAPE_KEEP)
        m_shape = SHAPE_NORMAL;
    else if (!visible) {
        m_shape = SHAPE_NORMAL;
        leaveKeep();
    }
    m_appHidCursor   = !visible;
    m_clientCursor   = visible;
    m_analyzePending = visible;
    m_recheckFrames  = visible ? 30 : 0;
    m_lastDebugMsg.clear();
    if (m_appHidCursor)
        clear();
    queueDamage();
}

bool CCursorTrail::cursorHidden() const {
    return m_appHidCursor || m_noCursorDrawn || (g_pHyprRenderer && g_pHyprRenderer->m_cursorHidden);
}

// Over text for at least cursor_text_delay ms: real I-beam, trail pauses. Quick passes over
// text (hovering across a page) keep the circle and the trail.
bool CCursorTrail::textMode() const {
    if (m_shape != SHAPE_TEXT || !g_config.cursorKeepText->value())
        return false;
    const float delay = std::max<float>(0.F, sc<float>(g_config.cursorTextDelay->value())) / 1000.F;
    return secondsBetween(m_textSince, TrailClock::now()) >= delay;
}

bool CCursorTrail::textPending() const {
    return m_shape == SHAPE_TEXT && g_config.cursorKeepText->value() && !textMode();
}

void CCursorTrail::clear() {
    m_samples.clear();
    m_points.clear();
    m_smoothedSpeed = 0.F;
}

// ───────────────────────────── geometry ─────────────────────────────────────

void CCursorTrail::prune(TrailClock::time_point now) {
    const float life = lifetimeSeconds();
    // keep exactly one sample older than the lifetime so the tail end can be interpolated
    while (m_samples.size() >= 2 && secondsBetween(m_samples[1].time, now) >= life)
        m_samples.pop_front();
    if (m_samples.size() == 1 && secondsBetween(m_samples.front().time, now) >= life)
        m_samples.clear();
}

float CCursorTrail::glowFactor() const {
    return g_config.glow->value() > 0.F ? std::max(1.F, fval(g_config.glowSize)) : 1.F;
}

float CCursorTrail::halfWidthAt(const SPoint& p) const {
    float scale = 1.F;
    if (g_config.velocitySize->value())
        scale += fval(g_config.velocityIntensity) * std::clamp(p.speed / SPEED_REFERENCE, 0.F, SPEED_SCALE_CAP);

    const float taper = std::pow(std::max(1.F - p.t, 0.F), WIDTH_TAPER_EXPONENT);
    return 0.5F * std::max(0.5F, fval(g_config.width)) * scale * taper * glowFactor();
}

void CCursorTrail::buildPoints(TrailClock::time_point now) {
    m_points.clear();
    m_trailBox = {};

    prune(now);
    if (m_samples.empty() || !CTCompat::pointer() || !g_config.enabled->value() || cursorHidden())
        return;

    const float life = lifetimeSeconds();

    // control points, newest (head) first. The head is the live cursor position so the
    // trail stays glued to the pointer even between input events.
    struct SCtrl {
        Vector2D pos;
        float    t, speed;
    };
    std::vector<SCtrl> ctrl;
    ctrl.reserve(m_samples.size() + 2);

    ctrl.push_back({CTCompat::pointer()->position(), 0.F, m_samples.back().speed});

    for (auto it = m_samples.rbegin(); it != m_samples.rend(); ++it) {
        const float age = secondsBetween(it->time, now) / life;

        const double dist = ctrl.back().pos.distance(it->pos);
        if (dist < MIN_SAMPLE_DISTANCE)
            continue;

        if (age >= 1.F) {
            // clip the tail exactly at end-of-life for a smooth retract instead of per-sample jumps
            const auto& prev = ctrl.back();
            const float span = age - prev.t;
            const float f    = span > 1e-6F ? (1.F - prev.t) / span : 0.F;
            ctrl.push_back({prev.pos + (it->pos - prev.pos) * f, 1.F, prev.speed + (it->speed - prev.speed) * f});
            break;
        }

        if (dist < CTRL_MIN_SPACING)
            continue;

        ctrl.push_back({it->pos, age, it->speed});
    }

    if (ctrl.size() < 2)
        return;

    // low-pass the interior control points; head (live cursor) and clipped tail stay exact
    for (int pass = 0; pass < CTRL_SMOOTH_PASSES && ctrl.size() >= 4; ++pass) {
        std::vector<Vector2D> smoothed(ctrl.size());
        for (size_t i = 1; i + 1 < ctrl.size(); ++i)
            smoothed[i] = (ctrl[i - 1].pos + ctrl[i].pos * 2.0 + ctrl[i + 1].pos) / 4.0;
        for (size_t i = 1; i + 1 < ctrl.size(); ++i)
            ctrl[i].pos = smoothed[i];
    }

    // spline through the control points
    m_points.reserve(ctrl.size() * 4);
    for (size_t i = 0; i + 1 < ctrl.size(); ++i) {
        const auto& c1 = ctrl[i];
        const auto& c2 = ctrl[i + 1];
        const auto  p0 = i == 0 ? c1.pos * 2.0 - c2.pos : ctrl[i - 1].pos;
        const auto  p3 = i + 2 < ctrl.size() ? ctrl[i + 2].pos : c2.pos * 2.0 - c1.pos;

        const int   steps = std::clamp(sc<int>(std::ceil(c1.pos.distance(c2.pos) / SUBDIVIDE_STEP)), 1, SUBDIVIDE_MAX);
        for (int s = 0; s < steps; ++s) {
            const double u = sc<double>(s) / steps;
            m_points.push_back({catmullRom(p0, c1.pos, c2.pos, p3, u), c1.t + (c2.t - c1.t) * sc<float>(u), c1.speed + (c2.speed - c1.speed) * sc<float>(u)});
        }
    }
    m_points.push_back({ctrl.back().pos, ctrl.back().t, ctrl.back().speed});

    // bounding box incl. glow, global coords
    double x1 = INFINITY, y1 = INFINITY, x2 = -INFINITY, y2 = -INFINITY;
    for (const auto& p : m_points) {
        const double hw = halfWidthAt(p) + 2.0;
        x1              = std::min(x1, p.pos.x - hw);
        y1              = std::min(y1, p.pos.y - hw);
        x2              = std::max(x2, p.pos.x + hw);
        y2              = std::max(y2, p.pos.y + hw);
    }
    m_trailBox = CBox{x1, y1, x2 - x1, y2 - y1}.round();
}

float CCursorTrail::cursorOuterRadius() const {
    const float maxScale = std::max(1.F, fval(g_config.cursorClickScale)) * std::max(1.F, fval(g_config.cursorHoverScale));
    const float glow     = g_config.cursorGlow->value() > 0.F ? std::max(0.F, fval(g_config.cursorGlowSize)) : 0.F;
    const float shadow   = g_config.cursorShadow->value() > 0.F ? std::max(0.F, fval(g_config.cursorShadowSize)) : 0.F;
    const float body     = std::max(fval(g_config.cursorSize) * maxScale, fval(g_config.cursorTextHeight) * 0.5F);
    return body + std::max({glow, shadow, 3.F}) + 3.F;
}

void CCursorTrail::updateCursorBox() {
    if (!g_config.cursorEnabled->value() || !CTCompat::pointer()) {
        m_cursorBox = {};
        return;
    }
    const auto   pos = CTCompat::pointer()->position();
    const double r   = cursorOuterRadius();
    m_cursorBox      = CBox{pos.x - r, pos.y - r, r * 2, r * 2}.round();
}

void CCursorTrail::tickClickAnimation() {
    const auto  now    = TrailClock::now();
    const float dt     = std::clamp(secondsBetween(m_lastAnimTick, now), 0.F, 0.1F);
    m_lastAnimTick     = now;

    const float target = targetScale();
    m_clickScale += (target - m_clickScale) * (1.F - std::exp(-dt / CLICK_ANIM_S));
    if (std::abs(target - m_clickScale) < 0.002F)
        m_clickScale = target;
}

float CCursorTrail::targetScale() const {
    float s = 1.F;
    if (m_shape == SHAPE_LINK)
        s *= std::max(0.1F, fval(g_config.cursorHoverScale));
    if (m_buttonsDown > 0)
        s *= std::max(0.1F, fval(g_config.cursorClickScale));
    return s;
}

bool CCursorTrail::clickAnimating() const {
    return g_config.cursorEnabled->value() && m_clickScale != targetScale();
}

// ───────────────────────────── scheduling ───────────────────────────────────

void CCursorTrail::queueDamage() {
    if (m_damageQueued || !g_pEventLoopManager)
        return;

    m_damageQueued = true;
    // go through g_trail instead of capturing `this`, in case the plugin is unloaded meanwhile
    g_pEventLoopManager->doLater([] {
        if (!g_trail || !g_pHyprRenderer)
            return;
        g_trail->flushDamage();
    });
}

void CCursorTrail::flushDamage() {
    m_damageQueued = false;

    // damage where we were and where we are now, so old pixels get cleaned up
    const auto current = unionBox(m_trailBox, m_cursorBox);
    const auto box     = unionBox(m_lastDamaged, current);
    if (!box.empty())
        g_pHyprRenderer->damageBox(box.copy().expand(2));

    m_lastDamaged = current;
}

bool CCursorTrail::fullscreenOn(PHLMONITOR monitor) const {
    return monitor && g_config.hideInFullscreen->value() && CTCompat::hasFullscreen(monitor->m_activeWorkspace) &&
        !monitor->m_activeSpecialWorkspace;
}

// cursor_keep: comma separated shape names whose real cursor always stays (crosshair while
// taking a screenshot, ...)
std::vector<std::string> CCursorTrail::keptShapes() const {
    std::vector<std::string> out;
    const std::string        list  = g_config.cursorKeep->value();
    size_t                   start = 0;
    while (start <= list.size()) {
        size_t end = list.find(',', start);
        if (end == std::string::npos)
            end = list.size();
        std::string_view name{list.data() + start, end - start};
        while (!name.empty() && std::isspace(sc<unsigned char>(name.front())))
            name.remove_prefix(1);
        while (!name.empty() && std::isspace(sc<unsigned char>(name.back())))
            name.remove_suffix(1);
        if (!name.empty())
            out.emplace_back(name);
        start = end + 1;
    }
    return out;
}

void CCursorTrail::preloadKeptImages() {
    m_keepImages.refresh(keptShapes());
}

// Runs at the start of a frame (GL context current): if the app's cursor image changed, check
// whether it's one of the kept shapes from the theme.
void CCursorTrail::analyzeClientCursor() {
    if (!m_clientCursor || !CTCompat::pointer())
        return;
    const auto tex = CTCompat::pointer()->getCurrentCursorTexture();
    if (!tex)
        return;
    // the app may fill its cursor buffer a moment after setting it (and Hyprland reuses the
    // texture object), so look again for a few frames after every new app cursor
    bool recheck = false;
    if (m_recheckFrames > 0 && m_shape != SHAPE_KEEP) {
        --m_recheckFrames;
        recheck = true;
    }
    if (!m_analyzePending && !recheck && tex.get() == m_analyzedTex)
        return;
    m_analyzePending = false;
    m_analyzedTex    = tex.get();

    m_keepImages.refresh(keptShapes());
    m_debugRead.clear();
    m_readEmpty = false;

    // Hyprland keeps a CPU copy of shm cursor images (that's how nearly every app sends them).
    // Use it when there is one; reading the GPU texture back is only the fallback.
    bool keep = false;
    if (int w = 0, h = 0; !m_keepImages.empty() && readClientCursorShm(w, h)) {
        keep = m_keepImages.contains(m_readScratch.data(), w, h);
        if (g_config.debug->value()) {
            size_t visible = 0;
            for (size_t i = 3; i < m_readScratch.size(); i += 4)
                visible += m_readScratch[i] > 0;
            m_debugRead = std::format("shm {}x{}, {} visible px, best same-size match {}%", w, h, visible, m_keepImages.bestMatch(m_readScratch.data(), w, h));
        }
    } else
        keep = !m_keepImages.empty() && matchesKeptImage(tex);
    if (m_readEmpty && m_readRetries < 30) { // buffer not uploaded yet: look again next frame
        ++m_readRetries;
        m_analyzePending = true;
        queueDamage();
        return;
    }
    m_readRetries = 0;

    if (g_config.debug->value()) {
        const char* type = tex->m_type == Render::TEXTURE_RGBA ? "RGBA" : tex->m_type == Render::TEXTURE_RGBX ? "RGBX" : tex->m_type == Render::TEXTURE_EXTERNAL ? "EXTERNAL" : "other";
        const auto  msg  = std::format("[cursortrail] app cursor {}x{} ({}) → {}\n{}\n{}", sc<int>(tex->m_size.x), sc<int>(tex->m_size.y), type,
                                       keep ? "KEPT (matches cursor_keep)" : "replaced", m_debugRead.empty() ? "not read" : m_debugRead, m_keepImages.describe());
        if (msg != m_lastDebugMsg) {
            m_lastDebugMsg = msg;
            HyprlandAPI::addNotification(PHANDLE, msg, keep ? CHyprColor{0.3, 1.0, 0.4, 1.0} : CHyprColor{1.0, 0.8, 0.2, 1.0}, 8000);
        }
    }

    if (keep) {
        if (m_shape != SHAPE_KEEP)
            queueDamage();
        m_shape = SHAPE_KEEP;
        rememberKeptCursor();
    } else if (m_shape == SHAPE_KEEP) {
        m_shape = SHAPE_NORMAL;
        leaveKeep();
    }
}

bool CCursorTrail::readClientCursorShm(int& w, int& h) {
    if (!CTCompat::pointer())
        return false;
    const auto wl  = CTCompat::pointer()->m_currentCursorImage.surface.lock();
    const auto res = wl ? wl->resource() : nullptr;
    if (!res || !res->m_role || res->m_role->role() != SURFACE_ROLE_CURSOR)
        return false;

    // the role (and with it the pixel copy) lives as long as `res`, which we hold
    const std::vector<uint8_t>* pixels = &CCursorSurfaceRole::cursorPixelData(res);
    const std::vector<uint8_t>& data   = *pixels;
    w                = sc<int>(res->m_current.bufferSize.x);
    h                = sc<int>(res->m_current.bufferSize.y);
    if (data.empty() || w <= 0 || h <= 0 || w > 512 || h > 512)
        return false;
    const size_t stride = data.size() / h;
    if (stride < sc<size_t>(w) * 4)
        return false;

    // ARGB8888 little endian = B,G,R,A bytes → R,G,B,A like the theme images
    m_readScratch.resize(sc<size_t>(w) * h * 4);
    for (int y = 0; y < h; ++y) {
        const uint8_t* src = data.data() + y * stride;
        uint8_t*       dst = m_readScratch.data() + sc<size_t>(y) * w * 4;
        for (int x = 0; x < w; ++x) {
            dst[x * 4 + 0] = src[x * 4 + 2];
            dst[x * 4 + 1] = src[x * 4 + 1];
            dst[x * 4 + 2] = src[x * 4 + 0];
            dst[x * 4 + 3] = src[x * 4 + 3];
        }
    }
    return true;
}

static GLuint linkProgram(const char* vert, const char* frag);

bool CCursorTrail::matchesKeptImage(const SP<Render::ITexture>& tex) {
    if (!tex || !tex->m_texID || tex->m_type == Render::TEXTURE_EXTERNAL || tex->m_size.x < 1 || tex->m_size.y < 1 || !ensureGL())
        return false;

    if (!m_copyProg) {
        m_copyProg = linkProgram(CURSORCOPY_VERT, CURSORCOPY_FRAG);
        if (!m_copyProg)
            return false;
    }

    const int w = std::min(sc<int>(tex->m_size.x), 256), h = std::min(sc<int>(tex->m_size.y), 256);

    // ── save everything we touch ──
    GLint prevRead = 0, prevDraw = 0, prevProg = 0, prevVao = 0, prevVbo = 0, prevActive = 0, prevTex = 0;
    GLint viewport[4] = {};
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevRead);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDraw);
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProg);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevVbo);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActive);
    glGetIntegerv(GL_VIEWPORT, viewport);
    const GLboolean scissorOn = glIsEnabled(GL_SCISSOR_TEST);
    const GLboolean blendOn   = glIsEnabled(GL_BLEND);
    const GLboolean stencilOn = glIsEnabled(GL_STENCIL_TEST);
    const GLboolean depthOn   = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean cullOn    = glIsEnabled(GL_CULL_FACE);
    GLboolean       colorMask[4] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
    glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
    // a bound pixel pack buffer would swallow glReadPixels, odd pack settings would scramble it
    GLint prevPackBuf = 0, prevPackAlign = 4, prevPackRow = 0, prevPackSkipPx = 0, prevPackSkipRows = 0;
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &prevPackBuf);
    glGetIntegerv(GL_PACK_ALIGNMENT, &prevPackAlign);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &prevPackRow);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &prevPackSkipPx);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &prevPackSkipRows);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);

    // ── our RGBA8 target ──
    if (!m_readTex || m_readTexSize != Vector2D{sc<double>(w), sc<double>(h)}) {
        if (!m_readTex)
            glGenTextures(1, &m_readTex);
        glBindTexture(GL_TEXTURE_2D, m_readTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        m_readTexSize = {sc<double>(w), sc<double>(h)};
    }
    if (!m_readFbo)
        glGenFramebuffers(1, &m_readFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_readFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_readTex, 0);

    bool custom = false; // here: "is one of the kept shapes"
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
        // ── copy 1:1 ──
        glViewport(0, 0, w, h);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClearColor(0.F, 0.F, 0.F, 0.F);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindTexture(GL_TEXTURE_2D, tex->m_texID);
        glUseProgram(m_copyProg);
        glUniform1i(glGetUniformLocation(m_copyProg, "uTex"), 0);
        m_vertexScratch = {{-1.F, -1.F, 0.F, 0.F}, {1.F, -1.F, 0.F, 0.F}, {-1.F, 1.F, 0.F, 0.F}, {1.F, 1.F, 0.F, 0.F}};
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sc<GLsizeiptr>(m_vertexScratch.size() * sizeof(SVertex)), m_vertexScratch.data(), GL_STREAM_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // ── read back ──
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glPixelStorei(GL_PACK_ROW_LENGTH, 0);
        glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_PACK_SKIP_ROWS, 0);
        m_readScratch.assign(sc<size_t>(w) * h * 4, 0);
        glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, m_readScratch.data());

        // nothing visible at all = the app's buffer isn't on the GPU yet → try again next frame
        m_readEmpty = std::ranges::none_of(m_readScratch, [](uint8_t b) { return b != 0; });

        custom = m_keepImages.contains(m_readScratch.data(), w, h);
        if (g_config.debug->value())
        {
            size_t visible = 0;
            for (size_t i = 3; i < m_readScratch.size(); i += 4)
                visible += m_readScratch[i] > 0;
            m_debugRead = std::format("read {}x{}, {} visible px, best same-size match {}%{}", w, h, visible, m_keepImages.bestMatch(m_readScratch.data(), w, h),
                                      m_readEmpty ? " (nothing visible, buffer empty?)" : "");
        }
    } else if (g_config.debug->value()) {
        m_debugRead = "framebuffer incomplete, couldn't read the image";
    }

    // ── restore ──
    glBindBuffer(GL_PIXEL_PACK_BUFFER, prevPackBuf);
    glPixelStorei(GL_PACK_ALIGNMENT, prevPackAlign);
    glPixelStorei(GL_PACK_ROW_LENGTH, prevPackRow);
    glPixelStorei(GL_PACK_SKIP_PIXELS, prevPackSkipPx);
    glPixelStorei(GL_PACK_SKIP_ROWS, prevPackSkipRows);
    glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
    stencilOn ? glEnable(GL_STENCIL_TEST) : glDisable(GL_STENCIL_TEST);
    depthOn ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
    cullOn ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, prevRead);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDraw);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    scissorOn ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST);
    blendOn ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, prevTex);
    glActiveTexture(prevActive);
    glBindBuffer(GL_ARRAY_BUFFER, prevVbo);
    glBindVertexArray(prevVao);
    glUseProgram(prevProg);
    return custom;
}

void CCursorTrail::rememberKeptCursor() {
    const auto* pm = CTCompat::pointer();
    if (!pm)
        return;
    if (auto tex = CTCompat::pointer()->getCurrentCursorTexture()) {
        m_keptTex     = tex; // holding it keeps the GPU texture alive after the app's surface is gone
        m_keptHotspot = pm->m_currentCursorImage.hotspot;
        m_keptScale   = std::max(pm->m_currentCursorImage.scale, 0.01F);
    }
}

void CCursorTrail::leaveKeep() {
    const int ms  = std::max<int>(0, sc<int>(g_config.cursorKeepLinger->value()));
    m_lingerUntil = m_keptTex && ms > 0 ? TrailClock::now() + std::chrono::milliseconds(ms) : TrailClock::time_point{};
    queueDamage();
}

bool CCursorTrail::lingering() const {
    return m_keptTex && TrailClock::now() < m_lingerUntil;
}

bool CCursorTrail::wantCircle(PHLMONITOR monitor) const {
    if (!g_config.cursorEnabled->value())
        return false;
    if (fullscreenOn(monitor) && !g_config.cursorInFullscreen->value())
        return false;
    return !textMode() && m_shape != SHAPE_KEEP;
}

bool CCursorTrail::wantStyledText(PHLMONITOR monitor) const {
    if (!g_config.cursorEnabled->value() || !g_config.cursorTextStyle->value() || !textMode())
        return false;
    return !(fullscreenOn(monitor) && !g_config.cursorInFullscreen->value());
}

// ───────────────────────────── render hooks ─────────────────────────────────

void CCursorTrail::onRenderLastMoment(PHLMONITOR monitor) {
    analyzeClientCursor();

    const auto now = TrailClock::now();
    buildPoints(now);
    tickClickAnimation();
    updateCursorBox();

    const CBox monBox    = {monitor->m_position, monitor->m_size};
    const bool drawTrail = m_points.size() >= 2 && !fullscreenOn(monitor) && intersects(monBox, m_trailBox);

    // locate the software cursor in this frame's pass (CTexPassElement with the cursor texture,
    // or hypr-dynamic-cursors' own element). Not found = hardware cursor / cursor hidden / other monitor.
    auto&                 pass      = g_pHyprRenderer->m_renderPass;
    auto&                 elems     = pass.m_passElements;
    const auto            cursorTex = CTCompat::pointer() ? CTCompat::pointer()->getCurrentCursorTexture() : nullptr;
    std::optional<size_t> cursorIdx;

    for (size_t i = elems.size(); i-- > 0;) {
        auto* el = elems[i].element.get();
        if (!el)
            continue;
        if ((el->type() == EK_TEXTURE && cursorTex && sc<CTexPassElement*>(el)->m_data.tex == cursorTex) ||
            std::string_view{el->passName()} == "CCursorPassElement") {
            cursorIdx = i;
            break;
        }
    }

    // kept theme shape (by name): remember its image for the linger
    if (m_shape == SHAPE_KEEP && !m_keptTex && cursorIdx)
        rememberKeptCursor();

    // linger: right after a kept cursor (slurp's crosshair) ends, draw no cursor at all for a
    // moment. With software cursors Hyprland bakes whatever cursor is showing into screenshots,
    // so this keeps the screenshot taken right after the selection clean. No trail meanwhile.
    if (m_shape != SHAPE_KEEP && lingering()) {
        if (cursorIdx)
            elems.erase(elems.begin() + *cursorIdx);
        queueDamage(); // keep frames coming until the linger is over
        return;
    }
    if (m_keptTex && m_shape != SHAPE_KEEP && TrailClock::now() >= m_lingerUntil)
        m_keptTex.reset();

    const bool replaceCircle = cursorIdx.has_value() && wantCircle(monitor);
    const bool replaceText   = cursorIdx.has_value() && wantStyledText(monitor);
    const bool replaceCursor = replaceCircle || replaceText;

    // With software cursors (cursor.no_hardware_cursors = 1) a frame on the pointer's monitor that
    // has no cursor element means the cursor is hidden (pointer locked in a game, inactive timeout, ...)
    // → no trail. With hardware cursors we can't tell this way and rely on the other checks.
    static auto PNOHW = CConfigValue<Config::INTEGER>("cursor:no_hardware_cursors");
    if (CTCompat::pointer() && *PNOHW == 1) {
        const CBox monLogical = {monitor->m_position, monitor->m_size};
        if (monLogical.containsPoint(CTCompat::pointer()->position())) {
            const bool hidden = !cursorIdx.has_value();
            if (hidden && !m_noCursorDrawn) {
                clear();
                m_trailBox = {};
                queueDamage(); // wipe what's left
            }
            m_noCursorDrawn = hidden;
        }
    }

    if (!replaceCursor && (!drawTrail || cursorHidden()))
        return;

    // our elements go exactly where the cursor is: trail below it, circle instead of it
    size_t pos = cursorIdx.value_or(elems.size());
    if (replaceCursor)
        elems.erase(elems.begin() + pos);

    auto insert = [&](UP<IPassElement>&& el) {
        pass.add(std::move(el));
        std::rotate(elems.begin() + pos, elems.end() - 1, elems.end());
        ++pos;
    };

    if (drawTrail && !cursorHidden())
        insert(makeUnique<CCursorTrailPassElement>(CCursorTrailPassElement::KIND_TRAIL));
    if (replaceCircle)
        insert(makeUnique<CCursorTrailPassElement>(CCursorTrailPassElement::KIND_CIRCLE));
    else if (replaceText)
        insert(makeUnique<CCursorTrailPassElement>(CCursorTrailPassElement::KIND_TEXT));
}

void CCursorTrail::onRenderPost(PHLMONITOR) {
    // keep frames coming while something animates, plus one more to wipe the trail once it's gone
    // textPending: keep checking until the dwell delay passes so the I-beam swaps in even if the mouse stopped
    if (!m_points.empty() || m_hadTrail || clickAnimating() || textPending() || (m_clientCursor && m_recheckFrames > 0 && m_shape != SHAPE_KEEP))
        queueDamage();
    m_hadTrail = !m_points.empty();
}

// ──────────────────────────────── GL ────────────────────────────────────────

static GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        std::array<char, 1024> log{};
        glGetShaderInfoLog(shader, log.size(), nullptr, log.data());
        HyprlandAPI::addNotification(PHANDLE, std::string{"[cursortrail] shader compile failed: "} + log.data(), CHyprColor{1.0, 0.2, 0.2, 1.0}, 10000);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint linkProgram(const char* vert, const char* frag) {
    const GLuint vs = compileShader(GL_VERTEX_SHADER, vert);
    const GLuint fs = compileShader(GL_FRAGMENT_SHADER, frag);
    if (!vs || !fs) {
        if (vs)
            glDeleteShader(vs);
        if (fs)
            glDeleteShader(fs);
        return 0;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glBindAttribLocation(prog, 0, "aPos");
    glBindAttribLocation(prog, 1, "aData");
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        std::array<char, 1024> log{};
        glGetProgramInfoLog(prog, log.size(), nullptr, log.data());
        HyprlandAPI::addNotification(PHANDLE, std::string{"[cursortrail] shader link failed: "} + log.data(), CHyprColor{1.0, 0.2, 0.2, 1.0}, 10000);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

bool CCursorTrail::ensureGL() {
    if (m_trailProg.id && m_circleProg.id && m_contrastProg.id && m_textProg.id)
        return true;
    if (m_glFailed)
        return false;

    m_trailProg.id    = linkProgram(CURSORCIRCLE_VERT, CURSORTRAIL_FRAG);
    m_circleProg.id   = linkProgram(CURSORCIRCLE_VERT, (std::string{CURSORSHAPE_LIB} + CURSORCIRCLE_FRAG).c_str());
    m_contrastProg.id = linkProgram(CURSORCIRCLE_VERT, (std::string{CURSORSHAPE_LIB} + CURSORCONTRAST_FRAG).c_str());
    m_textProg.id     = linkProgram(CURSORCIRCLE_VERT, CURSORTEXT_FRAG);
    if (!m_trailProg.id || !m_circleProg.id || !m_contrastProg.id || !m_textProg.id) {
        destroyGL();
        m_glFailed = true;
        return false;
    }

    for (const char* name : {"uProj", "uFullSize", "uPts", "uCount", "uHeadColor", "uTailColor", "uOpacity", "uGlow", "uCoreR", "uHot", "uAdditive"})
        m_trailProg.loc.push_back(glGetUniformLocation(m_trailProg.id, name));
    for (const char* name : {"uProj", "uFullSize", "uColor", "uCenterColor", "uOuter", "uRadius", "uRing", "uGlow", "uGlowSize", "uOutline", "uShadow", "uShadowSize", "uStyle"})
        m_circleProg.loc.push_back(glGetUniformLocation(m_circleProg.id, name));
    for (const char* name : {"uProj", "uFullSize", "uOuter", "uRadius", "uContrast", "uRing", "uStyle"})
        m_contrastProg.loc.push_back(glGetUniformLocation(m_contrastProg.id, name));
    for (const char* name : {"uProj", "uFullSize", "uColor", "uCenterColor", "uOuter", "uBar", "uSerif", "uRing", "uGlow", "uGlowSize", "uShadow", "uShadowSize", "uOutline"})
        m_textProg.loc.push_back(glGetUniformLocation(m_textProg.id, name));

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    // remember Hyprland's bindings so we can put them back
    GLint prevVao = 0, prevVbo = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevVbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(SVertex), rc<void*>(offsetof(SVertex, x)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SVertex), rc<void*>(offsetof(SVertex, a)));

    glBindVertexArray(prevVao);
    glBindBuffer(GL_ARRAY_BUFFER, prevVbo);
    return true;
}


void CCursorTrail::destroyGL() {
    if (m_vbo)
        glDeleteBuffers(1, &m_vbo);
    if (m_vao)
        glDeleteVertexArrays(1, &m_vao);
    if (m_trailProg.id)
        glDeleteProgram(m_trailProg.id);
    if (m_circleProg.id)
        glDeleteProgram(m_circleProg.id);
    if (m_contrastProg.id)
        glDeleteProgram(m_contrastProg.id);
    if (m_textProg.id)
        glDeleteProgram(m_textProg.id);
    if (m_readFbo)
        glDeleteFramebuffers(1, &m_readFbo);
    if (m_readTex)
        glDeleteTextures(1, &m_readTex);
    if (m_copyProg)
        glDeleteProgram(m_copyProg);
    m_readFbo = m_readTex = m_copyProg = 0;
    m_readTexSize = {};
    m_analyzedTex = nullptr;
    m_vbo = m_vao  = 0;
    m_trailProg    = {};
    m_circleProg   = {};
    m_contrastProg = {};
    m_textProg     = {};
}

// Binds `prog`, lets `setUniforms` fill it, uploads m_vertexScratch and draws it inside the
// current damage. Restores every bit of GL state Hyprland caches.
void CCursorTrail::uploadAndDraw(PHLMONITOR monitor, const SProgram& prog, GLenum mode) {
    auto& renderData = g_pHyprRenderer->m_renderData;

    CBox  fullBox = {0, 0, monitor->m_transformedSize.x, monitor->m_transformedSize.y};
    renderData.renderModif.applyToBox(fullBox);
    const auto proj = g_pHyprRenderer->projectBoxToTarget(fullBox).getMatrix();

    glUniformMatrix3fv(prog[0], 1, GL_TRUE, proj.data());
    glUniform2f(prog[1], sc<float>(fullBox.w), sc<float>(fullBox.h));

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sc<GLsizeiptr>(m_vertexScratch.size() * sizeof(SVertex)), m_vertexScratch.data(), GL_STREAM_DRAW);

    renderData.damage.forEachRect([this, mode](const auto& RECT) {
        g_pHyprOpenGL->scissor(&RECT, g_pHyprRenderer->m_renderData.transformDamage);
        glDrawArrays(mode, 0, sc<GLsizei>(m_vertexScratch.size()));
    });
    g_pHyprOpenGL->scissor(nullptr);
}

namespace {
    // saves/restores the GL bindings we touch; Hyprland caches the current program
    struct SGLStateGuard {
        GLint program = 0, vao = 0, vbo = 0;
        SGLStateGuard() {
            glGetIntegerv(GL_CURRENT_PROGRAM, &program);
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
            glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);
        }
        ~SGLStateGuard() {
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Hyprland's default: everything premultiplied
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBindVertexArray(vao);
            glUseProgram(program);
        }
    };
}

void CCursorTrail::drawTrail(PHLMONITOR monitor) {
    if (!monitor || m_points.size() < 2 || !ensureGL() || g_pHyprRenderer->m_renderData.damage.empty())
        return;

    const double scale = monitor->m_scale;

    // ── resample the spline to at most CURSORTRAIL_MAX_POINTS points, evenly by arc length ──
    double total = 0;
    for (size_t i = 0; i + 1 < m_points.size(); ++i)
        total += m_points[i].pos.distance(m_points[i + 1].pos);

    const int n = std::clamp(sc<int>(std::ceil(total / 3.0)) + 1, 2, CURSORTRAIL_MAX_POINTS);
    m_ptsScratch.clear();
    m_ptsScratch.reserve(n * 4);

    // The head sits right under the cursor. At full width the round end of its glow would poke out
    // in front of the cursor (and linger there after stopping), so the glow starts small and grows
    // to full size over a short distance: with the circle it starts at the circle's size, without
    // it at the core. The core itself keeps its width.
    const float coreFrac   = 1.F / glowFactor();
    const float headRadius = g_config.cursorEnabled->value() ? std::max(1.F, fval(g_config.cursorSize)) * m_clickScale * 0.85F : 0.F;
    m_coreScratch.clear();

    double x1 = INFINITY, y1 = INFINITY, x2 = -INFINITY, y2 = -INFINITY;
    size_t seg = 0;
    double segStart = 0; // arc length at m_points[seg]
    for (int k = 0; k < n; ++k) {
        const double target = total * k / (n - 1);
        while (seg + 2 < m_points.size() && segStart + m_points[seg].pos.distance(m_points[seg + 1].pos) < target) {
            segStart += m_points[seg].pos.distance(m_points[seg + 1].pos);
            ++seg;
        }
        const auto&  a   = m_points[seg];
        const auto&  b   = m_points[seg + 1];
        const double len = a.pos.distance(b.pos);
        const double f   = len > 1e-9 ? std::clamp((target - segStart) / len, 0.0, 1.0) : 0.0;

        SPoint       p{a.pos + (b.pos - a.pos) * f, a.t + (b.t - a.t) * sc<float>(f), a.speed + (b.speed - a.speed) * sc<float>(f)};
        const auto   l = (p.pos - monitor->m_position) * scale;
        const double full = halfWidthAt(p);
        const double core = full * coreFrac;
        const double head = std::clamp<double>(headRadius, core, full);
        const double ramp = std::clamp(target / std::max(full * HEAD_RAMP_LENGTH, 1.0), 0.0, 1.0);
        const double r    = (head + (full - head) * ramp * ramp * (3.0 - 2.0 * ramp)) * scale; // smoothstep
        m_coreScratch.push_back(sc<float>(core * scale));

        m_ptsScratch.insert(m_ptsScratch.end(), {sc<float>(l.x), sc<float>(l.y), sc<float>(r), p.t});
        x1 = std::min(x1, l.x - r), y1 = std::min(y1, l.y - r), x2 = std::max(x2, l.x + r), y2 = std::max(y2, l.y + r);
    }

    // ── one quad over the trail bbox (clipped to the monitor), uv = monitor px ──
    x1 = std::floor(std::max(x1 - 1, 0.0)), y1 = std::floor(std::max(y1 - 1, 0.0));
    x2 = std::ceil(std::min(x2 + 1, monitor->m_transformedSize.x)), y2 = std::ceil(std::min(y2 + 1, monitor->m_transformedSize.y));
    if (x2 <= x1 || y2 <= y1)
        return;

    const float fx1 = sc<float>(x1), fy1 = sc<float>(y1), fx2 = sc<float>(x2), fy2 = sc<float>(y2);
    m_vertexScratch = {{fx1, fy1, fx1, fy1}, {fx2, fy1, fx2, fy1}, {fx1, fy2, fx1, fy2}, {fx2, fy2, fx2, fy2}};

    const auto    head     = toRenderRGB(g_config.color->value(), true);
    const auto    tail     = g_config.gradient->value() ? toRenderRGB(g_config.tailColor->value(), true) : head;
    const bool    additive = g_config.blend->value() != "normal";

    SGLStateGuard guard;
    glUseProgram(m_trailProg.id);
    glUniform4fv(m_trailProg[TU_PTS], n, m_ptsScratch.data());
    glUniform1i(m_trailProg[TU_COUNT], n);
    glUniform3f(m_trailProg[TU_HEAD], head[0], head[1], head[2]);
    glUniform3f(m_trailProg[TU_TAIL], tail[0], tail[1], tail[2]);
    glUniform1f(m_trailProg[TU_OPACITY], std::clamp(fval(g_config.opacity), 0.F, 1.F));
    glUniform1f(m_trailProg[TU_GLOW], std::max(0.F, fval(g_config.glow)));
    glUniform1fv(m_trailProg[TU_CORER], n, m_coreScratch.data());
    glUniform1f(m_trailProg[TU_HOT], std::clamp(fval(g_config.hotCore), 0.F, 1.F));
    glUniform1i(m_trailProg[TU_ADDITIVE], additive ? 1 : 0);

    g_pHyprOpenGL->blend(true);
    if (additive)
        glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE); // light adds up, framebuffer alpha untouched
    else
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // premultiplied over

    uploadAndDraw(monitor, m_trailProg, GL_TRIANGLE_STRIP);
}

void CCursorTrail::drawCursor(PHLMONITOR monitor) {
    if (!monitor || !CTCompat::pointer() || !ensureGL() || g_pHyprRenderer->m_renderData.damage.empty())
        return;

    const float    scale    = sc<float>(monitor->m_scale);
    const float    radius   = std::max(1.F, fval(g_config.cursorSize)) * m_clickScale; // outer edge of the ring
    const float    ring     = std::clamp(fval(g_config.cursorRing) * m_clickScale, 0.5F, radius);
    const float    glowSize = g_config.cursorGlow->value() > 0.F ? std::max(0.F, fval(g_config.cursorGlowSize)) : 0.F;
    const float    shadowSz = g_config.cursorShadow->value() > 0.F ? std::max(0.F, fval(g_config.cursorShadowSize)) : 0.F;
    const float    outer    = radius + std::max({glowSize, shadowSz, 3.F}) + 2.F;
    const int      style    = std::clamp(sc<int>(g_config.cursorStyle->value()), 0, CURSOR_STYLE_COUNT - 1);

    const Vector2D c = (CTCompat::pointer()->position() - monitor->m_position) * scale;
    const float    h = outer * scale;
    const float    x = sc<float>(c.x), y = sc<float>(c.y);

    m_vertexScratch = {{x - h, y - h, -1.F, -1.F}, {x + h, y - h, 1.F, -1.F}, {x - h, y + h, -1.F, 1.F}, {x + h, y + h, 1.F, 1.F}};

    const auto    ringCol   = toRenderRGB(g_config.cursorColor->value(), true);
    const auto    centerCol = toRenderRGB(g_config.cursorCenterColor->value());

    SGLStateGuard guard;
    glUseProgram(m_circleProg.id);
    glUniform3f(m_circleProg[CU_COLOR], ringCol[0], ringCol[1], ringCol[2]);
    glUniform3f(m_circleProg[CU_CENTERCOLOR], centerCol[0], centerCol[1], centerCol[2]);
    glUniform1f(m_circleProg[CU_OUTER], h);
    glUniform1f(m_circleProg[CU_RADIUS], radius * scale);
    glUniform1f(m_circleProg[CU_RING], ring * scale);
    glUniform1f(m_circleProg[CU_GLOW], std::max(0.F, fval(g_config.cursorGlow)));
    glUniform1f(m_circleProg[CU_GLOWSIZE], glowSize * scale);
    glUniform1f(m_circleProg[CU_OUTLINE], std::clamp(fval(g_config.cursorOutline), 0.F, 1.F));
    glUniform1f(m_circleProg[CU_SHADOW], std::clamp(fval(g_config.cursorShadow), 0.F, 1.F));
    glUniform1f(m_circleProg[CU_SHADOWSIZE], shadowSz * scale);
    glUniform1i(m_circleProg[CU_STYLE], style);

    g_pHyprOpenGL->blend(true);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // solid cursor → plain premultiplied over

    uploadAndDraw(monitor, m_circleProg, GL_TRIANGLE_STRIP);

    // optional inverted hairline: always contrasts with whatever is behind the ring
    if (const float contrast = std::clamp(fval(g_config.cursorContrast), 0.F, 1.F); contrast > 0.F) {
        glUseProgram(m_contrastProg.id);
        glUniform1f(m_contrastProg[XU_OUTER], h);
        glUniform1f(m_contrastProg[XU_RADIUS], radius * scale);
        glUniform1f(m_contrastProg[XU_CONTRAST], contrast);
        glUniform1f(m_contrastProg[XU_RING], ring * scale);
        glUniform1i(m_contrastProg[XU_STYLE], style);
        glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
        uploadAndDraw(monitor, m_contrastProg, GL_TRIANGLE_STRIP);
    }
}

// Styled I-beam: same colors, glow and shadow as the circle. Hotspot = center of the beam.
void CCursorTrail::drawTextCursor(PHLMONITOR monitor) {
    if (!monitor || !CTCompat::pointer() || !ensureGL() || g_pHyprRenderer->m_renderData.damage.empty())
        return;

    const float    scale    = sc<float>(monitor->m_scale);
    const float    halfH    = std::max(3.F, fval(g_config.cursorTextHeight) * 0.5F);
    const float    halfW    = std::clamp(fval(g_config.cursorTextWidth) * 0.5F, 0.5F, halfH);
    const float    serifW   = halfW + std::max(1.5F, halfH * 0.2F);
    const float    serifH   = std::max(1.F, halfW * 0.65F);
    const float    ring     = std::max(0.5F, halfW * 0.56F);
    const float    glowSize = g_config.cursorGlow->value() > 0.F ? std::max(0.F, fval(g_config.cursorGlowSize)) * 0.85F : 0.F;
    const float    shadowSz = g_config.cursorShadow->value() > 0.F ? std::max(0.F, fval(g_config.cursorShadowSize)) * 0.9F : 0.F;
    const float    outer    = halfH + std::max({glowSize, shadowSz, 3.F}) + 2.F;

    const Vector2D c = (CTCompat::pointer()->position() - monitor->m_position) * scale;
    const float    h = outer * scale;
    const float    x = sc<float>(c.x), y = sc<float>(c.y);

    // uv = px offset from the hotspot
    m_vertexScratch = {{x - h, y - h, -h, -h}, {x + h, y - h, h, -h}, {x - h, y + h, -h, h}, {x + h, y + h, h, h}};

    const auto    ringCol   = toRenderRGB(g_config.cursorColor->value(), true);
    const auto    centerCol = toRenderRGB(g_config.cursorCenterColor->value());

    SGLStateGuard guard;
    glUseProgram(m_textProg.id);
    glUniform3f(m_textProg[IU_COLOR], ringCol[0], ringCol[1], ringCol[2]);
    glUniform3f(m_textProg[IU_CENTERCOLOR], centerCol[0], centerCol[1], centerCol[2]);
    glUniform1f(m_textProg[IU_OUTER], h);
    glUniform2f(m_textProg[IU_BAR], halfW * scale, halfH * scale);
    glUniform2f(m_textProg[IU_SERIF], serifW * scale, serifH * scale);
    glUniform1f(m_textProg[IU_RING], ring * scale);
    glUniform1f(m_textProg[IU_GLOW], std::max(0.F, fval(g_config.cursorGlow)));
    glUniform1f(m_textProg[IU_GLOWSIZE], glowSize * scale);
    glUniform1f(m_textProg[IU_SHADOW], std::clamp(fval(g_config.cursorShadow), 0.F, 1.F));
    glUniform1f(m_textProg[IU_SHADOWSIZE], shadowSz * scale);
    glUniform1f(m_textProg[IU_OUTLINE], std::clamp(fval(g_config.cursorOutline), 0.F, 1.F));

    g_pHyprOpenGL->blend(true);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    uploadAndDraw(monitor, m_textProg, GL_TRIANGLE_STRIP);
}
