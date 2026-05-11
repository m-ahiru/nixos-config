{ config, pkgs, ... }:
{
  programs.zsh = {
    enable = true;
    enableCompletion = true;
    autosuggestion.enable = true;
    syntaxHighlighting.enable = true;
    history.size = 10000;
    history.path = "$HOME/.zsh_history";
    history.ignoreAllDups = true;
    initContent = builtins.readFile ./zsh-init.sh;
    shellAliases = {
      ds3 = "protonhax run 374320 /home/mahiru/.wine/drive_c/Program\\ Files/Cheat\\ Engine/Cheat\\ Engine.exe";
      p5 = "protonhax run 1687950 /home/mahiru/.wine/drive_c/Program\\ Files/Cheat\\ Engine/Cheat\\ Engine.exe";
      sbr = "protonhax run 3489700 /home/mahiru/.wine/drive_c/Program\\ Files/Cheat\\ Engine/Cheat\\ Engine.exe";
      er = "protonhax run 1245620 /home/mahiru/.wine/drive_c/Program\\ Files/Cheat\\ Engine/Cheat\\ Engine.exe";
      ertool = "protonhax run 1245620 /home/mahiru/Documents/EldenRingTool.exe";
      ds3tool = "protonhax run 374320 /home/mahiru/Documents/SilkySouls3/SilkySouls3.exe";
      ttool = "protonhax run 1245620 /home/mahiru/Documents/TarnishedTool.exe";
      term = "setsid kitty --class ScratchpadTerm --title Scratchpad >/dev/null 2>&1 &";
      gubtool = "~/.cargo/bin/gubtool";
      compress = "/etc/nixos/config/programs/zsh/Scripts/resize_video.sh";
      crop43 = "/etc/nixos/config/programs/zsh/Scripts/crop43.sh";
      # ac = "cd ~/Apps/uClicker/;./uClicker";
      upscale = "/etc/nixos/config/programs/zsh/Scripts/upscale.sh";
      claude = "setsid chromium --app=https://claude.ai --class Claude >/dev/null 2>&1 &";
      whatsapp = "setsid chromium --app=https://web.whatsapp.com --class WhatsApp >/dev/null 2>&1 &";
      soundcloud = "setsid chromium --app=https://soundcloud.com --class Soundcloud >/dev/null 2>&1 &";
      scratchpad = "setsid foot --class ScratchpadTerm --title Scratchpadkitty &";
    };

    oh-my-zsh = {
      enable = true;
      plugins = [ "git" ];
      theme = "robbyrussell";
    };
  };
  home.sessionVariables = {
    hypr = "/etc/nixos/config/sessions/hyprland/";
    programs = "/etc/nixos/config/programs";
  };
}
