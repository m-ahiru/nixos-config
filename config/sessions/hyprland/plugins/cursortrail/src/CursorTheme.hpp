#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

// All images (every size) of some shapes of the active XCursor theme. Some apps don't ask for
// a shape by name but send the theme's image themselves (slurp sends the crosshair that way);
// comparing against these images still tells us which shape it is.
class CCursorThemeImages {
  public:
    // (re)loads when the shape list, XCURSOR_THEME or XCURSOR_PATH changed; cheap otherwise
    void refresh(const std::vector<std::string>& shapes);

    // rgba: w*h pixels, 4 bytes each in R,G,B,A order, premultiplied, rows top to bottom
    bool contains(const uint8_t* rgba, int w, int h) const;
    bool empty() const;

  private:
    struct SImage {
        int                  w = 0, h = 0;
        std::vector<uint8_t> rgba;
    };

    std::string                  m_key; // theme + search path + shapes we loaded for
    std::vector<SImage>          m_images;
    std::unordered_set<uint64_t> m_hashes;

    void        load(const std::string& theme, const std::vector<std::string>& searchPath, const std::vector<std::string>& shapes);
    void        loadFile(const std::string& path);
    static uint64_t hash(const uint8_t* rgba, int w, int h);
};
