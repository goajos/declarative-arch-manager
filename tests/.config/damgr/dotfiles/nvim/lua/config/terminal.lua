local M = {}

local function render_terminal_buffer()
	local width = math.floor(vim.o.columns * 0.8)
	local height = math.floor(vim.o.lines * 0.8)
	local row = math.floor((vim.o.lines - height) / 2)
	local col = math.floor((vim.o.columns - width) / 2)
	M.state.win = vim.api.nvim_open_win(M.state.buf, true, {
		relative = "editor",
		width = width,
		height = height,
		row = row,
		col = col,
		style = "minimal",
		border = "rounded",
	})
end

local function toggle_terminal()
	if M.state.is_open and M.state.win and vim.api.nvim_win_is_valid(M.state.win) then
		vim.api.nvim_win_close(M.state.win, false)
		M.state.is_open = false
		return
	end

	if not M.state.buf or not vim.api.nvim_buf_is_valid(M.state.buf) then
		M.state.buf = vim.api.nvim_create_buf(false, true)
		vim.bo[M.state.buf].bufhidden = "hide"
		render_terminal_buffer()
		vim.fn.jobstart("bash", { term = true })
	else
		render_terminal_buffer()
	end

	M.state.is_open = true
	vim.cmd("startinsert")
end

function M.setup()
	M.state = { buf = nil, win = nil, is_open = false }

	vim.keymap.set("n", "<leader>t", toggle_terminal, { desc = "Toggle floating terminal" })
	vim.keymap.set("t", "<esc>", "<c-\\><c-n>", { desc = "Leave terminal insert mode" })
	vim.keymap.set("t", "<c-v><esc>", "<esc>", { desc = "Send <esc> to terminal" })
end

M.setup()
