-- ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
--  ◈ AUTOSTART
-- ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
-- "hyprland.start" fires once per session, like exec-once did.

local startup = {
    "swww-daemon",
    "hypridle",
    "playerctld",
    "wl-paste --type text --watch cliphist store",
    "wl-paste --type image --watch cliphist store",
    "goxlr-daemon",
    "systemctl --user enable --now easyeffects",
    "~/.config/hypr/scripts/volume_listener.sh",
    "gsettings set org.gnome.desktop.interface cursor-theme 'ArcMidnight-Cursors'",
    "gsettings set org.gnome.desktop.interface cursor-size 24",
    "quickshell -p ~/.config/hypr/scripts/quickshell/Main.qml",
    "quickshell -p ~/.config/hypr/scripts/quickshell/TopBar.qml",
    "quickshell -p ~/.config/hypr/scripts/quickshell/Floating.qml",
    "python3 ~/.config/hypr/scripts/quickshell/focustime/focus_daemon.py &",

    "steam",
    "kitty --class ScratchpadTerm --title Scratchpad",
    "sleep 2 && obs --startreplaybuffer --multi",
    'bash -c "sleep 8 && setsid chromium --app=https://soundcloud.com --class Soundcloud >/dev/null 2>&1 & disown"',
    'bash -c "sleep 5 && chromium --profile-directory=Default --app=https://claude.ai & disown"',
    'bash -c "sleep 5 && chromium --profile-directory=Default --app=https://web.whatsapp.com & disown"',
    "sleep 3 && discord",
    "~/Apps/check_updates.sh",
    "sleep 6 && zen",
    "easyeffects --gapplication-service",
    "sleep 7 && kitty --title Clock tty-clock -c -C 6 -s",
    "pavucontrol -t 0",
    "sleep 5 && flatpak run io.github.victoralvesf.aonsoku",
}

hl.on("hyprland.start", function()
    for _, cmd in ipairs(startup) do
        hl.exec_cmd(cmd)
    end
end)
