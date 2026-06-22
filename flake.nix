{
  description = "mahiru's NixOS configuration";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";
    
    # Hyprland Pin
    nixpkgs-hyprland.url = "github:NixOS/nixpkgs/3e41b24abd260e8f71dbe2f5737d24122f972158";
    # Quickshell Pin
    nixpkgs-quickshell.url = "github:NixOS/nixpkgs/8fd9daa3db09ced9700431c5b7ad0e8ba199b575";
    nixpkgs-swww.url = "github:NixOS/nixpkgs/8fd9daa3db09ced9700431c5b7ad0e8ba199b575";
    nixpkgs-bwrap.url = "github:NixOS/nixpkgs/8fd9daa3db09ced9700431c5b7ad0e8ba199b575";
    nixpkgs-librewolf.url = "github:NixOS/nixpkgs/8fd9daa3db09ced9700431c5b7ad0e8ba199b575";
    nixpkgs-unstable.url = "github:NixOS/nixpkgs/nixos-unstable";
    
    home-manager = {
      url = "github:nix-community/home-manager/release-26.05";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    
    nix-flatpak.url = "github:gmodena/nix-flatpak/latest";
  };

outputs = { self, nixpkgs, nixpkgs-unstable, nixpkgs-hyprland, nixpkgs-quickshell, home-manager, nix-flatpak, ... }@inputs:
  let
    unstable = import nixpkgs-unstable {
      system = "x86_64-linux";
      config.allowUnfree = true;
    };
  in {
    nixosConfigurations.mahiru = nixpkgs.lib.nixosSystem {
      system = "x86_64-linux";
      
      specialArgs = { inherit inputs unstable; };
      
      modules = [
        ./configuration.nix
        home-manager.nixosModules.home-manager
        nix-flatpak.nixosModules.nix-flatpak
      ];
    };
  };
}
