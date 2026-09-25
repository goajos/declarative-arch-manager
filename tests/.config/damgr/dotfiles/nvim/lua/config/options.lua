local o = vim.o

o.background = "dark"
vim.api.nvim_set_hl(0, "Pmenu", { bg = "NvimDarkGrey2" }) -- sync background colors
vim.api.nvim_set_hl(0, "NormalFloat", { bg = "NvimDarkGrey2" }) -- sync background colors

o.termguicolors = true
o.cmdheight = 1
o.updatetime = 250 -- decrease update time
o.timeoutlen = 250 -- decrease the wait time for mapped sequences

-- blinking cursor in insert mode
vim.opt.guicursor = "n-v-c-sm:block"
vim.opt.guicursor:append("r-cr-o:hor20")
vim.opt.guicursor:append("i-ci-ve-t:block-blinkwait0-blinkon500-blinkoff500-TermCursor")

o.pumheight = 10 -- popupmenu height
o.pumblend = 0 -- popupmenu transparency
o.pumborder = "rounded" -- popupmenu border
o.winblend = 0 -- floating window transparency
o.winborder = "rounded" -- floating window border
-- popupmenu (also with 1 item) and no preselection
o.completeopt = "menu,menuone,noselect"

o.number = true
o.relativenumber = true

o.cursorline = true
-- o.signcolumn = "number"
o.signcolumn = "yes"
o.list = true

o.mouse = "a" --enable mouse for resizing
o.backspace = "indent,eol,start" -- better backspace behavior

o.shiftwidth = 2
o.tabstop = 2
o.softtabstop = 2
o.expandtab = true
o.breakindent = true

o.undofile = true
o.swapfile = false
o.backup = false
o.autoread = true -- auto reload files changed from outside

o.ignorecase = true
o.smartcase = true
o.inccommand = "split" -- live substitution with (%)s
vim.opt.iskeyword:append("-") -- dash part of words

o.grepprg = "rg --vimgrep"
vim.opt.path:append("**") -- include subdirectories in search

vim.wo.foldmethod = "expr"
o.fillchars = [[fold: ,foldopen:▼,foldclose:▶,foldsep: ,foldinner: ]]
o.foldcolumn = "auto"
o.foldenable = false

o.wildmenu = true
o.wildmode = "longest:full,full"

o.splitright = true
o.splitbelow = true

o.scrolloff = 10
o.sidescrolloff = 5

o.confirm = true -- raise dialog for confirmation
