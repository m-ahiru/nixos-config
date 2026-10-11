#pragma once

#include <hyprland/src/render/pass/PassElement.hpp>

// One element type for both the trail and the circle cursor, so PLUGIN_EXIT can
// remove everything with a single removeAllOfType().
class CCursorTrailPassElement : public IPassElement {
  public:
    enum eKind : uint8_t {
        KIND_TRAIL = 0,
        KIND_CIRCLE,
        KIND_TEXT,
    };

    explicit CCursorTrailPassElement(eKind kind) : m_kind(kind) {}
    virtual ~CCursorTrailPassElement() = default;

    virtual std::vector<UP<IPassElement>> draw() override;
    virtual bool                          needsLiveBlur() override {
        return false;
    }
    virtual bool needsPrecomputeBlur() override {
        return false;
    }
    virtual const char* passName() override {
        return "CCursorTrailPassElement";
    }
    virtual ePassElementType type() override {
        return EK_CUSTOM;
    }
    // no bounding box / opaque region: we scissor to the damage ourselves

  private:
    eKind m_kind;
};
