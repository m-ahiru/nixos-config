-- ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
--  ◈ SETTINGS
-- ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

-- Border colors come from matugen (~/.config/hypr/colors.lua).
-- Fallback so Hyprland still starts before matugen has run once.
local ok, colors = pcall(require, "colors")
if not ok or type(colors) ~= "table" then
    colors = {
        active_border   = "rgba(33ccffee)",
        inactive_border = "rgba(595959aa)",
    }
end

hl.config({
    general = {
        border_size             = 3,
        gaps_in                 = 9,
        gaps_out                = 15,
        float_gaps              = 6,
        resize_on_border        = true,
        extend_border_grab_area = 30,
        col = {
            active_border   = colors.active_border,
            inactive_border = colors.inactive_border,
        },
    },

    decoration = {
        rounding         = 15,
        active_opacity   = 1.0,
        inactive_opacity = 1.0,
        blur = {
            enabled           = true,
            size              = 10,
            passes            = 3,
            new_optimizations = true,
        },
        shadow = {
            enabled = true,
        },
    },

    input = {
        kb_layout     = "de",
        kb_options    = "grp:alt_shift_toggle",
        kb_variant    = "",
        kb_model      = "",
        kb_rules      = "",
        accel_profile = "flat",
        touchpad = {
            natural_scroll = true,
        },
    },

    misc = {
        focus_on_activate        = true,
        font_family              = "JetBrains Mono",
        disable_hyprland_logo    = true,
        disable_splash_rendering = true,
    },

    animations = {
        enabled = true,
    },

    cursor = {
        no_hardware_cursors = 1, -- 0 = off, 1 = on, 2 = auto (tri-state int since 0.5x)
    },
})

-- ───────── Animations ─────────
hl.curve("myBezier", { type = "bezier", points = { {0.0, 0.1}, {0.0, 1.0} } })

hl.animation({ leaf = "windows",             enabled = true, speed = 3, bezier = "myBezier", style = "slide" })
hl.animation({ leaf = "windowsOut",          enabled = true, speed = 5, bezier = "myBezier", style = "slide" })
hl.animation({ leaf = "layers",              enabled = true, speed = 5, bezier = "myBezier", style = "fade" })
hl.animation({ leaf = "layersIn",            enabled = true, speed = 5, bezier = "myBezier", style = "fade" })
hl.animation({ leaf = "layersOut",           enabled = true, speed = 5, bezier = "myBezier", style = "fade" })
hl.animation({ leaf = "fade",                enabled = true, speed = 5, bezier = "myBezier" })
hl.animation({ leaf = "workspaces",          enabled = true, speed = 5, bezier = "myBezier", style = "slide" })
hl.animation({ leaf = "specialWorkspaceIn",  enabled = true, speed = 4, bezier = "myBezier", style = "slide" })
hl.animation({ leaf = "specialWorkspaceOut", enabled = true, speed = 7, bezier = "myBezier", style = "slide" })
