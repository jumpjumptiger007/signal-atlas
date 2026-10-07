<p align="right">
  <a href="fork-guide.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Repository and Upstream Workflow

Signal Atlas is an independent product repository. It is not a GitHub fork of
`FoloToy/ai-passport`, and its `main` branch is the Signal Atlas product branch.
The repository does not require its `main` to mirror or stay synchronized with
the upstream `main`.

## Repository roles

```text
origin   https://github.com/jumpjumptiger007/signal-atlas.git
upstream https://github.com/FoloToy/ai-passport.git
```

- `origin` stores Signal Atlas product code, documentation, and release history.
- `upstream` is the FoloToy AI Passport source for reusable BSP, hardware, build,
  and engineering changes.
- Signal Atlas product code stays in this repository. Relevant upstream changes
  are checked and reviewed before anyone decides whether to adapt them here.
- `.github/workflows/sync-main.yml` checks upstream drift only. It never merges,
  rebases, cherry-picks, resets, opens a pull request, or pushes changes.

## Checking and contributing upstream

The scheduled and manually triggered upstream drift check reports the current
`FoloToy/ai-passport:main` SHA, its ahead/behind relationship to Signal Atlas
`main`, and commits since their common ancestor. A report that upstream is ahead
is a review prompt, not an instruction to integrate its commits. Review relevant
changes and make an explicit, scoped decision before adapting general BSP,
hardware, build, or engineering improvements into Signal Atlas.

General improvements that benefit AI Passport users may be contributed to
`FoloToy/ai-passport` through a pull request. Keep Signal Atlas product code and
product-specific behavior in this repository. If an upstream contribution
requires a GitHub fork, create or use a separate contributor fork and branch;
do not convert Signal Atlas itself into a fork. Use `origin` only for the
Signal Atlas repository.

When preparing an upstream contribution, verify which repository is the PR base
and which is the contributor head. Follow
[`docs/contribution/commit-and-pr.md`](contribution/commit-and-pr.md) and the
relevant contribution skill. Do not synchronize Signal Atlas `main` as a
side-effect of preparing an upstream PR.

## Signal Atlas documentation and releases

Signal Atlas product documentation, application records, and release notes
belong in this repository. Reusable engineering documentation may be proposed
upstream after review; Signal Atlas-specific requirements and implementation
details stay here.

The upstream repository reserves its root `README.md` for its own conventions
and keeps its project overview in `docs/README.md`. An upstream pull request
should avoid changing those reserved files unless the proposed change is
specifically intended for the upstream repository and follows its contribution
rules.

All paired repository documentation follows the language rule: English at the
default `.md` path and Simplified Chinese at `.zh_CN.md`, with reciprocal links.
