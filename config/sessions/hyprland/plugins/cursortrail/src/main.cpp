#include <bit>
#include <dlfcn.h>
#include <stdexcept>
#include <string>

#include <hyprland/src/includes.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include "compat.hpp"
#include <hyprland/src/desktop/view/WLSurface.hpp>
#include <hyprland/src/plugins/HookSystem.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/render/OpenGL.hpp>

#include "globals.hpp"
#include "Trail.hpp"

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

using namespace Config::Values;

// ───────────────────────────── config ───────────────────────────────────────

static void registerConfig() {
    // clang-format off
    g_config.enabled           = makeShared<CBoolValue>  ("plugin:cursortrail:enabled",            "enable the cursor trail", true);
    g_config.lifetime          = makeShared<CIntValue>   ("plugin:cursortrail:lifetime",           "trail lifetime in ms", 175, SIntValueOptions{.min = 10, .max = 2000});
    g_config.opacity           = makeShared<CFloatValue> ("plugin:cursortrail:opacity",            "trail opacity", 0.75F, SFloatValueOptions{.min = 0.F, .max = 1.F});
    g_config.blend             = makeShared<CStringValue>("plugin:cursortrail:blend",              "additive (glow) or normal", "additive");
    g_config.width             = makeShared<CFloatValue> ("plugin:cursortrail:width",              "core width in px", 8.F, SFloatValueOptions{.min = 0.5F, .max = 64.F});
    g_config.glow              = makeShared<CFloatValue> ("plugin:cursortrail:glow",               "glow strength, 0 = off", 1.0F, SFloatValueOptions{.min = 0.F, .max = 4.F});
    g_config.glowSize          = makeShared<CFloatValue> ("plugin:cursortrail:glow_size",          "glow width as a multiple of the core", 4.F, SFloatValueOptions{.min = 1.F, .max = 12.F});
    g_config.hotCore           = makeShared<CFloatValue> ("plugin:cursortrail:hot_core",           "how white-hot the core is", 0.5F, SFloatValueOptions{.min = 0.F, .max = 1.F});
    g_config.velocitySize      = makeShared<CBoolValue>  ("plugin:cursortrail:velocity_size",      "width reacts to cursor speed", true);
    g_config.velocityIntensity = makeShared<CFloatValue> ("plugin:cursortrail:velocity_intensity", "how much speed widens the trail", 0.15F, SFloatValueOptions{.min = 0.F, .max = 2.F});
    g_config.color             = makeShared<CColorValue> ("plugin:cursortrail:color",              "trail (head) color", 0xFFFF5800);
    g_config.gradient          = makeShared<CBoolValue>  ("plugin:cursortrail:gradient",           "fade to tail color", false);
    g_config.tailColor         = makeShared<CColorValue> ("plugin:cursortrail:tail_color",         "tail color for the gradient", 0xFFFFD84A);
    g_config.hideInFullscreen  = makeShared<CBoolValue>  ("plugin:cursortrail:hide_in_fullscreen", "don't draw over fullscreen windows", true);
    g_config.colorBoost        = makeShared<CFloatValue> ("plugin:cursortrail:color_boost",        "saturate trail/ring colors, keeps the hue", 0.F, SFloatValueOptions{.min = 0.F, .max = 1.F});

    g_config.cursorEnabled     = makeShared<CBoolValue>  ("plugin:cursortrail:cursor",             "replace the cursor with an osu!-style circle", true);
    g_config.debug             = makeShared<CBoolValue>  ("plugin:cursortrail:debug",              "show notifications about detected app cursors", false);
    g_config.cursorKeepLinger  = makeShared<CIntValue>   ("plugin:cursortrail:cursor_keep_linger", "ms without any cursor after a kept one (crosshair) ends, so the screenshot right after slurp is clean", 500, SIntValueOptions{.min = 0, .max = 5000});
    g_config.cursorKeep        = makeShared<CStringValue>("plugin:cursortrail:cursor_keep",        "cursor shapes that are never replaced, comma separated", "crosshair,cross,tcross,cross_reverse,diamond_cross");
    g_config.cursorKeepText    = makeShared<CBoolValue>  ("plugin:cursortrail:cursor_keep_text",   "keep the real text cursor (and pause the trail) over text", true);
    g_config.cursorTextDelay   = makeShared<CIntValue>   ("plugin:cursortrail:cursor_text_delay",  "ms over text before the I-beam / trail pause kicks in", 200, SIntValueOptions{.min = 0, .max = 5000});
    g_config.cursorTextStyle   = makeShared<CBoolValue>  ("plugin:cursortrail:cursor_text_style",  "draw the text cursor in the circle's style", true);
    g_config.cursorTextHeight  = makeShared<CFloatValue> ("plugin:cursortrail:cursor_text_height", "styled text cursor height in px", 20.F, SFloatValueOptions{.min = 6.F, .max = 96.F});
    g_config.cursorTextWidth   = makeShared<CFloatValue> ("plugin:cursortrail:cursor_text_width",  "styled text cursor bar thickness in px", 5.F, SFloatValueOptions{.min = 1.F, .max = 24.F});
    g_config.cursorInFullscreen = makeShared<CBoolValue> ("plugin:cursortrail:cursor_in_fullscreen", "circle also over fullscreen windows", true);
    g_config.cursorSize        = makeShared<CFloatValue> ("plugin:cursortrail:cursor_size",        "outer radius of the circle in px", 11.F, SFloatValueOptions{.min = 2.F, .max = 64.F});
    g_config.cursorRing        = makeShared<CFloatValue> ("plugin:cursortrail:cursor_ring",        "ring thickness in px", 4.F, SFloatValueOptions{.min = 0.5F, .max = 32.F});
    g_config.cursorGlow        = makeShared<CFloatValue> ("plugin:cursortrail:cursor_glow",        "glow outside the ring, 0 = off", 1.F, SFloatValueOptions{.min = 0.F, .max = 4.F});
    g_config.cursorStyle       = makeShared<CIntValue>   ("plugin:cursortrail:cursor_style",       "cursor shape, see the README for previews", 0, SIntValueOptions{.min = 0, .max = CURSOR_STYLE_COUNT - 1});
    g_config.cursorGlowSize    = makeShared<CFloatValue> ("plugin:cursortrail:cursor_glow_size",   "glow reach in px", 7.F, SFloatValueOptions{.min = 0.F, .max = 48.F});
    g_config.cursorOutline     = makeShared<CFloatValue> ("plugin:cursortrail:cursor_outline",     "dark edge so it reads on white, 0..1", 0.F, SFloatValueOptions{.min = 0.F, .max = 1.F});
    g_config.cursorShadow      = makeShared<CFloatValue> ("plugin:cursortrail:cursor_shadow",      "soft dark halo behind the circle, 0..1", 0.45F, SFloatValueOptions{.min = 0.F, .max = 1.F});
    g_config.cursorShadowSize  = makeShared<CFloatValue> ("plugin:cursortrail:cursor_shadow_size", "halo reach in px", 9.F, SFloatValueOptions{.min = 0.F, .max = 48.F});
    g_config.cursorContrast    = makeShared<CFloatValue> ("plugin:cursortrail:cursor_contrast",    "inverted hairline around the ring, 0..1", 0.F, SFloatValueOptions{.min = 0.F, .max = 1.F});
    g_config.cursorHoverScale  = makeShared<CFloatValue> ("plugin:cursortrail:cursor_hover_scale", "size over links / clickable things", 1.25F, SFloatValueOptions{.min = 0.25F, .max = 3.F});
    g_config.cursorClickScale  = makeShared<CFloatValue> ("plugin:cursortrail:cursor_click_scale", "expand factor while a mouse button is held", 1.2F, SFloatValueOptions{.min = 0.25F, .max = 3.F});
    g_config.cursorColor       = makeShared<CColorValue> ("plugin:cursortrail:cursor_color",       "ring / glow color", 0xFFFFE600);
    g_config.cursorCenterColor = makeShared<CColorValue> ("plugin:cursortrail:cursor_center_color","filled center color", 0xFFFFFFFF);
    // clang-format on

    for (const SP<IValue>& v : std::initializer_list<SP<IValue>>{
             g_config.enabled,       g_config.lifetime,      g_config.opacity,    g_config.blend,      g_config.width,           g_config.glow,
             g_config.glowSize,      g_config.hotCore,       g_config.velocitySize, g_config.velocityIntensity, g_config.color, g_config.gradient,
             g_config.tailColor,     g_config.hideInFullscreen, g_config.colorBoost, g_config.cursorEnabled, g_config.debug, g_config.cursorKeepLinger, g_config.cursorKeep, g_config.cursorKeepText, g_config.cursorTextDelay, g_config.cursorTextStyle, g_config.cursorTextHeight, g_config.cursorTextWidth, g_config.cursorInFullscreen, g_config.cursorStyle, g_config.cursorSize, g_config.cursorRing,
             g_config.cursorGlow,    g_config.cursorGlowSize, g_config.cursorOutline, g_config.cursorShadow, g_config.cursorShadowSize, g_config.cursorContrast, g_config.cursorHoverScale, g_config.cursorClickScale,
             g_config.cursorColor,   g_config.cursorCenterColor}) {
        HyprlandAPI::addConfigValueV2(PHANDLE, v);
    }
}

