#pragma once

// GLES 3.0 so it runs on every context Hyprland can create.

// ─────────────────────────────── trail ───────────────────────────────────────
// Single pass, no offscreen buffer: one quad over the trail's bounding box, every fragment
// evaluates the true distance to each capsule (segment) of the resampled trail and keeps the
// MAX contribution. Max instead of sum means overlaps, 180° turns and self-crossings never
// double up or fold; head and tail get round caps. Same vertex shader as the circle.

#define CURSORTRAIL_MAX_POINTS 64

inline constexpr const char* CURSORTRAIL_FRAG = R"glsl(#version 300 es
precision highp float;

#define MAX_POINTS 64

uniform vec4  uPts[MAX_POINTS]; // x, y (monitor px), glow radius (px), age (0 head .. 1 tail)
uniform float uCoreR[MAX_POINTS]; // core radius (px); the glow shrinks towards the head, the core doesn't
uniform int   uCount;
uniform vec3  uHeadColor;
uniform vec3  uTailColor;
uniform float uOpacity;
uniform float uGlow;     // glow strength (0 = none)
uniform float uHot;      // push the core towards white
uniform int   uAdditive;

in vec2 vUV; // monitor px

out vec4 fragColor;

void main() {
    vec4 best = vec4(0.0);

    for (int i = 0; i < MAX_POINTS - 1; ++i) {
        if (i >= uCount - 1)
            break;

        vec4  P0 = uPts[i], P1 = uPts[i + 1];
        vec2  AB = P1.xy - P0.xy;
        float L2 = dot(AB, AB);
        float h  = L2 > 1e-6 ? clamp(dot(vUV - P0.xy, AB) / L2, 0.0, 1.0) : 0.0;
        float r  = mix(P0.z, P1.z, h);
        float rc = mix(uCoreR[i], uCoreR[i + 1], h);
        float dp = length(vUV - (P0.xy + AB * h));
        float d  = dp / max(r, 1e-3); // 0 spine .. 1 glow edge

        if (d >= 1.0)
            continue;

        float age  = mix(P0.w, P1.w, h);
        float life = clamp(1.0 - age, 0.0, 1.0);

        float core = 1.0 - smoothstep(rc * 0.45, rc, dp);

        const float K = 5.0;
        float halo = (exp(-d * d * K) - exp(-K)) / (1.0 - exp(-K));
        halo *= uGlow * (0.55 + 0.45 * life);

        float intensity = uAdditive == 1 ? min(core + halo, 1.6) : clamp(core + halo * (1.0 - core), 0.0, 1.0);
        float a         = intensity * pow(life, 1.15) * uOpacity;

        vec3 col = mix(uHeadColor, uTailColor, age);
        col      = mix(col, vec3(1.0), core * uHot * life);

        best = max(best, vec4(col * a, a));
    }

    if (best.a <= 0.0)
        discard;

    // premultiplied; additive keeps the framebuffer alpha untouched
    fragColor = uAdditive == 1 ? vec4(best.rgb, 0.0) : vec4(best.rgb, min(best.a, 1.0));
}
)glsl";

// ───────────────────────── osu!-style circle cursor ─────────────────────────

inline constexpr const char* CURSORCIRCLE_VERT = R"glsl(#version 300 es
precision highp float;

uniform mat3 uProj;
uniform vec2 uFullSize;

in vec2 aPos;  // monitor-local pixels
in vec2 aData; // quad uv in -1 .. 1

out vec2 vUV;

void main() {
    vec3 p      = uProj * vec3(aPos / uFullSize, 1.0);
    gl_Position = vec4(p.xy, 0.0, 1.0);
    vUV         = aData;
}
)glsl";

// Shape library shared by the circle and its contrast pass. Every style fits inside
// uRadius, so size, hover/click scaling and the quad bounds work the same for all of them.
// Distances are in px, negative inside. Prepended to the two shaders below at link time.
inline constexpr const char* CURSORSHAPE_LIB = R"glsl(#version 300 es
precision highp float;

uniform int   uStyle;  // cursor_style
uniform float uRadius; // outer edge of the shape in px
uniform float uRing;   // ring / rim thickness in px

