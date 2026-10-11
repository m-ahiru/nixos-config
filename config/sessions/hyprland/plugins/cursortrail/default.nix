# Built with the same nixpkgs as the pinned Hyprland, so the plugin always matches it.
{
  lib,
  cmake,
  mkHyprlandPlugin,
}:
mkHyprlandPlugin {
  pluginName = "cursortrail";
  version = "1.0";
  src = ./.;

  nativeBuildInputs = [ cmake ];

  meta = {
    description = "osu!-style glowing cursor trail for Hyprland";
    license = lib.licenses.mit;
    platforms = lib.platforms.linux;
  };
}
