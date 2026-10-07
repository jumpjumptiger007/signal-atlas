<p align="right">
  <a href="CI-sync-main.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Upstream Drift Check

`.github/workflows/sync-main.yml` checks `FoloToy/ai-passport:main` against
Signal Atlas `main`. It runs daily at 00:00 UTC and by manual dispatch. Signal
Atlas is an independent repository; this workflow does not synchronize its
product branch with upstream.

The workflow uses `contents: read`, checks out Signal Atlas `main`, fetches the
upstream `main`, and reports both SHAs, ahead/behind counts, the common ancestor,
and upstream commits since that ancestor in the workflow summary. When upstream
has commits Signal Atlas does not contain, it emits a warning that calls for
human review. It does not create a PR or write to any branch or remote.

The workflow does not merge, rebase, cherry-pick, reset, or push. An upstream
drift warning is informational: review changes and decide explicitly whether to
adapt them to Signal Atlas or prepare a separate contribution to
`FoloToy/ai-passport`. See the [repository and upstream workflow](../../fork-guide.md).

The checkout explicitly sets `ref: main` so the check always compares the
Signal Atlas product branch, even when a manual dispatch is started from another
branch. Checkout credentials are not persisted. The upstream remote is added
only inside the temporary Actions runner.
