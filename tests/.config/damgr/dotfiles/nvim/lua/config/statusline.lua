local M = {}

function M.set_highlights()
	vim.api.nvim_set_hl(0, "StatusLine", { bg = "NvimDarkGrey3" })
	vim.api.nvim_set_hl(0, "MyGitAdd", { fg = "#00ff00" })
	vim.api.nvim_set_hl(0, "MyGitDel", { fg = "#ff0000" })
end

local function get_icon()
	local ok, devicons = pcall(require, "nvim-web-devicons")
	if not ok then
		return ""
	end
	local f = vim.fn.expand("%:t")
	local e = vim.fn.expand("%:e")
	local icon = devicons.get_icon(f, e, { default = true })
	return icon and (icon .. " ") or ""
end

local function get_diagnostics()
	if not vim.diagnostic then
		return ""
	end
	local d = vim.diagnostic.get(0)
	local e, w, i, h = 0, 0, 0, 0
	for _k, v in ipairs(d) do
		if v.severity == vim.diagnostic.severity.ERROR then
			e = e + 1
		elseif v.severity == vim.diagnostic.severity.WARN then
			w = w + 1
		elseif v.severity == vim.diagnostic.severity.INFO then
			i = i + 1
		elseif v.severity == vim.diagnostic.severity.HINT then
			h = h + 1
		end
	end
	local st = ""
	if e > 0 then
		st = st .. "e:" .. e .. " "
	end
	if w > 0 then
		st = st .. "w:" .. w .. " "
	end
	if i > 0 then
		st = st .. "i:" .. i .. " "
	end
	if h > 0 then
		st = st .. "h:" .. h .. " "
	end
	return st
end

local branch_timeout = 10000 -- check every x seconds
local cached_branch = ""
local last_branch_get = 0
local function get_git_branch()
	local now = vim.loop.now()
	if now - last_branch_get > branch_timeout then
		cached_branch = vim.fn.system("git branch --show-current 2>/dev/null | tr -d '\n'")
		last_branch_get = now
	end
	if cached_branch ~= "" then
		return cached_branch
	end
	return ""
end
local cached_diff = ""
local function get_git_diff()
	local active_buf = vim.api.nvim_get_current_buf()
	local buf_name = vim.api.nvim_buf_get_name(active_buf)
	if buf_name ~= "" then
		cached_diff = vim.fn.system(
			"git diff --numstat " .. buf_name .. ' 2>/dev/null | awk \'{print $1"+" " " $2"-"}\' | tr -d \'\n\''
		)
	else
		cached_diff = ""
	end
	if cached_diff ~= "" then
		return cached_diff
			:gsub("(%d+)%+", "%1%%#MyGitAdd#+%%#NvimDarkGrey3#")
			:gsub("(%d+)%-", "%1%%#MyGitDel#-%%#NvimDarkGrey3#")
	end
	return ""
end
vim.api.nvim_create_autocmd("BufWinEnter", {
	pattern = { "*" },
	group = vim.api.nvim_create_augroup("StatuslineUserGroup", { clear = true }),
	callback = function()
		cached_diff = get_git_diff()
	end,
})

function M.build()
	M.set_highlights()
	local st = ""

	local branch = get_git_branch()
	if branch ~= "" then
		st = st .. "(" .. branch .. ")" .. " "
	end

	local fnm = vim.fn.expand("%:.")
	if fnm ~= "" then
		st = st .. fnm .. " "
	end

	if cached_diff ~= "" then
		st = st .. cached_diff .. " "
	end

	st = st .. "%=" -- right indent

	local di = get_diagnostics()
	if di ~= "" then
		st = st .. di .. " "
	end

	local ft = vim.bo.filetype
	if ft ~= "" then
		st = st .. get_icon() .. ft .. " "
	end

	st = st .. "%l:%c" .. " " .. "%p%%"

	return st
end

vim.o.laststatus = 3 -- global statusline
vim.o.statusline = '%!v:lua.require("config.statusline").build()'
return M