// ───────────────────── cursor shape tracking (function hooks) ────────────────
// Hyprland doesn't expose the current cursor shape, so we hook the two places that
// set it. Same approach as hypr-dynamic-cursors. Only used to decide whether the
// arrow is showing (circle) or e.g. the text/resize cursor (keep the real one).

// address of a non-virtual member function (Itanium ABI)
template <typename T>
static void* pmfAddress(T pmf) {
    struct SPMF {
        uintptr_t ptr;
        ptrdiff_t adj;
    };
    static_assert(std::is_member_function_pointer_v<T>);
    static_assert(sizeof(T) == sizeof(SPMF));
    const auto repr = std::bit_cast<SPMF>(pmf);
    if (repr.ptr & 1) // virtual → vtable offset, not an address
        return nullptr;
    return rc<void*>(repr.ptr);
}

static CFunctionHook* hookChecked(void* target, const char* expectedSymbol, void* handler) {
    if (!target)
        return nullptr;

#ifdef __GLIBCXX__
    // make sure the function still has the signature we were compiled for
    Dl_info info = {};
    if (!dladdr(target, &info) || !info.dli_sname || std::string_view{info.dli_sname} != expectedSymbol)
        return nullptr;
#endif

    auto* hook = HyprlandAPI::createFunctionHook(PHANDLE, target, handler);
    if (!hook || !hook->hook()) {
        if (hook)
            HyprlandAPI::removeFunctionHook(PHANDLE, hook);
        return nullptr;
    }
    return hook;
}

