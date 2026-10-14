#include "CursorTheme.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

namespace fs = std::filesystem;

namespace {
    constexpr uint32_t XCURSOR_MAGIC      = 0x72756358; // "Xcur"
    constexpr uint32_t XCURSOR_IMAGE_TYPE = 0xfffd0002;
    constexpr int      MAX_DIM            = 512;
    constexpr int      CLOSE_CHANNEL_DIFF = 8;  // per channel, for "same image" with rounding noise
    constexpr int      CLOSE_SHARE        = 98; // % of pixels that must be that close

    std::string home() {
        const char* h = std::getenv("HOME");
        return h ? h : "";
    }

    std::string expand(const std::string& p) {
        return !p.empty() && p[0] == '~' ? home() + p.substr(1) : p;
    }

    // XCURSOR_PATH, or libXcursor's default list plus the XDG data dirs (that's where NixOS keeps them)
    std::vector<std::string> searchPath() {
        std::vector<std::string> out;
        auto                     split = [&](const std::string& s, const std::string& suffix) {
            std::stringstream ss(s);
            std::string       part;
            while (std::getline(ss, part, ':'))
                if (!part.empty())
                    out.push_back(expand(part) + suffix);
        };

        if (const char* p = std::getenv("XCURSOR_PATH"); p && *p) {
            split(p, "");
            return out;
        }
        const char* dataHome = std::getenv("XDG_DATA_HOME");
        out.push_back(dataHome && *dataHome ? std::string{dataHome} + "/icons" : home() + "/.local/share/icons");
        out.push_back(home() + "/.icons");
        const char* dataDirs = std::getenv("XDG_DATA_DIRS");
        split(dataDirs && *dataDirs ? dataDirs : "/usr/local/share:/usr/share", "/icons");
        out.push_back("/usr/share/pixmaps");
        return out;
    }

    std::vector<std::string> inherits(const std::string& indexTheme) {
        std::ifstream            in(indexTheme);
        std::string              line;
        std::vector<std::string> out;
        while (std::getline(in, line)) {
            const auto start = line.find_first_not_of(" \t");
            if (start == std::string::npos || line.compare(start, 8, "Inherits") != 0)
                continue;
            const auto eq = line.find('=', start);
            if (eq == std::string::npos)
                continue;
            std::stringstream ss(line.substr(eq + 1));
            std::string       t;
            while (std::getline(ss, t, ',')) {
                t.erase(0, t.find_first_not_of(" \t\r"));
                t.erase(t.find_last_not_of(" \t\r") + 1);
                if (!t.empty())
                    out.push_back(t);
            }
        }
        return out;
    }
}

uint64_t CCursorThemeImages::hash(const uint8_t* rgba, int w, int h) {
    uint64_t x = 1469598103934665603ULL; // FNV-1a
    auto     mix = [&](uint8_t b) {
        x ^= b;
        x *= 1099511628211ULL;
    };
    for (int i = 0; i < 4; ++i)
        mix(static_cast<uint8_t>(w >> (i * 8))), mix(static_cast<uint8_t>(h >> (i * 8)));
    for (size_t i = 0; i < static_cast<size_t>(w) * h * 4; ++i)
        mix(rgba[i]);
    return x;
}

void CCursorThemeImages::refresh(const std::vector<std::string>& shapes) {
    const char* themeEnv = std::getenv("XCURSOR_THEME");
    const char* pathEnv  = std::getenv("XCURSOR_PATH");
    std::string theme    = themeEnv && *themeEnv ? themeEnv : "default";
    std::string key      = theme + '\n' + (pathEnv ? pathEnv : "");
    for (const auto& s : shapes)
        key += '\n' + s;
    if (key == m_key)
        return;
    m_key = std::move(key);
    load(theme, searchPath(), shapes);
}

bool CCursorThemeImages::empty() const {
    return m_images.empty();
}

void CCursorThemeImages::load(const std::string& theme, const std::vector<std::string>& dirs, const std::vector<std::string>& shapes) {
    m_images.clear();
    m_hashes.clear();

    // the theme and everything it inherits from (breadth first, each theme once)
    std::vector<std::string> todo = {theme};
    std::set<std::string>    seenThemes, seenFiles;
    for (size_t t = 0; t < todo.size() && t < 16; ++t) {
        if (!seenThemes.insert(todo[t]).second)
            continue;
        for (const auto& dir : dirs) {
            std::error_code ec;
            const fs::path  base = fs::path{dir} / todo[t];
            if (fs::is_directory(base / "cursors", ec)) {
                for (const auto& shape : shapes) {
                    if (shape.empty() || shape.find('/') != std::string::npos)
                        continue;
                    const auto real = fs::weakly_canonical(base / "cursors" / shape, ec).string();
                    if (!ec && fs::is_regular_file(real, ec) && seenFiles.insert(real).second)
                        loadFile(real);
                }
            }
            if (fs::exists(base / "index.theme", ec))
                for (auto& parent : inherits((base / "index.theme").string()))
                    todo.push_back(parent);
        }
    }
}

void CCursorThemeImages::loadFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return;
    std::vector<uint8_t> d((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    auto                 u32 = [&](size_t off) -> uint32_t {
        if (off + 4 > d.size())
            return 0;
        return d[off] | (d[off + 1] << 8) | (d[off + 2] << 16) | (static_cast<uint32_t>(d[off + 3]) << 24);
    };
    if (d.size() < 16 || u32(0) != XCURSOR_MAGIC)
        return;

    const uint32_t ntoc = std::min<uint32_t>(u32(12), 4096);
    for (uint32_t i = 0; i < ntoc; ++i) {
        const size_t toc = 16 + static_cast<size_t>(i) * 12;
        if (u32(toc) != XCURSOR_IMAGE_TYPE)
            continue;
        const size_t pos = u32(toc + 8);
        // chunk: header, type, subtype, version, width, height, xhot, yhot, delay, pixels
        const int w = static_cast<int>(u32(pos + 16)), h = static_cast<int>(u32(pos + 20));
        if (w <= 0 || h <= 0 || w > MAX_DIM || h > MAX_DIM || pos + 36 + static_cast<size_t>(w) * h * 4 > d.size())
            continue;

        SImage img{w, h, std::vector<uint8_t>(static_cast<size_t>(w) * h * 4)};
        for (size_t p = 0; p < static_cast<size_t>(w) * h; ++p) { // ARGB u32 little endian = B,G,R,A bytes
            const uint8_t* src = &d[pos + 36 + p * 4];
            img.rgba[p * 4 + 0] = src[2];
            img.rgba[p * 4 + 1] = src[1];
            img.rgba[p * 4 + 2] = src[0];
            img.rgba[p * 4 + 3] = src[3];
        }
        m_hashes.insert(hash(img.rgba.data(), w, h));
        m_images.push_back(std::move(img));
    }
}

bool CCursorThemeImages::contains(const uint8_t* rgba, int w, int h) const {
    if (m_hashes.contains(hash(rgba, w, h)))
        return true;

    // not bit-identical (rounding, a different premultiply): accept a near match of the same size
    const size_t n = static_cast<size_t>(w) * h;
    for (const auto& img : m_images) {
        if (img.w != w || img.h != h)
            continue;
        size_t close = 0;
        for (size_t p = 0; p < n; ++p) {
            bool ok = true;
            for (int c = 0; c < 4 && ok; ++c)
                ok = std::abs(int{img.rgba[p * 4 + c]} - int{rgba[p * 4 + c]}) <= CLOSE_CHANNEL_DIFF;
            close += ok;
        }
        if (close * 100 >= n * CLOSE_SHARE)
            return true;
    }
    return false;
}
