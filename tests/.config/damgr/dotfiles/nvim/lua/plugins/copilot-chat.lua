vim.pack.add({ "https://github.com/nvim-lua/plenary.nvim" })
vim.pack.add({ "https://github.com/github/copilot.vim" })
vim.pack.add({ "https://github.com/CopilotC-Nvim/CopilotChat.nvim" })

vim.g.copilot_enabled = false
require("CopilotChat").setup({
  model = "claude-haiku-4.5",
  auto_insert_mode = false,
})
