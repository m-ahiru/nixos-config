-- FIX: Auto-create undo directory to prevent E828 error
vim.opt.packpath:prepend(vim.fn.expand("~/.local/share/nvim/site"))
vim.cmd("packloadall!")
local undodir = vim.fn.stdpath("state") .. "/undo"
if vim.fn.isdirectory(undodir) == 0 then
  vim.fn.mkdir(undodir, "p")
end
vim.opt.undodir = undodir
vim.opt.undofile = true

vim.opt.number = true
vim.opt.relativenumber = true
vim.opt.mouse = 'a'
vim.opt.clipboard = 'unnamedplus'
vim.opt.breakindent = true
vim.opt.ignorecase = true
vim.opt.smartcase = true
vim.opt.termguicolors = true
vim.opt.signcolumn = 'yes'
vim.g.mapleader = ' '
vim.g.maplocalleader = ' '
vim.opt.smartindent = true
vim.opt.autoindent = true
vim.opt.expandtab = true
vim.opt.shiftwidth = 2
vim.opt.tabstop = 2

-- DYNAMIC THEME LOGIC
_G.reload_matugen_colors = function()
  -- vim.schedule ensures this massive UI update runs safely on the main event loop
  -- otherwise RPC calls can silently fail to update the screen.
  vim.schedule(function()
    local matugen_path = vim.fn.stdpath("config") .. "/matugen_colors.lua"
    local overrides = {}
    local mg = {}   -- raw matugen colors, used for notify background

    if vim.fn.filereadable(matugen_path) == 1 then
      -- loadfile is safer than dofile here as it compiles the chunk without executing it immediately
      local chunk = loadfile(matugen_path)
      if chunk then
        local colors = chunk()
        if type(colors) == "table" then
          -- Cover both bases: 'all' and the specific 'mocha' flavour
          overrides = { all = colors, mocha = colors }
          mg = colors
        end
      end
    end

    -- FIX: Only clear catppuccin. Do NOT clear lualine, or it will leak highlight groups!
    for k, _ in pairs(package.loaded) do
      if k:match("^catppuccin") then
        package.loaded[k] = nil
      end
    end

    -- Nuke Neovim's existing highlights
    vim.cmd("hi clear")
    if vim.fn.exists("syntax_on") then
      vim.cmd("syntax reset")
    end
    vim.g.colors_name = nil

    require("catppuccin").setup({
      flavour = "mocha",
      transparent_background = true,
      compile = { enabled = false }, -- MUST be false for dynamic overrides
      color_overrides = overrides,
      integrations = {
        cmp = true,
        gitsigns = true,
        nvimtree = true,
        treesitter = true,
        bufferline = true,
        noice = true,
        telescope = { enabled = true },
        indent_blankline = { enabled = true },
        native_lsp = {
          enabled = true,
          underlines = {
            errors = { "undercurl" },
            hints = { "undercurl" },
            warnings = { "undercurl" },
            information = { "undercurl" },
          },
        },
      },
    })
    
    -- Re-apply the colorscheme
    vim.cmd("colorscheme catppuccin")

    -- Notify background dynamically from matugen (base = surface).
    -- Fallback chain guards against a missing key so no warning/crash occurs.
    local ok_notify, notify = pcall(require, "notify")
    if ok_notify then
      notify.setup({
        background_colour = mg.base or mg.mantle or mg.crust or "#1e1e2e",
      })
    end

    -- Reload lualine dynamically (safely updates without module clearing)
    local ok_lualine, lualine = pcall(require, "lualine")
    if ok_lualine then
      lualine.setup { options = { theme = 'auto' } }
    end
    
    -- Force Neovim to redraw
    vim.cmd("redraw!")

    -- Provide visual confirmation that the RPC command successfully triggered the function
    vim.notify("Matugen colors reloaded!", vim.log.levels.INFO)
  end)
end

-- Initialize the colors immediately on startup
_G.reload_matugen_colors()

-- PLUGIN CONFIGURATIONS
vim.defer_fn(function()
  require('nvim-treesitter.config').setup {
    highlight = { enable = true },
    indent = { enable = true },
  }
end, 0)

require('searchbox').setup({
  popup = {
    position = { row = '50%', col = '50%' },
    size = 50,
    border = {
      style = 'rounded',
      text = { top = ' Search ', top_align = 'center' },
    },
  }
})

require("ibl").setup()
require('gitsigns').setup()
require('nvim-autopairs').setup({})
require('Comment').setup()
require('which-key').setup()

-- NOICE: zentrierte Cmdline + Suche, LSP-Popups, Notifications
require("noice").setup({
  presets = {
    bottom_search = false,        -- Suche mittig statt unten
    command_palette = true,       -- : + Popupmenu zusammen zentriert
    long_message_to_split = true, -- lange Messages in Split
    lsp_doc_border = true,        -- Border um Hover/Signature
  },
})

local alpha = require('alpha')
local dashboard = require('alpha.themes.dashboard')