const float NONE = 1e5;
const float PI   = 3.14159265;

float sdCircle(vec2 p, float r) { return length(p) - r; }
float sdRing(vec2 p, float rc, float t) { return abs(length(p) - rc) - t * 0.5; }
float sdBox(vec2 p, vec2 b) {
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}
float sdRoundBox(vec2 p, vec2 b, float r) { return sdBox(p, b - r) - r; }
float sdRhombus(vec2 p, float r) { // square turned 45°, corners at r
    vec2 q = vec2(p.x + p.y, p.y - p.x) * 0.70710678;
    return sdRoundBox(q, vec2(r * 0.70710678), r * 0.08);
}
float sdHexagon(vec2 p, float r) { // r = inradius
    const vec3 k = vec3(-0.866025404, 0.5, 0.577350269);
    p = abs(p);
    p -= 2.0 * min(dot(k.xy, p), 0.0) * k.xy;
    p -= vec2(clamp(p.x, -k.z * r, k.z * r), r);
    return length(p) * sign(p.y);
}
float sdStar5(vec2 p, float r, float rf) {
    const vec2 k1 = vec2(0.809016994375, -0.587785252292);
    const vec2 k2 = vec2(-k1.x, k1.y);
    p.x = abs(p.x);
    p -= 2.0 * max(dot(k1, p), 0.0) * k1;
    p -= 2.0 * max(dot(k2, p), 0.0) * k2;
    p.x = abs(p.x);
    p.y -= r;
    vec2  ba = rf * vec2(-k1.y, k1.x) - vec2(0.0, 1.0);
    float h  = clamp(dot(p, ba) / dot(ba, ba), 0.0, r);
    return length(p - ba * h) * sign(p.y * ba.x - p.x * ba.y);
}
// ring cut into n arcs with round ends (exact distance); duty = arc share of each segment
float sdDashRing(vec2 p, float rc, float t, float n, float duty, float phase) {
    float seg = 2.0 * PI / n;
    float ac  = floor((atan(p.y, p.x) + phase) / seg + 0.5) * seg - phase; // nearest arc center
    float ca = cos(PI * 0.5 - ac), sa = sin(PI * 0.5 - ac);
    vec2  q   = vec2(ca * p.x - sa * p.y, sa * p.x + ca * p.y);              // that center → +y
    float ap  = max(seg * duty * 0.5 - t * 0.5 / rc, 0.0);                  // half aperture minus the cap
    vec2  sc  = vec2(sin(ap), cos(ap));
    q.x       = abs(q.x);
    return ((sc.y * q.x > sc.x * q.y) ? length(q - sc * rc) : abs(length(q) - rc)) - t * 0.5;
}
// four arms from r0 to r1, thickness t, round ends
float sdPlus(vec2 p, float r0, float r1, float t) {
    p = abs(p);
    if (p.y > p.x)
        p = p.yx;
    return sdRoundBox(p - vec2((r0 + r1) * 0.5, 0.0), vec2((r1 - r0) * 0.5 + t * 0.5, t * 0.5), t * 0.5);
}

