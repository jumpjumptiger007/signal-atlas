---
name: experience-pr
description: After a firmware release, collect reusable development experience and submit it as a documentation pull request to the upstream FoloToy ai-passport project.
---

<p align="right">
  <a href="SKILL.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Collect Development Experience and Submit a PR

This skill captures the reusable, durable learnings from a firmware release and
proposes them as a documentation pull request to the upstream
`FoloToy/ai-passport` project. It runs only after a release and only when the
developer explicitly asks to collect experience.

## Safety and consent gate (run first)

Do not collect, draft, or submit anything until every gate below is satisfied.
These gates are the highest-priority constraints.

1. **Confirm consent up front.** This work touches project-private content.
   Ask the developer to confirm they agree to start the post-release follow-up
   before collecting anything. If they decline, stop immediately.
2. **Check GitHub access.** Confirm a GitHub channel is available before
   submitting. Try the configured channels in order and use the first one that
   works: GitHub MCP (when present and authenticated), a GitHub skill (for
   example the installed `cindy-github` capability), then the `gh` CLI
   (`gh auth status`). If none is available, generate the complete change
   content for the developer to paste manually and stop; never submit on their
   behalf.
3. **Never modify or commit on the current branch.** The release experience is
   usually derived from the current working branch, but the change must **not**
   be committed on the developer's current branch. Base the work on the latest
   upstream `main` for a clean baseline, create a dedicated branch or worktree,
   and keep Signal Atlas `origin` untouched. If a push is needed, use a separate
   contributor fork configured as its own remote (for example, `contributor`),
   then open the PR from that contributor branch against `FoloToy/ai-passport`.
   Leave the current checkout untouched.
4. **Never submit before review.** Draft everything first, show it to the
   developer, and wait for explicit approval. Do not commit, push, or open a PR
   until the developer has reviewed and authorized it. Opening a PR also
   requires separate confirmation.
5. **No credentials or private data.** Never include credentials, device QR
   secrets, private device links, personal data, or unsanitized logs. Run
   `python3 tools/check_repo.py` before committing anything.

## Upstream remote

The workflow assumes a remote named `upstream` points to
`https://github.com/FoloToy/ai-passport.git`. In Signal Atlas, `origin` points
to `https://github.com/jumpjumptiger007/signal-atlas.git` and must not be used
as the upstream PR head. If `upstream` is not configured, add it first:

```bash
git remote add upstream https://github.com/FoloToy/ai-passport.git
```

Fetch `upstream` to establish the current baseline. Do not assume Signal Atlas
`origin/main` mirrors upstream, or use it as a substitute for `upstream`.

## Collect reusable experience

Focus on reusable engineering learnings in Signal Atlas `docs/` that may benefit
the upstream project. Compare relevant documents with the upstream baseline:

```bash
# Files under docs/ that differ from upstream (created or changed in Signal Atlas)
git diff --name-only upstream/main...HEAD -- docs/

# Files under docs/ that exist here but not on upstream main
comm -23 \
  <(git ls-tree -r --name-only HEAD -- docs/ | sort) \
  <(git ls-tree -r --name-only upstream/main -- docs/ | sort)
```

From the differing documents, extract only durable, reusable learnings:

- What Signal Atlas documents that upstream does not, and why it may help other
  AI Passport users.
- Hardware facts, interfaces, timings, resource budgets, or failure behavior the
  Signal Atlas recorded.
- Build, validation, or release-flow improvements the fork made.
- Generalizations that apply to the next release.

Do **not** preserve transient debugging notes, half-finished experiments, or
anything that explains only this one-off release.

## Route the experience

Not every fork difference belongs upstream. Decide where each learning belongs
before submitting:

- **Upstream the reusable, general experience** — learnings that benefit any
  AI Passport user and belong in the upstream baseline (for example general
  build/validation improvements, durable hardware facts open to upstream,
  reusable interfaces or release-flow improvements). Submit these as a PR to the
  upstream `FoloToy/ai-passport`.
- **Keep Signal Atlas-specific material in this repository** — product
  requirements, product-specific behavior, and Signal Atlas assets are not
  upstream baseline changes. Do **not** submit them upstream (see
  [`docs/fork-guide.md`](../../docs/fork-guide.md)).

Route each entry according to this split; do not send Signal Atlas-specific
product customization to the upstream PR.

## Write the experience entry

Each experience capture may produce **one or more** entries, each as its own
`.md` file with its paired `.zh_CN.md` under `docs/reference/<username>/`,
named after the entry's content summary in lowercase-kebab-case (for example
`audio-compression-trade-offs.md`), where `<username>` is the contributing
developer's GitHub username (lowercase-kebab-case) grouping that developer's
entries together. This repository requires English at the default `.md` path and a
paired `.zh_CN.md`, with reciprocal language links and no Chinese prose in the
English file. Link each new entry from the index at
`docs/reference/README.md`.

A developer is not limited to one entry. The archive holds **one or more entries
per developer**, each as its own `.md` file (with its paired `.zh_CN.md`) under
that developer's folder. Capture each reusable, post-release learning as a new
entry rather than merging it into an existing one, so each entry stays a single,
self-contained topic.

Keep this change on a **dedicated branch or worktree**. Do not commit it on the
developer's current branch.

## Review and submit

1. Present the diff and draft to the developer, confirm the routing decision
   (upstream vs fork-local), and wait for explicit authorization.
2. On approval, commit on the dedicated branch (English imperative Conventional
   Commit title, for example
   `docs(development): add post-release experience notes`). If a remote push is
   needed, use a separate contributor fork remote; never push this upstream PR
   branch to Signal Atlas `origin`.
3. Fill the upstream `.github/PULL_REQUEST_TEMPLATE.md` completely, in English,
   and report Build, Host tests, Device tests, and Unverified separately.
4. Ask for separate confirmation, then open the PR from the contributor fork branch against
   the upstream `FoloToy/ai-passport` through the first available GitHub channel
   — GitHub MCP, a GitHub skill, or
   `gh pr create --repo FoloToy/ai-passport --base main --head <fork>:<branch>` —
   and read it back to confirm.

## Delivery reporting

Report the result as `Build`, `Host tests`, `Device tests`, and `Unverified`
separately. A docs-only change has no firmware or host-test work, so report
those as NOT RUN and note the reason. An experience pull request leaves the
paired changelog files unchanged; describe any user-visible or release-flow
impact in the pull-request body for the release maintainer to aggregate later.

## What this skill does not do

- It does not publish firmware or run the publisher workflow.
- It does not submit experience content to a separate public location.
- It does not commit on or modify the developer's current branch.
- It does not convert Signal Atlas into a fork or use Signal Atlas `origin` as
  an upstream PR head.
- It does not auto-submit anything without developer review and consent.

## Related documents

- Project completion overview: `docs/development/release/project-completion.md`
- Experience index: `docs/reference/README.md`
- Experience entries: `docs/reference/`
- Firmware publishing: `docs/development/release/publish-to-community.md`
- PR template: `.github/PULL_REQUEST_TEMPLATE.md`
- Contribution and commit rules: `docs/contribution/commit-and-pr.md`
- Fork branch and PR workflow: `docs/fork-guide.md`
