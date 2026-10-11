-- ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
--  ◈ WINDOW & LAYER RULES
-- ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

-- ─────────────────────────────
-- Layer rules (OSD / overlays)
-- ─────────────────────────────
hl.layer_rule({ match = { namespace = "^(volume_osd)$" },     no_anim = true })
hl.layer_rule({ match = { namespace = "^(brightness_osd)$" }, no_anim = true })
hl.layer_rule({ match = { namespace = "hyprpicker" },         no_anim = true })
hl.layer_rule({ match = { namespace = "qsdock" },             no_anim = true })
hl.layer_rule({ match = { namespace = "ext-session-lock" },   blur = true, ignore_alpha = 0.2 })

-- ─────────────────────────────
-- Window rules
-- ─────────────────────────────

-- ───────── App Launcher ─────────
hl.window_rule({
    match     = { title = "^(app-launcher)$" },
    float     = true,
    center    = true,
    size      = "1200 600",
    animation = "slide",
})

-- ───────── MASTER QUICKSHELL CONTAINER ─────────
-- All widgets now live inside this single, shape-shifting window.
-- hl.window_rule({
--     match = { title = "^(qs-master)$" },
--     float = true, no_shadow = true, no_border = true, no_initial_focus = true,
-- })

-- --------------------------
-- Workspaces / Monitor-Zuweisung
-- --------------------------
hl.workspace_rule({ workspace = "9",       monitor = "HDMI-A-1" })
hl.workspace_rule({ workspace = "special", monitor = "DP-2" })
for ws = 1, 8 do
    hl.workspace_rule({ workspace = tostring(ws), monitor = "DP-2" })
end

-- --------------------------
-- Workspaces / App placement
-- --------------------------
hl.window_rule({ match = { title = "^(Clock)$" },                             workspace = "9" })
hl.window_rule({ match = { class = "^(discord-canary|discord|vesktop)$" },    workspace = "9" })
hl.window_rule({ match = { class = "^(zen)$" },                               workspace = "3" })
hl.window_rule({ match = { class = "^(steam)$" },                             workspace = "2 silent" })
hl.window_rule({ match = { class = "^(com\\.obsproject\\.Studio)$" },         workspace = "5" })
hl.window_rule({ match = { class = "^(steam_app_\\d+)$" },                    workspace = "4" })
hl.window_rule({ match = { class = "^(retroarch)$" },                         workspace = "4" })

-- Claude (PWA)
hl.window_rule({
    name      = "pwa-claude",
    match     = { class = "^(chrome-claude\\.ai__-Default)$" },
    workspace = "special:claude silent",
    monitor   = "DP-2",
    float     = true,
    size      = "1200 800",
    center    = true,
    no_anim   = true,
    no_blur   = true,
})

-- WhatsApp (PWA)
hl.window_rule({
    name      = "pwa-whatsapp",
    match     = { class = "^(chrome-web\\.whatsapp\\.com__-Default)$" },
    workspace = "special:whatsapp silent",
    monitor   = "DP-2",
    float     = true,
    size      = "1000 700",
    center    = true,
    no_anim   = true,
    no_blur   = true,
})

-- Calculator
hl.window_rule({
    name    = "calculator",
    match   = { class = "^(org\\.gnome\\.Calculator)$" },
    float   = true,
    size    = "500 700",
    center  = true,
    no_anim = true,
})

-- SoundCloud (PWA)
hl.window_rule({
    name      = "pwa-soundcloud",
    match     = { class = "^(chrome-soundcloud\\.com__-Default)$" },
    workspace = "special:soundcloud silent",
    monitor   = "DP-2",
    float     = true,
    size      = "1200 800",
    center    = true,
    no_anim   = true,
    no_blur   = true,
})

-- Scratchpad Terminal
hl.window_rule({
    name      = "scratchpad-term",
    match     = { class = "^(ScratchpadTerm)$" },
    workspace = "special:term silent",
    float     = true,
    size      = "1400 800",
    center    = true,
    no_anim   = true,
})

-- Pavucontrol
hl.window_rule({
    name      = "pavucontrol",
    match     = { class = "^(org\\.pulseaudio\\.pavucontrol)$" },
    workspace = "special:pavu silent",
    float     = true,
    size      = "1000 700",
    center    = true,
    no_anim   = true,
})

-- Aonsoku
hl.window_rule({
    name      = "aonsoku",
    match     = { class = "^(?i)(aonsoku)$" },
    workspace = "special:aonsoku silent",
    monitor   = "DP-2",
    float     = true,
    size      = "1200 900",
    center    = true,
    no_anim   = true,
    no_blur   = true,
})

-- Nautilus
hl.window_rule({
    name    = "nautilus",
    match   = { class = "^(org\\.gnome\\.Nautilus)$" },
    opacity = "0.90 0.80",
})