dashboard.section.buttons.val = {
  dashboard.button('f', '󰱼 ❯ Find File',     '<cmd>Telescope find_files<cr>'),
  dashboard.button('r', '󱧶 ❯ Recent Files',   '<cmd>Telescope oldfiles<cr>'),
  dashboard.button('n', ' ❯ NixOS Config',   '<cmd>Telescope find_files cwd=/etc/nixos<cr>'),
  dashboard.button('t', ' ❯ New File', function()
  vim.ui.input({ prompt = 'New file: ', default = vim.fn.expand('~/') }, function(input)
    if input and input ~= '' then
      -- mkdir -p für alle parent directories
      vim.fn.mkdir(vim.fn.fnamemodify(input, ':h'), 'p')
      vim.cmd('edit ' .. input)
    end
  end)
end),
  dashboard.button('q', '󰱝 ❯ Quit',           '<cmd>qa<cr>'),
}

    dashboard.section.header.val = {
      [[ ⠀⠀⠀⠀⠀⠀⠀⠀⢀⣀⣀⣀⣀⣀⣀⠀⡀⢀⠀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀ ]],
      [[ ⣶⣶⣶⣶⣿⣿⣿⣿⣿⣿⣿⣿⣿⠟⣽⠃⠀⠀⠀⢼⠻⣿⣿⣟⣿⣿⣿⣿⣶⣶⣶⣶⣤⣤⣤⣤⣤ ]],
      [[ ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠏⠀⠀⠛⡶⢶⢺⠁⠀⠈⢿⣿⣿⣿⣿⣿⣿⣏⣿⣿⣿⣿⣿⣿⣿ ]],
      [[ ⣯⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡟⠀⠀⣤⠀⣀⣠⡛⣣⡀⠀⠈⢿⣿⣿⣻⣏⣿⣿⣿⣿⣿⣿⣟⣿⠿ ]],
      [[ ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠃⠀⠀⣳⣶⣿⣿⣷⣾⠱⠀⠀⠊⢿⠿⠿⢛⣽⣿⡿⢿⣿⣟⠿⠿⠿ ]],
      [[ ⠉⠉⠉⠛⠛⠛⠋⠛⠛⠛⣧⠀⡀⠀⠀⢿⣿⣿⡿⠃⠀⠀⠀⠀⠀⠀⠀⠀⠁⠀⠀⠀⠀⠅⢀⢀⡀ ]],
      [[ ⠔⠄⢀⡀⠀⠀⠀⠄⠐⠸⠿⡀⠀⠀⠀⢘⣿⢷⣇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⠀⠀⠰⣠⣇ ]],
      [[ ⣷⣆⣴⣮⢻⡲⡲⠀⠁⠀⠀⠀⠀⠀⠀⠹⡿⠘⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⣀⡘⢷⣏ ]],
      [[ ⣿⣿⣿⣗⠿⢈⠁⡀⠀⠁⠀⠀⠀⠀⠀⠀⠉⠀⠀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠠⢀⠄⠀⠄⠈⢿⣮⢿ ]],
      [[ ⣿⣟⡿⣾⠀⠀⠀⠀⠀⠀⠀⢀⡤⠄⠀⠀⠀⠀⠸⠁⢠⣦⣤⢀⠀⠀⠀⠀⠀⠀⠀⠀⠐⠀⠈⣿⠀ ]],
      [[ ⣿⣿⠏⠁⢀⡇⠀⠀⠀⠀⠀⠀⡄⠀⠀⠀⠘⡏⣷⣵⡻⠃⠄⢴⣆⠀⠀⠀⠀⠀⠀⠀⠰⠀⣆⣷⣿ ]],
      [[ ⣿⡿⣻⠗⠀⢠⠀⠀⠀⠀⠀⠃⠀⠀⠀⠀⢠⣤⣄⢰⣶⢯⣤⡈⠋⠀⠀⠀⠀⠀⠀⠀⠀⠆⠀⣿⣼ ]],
    }


alpha.setup(dashboard.config)

require("nvim-tree").setup({
  filters = { dotfiles = false },
  view = { width = 30 }
})

local telescope = require('telescope')
telescope.setup {
  extensions = {
    ["ui-select"] = { require("telescope.themes").get_dropdown {} }
  }
}
pcall(telescope.load_extension, 'ui-select')

local builtin = require('telescope.builtin')
vim.keymap.set('n', '<leader>ff', builtin.find_files, { desc = 'Find Files' })
vim.keymap.set('n', '<leader>fg', builtin.live_grep, { desc = 'Live Grep' })
vim.keymap.set('n', '<leader>fb', builtin.buffers, { desc = 'Find Buffers' })
vim.keymap.set('n', '<C-n>', ':NvimTreeToggle<CR>', { desc = 'Toggle File Explorer' })

require("bufferline").setup{
  options = {
    mode = "buffers",
    diagnostics = "nvim_lsp",
    separator_style = "slant",
    offsets = {
      {
        filetype = "NvimTree",
        text = "File Explorer",
        text_align = "left",
        separator = true
      }
    },
  }
}

vim.keymap.set("n", "<Tab>", ":BufferLineCycleNext<CR>", { silent = true })
vim.keymap.set("n", "<S-Tab>", ":BufferLineCyclePrev<CR>", { silent = true })
vim.keymap.set("n", "<leader>x", ":bdelete<CR>", { silent = true, desc = "Close Buffer" })

