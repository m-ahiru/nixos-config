#!/usr/bin/env bash

COLOR_FILE="/tmp/qs_colors.json"

get_color() {
  grep -E "\"$1\"\s*:\s*\"[^\"]+\"" "$COLOR_FILE" 2>/dev/null | cut -d '"' -f 4
}

BLUE=$(get_color "blue")
SURFACE0=$(get_color "surface0")
SURFACE1=$(get_color "surface1")
TEXT=$(get_color "text")
SUBTEXT1=$(get_color "subtext1")

BLUE=${BLUE:-"#89b4fa"}
SURFACE0=${SURFACE0:-"#313244"}
SURFACE1=${SURFACE1:-"#45475a"}
TEXT=${TEXT:-"#cdd6f4"}
SUBTEXT1=${SUBTEXT1:-"#bac2de"}

tmux set-option -g status-style "bg=default,fg=$TEXT"

tmux set-option -g status-left "\
#[fg=$SURFACE0,bg=$BLUE,bold] #S \
#[fg=$BLUE,bg=default]"

tmux set-option -g status-right "\
#[fg=$SURFACE1,bg=default]\
#[fg=$TEXT,bg=$SURFACE1]  %b %d \
#[fg=$BLUE,bg=$SURFACE1]\
#[fg=$SURFACE0,bg=$BLUE,bold]  %H:%M "

tmux set-window-option -g window-status-current-format "\
#[fg=$BLUE,bg=default]\
#[fg=$SURFACE0,bg=$BLUE,bold] #W \
#[fg=$BLUE,bg=default]"

tmux set-window-option -g window-status-format "\
#[fg=$SURFACE1,bg=default]\
#[fg=$TEXT,bg=$SURFACE1] #W \
#[fg=$SURFACE1,bg=default]"

tmux set-option -g pane-active-border-style "fg=$BLUE"
tmux set-option -g pane-border-style "fg=$SURFACE0"
