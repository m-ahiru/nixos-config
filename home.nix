{ config, pkgs, ... }:
let
  programsDir = ./config/programs;
  files = builtins.readDir programsDir;
  directories = builtins.filter 
    (name: files.${name} == "directory") 
    (builtins.attrNames files);
  programImports = map (name: programsDir + "/${name}") directories;
in
{
  imports = [
    ./config/sessions/hyprland/default.nix
  ] ++ programImports; 
  home.username = "mahiru";
  home.homeDirectory = "/home/mahiru";
  home.stateVersion = "25.11"; 
  home.sessionPath = [ "$HOME/.cargo/bin" ];
  
  home.packages = with pkgs; [
    adwaita-icon-theme
    adw-gtk3 
    libsForQt5.qt5ct      
    tmux
    qt6Packages.qt6ct
  ];

  home.pointerCursor = {
    gtk.enable = true;
    x11.enable = true;
    name = "Bibata-Modern-Classic";
    size = 24;
    package = pkgs.bibata-cursors;
  };

  dconf.settings = {
    "org/gnome/desktop/interface" = {
      color-scheme = "prefer-dark";
      gtk-theme = "adw-gtk3-dark";
    };
  };
  
  home.sessionVariables = {};
  services.easyeffects.enable = true;  

  gtk = {
    enable = true;
    gtk3.extraCss = ''@import url("file:///home/mahiru/.cache/matugen/colors-gtk.css");'';
    gtk4.extraCss = ''@import url("file:///home/mahiru/.cache/matugen/colors-gtk.css");'';
    gtk3.extraConfig = {
      gtk-application-prefer-dark-theme = 1;
      gtk-theme-name = "adw-gtk3-dark";
    };
    gtk4.extraConfig = {
      gtk-application-prefer-dark-theme = 1;
    };
  };
  
  qt = {
    enable = true;
    platformTheme.name = "qt6ct";
  };
  
  xdg.portal = {
    enable = true;
    extraPortals = with pkgs; [ xdg-desktop-portal-gtk ];
    config.common.default = "*";
  };

  programs.home-manager.enable = true;
  fonts.fontconfig.enable = true; 
  
home.file = {
  ".local/share/fonts/" = {
    source = config/fonts;
    recursive = true;
  };
  ".config/tmux/tmux.conf".text = ''
    set -g default-terminal "tmux-256color"
    set -ag terminal-overrides ",*:RGB"
    set -g history-limit 10000
    set -sg escape-time 0
    set -g mode-keys vi
    set -g mouse on
    set -g base-index 0
    set -g status-interval 5
    set -g status-left-length 50
    set -g status-right-length 150
    set-option -g status "on"

    bind -n M-1 select-window -t 0
    bind -n M-2 select-window -t 1

    set-hook -g session-created "run-shell '/etc/nixos/config/programs/zsh/Scripts/tmux-colors.sh'"
    set-hook -g client-attached "run-shell '/etc/nixos/config/programs/zsh/Scripts/tmux-colors.sh'"
  '';
};
}