local cmp = require 'cmp'
local luasnip = require 'luasnip'
 
require("luasnip.loaders.from_vscode").lazy_load()

-- NOICE übernimmt jetzt / und ? als zentriertes Popup.
-- Die alten searchbox-Mappings sind daher deaktiviert:
-- vim.keymap.set('n', '/', ':SearchBoxIncSearch<CR>', { desc = 'Search' })
-- vim.keymap.set('n', '?', ':SearchBoxIncSearch reverse=true<CR>', { desc = 'Search reverse' })
vim.keymap.set('n', '<leader>h', '<cmd>Alpha<cr>', { desc = 'Home Screen' })

-- Terminal toggle
local term_buf = -1
local term_win = -1

vim.keymap.set('n', '<leader>t', function()
  if vim.api.nvim_win_is_valid(term_win) then
    vim.api.nvim_win_hide(term_win)
  elseif vim.api.nvim_buf_is_valid(term_buf) then
    vim.cmd('botright 15split')
    term_win = vim.api.nvim_get_current_win()
    vim.api.nvim_win_set_buf(term_win, term_buf)
  else
    vim.cmd('botright 15split | terminal')
    term_buf = vim.api.nvim_get_current_buf()
    term_win = vim.api.nvim_get_current_win()
  end
  vim.cmd('startinsert')
end)

-- Mit Escape zurück in den normalen Modus im Terminal
vim.keymap.set('t', '<Esc>', '<C-\\><C-n>', { noremap = true })
cmp.setup {
  snippet = {
    expand = function(args)
      luasnip.lsp_expand(args.body)
    end,
  },
  mapping = cmp.mapping.preset.insert {
    ['<C-n>'] = cmp.mapping.select_next_item(),
    ['<C-p>'] = cmp.mapping.select_prev_item(),
    ['<C-d>'] = cmp.mapping.scroll_docs(-4),
    ['<C-f>'] = cmp.mapping.scroll_docs(4),
    ['<C-Space>'] = cmp.mapping.complete(),
    ['<CR>'] = cmp.mapping.confirm {
      behavior = cmp.ConfirmBehavior.Replace,
      select = true,
    },
    ['<Tab>'] = cmp.mapping(function(fallback)
      if cmp.visible() then
        cmp.select_next_item()
      elseif luasnip.expand_or_jumpable() then
        luasnip.expand_or_jump()
      else
        fallback()
      end
    end, { 'i', 's' }),
  },
  sources = {
    { name = 'nvim_lsp' },
    { name = 'luasnip' },
    { name = 'buffer' },
    { name = 'path' },
  },
}

-- LSP NATIVE SETUP
local capabilities = require('cmp_nvim_lsp').default_capabilities()
 
local function setup_server(server_name, config)
  local ok, server_config = pcall(require, "lspconfig.server_configurations." .. server_name)
  if not ok then return end
  
  local default_config = server_config.default_config
  local final_config = vim.tbl_deep_extend("force", default_config, config or {})
  final_config.capabilities = vim.tbl_deep_extend("force", final_config.capabilities or {}, capabilities)

  vim.api.nvim_create_autocmd("FileType", {
     pattern = final_config.filetypes,
     callback = function(args)
	local instance_config = vim.tbl_deep_extend("force", {}, final_config)

	local root_dir = final_config.root_dir
	if type(root_dir) == "function" then
		root_dir = root_dir(args.file)
    	end
    	instance_config.root_dir = root_dir or vim.fs.dirname(args.file)

    	vim.lsp.start(instance_config)
     end,
  })
end

setup_server("pyright", {})
setup_server("nil_ls", {})
setup_server("lua_ls", {
  settings = {
    Lua = {
      runtime = { version = 'LuaJIT' },
      globals = { 'vim' },
      workspace = { checkThirdParty = false },
      telemetry = { enable = false },
    },
  }
})

-- FILE WATCHER: Automatically reload when Matugen updates the file
local uv = vim.uv or vim.loop -- Compat for Nvim 0.9 and 0.10+
local matugen_path = vim.fn.stdpath("config") .. "/matugen_colors.lua"

local watcher = uv.new_fs_event()
local reload_timer = nil

watcher:start(matugen_path, {}, vim.schedule_wrap(function(err, filename, events)
  if not err then
    -- FIX: Debounce the reload to prevent rapid execution which leads to E849
    if reload_timer then
      reload_timer:stop()
      reload_timer:close()
    end
    
    reload_timer = uv.new_timer()
    reload_timer:start(100, 0, vim.schedule_wrap(function()
      _G.reload_matugen_colors()
      if reload_timer then
        reload_timer:stop()
        reload_timer:close()
        reload_timer = nil
      end
    end))
  end
end))

-- SIGNAL LISTENER: Trigger reload when receiving SIGUSR1 from the OS
vim.api.nvim_create_autocmd("Signal", {
  pattern = "SIGUSR1",
  callback = function()
    _G.reload_matugen_colors()
  end,
})