using FSetCursorFromName = void (*)(void*, const std::string&);
using FSetCursorSurface  = void (*)(void*, SP<Desktop::View::CWLSurface>, const Vector2D&);

static CFunctionHook* g_setCursorFromNameHook = nullptr;
static CFunctionHook* g_setCursorSurfaceHook  = nullptr;
static bool           g_shapeHooksOk          = false;

static void hkSetCursorFromName(void* thisptr, const std::string& name) {
    if (g_trail)
        g_trail->onCursorShape(name);
    rc<FSetCursorFromName>(g_setCursorFromNameHook->m_original)(thisptr, name);
}

static void hkSetCursorSurface(void* thisptr, SP<Desktop::View::CWLSurface> surf, const Vector2D& hotspot) {
    if (g_trail)
        g_trail->onCursorClientSurface(surf && surf->resource()); // no resource = app hides the cursor
    rc<FSetCursorSurface>(g_setCursorSurfaceHook->m_original)(thisptr, surf, hotspot);
}

static void installShapeHooks() {
    g_setCursorFromNameHook = hookChecked(pmfAddress(&CTCompat::CursorManager::setCursorFromName),
                                          CTCompat::SET_CURSOR_FROM_NAME_SYM, rc<void*>(&hkSetCursorFromName));
    g_setCursorSurfaceHook  = hookChecked(pmfAddress(&CTCompat::CursorManager::setCursorSurface),
                                          CTCompat::SET_CURSOR_SURFACE_SYM,
                                          rc<void*>(&hkSetCursorSurface));

    g_shapeHooksOk = g_setCursorFromNameHook && g_setCursorSurfaceHook;
    if (!g_shapeHooksOk) {
        if (g_setCursorFromNameHook)
            HyprlandAPI::removeFunctionHook(PHANDLE, g_setCursorFromNameHook);
        if (g_setCursorSurfaceHook)
            HyprlandAPI::removeFunctionHook(PHANDLE, g_setCursorSurfaceHook);
        g_setCursorFromNameHook = g_setCursorSurfaceHook = nullptr;

        // without shape info the circle simply replaces every cursor, text cursor included
        HyprlandAPI::addNotification(PHANDLE, "[cursortrail] couldn't track cursor shapes: circle replaces the text cursor too, no link highlight",
                                     CHyprColor{1.0, 0.7, 0.2, 1.0}, 8000);
    }
}

