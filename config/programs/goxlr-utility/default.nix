{ lib, ... }:
{
  home.activation.goxlrProfiles = lib.hm.dag.entryAfter [ "writeBoundary" ] ''
    mkdir -p ~/.local/share/goxlr-utility/{profiles,mic-profiles}
    cp -n ${./profiles}/*.goxlr ~/.local/share/goxlr-utility/profiles/ || true
    cp -n ${./mic-profiles}/*.goxlrMicProfile ~/.local/share/goxlr-utility/mic-profiles/ || true
  '';
}
