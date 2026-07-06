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
      rebuild = "sudo nixos-rebuild switch --flake /etc/nixos#mahiru |& nom";
      rebuildcope = "sudo nixos-rebuild switch --flake /etc/nixos#mahiru && sudo systemctl restart home-manager-mahiru.service";
      rebuildboot = "sudo nixos-rebuild boot --flake /etc/nixos#mahiru |& nom";
      term = "setsid kitty --class ScratchpadTerm --title Scratchpad >/dev/null 2>&1 &";
      clock = "setsid kitty --title Clock tty-clock -c -C 6 -s >/dev/null 2>&1 &";
      gubtool = "~/.cargo/bin/gubtool";
      compress = "/etc/nixos/config/programs/zsh/Scripts/resize_video.sh";
      videodl = ''yt-dlp -f "bestvideo[ext=mp4]+bestaudio[ext=m4a]/best[ext=mp4]" --merge-output-format mp4 -P ~/Videos'';
      crop43 = "/etc/nixos/config/programs/zsh/Scripts/crop43.sh";
      # ac = "cd ~/Apps/uClicker/;./uClicker";
      upscale = "/etc/nixos/config/programs/zsh/Scripts/upscale.sh";
      claude = "setsid chromium --app=https://claude.ai --class Claude >/dev/null 2>&1 &";
      whatsapp = "setsid chromium --app=https://web.whatsapp.com --class WhatsApp >/dev/null 2>&1 &";
      soundcloud = "setsid chromium --app=https://soundcloud.com --class Soundcloud >/dev/null 2>&1 &";
      ssh = "TERM=xterm-256color ssh";
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
