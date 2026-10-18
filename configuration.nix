# Edit this configuration file to define what should be installed on
# your system. Help is available in the configuration.nix(5) man page
# and in the NixOS manual (accessible by running ‘nixos-help’).
###################################################
#### NOTE TO SELF:
###  PROTON_REMOTE_DEBUG_CMD="/home/mahiru/.wine/drive_c/Program\ Files/Cheat\ Engine/Cheat\ Engine.exe" %command%
###################################################

{ config, pkgs, lib, inputs, unstable, ... }:

{
  # Imports
  imports =
    [ # Include the results of the hardware scan.
      ./hardware-configuration.nix
    ];

  home-manager.backupFileExtension = "backup";

  # System packages
  environment.systemPackages = with pkgs; [
    wget 
    file-roller
#    (inputs.nixpkgs-librewolf.legacyPackages.${pkgs.system}.librewolf)
    protontricks
    unstable.protonplus
    blender
    pkgs.android-tools
    runelite
    mame-tools
    lutris
    samba
    (pkgs.writeShellScriptBin "ai" ''
  exec ${pkgs.llama-cpp-vulkan}/bin/llama-server \
    -hf unsloth/Qwen3.6-35B-A3B-GGUF:Q4_K_M \
    --device Vulkan0 -ngl 99 --n-cpu-moe 20 \
    -c 8192 --host 127.0.0.1 --port 8080 "$@"
'')
    protonup-qt
    linux-wallpaperengine
    ryubing
    icu
    yt-dlp
    inputs.zen-browser.packages.${pkgs.stdenv.hostPlatform.system}.default
    rustup
    audacity
    gcc
    kdePackages.kdenlive
    nix-output-monitor
    wine
    taskwarrior3
    tty-clock
    esptool
    ffmpegthumbnailer
    inotify-tools
    gst_all_1.gst-plugins-bad
    gst_all_1.gst-plugins-ugly
    gst_all_1.gst-libav
    gamescope
    adwsteamgtk
    mesa-demos
    vulkan-tools
    unstable.discord
    losslesscut-bin
    file
    pipes 
    vesktop
    cbonsai
    git
    killall
    btop  
    mpv
    zenity
    nemo
    tailscale
    matugen
    gpu-screen-recorder
    neovim 
    fzf
    direnv
    zbar
    python3
    ffmpeg
    python314
    (wrapFirefox (pkgs.firefox-unwrapped.override { pipewireSupport = true; }) {})
    kitty
    libreoffice-qt
    hunspell
    hunspellDicts.ru_RU
    hunspellDicts.en_US
    obsidian
    easyeffects
    chromium
    pavucontrol
    obs-studio
    p7zip
    papers
    fastfetch
    quickshell
    gnome-shell-extensions
    grim
    playerctl
    satty
    yq-go
    xdg-desktop-portal-gtk
    eww
    swappy
    slurp
    mpvpaper
    gnome-tweaks
    pkgsCross.mingwW64.stdenv.cc
    wmctrl
    qbittorrent
    power-profiles-daemon
    jdk8
  ];

  environment.pathsToLink = [ "/share/gsettings-schemas" ];

  # User accounts and security
  users.users.mahiru = {
    isNormalUser = true;
    description = "mahiru";
    extraGroups = [ "docker" "networkmanager" "wheel" "video" "adbusers" "libvirtd"]; 
    packages = with pkgs; [
    #  thunderbird
    ];
    useDefaultShell = true;
    shell = pkgs.zsh;
  };    

  users.defaultUserShell = pkgs.zsh;
  system.userActivationScripts.zshrc = "touch .zshrc";

systemd.user.services = {
  pipewire.restartIfChanged = false;
  wireplumber.restartIfChanged = false;
  pipewire-pulse.restartIfChanged = false;
};

services.udev.extraRules = ''
  SUBSYSTEM=="hidraw", ATTRS{idVendor}=="33e4", MODE="0666"
'';

security.wrappers.bwrap = {
  owner = "root";
  group = "root";
  setuid = true;
  source = "${pkgs.bubblewrap}/bin/bwrap";
};
  security.sudo.extraRules = [
    {
      users = [ "mahiru" ];
      commands = [
        {
          command = "ALL";
          options = [ "NOPASSWD" ];
        }
      ];
    }
  ];

  services.logind.settings.Login = {
    HandlePowerKey = "ignore";
  }; 
  # Program configurations
  programs.appimage = {
    enable = true;
    binfmt = true;
};
 
  programs.uclicker = {
  enable = true;
  users = [ "mahiru" ];
};

  programs.zsh.enable = true;


  # Install firefox.
  programs.firefox.enable = true;

  programs.dconf = {
    enable = true;
  };

  programs.steam = {
    enable = true;
    remotePlay.openFirewall = true; 
    dedicatedServer.openFirewall = true; 
  };
  programs.gamemode.enable = false;

  # Home manager
  home-manager.useGlobalPkgs = true;
  home-manager.useUserPackages = true; 
  
  home-manager.extraSpecialArgs = { inherit inputs; system = "x86_64-linux"; };
  home-manager.users.mahiru = {
  imports = [ ./home.nix ];
};

  # Desktop environment, window managers and theme
  services.xserver.enable = true;

  #Mullvad
  services.mullvad-vpn.enable = true;
  services.mullvad-vpn.package = pkgs.mullvad-vpn;

  #GoXLR-Utility
  services.goxlr-utility.enable = true;


  # Enable the GNOME Desktop Environment.
  services.displayManager.gdm.enable = true;
  services.desktopManager.gnome.enable = true;
  
  services.displayManager.autoLogin = {
    enable = true;
    user = "mahiru";
  };
  services.displayManager.defaultSession = "hyprland";
  # Tailscale

  services.tailscale.enable = true;



  # Hyprland
programs.hyprland = {
  enable = true;
  package = inputs.nixpkgs-hyprland.legacyPackages.${pkgs.system}.hyprland;
  portalPackage = inputs.nixpkgs-hyprland.legacyPackages.${pkgs.system}.xdg-desktop-portal-hyprland;
};
  
  # XDG Portals
  xdg.portal = {
    enable = true;
    extraPortals = with pkgs; [
      xdg-desktop-portal-gtk
    ];
  };

  # Standard-Terminal für xdg-terminal-exec (Nautilus "Open in Terminal")
  xdg.terminal-exec = {
    enable = true;
    settings = {
      default  = [ "kitty.desktop" ];
      GNOME    = [ "kitty.desktop" ];
      Hyprland = [ "kitty.desktop" ];
    };
  };

  # Nautilus: "Open in kitty" per Extension (Nautilus ignoriert xdg-terminal-exec)
  programs.nautilus-open-any-terminal = {
    enable = true;
    terminal = "kitty";
  };

  # GNOME Console rauswerfen, damit "Open in Console" verschwindet
  environment.gnome.excludePackages = [ pkgs.gnome-console ];

  # Configure keymap in X11
  services.xserver.xkb = {
    layout = "de";
    variant = "";
  };

  # Fonts
  fonts.packages = with pkgs; [
    udev-gothic-nf
    noto-fonts
    liberation_ttf
  ]; 

  fonts.fontconfig = {
    enable = true;
    hinting.style = "slight"; 
    subpixel.rgba = "rgb"; 
  };

# Flatpak
services.flatpak = {
  enable = true;

  remotes = [{
    name = "flathub";
    location = "https://dl.flathub.org/repo/flathub.flatpakrepo";
  }];

  packages = [
    "io.github.victoralvesf.aonsoku"
    "it.mijorus.gearlever"
  ];

  overrides."io.github.victoralvesf.aonsoku".Environment = {
    "PULSE_PROP_application.name" = "Aonsoku";
  };

  uninstallUnmanaged = true;

  update.auto = {
    enable = true;
    onCalendar = "weekly";
  };
};


  # Environment Variables
  # environment.variables.XDG_DATA_DIRS = lib.mkForce "/home/mahiru/.nix-profile/share:/run/current-system/sw/share";

  # Networking and time
  networking.hostName = "mahiru"; 
  
  networking.networkmanager = {
    enable = true;
    wifi.powersave = false; 
  };
   # Set your time zone.
  time.timeZone = "Europe/Berlin";

  # Select internationalisation properties.
  i18n.defaultLocale = "en_US.UTF-8";

  i18n.extraLocaleSettings = {
    LC_ADDRESS = "en_US.UTF-8";
    LC_IDENTIFICATION = "en_US.UTF-8";
    LC_MEASUREMENT = "en_US.UTF-8";
    LC_MONETARY = "en_US.UTF-8";
    LC_NAME = "en_US.UTF-8";
    LC_NUMERIC = "en_US.UTF-8";
    LC_PAPER = "en_US.UTF-8";
    LC_TELEPHONE = "en_US.UTF-8";
    LC_TIME = "en_US.UTF-8";
  };

  # Audio and system services
  services.pulseaudio.enable = false;
  security.rtkit.enable = true;
  services.pipewire = {
    enable = true;
    alsa.enable = true;
    alsa.support32Bit = true;
    pulse.enable = true;
  };
  services.blueman.enable = true;

  # Enable CUPS to print documents.
  services.printing.enable = true;

  # Enable the OpenSSH daemon.
  services.openssh.enable = true;

  # Power Management Services
  services.power-profiles-daemon.enable = true; 

  # Nix settings and maintenance
  nixpkgs.config.allowUnfree = true;
  fileSystems."/mnt/secondary" = {
  device = "/dev/disk/by-uuid/6be3dc24-6b49-4341-b0f5-6670d66ee0e4";
  fsType = "ext4";
  };

  fileSystems."/mnt/tertiary" = {
  device = "/dev/disk/by-uuid/18e4a258-940f-44d9-b7a8-9c5fa40a52fe";
  fsType = "ext4";
  };


  nix.settings.experimental-features = [ "nix-command" "flakes" ];

  nix.gc = {
    automatic = true;
    dates = "daily";
    options = "--delete-older-than 14d";
  };
  boot.kernelParams = [
    "boot.shell_on_fail"
    "amd_pstate=active"
    "tsc=reliable"
  ];
  # Virtualization
  virtualisation.libvirtd.enable = true;
  virtualisation.docker.enable = true;
  virtualisation.docker.enableOnBoot = false;
  programs.virt-manager.enable = true;
  # Bootloader and kernel
  boot.loader.grub = {
    enable = true;
    efiSupport = true;
    device = "nodev";
    gfxmodeEfi = "1920x1080";
    gfxpayloadEfi = "keep";
    configurationLimit = 10;
  };
  boot.loader.efi.canTouchEfiVariables = true;
  boot.extraModprobeConfig = ''
  options snd-usb-audio vid=0x054c pid=0x0df2 index=-2
  '';

  # Kernel Packages and Optimization
  boot.kernelPackages = pkgs.linuxPackages_latest;
  boot.kernel.sysctl."kernel.yama.ptrace_scope" = 0;
  hardware.cpu.amd.updateMicrocode = true;
  hardware.enableRedistributableFirmware = true;

  boot.kernelModules = [ "tcp_bbr" ]; # FIX: Network Congestion Control (Helps with packet jitter)
  boot.kernel.sysctl = {
    "net.ipv4.tcp_congestion_control" = "bbr";
    "net.core.default_qdisc" = "fq";
    "net.core.wmem_max" = 1073741824;
    "net.core.rmem_max" = 1073741824;
    "net.ipv4.tcp_rmem" = "4096 87380 1073741824";
    "net.ipv4.tcp_wmem" = "4096 87380 1073741824";
  };

  # FIX: Force CPU to run at max clock speed to prevent frame-time jitter
  powerManagement.cpuFreqGovernor = "performance";
services.xserver.videoDrivers = [ "amdgpu" ];
hardware.graphics = {
	enable = true;
	enable32Bit = true;
  extraPackages = with pkgs; [
      rocmPackages.clr.icd
  ];
}; 
  system.stateVersion = "25.11"; 
}

