{ config, pkgs, ... }:

let
  # 1. Define the path to your programs directory
  programsDir = ./config/programs;

  # 2. Get the content of the directory
  files = builtins.readDir programsDir;

  # 3. Filter for directories only (ignoring regular files like .DS_Store or READMEs)
  directories = builtins.filter 
    (name: files.${name} == "directory") 
    (builtins.attrNames files);

  # 4. Map the directory names to import paths
  programImports = map (name: programsDir + "/${name}") directories;
in
{
  imports = [
    # sessions
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
      qt6Packages.qt6ct
  ];

  # set cursor 
home.pointerCursor = {
  gtk.enable = true;
  x11.enable = true;
  name = "Bibata-Modern-Classic";
  size = 24;
  package = pkgs.bibata-cursors;
};

  # Force the dark color scheme and explicitly set GTK3 theme in dconf
  dconf.settings = {
    "org/gnome/desktop/interface" = {
      color-scheme = "prefer-dark";
      gtk-theme = "adw-gtk3-dark";
    };
  };
  
  home.sessionVariables = {
    # Left intentionally blank to prevent GTK variable overrides
  };

  services.easyeffects.enable = true;  

  gtk = {
    enable = true;
    
    # IMPORT DYNAMIC MATUGEN COLORS 
    gtk3.extraCss = ''@import url("file:///home/mahiru/.cache/matugen/colors-gtk.css");'';
    gtk4.extraCss = ''@import url("file:///home/mahiru/.cache/matugen/colors-gtk.css");'';
    
    # Target GTK3 specifically
    gtk3.extraConfig = {
      gtk-application-prefer-dark-theme = 1;
      gtk-theme-name = "adw-gtk3-dark";
    };
    
    # Keep GTK4 native but ensure it requests the dark preference
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
    extraPortals = with pkgs; [
      xdg-desktop-portal-gtk
    ];
    config.common.default = "*";
  };

  programs.home-manager.enable = true;

  fonts.fontconfig.enable = true; 
  
  home.file = {
    ".local/share/fonts/" = {
      source = config/fonts; 
      recursive = true;
    };
  };
}
