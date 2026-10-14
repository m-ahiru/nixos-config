#pragma once

#include <chrono>
#include <deque>
#include <optional>
#include <string>
#include <vector>

#include <GLES3/gl32.h>

#include <hyprland/src/helpers/math/Math.hpp>
#include <hyprland/src/helpers/memory/Memory.hpp>
#include <hyprland/src/desktop/DesktopTypes.hpp>

#include "CursorTheme.hpp"

namespace Render {
    class ITexture;
}

using TrailClock = std::chrono::steady_clock;

// Draws the glowing trail and (optionally) replaces the arrow cursor with an osu!-style circle.
class CCursorTrail {
  public:
    CCursorTrail()  = default;
    ~CCursorTrail() = default;

    // input side
    void onPointerMoved();
    void onPointerButton(bool pressed);
    void onCursorShape(const std::string& shape); // theme cursor by name (left_ptr, text, ...)
    void onCursorClientSurface(bool visible);     // app supplied its own cursor image (or hid it)
    void preloadKeptImages();                     // read the cursor_keep shapes from the theme now

    // render side (render-stage listener, GL context is current)
    void onRenderLastMoment(PHLMONITOR monitor);
    void onRenderPost(PHLMONITOR monitor);

    // called by the pass elements
    void drawTrail(PHLMONITOR monitor);
    void drawCursor(PHLMONITOR monitor);
    void drawTextCursor(PHLMONITOR monitor);

    // applies the damage scheduled by queueDamage()
    void flushDamage();

    // releases GL objects, must be called with a current GL context (plugin exit)
    void destroyGL();

    void clear();

  private:
    struct SSample {
        Vector2D               pos;   // global layout coordinates
        TrailClock::time_point time;
        float                  speed; // smoothed px/s at this sample
    };

    struct SPoint {
        Vector2D pos;
        float    t;     // normalized age, 0 = head, 1 = end of life
        float    speed; // smoothed px/s
    };

    struct SVertex {
        float x, y; // monitor-local pixel coords
        float a, b; // trail: age / side   circle: local uv
    };

    struct SProgram {
        GLuint                     id = 0;
        std::vector<GLint>         loc;
        GLint                      operator[](size_t i) const {
            return loc[i];
        }
    };

    // trail state
    std::deque<SSample> m_samples;
    float               m_smoothedSpeed = 0.F;
    std::vector<SPoint> m_points;
    bool                m_hadTrail = false;

    // cursor state
    enum eShape : uint8_t {
        SHAPE_NORMAL = 0, // arrow, resize, move, app-supplied images, ...
        SHAPE_LINK,       // pointer / hand: something clickable → circle grows a bit
        SHAPE_TEXT,       // I-beam: real cursor stays, trail pauses
        SHAPE_KEEP,       // cursor_keep (crosshair, ...): real cursor stays right away, trail pauses
    };
    eShape                 m_shape = SHAPE_NORMAL;
    bool                   m_appHidCursor    = false; // app set an empty cursor (games while playing)
    TrailClock::time_point m_textSince{};             // when the cursor turned into the I-beam
    bool                   m_noCursorDrawn   = false; // last frame on the pointer's monitor had no cursor
    int                    m_buttonsDown   = 0;
    float                  m_clickScale    = 1.F;
    TrailClock::time_point m_lastAnimTick{};
    bool                   m_cursorDrawnThisFrame = false;

    // apps that send a theme cursor as an image (slurp's crosshair): compare it with the
    // cursor_keep shapes of the theme, once per new image
    bool                   m_clientCursor   = false; // current cursor is an image from the app
    bool                   m_analyzePending = false;
    void*                  m_analyzedTex    = nullptr;
    CCursorThemeImages     m_keepImages;
    GLuint                 m_readFbo = 0, m_readTex = 0, m_copyProg = 0;
    Vector2D               m_readTexSize;
    std::vector<uint8_t>   m_readScratch;

    // damage tracking (global logical coords)
    CBox m_trailBox, m_cursorBox, m_lastDamaged;
    bool m_damageQueued = false;

    // GL
    SProgram           m_trailProg, m_circleProg, m_contrastProg, m_textProg;
    GLuint             m_vao = 0, m_vbo = 0; // one quad buffer for everything
    std::vector<float> m_ptsScratch;         // trail points for the uniform array
    std::vector<float> m_coreScratch;        // core radius per trail point
    bool                 m_glFailed = false;
    std::vector<SVertex> m_vertexScratch;

    void                 prune(TrailClock::time_point now);
    void                 buildPoints(TrailClock::time_point now);
    void                 updateCursorBox();
    void                 tickClickAnimation();
    bool                 clickAnimating() const;
    float                targetScale() const;
    bool                 textMode() const;
    bool                 textPending() const; // over text, dwell delay not reached yet
    bool                 cursorHidden() const; // app hid it, Hyprland hid it, or no cursor was drawn
    float                halfWidthAt(const SPoint& p) const;
    float                glowFactor() const;
    float                cursorOuterRadius() const; // incl. glow & max click scale, logical px
    bool                 fullscreenOn(PHLMONITOR monitor) const;
    std::vector<std::string> keptShapes() const;   // cursor_keep, split
    void                 analyzeClientCursor();     // client image = one of the kept shapes?
    bool                 matchesKeptImage(const SP<Render::ITexture>& tex);
    bool                 wantCircle(PHLMONITOR monitor) const;
    bool                 wantStyledText(PHLMONITOR monitor) const;
    bool                 ensureGL();
    void                 uploadAndDraw(PHLMONITOR monitor, const SProgram& prog, GLenum mode);
    void                 queueDamage();
};

inline UP<CCursorTrail> g_trail;
