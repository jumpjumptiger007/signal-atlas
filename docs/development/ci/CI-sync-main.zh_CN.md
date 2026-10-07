<p align="right">
  <strong>简体中文</strong> · <a href="CI-sync-main.md">English</a>
</p>

# 上游漂移检查

`.github/workflows/sync-main.yml` 检查 `FoloToy/ai-passport:main` 与
Signal Atlas `main` 的差异。工作流每天 UTC 00:00 定时运行，也支持手动触发。
Signal Atlas 是独立仓库；该工作流不会让产品分支与 upstream 同步。

工作流使用 `contents: read`，检出 Signal Atlas `main`，获取 upstream `main`，
并在 workflow summary 中报告双方 SHA、ahead/behind 数量、共同祖先以及共同祖先
之后的上游提交。当 upstream 有 Signal Atlas 尚未包含的提交时，工作流会发出需要
人工 review 的 warning。它不会创建 PR，也不会写入任何分支或 remote。

工作流不会 merge、rebase、cherry-pick、reset 或 push。上游漂移 warning 仅供参考：
请审阅相关改动，并明确决定是否将其适配到 Signal Atlas，或另行准备贡献给
`FoloToy/ai-passport` 的 PR。参见[仓库与上游维护流程](../../fork-guide.zh_CN.md)。

checkout 显式设置 `ref: main`，因此即使从其他分支手动触发，也始终检查
Signal Atlas 产品主分支。checkout 不保留凭证；upstream remote 仅在临时的
Actions runner 中添加。
