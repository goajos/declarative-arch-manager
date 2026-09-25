-- grn = rename symbol
-- gra = code actions
-- grr = find references
-- gri = go to implementation
-- grt = go to type definition
-- gO  = document symbols

local lsp_keymaps = {
	{ keys = "K", func = vim.lsp.buf.hover, desc = "Hover documentation", has = "hoverProvider" },
	{ keys = "gd", func = vim.lsp.buf.definition, desc = "Goto definition", has = "definitionProvider" },
}

local completion = vim.g.completion_mode or "native"
vim.api.nvim_create_autocmd("LspAttach", {
	pattern = { "*" },
	group = vim.api.nvim_create_augroup("LspUserGroup", { clear = true }),
	callback = function(cargs)
		local client = vim.lsp.get_client_by_id(cargs.data.client_id)
		local buf = cargs.buf
		if client then
			if completion == "native" and client:supports_method("textDocument/completion") then
				vim.lsp.completion.enable(true, client.id, buf, { autotrigger = true })
			end

			if client:supports_method("textDocument/inlayHint") then
				vim.lsp.inlay_hint.enable(true, { bufnr = buf })
			end

			for _, km in ipairs(lsp_keymaps) do
				if not km.has or client.server_capabilities[km.has] then
					vim.keymap.set("n", km.keys, km.func, { buffer = buf, desc = "LSP: " .. km.desc })
				end
			end
		end
	end,
})

vim.lsp.enable({
	"bashls",
	"clangd",
	"lua_ls",
	"ty",
})
