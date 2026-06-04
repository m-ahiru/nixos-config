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
    ./config/programs/nautilus
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
  ".config/hypr/xdph.conf".text = ''
    screencopy {
      allow_token_by_default = true
    }
  '';
  ".local/share/fonts/" = {
    source = config/fonts;
    recursive = true;
  };
  ".local/share/easyeffects/input/compressor.json" = {
    source = ./config/programs/easyeffects/input/compressor.json;
  };
};
}
