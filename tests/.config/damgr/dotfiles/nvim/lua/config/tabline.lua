local M = {}

function M.set_highlights()
	vim.api.nvim_set_hl(0, "MyBufInactive", { bg = "NvimDarkGrey3", fg = "Foreground" })
	vim.api.nvim_set_hl(0, "MyBufActive", { bold = true, bg = "NvimDarkGrey2", fg = "Foreground" })
	vim.api.nvim_set_hl(0, "MyBufSeparator", { fg = "NvimDarkGrey3", bg = "NvimDarkGrey3" })
end

local function get_icon(file_name, buf_name)
	local ok, devicons = pcall(require, "nvim-web-devicons")
	if not ok or not buf_name or buf_name == "" then
		return ""
	end
	local ext = vim.fn.fnamemodify(buf_name, ":e")
	local icon = devicons.get_icon(file_name, ext, { default = true })
	return icon and (icon .. " ") or ""
end

local function get_content(buf, buf_name, icon)
	if buf_name == "" then
		return tostring(buf) .. ": [No Name]"
	end
	local parts = vim.split(buf_name, "/", { plain = true })
	return tostring(buf) .. ": " .. icon .. parts[#parts]
end

function _G.tabline()
	local active_buf = vim.api.nvim_get_current_buf()
	local buffers = {}

	for _, buf in ipairs(vim.api.nvim_list_bufs()) do
		if vim.api.nvim_buf_is_loaded(buf) and vim.bo[buf].buflisted then
			local buf_name = vim.api.nvim_buf_get_name(buf)
			local file_name = (buf_name ~= "" and vim.fn.fnamemodify(buf_name, ":t")) or "[No Name]"
			local icon = get_icon(file_name, buf_name)
			local buf_content = get_content(buf, buf_name, icon)
			if buf == active_buf then
				table.insert(
					buffers,
					table.concat({
						"%#MyBufActive# ",
						buf_content,
						" %#MyBufSeparator#",
					})
				)
			else
				table.insert(
					buffers,
					table.concat({
						"%#MyBufInactive# ",
						buf_content,
						" %#MyBufSeparator#",
					})
				)
			end
		end
	end

	if #buffers == 0 then
		return ""
	end

	local tabline = table.concat(buffers)
	return tabline
end

function M.setup()
	M.set_highlights()

	vim.api.nvim_create_autocmd("ColorScheme", {
		pattern = { "*" },
		group = vim.api.nvim_create_augroup("TablineUserGroup", { clear = true }),
		callback = M.set_highlights,
	})

	vim.opt.showtabline = 2
	vim.opt.tabline = "%!v:lua.tabline()"
end

M.setup()
