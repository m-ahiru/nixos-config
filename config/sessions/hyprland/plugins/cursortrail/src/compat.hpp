#pragma once

// Small layer over the parts of Hyprland's internals that moved between versions.
// 0.56 moved pointer/cursor/monitor code into namespaces and replaced a few globals
// with accessors. Everything version-specific lives here.

#if __has_include(<hyprland/src/pointer/PointerManager.hpp>)
#define CT_HYPRLAND_056 1
#endif

#ifdef CT_HYPRLAND_056
#include <hyprland/src/output/Monitor.hpp>
#include <hyprland/src/pointer/PointerManager.hpp>
#include <hyprland/src/pointer/cursor/CursorManager.hpp>
#include <hyprland/src/state/MonitorState.hpp>
#include <hyprland/src/managers/fullscreen/FullscreenController.hpp>
#else
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/managers/PointerManager.hpp>
#include <hyprland/src/managers/CursorManager.hpp>
#endif
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/Workspace.hpp>
#include <hyprland/src/desktop/view/WLSurface.hpp>
#include <hyprland/src/protocols/core/Compositor.hpp>
#include <hyprland/src/protocols/core/Seat.hpp>

namespace CTCompat {

#ifdef CT_HYPRLAND_056
    using CursorManager = Pointer::Cursor::CCursorManager;

    // mangled names, checked against the hooked symbol so we never hook the wrong function
    inline constexpr const char* SET_CURSOR_FROM_NAME_SYM =
        "_ZN7Pointer6Cursor14CCursorManager17setCursorFromNameERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE";
    inline constexpr const char* SET_CURSOR_SURFACE_SYM =
        "_ZN7Pointer6Cursor14CCursorManager16setCursorSurfaceEN9Hyprutils6Memory14CSharedPointerIN7Desktop4View10CWLSurfaceEEERKNS2_4Math8Vector2DE";

    inline Pointer::CPointerManager* pointer() {
        return Pointer::mgr().get();
    }

    inline PHLMONITOR monitorAt(const Vector2D& pos) {
        return State::monitorState() ? State::monitorState()->query().vec(pos).run() : nullptr;
    }

    inline bool hasFullscreen(const PHLWORKSPACE& ws) {
        return ws && Fullscreen::controller() && Fullscreen::controller()->hasFullscreen(ws);
    }
#else
    using CursorManager = CCursorManager;

    inline constexpr const char* SET_CURSOR_FROM_NAME_SYM =
        "_ZN14CCursorManager17setCursorFromNameERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE";
    inline constexpr const char* SET_CURSOR_SURFACE_SYM =
        "_ZN14CCursorManager16setCursorSurfaceEN9Hyprutils6Memory14CSharedPointerIN7Desktop4View10CWLSurfaceEEERKNS0_4Math8Vector2DE";

    inline CPointerManager* pointer() {
        return g_pPointerManager.get();
    }

    inline PHLMONITOR monitorAt(const Vector2D& pos) {
        return g_pCompositor ? g_pCompositor->getMonitorFromVector(pos) : nullptr;
    }

    inline bool hasFullscreen(const PHLWORKSPACE& ws) {
        return ws && ws->m_hasFullscreenWindow;
    }
#endif

}
