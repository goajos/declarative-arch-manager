vim.pack.add({ "https://github.com/stevearc/conform.nvim" })

require("conform").setup({
	formatters_by_ft = {
		bash = { "shfmt" },
		c = { "clang-format" },
		lua = { "stylua" },
		python = { "ruff" },
		sh = { "shfmt" },
	},
	format_on_save = {
		timeout_ms = 500,
		lsp_format = "fallback",
	},
})

vim.o.formatexpr = "v:lua.require'conform'.formatexpr()"
