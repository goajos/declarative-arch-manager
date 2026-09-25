vim.diagnostic.config({
	undeline = true,
	severity_sort = true,
	update_in_insert = false,
	virtual_text = { true, current_line = false }, -- end of line text
	-- virtual_lines = { current_line = true }, -- underneath line text
})

vim.api.nvim_create_autocmd("InsertEnter", {
	pattern = { "*" },
	group = vim.api.nvim_create_augroup("DiagnosticsUserGroup", { clear = true }),
	callback = function()
		vim.diagnostic.enable(false)
	end,
})
vim.api.nvim_create_autocmd("InsertLeave", {
	pattern = { "*" },
	group = vim.api.nvim_create_augroup("DiagnosticsUserGroup", { clear = true }),
	callback = function()
		vim.diagnostic.enable(true)
	end,
})
vim.api.nvim_create_autocmd({ "CursorHold", "InsertLeave" }, {
	pattern = { "*" },
	group = vim.api.nvim_create_augroup("DiagnosticsUserGroup", { clear = true }),
	callback = function()
		local opts = {
			focusable = false,
			scope = "cursor",
			close_events = { "BufLeave", "WinLeave", "BufWinLeave", "CursorMoved", "InsertEnter" },
		}
		vim.diagnostic.open_float(nil, opts)
	end,
})
