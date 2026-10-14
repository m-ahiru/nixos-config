-- ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
--  ◈ CURSOR TRAIL (plugin: plugins/cursortrail)
-- ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
-- Edits here apply on save. Colors take any Hyprland color string: "rgb(ff5800)", "rgba(ff5800ff)", ...

-- Store path of the plugin, written by Home Manager (~/.config/hypr/nix-plugins.lua)
local ok, plugins = pcall(require, "nix-plugins")
if ok and type(plugins) == "table" and plugins.cursortrail then
    hl.plugin.load(plugins.cursortrail)
end

-- The plugin registers its options when it loads, which happens right after this
-- file is parsed the first time. Only set them once they exist, so the first
-- parse doesn't throw "unknown config key".
if hl.get_config("plugin.cursortrail.enabled") == nil then
    return
end

-- Colors follow the wallpaper via matugen (~/.config/hypr/colors.lua), with fallbacks
-- until matugen has run once.
local okc, colors = pcall(require, "colors")
if not okc or type(colors) ~= "table" then colors = {} end
local c = {
    primary           = colors.primary           or "rgb(ffb59a)",
    secondary         = colors.secondary         or "rgb(e7bdb0)",
    tertiary          = colors.tertiary          or "rgb(d4c58f)",
    primary_container = colors.primary_container or "rgb(7a2f12)",
}

-- How circle and trail relate, all from the same wallpaper palette:
--   "kontrast": circle = primary, trail = tertiary → secondary   (different hues, still matching)
--   "analog":   circle = primary, trail = secondary → primary_container (same hue family, softer)
--   "mono":     everything primary
local SCHEME = "kontrast"

local schemes = {
    kontrast = { cursor = c.primary, head = c.tertiary,  tail = c.secondary },
    analog   = { cursor = c.primary, head = c.secondary, tail = c.primary_container },
    mono     = { cursor = c.primary, head = c.primary,   tail = c.primary },
}
local pal = schemes[SCHEME] or schemes.kontrast

hl.config({
    plugin = {
        cursortrail = {
            -- ── trail ──
            enabled            = true,
            lifetime           = 175,          -- ms
            opacity            = 0.75,
            blend              = "additive",   -- "additive" (glow) | "normal" (also visible on white)

            width              = 15,            -- core width in px
            glow               = 1.0,          -- glow strength, 0 = off
            glow_size          = 4.0,          -- glow width as multiple of the core
            hot_core           = 0.5,          -- 0..1, how white-hot the center burns

            velocity_size      = true,         -- faster = wider
            velocity_intensity = 0.15,

            color              = pal.head,
            gradient           = true,         -- fade to tail_color along the trail
            tail_color         = pal.tail,

            color_boost        = 0.6,          -- 0..1 matugen dark-mode colors are pastel; this saturates
                                               -- trail + ring (hue stays from the wallpaper), 0 = exact matugen colors
            hide_in_fullscreen = false,        -- true: no trail over fullscreen windows (games, videos)

            -- ── osu!-style circle cursor (needs cursor.no_hardware_cursors = 1, see settings.lua) ──
            -- replaces every cursor; over text the normal I-beam stays and the trail pauses
            cursor              = true,
            cursor_style        = 3,            -- shape 0..15, previews: github.com/m-ahiru/hypr-cursortrail#cursor-styles
            cursor_keep         = "crosshair,cross,tcross,cross_reverse,diamond_cross", -- these stay the real cursor (screenshots, ...), trail pauses
            cursor_keep_text    = true,         -- false: circle replaces the I-beam too (terminals show it everywhere)
            cursor_text_delay   = 200,          -- ms over text before I-beam + trail pause kick in; quick passes don't count
            cursor_text_style   = true,         -- I-beam in the circle's look (false: theme's normal I-beam)
            cursor_text_height  = 20,
            cursor_text_width   = 5,
            cursor_in_fullscreen = true,        -- circle also in fullscreen games/apps (trail stays off there)
            cursor_size         = 11,           -- outer radius in px
            cursor_ring         = 4,            -- ring thickness
            cursor_glow         = 0.4,          -- soft glow outside the ring, 0 = off
            cursor_glow_size    = 4,
            -- visibility on any background:
            cursor_shadow       = 0.25,         -- soft dark halo behind the circle (helps on light/busy bg)
            cursor_shadow_size  = 9,
            cursor_contrast     = 0,            -- 0..1 inverted hairline, visible on literally anything
            cursor_outline      = 0,            -- 0..1 hard dark edge
            cursor_hover_scale  = 1.25,         -- bigger over links / buttons
            cursor_click_scale  = 1.2,          -- expands while a mouse button is held
            cursor_color        = pal.cursor,   -- ring + glow
            cursor_center_color = "rgb(ffffff)",
        },
    },
})
