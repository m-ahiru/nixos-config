{
  description = "mahiru's NixOS configuration";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";
    
    # Hyprland Pin
    nixpkgs-hyprland.url = "github:NixOS/nixpkgs/8fd9daa3db09ced9700431c5b7ad0e8ba199b575";
    
    home-manager = {
      url = "github:nix-community/home-manager/release-25.11";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    
    nix-flatpak.url = "github:gmodena/nix-flatpak/latest";
  };

  outputs = { self, nixpkgs, nixpkgs-hyprland, home-manager, nix-flatpak, ... }@inputs: {
    nixosConfigurations.mahiru = nixpkgs.lib.nixosSystem {
      system = "x86_64-linux";
      
      specialArgs = { inherit inputs; };
      
      modules = [
        ./configuration.nix
        home-manager.nixosModules.home-manager
        nix-flatpak.nixosModules.nix-flatpak
      ];
    };
  };
}
