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

uniform vec4  uPts[MAX_POINTS]; // x, y (monitor px), radius (px), age (0 head .. 1 tail)
uniform int   uCount;
uniform vec3  uHeadColor;
uniform vec3  uTailColor;
uniform float uOpacity;
uniform float uGlow;     // glow strength (0 = none)
uniform float uCoreFrac; // core width / full width
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
        float d  = length(vUV - (P0.xy + AB * h)) / max(r, 1e-3); // 0 spine .. 1 glow edge

        if (d >= 1.0)
            continue;

        float age  = mix(P0.w, P1.w, h);
        float life = clamp(1.0 - age, 0.0, 1.0);

        float core = 1.0 - smoothstep(uCoreFrac * 0.45, uCoreFrac, d);

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

inline constexpr const char* CURSORCIRCLE_FRAG = R"glsl(#version 300 es
precision highp float;

// solid osu!-style circle: filled center, thick ring, soft glow outside
uniform vec3  uColor;       // ring + glow
uniform vec3  uCenterColor; // filled center
uniform float uOuter;       // quad half size in px
uniform float uRadius;      // outer edge of the ring in px
uniform float uRing;        // ring thickness in px
uniform float uGlow;        // glow strength
uniform float uGlowSize;    // glow reach in px
uniform float uOutline;     // dark edge opacity
uniform float uShadow;      // soft dark halo behind everything (contrast on light backgrounds)
uniform float uShadowSize;  // its reach in px
uniform float uContrast;    // 0..1: inverted hairline around the ring, visible on any background

in vec2 vUV;

out vec4 fragColor;

// premultiplied "over"
void over(inout vec4 dst, vec3 col, float a) {
    dst.rgb = col * a + dst.rgb * (1.0 - a);
    dst.a   = a + dst.a * (1.0 - a);
}

void main() {
    float r     = length(vUV) * uOuter;
    float aa    = 0.8;
    float inner = uRadius - uRing;

    vec4 c = vec4(0.0);

    // soft drop shadow first: barely visible on dark, separates the cursor from light backgrounds
    if (uShadow > 0.0 && uShadowSize > 0.0) {
        float s      = max(r - uRadius * 0.7, 0.0) / uShadowSize;
        float shadow = exp(-s * s * 2.0) * (1.0 - smoothstep(uOuter - 2.0, uOuter, r));
        over(c, vec3(0.0), clamp(shadow * uShadow, 0.0, 1.0));
    }

    // glow outside the ring, reaches exactly 0 before the quad edge
    if (uGlow > 0.0 && uGlowSize > 0.0) {
        float g    = max(r - uRadius, 0.0) / uGlowSize;
        float glow = exp(-g * g * 2.5) * (1.0 - smoothstep(uOuter - 2.0, uOuter, r));
        over(c, uColor, clamp(glow * uGlow * 0.75, 0.0, 1.0));
    }

    // optional dark edge so it stays readable on white
    if (uOutline > 0.0) {
        float o = 1.0 - smoothstep(0.0, 1.4, abs(r - (uRadius + 0.7)) - 0.3);
        over(c, vec3(0.0), o * uOutline);
    }

    // solid disc = ring color, then the solid center on top
    float disc = 1.0 - smoothstep(uRadius - aa, uRadius + aa, r);
    over(c, uColor, disc);

    float center = 1.0 - smoothstep(inner - aa, inner + aa, r);
    over(c, uCenterColor, center);

    fragColor = c;
}
)glsl";

// Second pass for the circle: a hairline right outside the ring that inverts whatever is
// behind it (blend ONE_MINUS_DST_COLOR, ZERO), so there's always contrast, even on a
// background with the exact ring color. Same vertex shader as the circle.
inline constexpr const char* CURSORCONTRAST_FRAG = R"glsl(#version 300 es
precision highp float;

uniform float uOuter;
uniform float uRadius;
uniform float uContrast;

in vec2 vUV;

out vec4 fragColor;

void main() {
    float r    = length(vUV) * uOuter;
    float edge = 1.0 - smoothstep(0.35, 1.15, abs(r - (uRadius + 1.1)));
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
