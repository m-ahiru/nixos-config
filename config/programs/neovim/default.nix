{ config, pkgs, inputs, ... }:
let
  nvimPkgs = inputs.nixpkgs-neovim.legacyPackages.${pkgs.system};
in
{
  programs.neovim = {
    enable = true;
    defaultEditor = true;
    viAlias = true;
    vimAlias = true;
    package = nvimPkgs.neovim-unwrapped;
    extraPackages = with nvimPkgs; [
      ripgrep
      fd
      lua-language-server
      pyright
      nil
      nixpkgs-fmt
    ];
    plugins = with nvimPkgs.vimPlugins; [
      catppuccin-nvim
      nvim-web-devicons
      alpha-nvim
      plenary-nvim
      nui-nvim
      searchbox-nvim
      nvim-treesitter.withAllGrammars 
      lualine-nvim
      bufferline-nvim
      indent-blankline-nvim
      gitsigns-nvim
      which-key-nvim
      nvim-tree-lua
      telescope-nvim
      telescope-ui-select-nvim
      nvim-autopairs
      comment-nvim
      nvim-lspconfig
      nvim-cmp
      cmp-nvim-lsp
      cmp-buffer
      cmp-path
      cmp-cmdline
      luasnip
      cmp_luasnip
      friendly-snippets
    ];
  };
  programs.neovim.extraLuaConfig = builtins.readFile ./nvim/init.lua;
}
