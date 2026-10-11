{ config, pkgs, lib, inputs, system, ... }:
let
  # Same nixpkgs as the pinned Hyprland -> plugin is always built against the exact running version
  hyprlandPkgs = inputs.nixpkgs-hyprland.legacyPackages.${system};
  cursortrail = hyprlandPkgs.callPackage ./plugins/cursortrail {
    inherit (hyprlandPkgs.hyprlandPlugins) mkHyprlandPlugin;
  };
in
{
  imports = [
    ./hypridle.nix 
  ];
wayland.windowManager.hyprland = {
  enable = true;
  package = null;
  portalPackage = null;
  # Lua config (Hyprland >= 0.55). HM writes ~/.config/hypr/hyprland.lua with the
  # systemd/dbus activation in a "hyprland.start" hook, then loads our modules from
  # ~/.config/hypr/config/init.lua (out-of-store symlink below, so edits apply live).
  configType = "lua";
  extraConfig = ''
    require("config")
  '';
};
  home.packages = with pkgs; [
    rofi
    pavucontrol
    fortune
    wl-screenrec
    alsa-utils
    (inputs.nixpkgs-swww.legacyPackages.${system}.swww)
    networkmanager_dmenu
    wl-clipboard
    fd
    qt6.qtmultimedia
    qt6.qt5compat
    qt6.qtwebsockets
    qt6.qtwebengine
    ripgrep
    gtk3
    cava
    cliphist
    tree
    jq
    socat 
    pamixer 
    brightnessctl
    acpi
    iw
    bluez
    libnotify
    networkmanager
    lm_sensors
    bc
    pulseaudio
    ladspaPlugins
    ladspa-sdk
    imagemagick
  ];
  home.sessionVariables.NIXOS_OZONE_WL = "1";
  home.file.".config/hypr/scripts".source = config.lib.file.mkOutOfStoreSymlink "/etc/nixos/config/sessions/hyprland/scripts";
  # Store paths of our plugins, read by config/cursortrail.lua via require("nix-plugins")
  home.file.".config/hypr/nix-plugins.lua".text = ''
    return {
      cursortrail = "${cursortrail}/lib/libcursortrail.so",
    }
  '';
  home.file.".config/hypr/config".source = config.lib.file.mkOutOfStoreSymlink "/etc/nixos/config/sessions/hyprland/config";
}
