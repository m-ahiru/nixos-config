#include "TrailPassElement.hpp"
#include "Trail.hpp"

#include <hyprland/src/render/Renderer.hpp>

std::vector<UP<IPassElement>> CCursorTrailPassElement::draw() {
    if (!g_trail)
        return {};

    const auto monitor = g_pHyprRenderer->m_renderData.pMonitor.lock();
    switch (m_kind) {
        case KIND_TRAIL: g_trail->drawTrail(monitor); break;
        case KIND_CIRCLE: g_trail->drawCursor(monitor); break;
        case KIND_TEXT: g_trail->drawTextCursor(monitor); break;
    }
    return {};
}
