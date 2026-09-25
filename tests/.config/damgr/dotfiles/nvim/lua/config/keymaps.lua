local m = vim.keymap.set
local _opts = { noremap = true, silent = true }

m("n", "<tab>", ":bnext<cr>", { desc = "Next buffer" })
m("n", "<s-tab>", ":bprevious<cr>", { desc = "Previous buffer" })

m("n", "<c-h>", "<c-w>h", { remap = true })
m("n", "<c-j>", "<c-w>j", { remap = true })
m("n", "<c-k>", "<c-w>k", { remap = true })
m("n", "<c-l>", "<c-w>l", { remap = true })

-- <c-w>c = close window (not buffer!)
-- <c-w>p = previous window

m({ "n", "x" }, "gl", "$", { desc = "Goto end of line" })
m({ "n", "x" }, "gh", "^", { desc = "Goto start of line" })

m("n", "<leader>/", "<cmd>nohlsearch<cr>")
m("n", "n", "nzzzv", { desc = "Next search centered" })
m("n", "N", "Nzzzv", { desc = "Previous search centered" })

m("n", "<c-d>", "<c-d>zz", { desc = "Page down centered" })
m("n", "<c-u>", "<c-u>zz", { desc = "Page up centered" })

m({ "n", "x" }, "gy", '"+y', { desc = "Yank to system clipboard" })
m({ "n", "x" }, "gp", '"+p', { desc = "Paste from system clipboard" })
m({ "x" }, "<leader>p", '"_dP', { desc = "Paste without yanking" })

-- indent and stay in visual mode
m("x", "<", "<gv")
m("x", ">", ">gv")

m("t", "<esc>", "<c-\\><c-n>", { desc = "Leave terminal buffer insert mode" })
m("t", "<c-v><esc>", "<esc>", { desc = "Send <Esc> to terminal buffer" })

m("n", "<leader>w", ":write<cr>", { desc = "Write current buffer to file" })

m("n", "<leader>f", ":find ", { desc = "Toggle find" })
m("n", "<leader>g", ":grep ", { desc = "Toggle grep" })

local toggle_netrw = function()
	local active_buf = vim.api.nvim_get_current_buf()
	if vim.bo[active_buf].filetype == "netrw" then
		vim.api.nvim_buf_delete(active_buf, {})
		return
	end
	vim.cmd("Explore")
end
m("n", "<leader>e", toggle_netrw, { desc = "Toggle netrw explorer" })

-- local chat_is_open = false
-- local toggle_copilot_chat = function()
--   if chat_is_open then
--     vim.cmd("CopilotChatClose")
--     chat_is_open = false
--   else
--     vim.cmd("CopilotChat")
--     chat_is_open = true
--   end
-- end
-- m("n", "<leader>c", toggle_copilot_chat, { desc = "Toggle copilot chat" })

-- dap debug
-- m("n", "<leader>d", function()
-- 	require("osv").launch({ port = 8080 })
-- end, { desc = "Launch osv debugger" })
local sidebar_is_open = false
local cur_sidebar
local toggle_dap_sidebar = function()
	if sidebar_is_open then
		cur_sidebar.close()
		sidebar_is_open = false
	else
		local widgets = require("dap.ui.widgets")
		cur_sidebar = widgets.sidebar(widgets.scopes)
		cur_sidebar.open()
		sidebar_is_open = true
	end
end
m("n", "<leader>ds", toggle_dap_sidebar, { desc = "Toggle the dap sidebar" })
local repl_is_open = false
local repl
local toggle_dap_repl = function()
	if repl_is_open then
		repl.close()
		repl_is_open = false
	else
		repl = require("dap.repl")
		repl.open()
		repl_is_open = true
	end
end
m("n", "<leader>dr", toggle_dap_repl, { desc = "Toggle the dap repl" })
m("n", "<f5>", ":DapContinue<cr>", { desc = "Continue the dap debugger" })
-- m("n", "<f17>", ":DapTerminate<cr>", { desc = "Terminate the dap debugger" })
m("n", "<s-f5>", ":DapTerminate<cr>", { desc = "Terminate the dap debugger" })
m("n", "<f9>", ":DapToggleBreakpoint<cr>", { desc = "Toggle breakpoint on current line" })
m("n", "<f10>", ":DapStepOver<cr>", { desc = "Step over" })
m("n", "<f11>", ":DapStepInto<cr>", { desc = "Step into" })
-- m("n", "<f23>", ":dapstepout<cr>", { desc = "step out" })
m("n", "<s-f11>", ":dapstepout<cr>", { desc = "step out" })
