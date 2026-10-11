{ config, ... }:

{
  # Unpacked extension that recolors claude.ai with matugen colors.
  # Out-of-store symlink so matugen can write colors.json into it.
  # Load once: chrome://extensions -> Developer mode -> Load unpacked
  #            -> ~/.local/share/claude-matugen
  xdg.dataFile."claude-matugen".source =
    config.lib.file.mkOutOfStoreSymlink "/etc/nixos/config/programs/chromium/claude-matugen";
}
