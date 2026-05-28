{ config, pkgs, ... }:
let
  scripts = "/etc/nixos/config/programs/zsh/Scripts";
in
{
  home.packages = with pkgs; [ nautilus ffmpeg ];

  home.file.".local/share/nautilus/scripts/Compress 15MB" = {
    executable = true;
    text = ''
      #!/usr/bin/env bash
      export PATH="${pkgs.ffmpeg}/bin:$PATH"
      IFS=$'\n'
      for f in $NAUTILUS_SCRIPT_SELECTED_FILE_PATHS; do
        ${scripts}/resize_video.sh "$f" 15
      done
    '';
  };

  home.file.".local/share/nautilus/scripts/Compress 25MB" = {
    executable = true;
    text = ''
      #!/usr/bin/env bash
      export PATH="${pkgs.ffmpeg}/bin:$PATH"
      IFS=$'\n'
      for f in $NAUTILUS_SCRIPT_SELECTED_FILE_PATHS; do
        ${scripts}/resize_video.sh "$f"
      done
    '';
  };

  home.file.".local/share/nautilus/scripts/Crop 4:3" = {
    executable = true;
    text = ''
      #!/usr/bin/env bash
      export PATH="${pkgs.ffmpeg}/bin:$PATH"
      IFS=$'\n'
      for f in $NAUTILUS_SCRIPT_SELECTED_FILE_PATHS; do
        ${scripts}/crop43.sh "$f"
      done
    '';
  };
}
