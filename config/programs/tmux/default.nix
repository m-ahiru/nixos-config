# /etc/nixos/config/programs/tmux/default.nix
{ ... }:
{
  home.file.".config/tmux/tmux.conf".text = ''
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
    set-option -g destroy-unattached on
    set-option -g status "on"
    bind -n M-1 select-window -t 0
    bind -n M-2 select-window -t 1
    set-hook -g session-created "run-shell '/etc/nixos/config/programs/zsh/Scripts/tmux-colors.sh'"
    set-hook -g client-attached "run-shell '/etc/nixos/config/programs/zsh/Scripts/tmux-colors.sh'"
  '';
}
