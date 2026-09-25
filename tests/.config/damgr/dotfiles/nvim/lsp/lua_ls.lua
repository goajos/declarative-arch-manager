return {
	cmd = { "lua-language-server" },
	filetypes = { "lua" },
	root_markers = { ".git" },
	settings = {
		Lua = {
			workspace = {
				library = vim.api.nvim_get_runtime_file("", true),
				checkThirdParty = false,
			},
			codelens = { enable = true },
			hint = { enable = true, semicolon = "Disable" },
			runtime = { version = "LuaJIT" },
			diagnostics = { globals = { "vim" } },
		},
	},
}
