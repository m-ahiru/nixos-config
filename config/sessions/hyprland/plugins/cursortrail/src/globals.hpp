#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/config/values/types/BoolValue.hpp>
#include <hyprland/src/config/values/types/ColorValue.hpp>
#include <hyprland/src/config/values/types/FloatValue.hpp>
#include <hyprland/src/config/values/types/IntValue.hpp>
#include <hyprland/src/config/values/types/StringValue.hpp>

inline HANDLE PHANDLE = nullptr;

// number of cursor_style shapes (CURSORSHAPE_LIB in Shaders.hpp)
inline constexpr int CURSOR_STYLE_COUNT = 16;

// All values live under plugin.cursortrail.* (Lua) / plugin:cursortrail:* (hyprlang).
struct SCursorTrailConfig {
    SP<Config::Values::CBoolValue>   enabled;
    SP<Config::Values::CIntValue>    lifetime;           // ms
    SP<Config::Values::CFloatValue>  opacity;            // 0..1
    SP<Config::Values::CStringValue> blend;              // "additive" | "normal"
    SP<Config::Values::CFloatValue>  width;              // core width, logical px
    SP<Config::Values::CFloatValue>  glow;               // glow strength, 0 = off
    SP<Config::Values::CFloatValue>  glowSize;           // glow width as multiple of core width
    SP<Config::Values::CFloatValue>  hotCore;            // how much the core is pushed towards white
    SP<Config::Values::CBoolValue>   velocitySize;       // velocity-reactive width
    SP<Config::Values::CFloatValue>  velocityIntensity;  // how strongly speed widens the trail
    SP<Config::Values::CColorValue>  color;              // head color
    SP<Config::Values::CBoolValue>   gradient;           // fade to tail color
    SP<Config::Values::CColorValue>  tailColor;          // tail color
    SP<Config::Values::CBoolValue>   hideInFullscreen;   // don't draw over fullscreen windows
    SP<Config::Values::CFloatValue>  colorBoost;         // 0..1 push trail/ring colors towards full saturation (matugen pastels)

    // osu!-style circle cursor (needs software cursors: cursor.no_hardware_cursors = 1)
    // Replaces every cursor shape except the text cursor (there the trail pauses too).
    SP<Config::Values::CBoolValue>   cursorEnabled;
    SP<Config::Values::CBoolValue>   debug;              // notifications about what the plugin detects
    SP<Config::Values::CIntValue>    cursorKeepLinger;   // ms without a cursor after a kept one (crosshair) ends
    SP<Config::Values::CStringValue> cursorKeep;         // theme shapes that are never replaced (crosshair, ...)
    SP<Config::Values::CBoolValue>   cursorKeepText;     // keep the real I-beam (and pause the trail) over text
    SP<Config::Values::CIntValue>    cursorTextDelay;    // ms over text before that kicks in (quick passes don't count)
    SP<Config::Values::CBoolValue>   cursorTextStyle;    // draw the I-beam in the circle's style instead of the theme's
    SP<Config::Values::CFloatValue>  cursorTextHeight;   // I-beam height, logical px
    SP<Config::Values::CFloatValue>  cursorTextWidth;    // I-beam bar thickness, logical px
    SP<Config::Values::CBoolValue>   cursorInFullscreen; // circle also over fullscreen windows (trail stays hidden there)
    SP<Config::Values::CIntValue>    cursorStyle;        // shape, 0 .. CURSOR_STYLE_COUNT-1 (see README)
    SP<Config::Values::CFloatValue>  cursorSize;         // outer radius of the ring, logical px
    SP<Config::Values::CFloatValue>  cursorRing;         // ring thickness, logical px
    SP<Config::Values::CFloatValue>  cursorGlow;         // glow strength outside the ring
    SP<Config::Values::CFloatValue>  cursorGlowSize;     // glow reach, logical px
    SP<Config::Values::CFloatValue>  cursorOutline;      // dark edge for light backgrounds, 0..1
    SP<Config::Values::CFloatValue>  cursorShadow;       // soft dark halo behind the circle, 0..1
    SP<Config::Values::CFloatValue>  cursorShadowSize;   // halo reach, logical px
    SP<Config::Values::CFloatValue>  cursorContrast;     // inverted hairline around the ring, 0..1
    SP<Config::Values::CFloatValue>  cursorHoverScale;   // size over links / clickable things (pointer shape)
    SP<Config::Values::CFloatValue>  cursorClickScale;   // expand factor while a button is held
    SP<Config::Values::CColorValue>  cursorColor;        // ring / glow color
    SP<Config::Values::CColorValue>  cursorCenterColor;  // filled center
};

inline SCursorTrailConfig g_config;
