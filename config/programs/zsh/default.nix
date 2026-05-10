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
      edit = "sudo -E nvim -n";
      gitavail = "ssh-add $HOME/Documents/Важное/recovery_keys/GitHub/github_remote_keys/key";
      update = "sudo nixos-rebuild switch";
      stop = "shutdown now";
      edconf = "sudo -E nvim /etc/nixos/configuration.nix";
      out = "loginctl terminate-user mahiru";
      ds3 = "protonhax run 374320 /home/mahiru/.wine/drive_c/Program\\ Files/Cheat\\ Engine/Cheat\\ Engine.exe";
      p5 = "protonhax run 1687950 /home/mahiru/.wine/drive_c/Program\\ Files/Cheat\\ Engine/Cheat\\ Engine.exe";
      sbr = "protonhax run 3489700 /home/mahiru/.wine/drive_c/Program\\ Files/Cheat\\ Engine/Cheat\\ Engine.exe";
      er = "protonhax run 1245620 /home/mahiru/.wine/drive_c/Program\\ Files/Cheat\\ Engine/Cheat\\ Engine.exe";
      ertool = "protonhax run 1245620 /home/mahiru/Documents/EldenRingTool.exe";
      ds3tool = "protonhax run 374320 /home/mahiru/Documents/SilkySouls3/SilkySouls3.exe";
      ttool = "protonhax run 1245620 /home/mahiru/Documents/TarnishedTool.exe";
      silk = "protonhax run 1030300 /home/mahiru/.wine/drive_c/Program\\ Files/Cheat\\ Engine/Cheat\\ Engine.exe";
      term = "setsid kitty --class ScratchpadTerm --title Scratchpad >/dev/null 2>&1 &";
      fix-controller = "sudo modprobe -r hid_playstation";
      revert-controller = "sudo modprobe hid_playstation";
      gubtool = "~/.cargo/bin/gubtool";
      compress = "~/Apps/resize_video.sh";
      crop43 = "~/Apps/crop43.sh";
      splitmanga = "~/Apps/splitmanga.sh";
      ac = "cd ~/Apps/uClicker/;./uClicker";
      upscale = "~/Apps/upscale.sh";
      blur = "~/Apps/blur/blur-Linux-Release-x64";
      claude = "setsid chromium --app=https://claude.ai --class Claude >/dev/null 2>&1 &";
      whatsapp = "setsid chromium --app=https://web.whatsapp.com --class WhatsApp >/dev/null 2>&1 &";
      soundcloud = "setsid chromium --app=https://soundcloud.com --class Soundcloud >/dev/null 2>&1 &";
      scratchpad = "setsid foot --class ScratchpadTerm --title Scratchpadkitty &";
      filezilla = "/home/mahiru/Apps/FileZilla3/bin/filezilla";
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
