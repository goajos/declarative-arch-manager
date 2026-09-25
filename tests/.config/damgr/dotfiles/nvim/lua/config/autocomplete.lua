if vim.fn.executable("rg") == 1 then
  function _G.RgFindFiles(cmdarg, _cmdcomplete)
    local fnames = vim.fn.systemlist('rg --files --hidden --color=never --glob="!.git"')
    if #cmdarg == 0 then
      return fnames
    else
      return vim.fn.matchfuzzy(fnames, cmdarg)
    end
  end

  vim.opt.findfunc = "v:lua.RgFindFiles"
end

local function debounce(func, delay_ms)
  local timer = nil
  local running = nil
  return function(...)
    if not running then
      timer = assert(vim.uv.new_timer())
    end
    local argv = { ... }
    assert(timer):start(delay_ms, 0, function()
      assert(timer):stop()
      running = nil
      func(unpack(argv, 1, table.maxn(argv)))
    end)
  end
end

-- <c-y> to accept the current wildmenu selection
-- <c-w> to delete the current cmdline words
vim.api.nvim_create_autocmd({ "CmdlineChanged", "CmdlineLeave" }, {
  pattern = { "*" },
  group = vim.api.nvim_create_augroup("AutocompleteUserGroup", { clear = true }),
  callback = debounce(
    vim.schedule_wrap(function(cargs)
      local function should_enable_autocomplete()
        local cmdline_cmd = vim.fn.split(vim.fn.getcmdline(), " ")[1]
        local cmdline_type = vim.fn.getcmdtype()
        local compl = vim.fn.getcmdcompltype()

        return cmdline_type == "/"
            or cmdline_type == "?"
            or (cmdline_type == ":" and (cmdline_cmd == "find" or compl == "help" or compl == "buffer"))
      end

      if cargs.event == "CmdlineChanged" and should_enable_autocomplete() then
        vim.opt.wildmode = "noselect:lastused,full"
        vim.fn.wildtrigger()
      end

      if cargs.event == "CmdlineLeave" then
        vim.opt.wildmode = "longest:full,full"
      end
    end),
    250
  ),
})
