{ config, pkgs, lib, inputs, system, ... }:
{
  imports = [
    ./hypridle.nix 
  ];
wayland.windowManager.hyprland = {
  enable = true;
  package = null;
  portalPackage = null;
  extraConfig = ''
    source = /etc/nixos/config/sessions/hyprland/hyprland.conf
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
  home.file.".config/hypr/config".source = config.lib.file.mkOutOfStoreSymlink "/etc/nixos/config/sessions/hyprland/config";
  home.file.".config/hypr/templates".source = config.lib.file.mkOutOfStoreSymlink "/etc/nixos/config/sessions/hyprland/templates";
}
