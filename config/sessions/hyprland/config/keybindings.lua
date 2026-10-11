-- ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
--  ◈ KEYBINDINGS
-- ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
-- Key strings: modifiers first, joined with " + " (e.g. "SUPER + SHIFT + Q").
-- Old flags: binde → { repeating = true }, bindl → { locked = true },
--            bindel → both, bindm → hl.dsp.window.drag()/resize() with { mouse = true }

local vars     = require("config.variables")
local mainMod  = vars.mainMod
local terminal = vars.terminal

local function key(...)
    return table.concat({ ... }, " + ")
end

local exec   = hl.dsp.exec_cmd
local qs     = "~/.config/hypr/scripts/qs_manager.sh"
local shot   = "~/.config/hypr/scripts/screenshot.sh"
local locked = { locked = true }

-- ───────── Submaps ─────────
-- Dummy bind to force Hyprland to register the submap in memory.
-- Since we will never press this, all normal keystrokes fall through to Quickshell
hl.define_submap("passthru", function()
    hl.bind("SUPER + SHIFT + CTRL + ALT + F35", exec("true"))
end)

-- ───────── Mouse & Gestures ─────────
hl.gesture({ fingers = 3, direction = "horizontal", action = "workspace" })

hl.bind(key(mainMod, "mouse:272"), hl.dsp.window.drag(),   { mouse = true })
hl.bind(key(mainMod, "mouse:273"), hl.dsp.window.resize(), { mouse = true })

-- ───────── Window Management ─────────
local resizeStep = 50
hl.bind(key(mainMod, "SHIFT", "left"),  hl.dsp.window.resize({ x = -resizeStep, y = 0, relative = true }), { repeating = true })
hl.bind(key(mainMod, "SHIFT", "right"), hl.dsp.window.resize({ x =  resizeStep, y = 0, relative = true }), { repeating = true })
hl.bind(key(mainMod, "SHIFT", "up"),    hl.dsp.window.resize({ x = 0, y = -resizeStep, relative = true }), { repeating = true })
hl.bind(key(mainMod, "SHIFT", "down"),  hl.dsp.window.resize({ x = 0, y =  resizeStep, relative = true }), { repeating = true })

local directions = { H = "left", L = "right", K = "up", J = "down" }
for k, dir in pairs(directions) do
    hl.bind(key("CTRL", k),    hl.dsp.window.move({ direction = dir }))
    hl.bind(key(mainMod, k),   hl.dsp.focus({ direction = dir }))
end

hl.bind(key(mainMod, "Q"),          hl.dsp.window.close())
hl.bind(key(mainMod, "SHIFT", "F"), hl.dsp.window.float({ action = "toggle" }))

-- ───────── System & Hardware ─────────
-- hl.bind(key("ALT", "Shift_L"), exec("hyprctl switchxkblayout main prev"), locked)

hl.bind("Caps_Lock",             exec("sleep 0.1 && swayosd-client --caps-lock"), locked)
hl.bind("XF86MonBrightnessDown", exec("swayosd-client --brightness lower"),       locked)
hl.bind("XF86MonBrightnessUp",   exec("swayosd-client --brightness raise"),       locked)
hl.bind("Print",                         exec(shot),                     locked)
hl.bind(key("SHIFT", "Print"),           exec(shot .. " --edit"),        locked)
hl.bind(key("SUPER", "Print"),           exec(shot .. " --full"),        locked)
hl.bind(key("SUPER", "SHIFT", "Print"),  exec(shot .. " --full --edit"), locked)
-- hl.bind("XF86PowerOff", exec("bash ~/.config/hypr/scripts/lock.sh"), locked)

-- ───────── Media & Audio ─────────
-- hl.bind(key(mainMod, "SPACE"), exec("playerctl play-pause"), locked)
hl.bind("XF86AudioPause",       exec("playerctl play-pause"),                         locked)
hl.bind("XF86AudioPlay",        exec("playerctl play-pause"),                         locked)
hl.bind("XF86AudioMicMute",     exec("swayosd-client --input-volume mute-toggle"),    locked)
hl.bind("XF86AudioMute",        exec("swayosd-client --output-volume mute-toggle"),   locked)
hl.bind("XF86AudioLowerVolume", exec("swayosd-client --output-volume lower"), { locked = true, repeating = true })
hl.bind("XF86AudioRaiseVolume", exec("swayosd-client --output-volume raise"), { locked = true, repeating = true })

-- ───────── Applications & Launchers ─────────
hl.bind(key(mainMod, "F"),      exec("zen"))
hl.bind(key(mainMod, "E"),      exec("nautilus"))
hl.bind(key(mainMod, "Return"), exec(terminal))

-- ───────── Quickshell Controls ─────────
-- hl.bind(key(mainMod, "M"), exec("bash " .. qs .. " toggle monitors"))
hl.bind(key(mainMod, "R"),          exec("bash ~/.config/hypr/scripts/reload.sh"))
hl.bind(key(mainMod, "SPACE"),      exec(qs .. " toggle applauncher"))
hl.bind(key(mainMod, "C"),          exec(qs .. " toggle clipboard"))
hl.bind(key(mainMod, "SHIFT", "S"), exec("bash " .. qs .. " toggle settings"))
-- hl.bind(key(mainMod, "Q"), exec("bash " .. qs .. " toggle music"))
hl.bind(key(mainMod, "B"),          exec("bash " .. qs .. " toggle battery"))
hl.bind(key(mainMod, "W"),          exec("bash " .. qs .. " toggle wallpaper"))
-- hl.bind(key(mainMod, "S"), exec("bash " .. qs .. " toggle calendar"))
hl.bind(key(mainMod, "N"),          exec("bash " .. qs .. " toggle network"))
hl.bind(key(mainMod, "SHIFT", "T"), exec("bash " .. qs .. " toggle focustime"))
hl.bind(key(mainMod, "V"),          exec("bash " .. qs .. " toggle volume"))
-- hl.bind(key(mainMod, "H"), exec("bash " .. qs .. " toggle guide"))

-- ───────── Workspaces ─────────
for i = 1, 10 do
    local k = tostring(i % 10) -- workspace 10 sits on key 0
    hl.bind(key(mainMod, k),          exec(qs .. " " .. i))
    hl.bind(key(mainMod, "SHIFT", k), exec(qs .. " " .. i .. " move"))
end

-- ###### MEIN ZEUG #####

hl.bind(key("SUPER", "Escape"), hl.dsp.window.fullscreen({ mode = "fullscreen" })) -- toggle the window between focus and fullscreen

-- Special workspaces pinned to the main monitor: focus DP-2 first, then toggle
local function specialOnMain(name)
    return function()
        hl.dispatch(hl.dsp.focus({ monitor = "DP-2" }))
        hl.dispatch(hl.dsp.workspace.toggle_special(name))
    end
end

hl.bind(key("ALT", "SPACE"),  specialOnMain("claude"))
hl.bind(key("ALT", "Return"), specialOnMain("term"))
hl.bind(key("ALT", "O"),      specialOnMain("soundcloud"))
hl.bind(key("ALT", "W"),      specialOnMain("whatsapp"))
hl.bind(key("ALT", "S"),      specialOnMain("aonsoku"))
hl.bind(key("ALT", "P"),      hl.dsp.workspace.toggle_special("pavu"))
hl.bind(key(mainMod, "S"),    exec('grim -g "$(slurp)" - | wl-copy'))
hl.bind(key(mainMod, "U"),    exec("gubtool quitout"))