// dA is painted in uColor, dB in uCenterColor on top, dC in uColor on top of that.
void cursorShape(vec2 p, out float dA, out float dB, out float dC) {
    float R = uRadius, T = uRing;
    dA = NONE; dB = NONE; dC = NONE;

    if (uStyle == 1) {        // ring
        dA = sdRing(p, R - T * 0.5, T);
    } else if (uStyle == 2) { // solid
        dA = sdCircle(p, R);
    } else if (uStyle == 3) { // ring + dot
        dA = sdRing(p, R - T * 0.35, T * 0.7);
        dB = sdCircle(p, max(R * 0.28, 1.5));
    } else if (uStyle == 4) { // double ring
        dA = min(sdRing(p, R - T * 0.3, T * 0.6), sdRing(p, R * 0.5, T * 0.6));
        dB = sdCircle(p, max(R * 0.13, 1.2));
    } else if (uStyle == 5) { // bullseye
        dA = sdCircle(p, R);
        dB = dA + T * 0.9;
        dC = sdCircle(p, max(R * 0.3, 1.5));
    } else if (uStyle == 6) { // dashed ring
        dA = sdDashRing(p, R - T * 0.375, T * 0.75, 6.0, 0.62, 0.0);
    } else if (uStyle == 7) { // split ring + dot
        dA = sdDashRing(p, R - T * 0.4, T * 0.8, 4.0, 0.72, PI / 4.0);
        dB = sdCircle(p, max(R * 0.22, 1.5));
    } else if (uStyle == 8) { // crosshair
        dA = sdPlus(p, R * 0.32, R, max(T * 0.6, 1.6));
        dB = sdCircle(p, max(T * 0.4, 1.2));
    } else if (uStyle == 9) { // orb (center → ring color gradient, see main)
        dA = sdCircle(p, R);
    } else if (uStyle == 10) { // diamond
        dA = sdRhombus(p, R);
        dB = dA + T;
    } else if (uStyle == 11) { // hexagon
        dA = sdHexagon(p, R * 0.866);
        dB = dA + T;
    } else if (uStyle == 12) { // rounded square
        dA = sdRoundBox(p, vec2(R * 0.74), R * 0.3);
        dB = dA + T;
    } else if (uStyle == 13) { // star
        dA = sdStar5(vec2(p.x, -p.y), R, 0.5);
        dB = dA + T * 0.75;
    } else if (uStyle == 14) { // scope
        dA = min(sdRing(p, R - T * 0.3, T * 0.6), sdPlus(p, R * 0.3, R * 0.7, max(T * 0.45, 1.4)));
        dB = sdCircle(p, max(R * 0.1, 1.0));
    } else if (uStyle == 15) { // small dot
        dA = sdCircle(p, R * 0.5);
        dB = dA + max(T * 0.5, 1.0);
    } else {                  // 0: classic osu! circle
        dA = sdCircle(p, R);
        dB = dA + T;
    }
}

float cursorSDF(vec2 p) {
    float dA, dB, dC;
    cursorShape(p, dA, dB, dC);
    return min(dA, min(dB, dC));
}
)glsl";

inline constexpr const char* CURSORCIRCLE_FRAG = R"glsl(
// solid osu!-style cursor: shape from cursorShape(), soft glow + shadow around it
uniform vec3  uColor;       // ring + glow
uniform vec3  uCenterColor; // filled center
uniform float uOuter;       // quad half size in px
uniform float uGlow;        // glow strength
uniform float uGlowSize;    // glow reach in px
uniform float uOutline;     // dark edge opacity
uniform float uShadow;      // soft dark halo behind everything (contrast on light backgrounds)
uniform float uShadowSize;  // its reach in px

in vec2 vUV;

out vec4 fragColor;

// premultiplied "over"
void over(inout vec4 dst, vec3 col, float a) {
    dst.rgb = col * a + dst.rgb * (1.0 - a);
    dst.a   = a + dst.a * (1.0 - a);
}

float fill(float d) {
    return 1.0 - smoothstep(-0.8, 0.8, d);
}

void main() {
    vec2  p = vUV * uOuter;
    float dA, dB, dC;
    cursorShape(p, dA, dB, dC);
    float d    = min(dA, min(dB, dC));
    float edge = 1.0 - smoothstep(uOuter - 2.0, uOuter, length(p)); // everything reaches 0 before the quad edge

    vec4 c = vec4(0.0);

    // soft drop shadow first: barely visible on dark, separates the cursor from light backgrounds
    if (uShadow > 0.0 && uShadowSize > 0.0) {
        float s = max(d + uRadius * 0.3, 0.0) / uShadowSize;
        over(c, vec3(0.0), clamp(exp(-s * s * 2.0) * edge * uShadow, 0.0, 1.0));
    }

    // glow around the shape
    if (uGlow > 0.0 && uGlowSize > 0.0) {
        float g = max(d, 0.0) / uGlowSize;
        over(c, uColor, clamp(exp(-g * g * 2.5) * edge * uGlow * 0.75, 0.0, 1.0));
    }

    // optional dark edge so it stays readable on white
    if (uOutline > 0.0)
        over(c, vec3(0.0), (1.0 - smoothstep(0.0, 1.4, abs(d - 0.7) - 0.3)) * uOutline);

    vec3 colA = uColor;
    if (uStyle == 9)
        colA = mix(uCenterColor, uColor, smoothstep(uRadius * 0.1, uRadius, length(p)));

    over(c, colA, fill(dA));
    over(c, uCenterColor, fill(dB));
    over(c, uColor, fill(dC));

    fragColor = c;
}
)glsl";