// ───────────────────────────── lifecycle ────────────────────────────────────

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH        = __hyprland_api_get_hash();
    const std::string CLIENT_HASH = __hyprland_api_get_client_hash();

    if (HASH != CLIENT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[cursortrail] Version mismatch: plugin was built against a different Hyprland. Rebuild it.", CHyprColor{1.0, 0.2, 0.2, 1.0},
                                     8000);
        throw std::runtime_error("[cursortrail] version mismatch");
    }

    registerConfig();

    g_trail = makeUnique<CCursorTrail>();
    g_trail->preloadKeptImages(); // read the cursor_keep shapes from the theme now, not mid-frame
    installShapeHooks();

    static auto onMove = Event::bus()->m_events.input.mouse.move.listen([](Vector2D, Event::SCallbackInfo&) {
        if (g_trail)
            g_trail->onPointerMoved();
    });

    static auto onButton = Event::bus()->m_events.input.mouse.button.listen([](IPointer::SButtonEvent e, Event::SCallbackInfo&) {
        if (g_trail)
            g_trail->onPointerButton(e.state == WL_POINTER_BUTTON_STATE_PRESSED);
    });

    static auto onStage = Event::bus()->m_events.render.stage.listen([](eRenderStage stage) {
        if (!g_trail)
            return;

        const auto monitor = g_pHyprRenderer->m_renderData.pMonitor.lock();
        if (!monitor)
            return;

        if (stage == RENDER_LAST_MOMENT)
            g_trail->onRenderLastMoment(monitor);
        else if (stage == RENDER_POST)
            g_trail->onRenderPost(monitor);
    });

    // values exist now → re-run the config so plugin.cursortrail.* gets applied
    HyprlandAPI::reloadConfig();

    return {"cursortrail", "osu!-style glowing cursor trail and circle cursor", "mahiru", "1.0.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    if (g_setCursorFromNameHook)
        HyprlandAPI::removeFunctionHook(PHANDLE, g_setCursorFromNameHook);
    if (g_setCursorSurfaceHook)
        HyprlandAPI::removeFunctionHook(PHANDLE, g_setCursorSurfaceHook);
    g_setCursorFromNameHook = g_setCursorSurfaceHook = nullptr;

    if (g_pHyprRenderer)
        g_pHyprRenderer->m_renderPass.removeAllOfType("CCursorTrailPassElement");

    if (g_trail) {
        if (Render::GL::g_pHyprOpenGL)
            Render::GL::g_pHyprOpenGL->makeEGLCurrent();
        g_trail->destroyGL();
        g_trail.reset();
    }
}
