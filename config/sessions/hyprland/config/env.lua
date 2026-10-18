-- ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
--  ◈ ENVIRONMENT VARIABLES
-- ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

hl.env("NIXOS_OZONE_WL", "1")

-- cursor theme from home.pointerCursor (generated as nix-cursor.lua by default.nix). Without it,
-- apps started from Hyprland (slurp, ...) can't find the theme and fall back to a built-in arrow.
local ok, cursor = pcall(require, "nix-cursor")
if ok and type(cursor) == "table" then
    hl.env("XCURSOR_THEME", cursor.theme)
    hl.env("XCURSOR_SIZE", tostring(cursor.size))
end