// Second pass for the cursor: a hairline right outside the shape that inverts whatever is
// behind it (blend ONE_MINUS_DST_COLOR, ZERO), so there's always contrast, even on a
// background with the exact ring color. Same vertex shader as the circle.
inline constexpr const char* CURSORCONTRAST_FRAG = R"glsl(
uniform float uOuter;
uniform float uContrast;

in vec2 vUV;

out vec4 fragColor;

void main() {
    float d    = cursorSDF(vUV * uOuter);
    float edge = 1.0 - smoothstep(0.35, 1.15, abs(d - 1.1));
    float k    = edge * uContrast;
    // with blend (ONE_MINUS_DST_COLOR, ONE_MINUS_SRC_ALPHA): out = k*(1-dst) + (1-k)*dst
    fragColor = vec4(vec3(k), k);
}
)glsl";

// ───────────────────────── styled text cursor (I-beam) ──────────────────────
// Same look as the circle (colored rim, light core, glow, shadow) on an I-beam shape built
// from rounded boxes. vUV = px offset from the hotspot (center of the beam).

inline constexpr const char* CURSORTEXT_FRAG = R"glsl(#version 300 es
precision highp float;

uniform vec3  uColor;       // rim + glow
uniform vec3  uCenterColor; // core
uniform float uOuter;       // quad half size in px
uniform vec2  uBar;         // half size of the vertical bar
uniform vec2  uSerif;       // half size of the top/bottom serifs
uniform float uRing;        // rim thickness
uniform float uGlow;
uniform float uGlowSize;
uniform float uShadow;
uniform float uShadowSize;
uniform float uOutline;

in vec2 vUV;

out vec4 fragColor;

float sdBox(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

float smin(float a, float b, float k) {
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}

void over(inout vec4 dst, vec3 col, float a) {
    dst.rgb = col * a + dst.rgb * (1.0 - a);
    dst.a   = a + dst.a * (1.0 - a);
}

void main() {
    vec2  p   = vUV;
    float off = uBar.y - uSerif.y;

    float d = sdBox(p, uBar, min(uBar.x, uBar.y));
    d       = smin(d, sdBox(p - vec2(0.0, off), uSerif, uSerif.y), 1.2);
    d       = smin(d, sdBox(p + vec2(0.0, off), uSerif, uSerif.y), 1.2);

    float aa   = 0.7;
    float edge = 1.0 - smoothstep(uOuter - 2.0, uOuter, max(abs(p.x), abs(p.y)));
    vec4  c    = vec4(0.0);

    if (uShadow > 0.0 && uShadowSize > 0.0) {
        float s = max(d + 1.0, 0.0) / uShadowSize;
        over(c, vec3(0.0), clamp(exp(-s * s * 2.0) * uShadow * edge, 0.0, 1.0));
    }

    if (uGlow > 0.0 && uGlowSize > 0.0) {
        float g = max(d, 0.0) / uGlowSize;
        over(c, uColor, clamp(exp(-g * g * 2.5) * uGlow * 0.75 * edge, 0.0, 1.0));
    }

    if (uOutline > 0.0) {
        float o = 1.0 - smoothstep(0.0, 1.2, abs(d - 0.6) - 0.3);
        over(c, vec3(0.0), o * uOutline);
    }

    over(c, uColor, 1.0 - smoothstep(-aa, aa, d));                 // colored body
    over(c, uCenterColor, 1.0 - smoothstep(-aa, aa, d + uRing));   // light core

    fragColor = c;
}
)glsl";
