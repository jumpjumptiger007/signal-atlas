<p align="right">
  <strong>简体中文</strong> · <a href="fork-guide.md">English</a>
</p>

# 仓库与上游维护流程

Signal Atlas 是独立的产品仓库，不是 `FoloToy/ai-passport` 的 GitHub fork。
本仓库的 `main` 是 Signal Atlas 产品主分支，不要求与 upstream 的 `main`
镜像或保持同步。

## 仓库角色

```text
origin   https://github.com/jumpjumptiger007/signal-atlas.git
upstream https://github.com/FoloToy/ai-passport.git
```

- `origin` 保存 Signal Atlas 产品代码、文档和发布历史。
- `upstream` 是 FoloToy AI Passport 中可复用 BSP、hardware、build 和
  engineering 改进的来源。
- Signal Atlas 产品代码留在本仓库。检查并 review 上游变化后，再明确决定
  是否将相关改进适配到本项目。
- `.github/workflows/sync-main.yml` 只检查 upstream 漂移，不会 merge、rebase、
  cherry-pick、reset、创建 PR 或 push。

## 检查上游与贡献

定期或手动运行的 upstream drift check 会报告当前
`FoloToy/ai-passport:main` SHA、它与 Signal Atlas `main` 的 ahead/behind
关系，以及共同祖先之后的上游提交。上游领先只表示需要人工 review，不代表
自动集成。审阅相关改动后，再明确决定是否将通用 BSP、hardware、build 或
engineering 改进适配到 Signal Atlas。

有益于 AI Passport 用户的通用改进可以通过 PR 贡献给
`FoloToy/ai-passport`。Signal Atlas 产品代码和产品专属行为继续保留在本仓库。
如果向上游提交 PR 需要 GitHub fork，请单独创建或使用 contributor fork 和
branch；不要把 Signal Atlas 本身转成 fork。`origin` 只用于 Signal Atlas 仓库。

准备上游贡献时，确认 PR 的 base repository 和 contributor head repository，遵守
[`docs/contribution/commit-and-pr.md`](contribution/commit-and-pr.md) 及相关贡献
skill。准备上游 PR 不应顺带同步 Signal Atlas `main`。

## Signal Atlas 文档与发布

Signal Atlas 产品文档、应用记录和发布说明归本仓库维护。经过 review 后，可将
可复用的工程文档改进提议给 upstream；Signal Atlas 专属需求和实现细节留在本仓库。

Upstream 将根目录 `README.md` 保留给自身约定，项目概览位于 `docs/README.md`。
上游 PR 应避免修改这些保留文件，除非变更明确服务于 upstream 且符合其贡献规则。

仓库中的双语文档遵循语言规则：默认 `.md` 使用英文，配对的 `.zh_CN.md` 使用
简体中文，并提供相互链接。
