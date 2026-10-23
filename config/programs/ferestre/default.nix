{ pkgs, ... }:

# Ferestre (Xbox/GDK-Spiele unter Linux) in einer eigenen FHS-Umgebung.
# appimage-run hat kein 32-bit und keine Vulkan-ICDs, beides braucht Ferestre.
# Erwartet das AppImage unter ~/AppImages/ferestre.appimage und entpackt es
# beim ersten Start (und nach jedem Update der Datei) nach ~/.local/share/ferestre-app.
let
  fhsArgs = pkgs.appimageTools.defaultFhsEnvArgs;

  ferestre = pkgs.buildFHSEnv (fhsArgs // {
    name = "ferestre";
    multiArch = true;

    targetPkgs = p: (fhsArgs.targetPkgs p) ++ (with p; [
      libadwaita
      gtk4
      webkitgtk_4_1
      libsoup_3
      glib-networking
    ]);

    multiPkgs = p: (fhsArgs.multiPkgs p) ++ (with p; [
      mesa        # Vulkan-ICDs (radv) für 64- und 32-bit nach /usr/share/vulkan
      bzip2
      libva
    ]);

    runScript = pkgs.writeShellScript "ferestre-run" ''
      img="$HOME/AppImages/ferestre.appimage"
      dir="$HOME/.local/share/ferestre-app"

      if [ ! -f "$img" ]; then
        echo "ferestre: $img nicht gefunden" >&2
        exit 1
      fi

      if [ ! -x "$dir/AppRun" ] || [ "$img" -nt "$dir/AppRun" ]; then
        # Offset des squashfs = Ende der ELF-Section-Header des Runtimes
        shoff=$(od -An -t u8 -j 40 -N 8 "$img" | tr -d ' ')
        shentsize=$(od -An -t u2 -j 58 -N 2 "$img" | tr -d ' ')
        shnum=$(od -An -t u2 -j 60 -N 2 "$img" | tr -d ' ')
        offset=$(( shoff + shentsize * shnum ))

        rm -rf "$dir"
        ${pkgs.squashfsTools}/bin/unsquashfs -q -n -o "$offset" -d "$dir" "$img" >/dev/null || exit 1
        touch "$dir/AppRun"
      fi

      exec "$dir/AppRun" "$@"
    '';
  });
in
{
  home.packages = [ ferestre ];
}
