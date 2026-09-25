vim.pack.add({
	{
		src = "https://github.com/nvim-treesitter/nvim-treesitter",
		version = "main",
	},
	{
		src = "https://github.com/nvim-treesitter/nvim-treesitter-textobjects",
		version = "main",
	},
})

require("nvim-treesitter").setup({})
require("nvim-treesitter").install({
	"bash",
	"c",
	"cmake",
	"cpp",
	"css",
	"csv",
	"cuda",
	"diff",
	"dockerfile",
	"gitcommit",
	"gitignore",
	"html",
	"ini",
	"javascript",
	"json",
	"kdl",
	"lua",
	"luadoc",
	"make",
	"markdown",
	"markdown_inline",
	"python",
	"query",
	"sql",
	"toml",
	"typescript",
	"vim",
	"vimdoc",
	"xml",
	"yaml",
})

-- TODO: how to use textobjects?
require("nvim-treesitter-textobjects").setup({})

vim.wo.foldexpr = "v:lua.vim.treesitter.foldexpr()"
vim.bo.indentexpr = "v:lua.require'nvim-treesitter'.indentexpr()"

vim.api.nvim_create_autocmd("FileType", {
	pattern = { "*" },
	group = vim.api.nvim_create_augroup("TreesitterUserGroup", { clear = true }),
	callback = function()
		local filetype = vim.bo.filetype
		if filetype and filetype ~= "" then
			local success = pcall(function()
				vim.treesitter.start()
			end)
			if not success then
				return
			end
		end
	end,
})
