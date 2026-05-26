{ config, pkgs, ... }:

{
  programs.neovim = {
    enable = true;
    defaultEditor = true;
    viAlias = true;
    vimAlias = true;

    extraPackages = with pkgs; [
      ripgrep
      fd
      lua-language-server
      pyright
      nil
      nixpkgs-fmt
    ];

    plugins = with pkgs.vimPlugins; [
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

  # Target only the specific file so the parent directory remains writable
  programs.neovim.extraLuaConfig = builtins.readFile ./nvim/init.lua;
}
