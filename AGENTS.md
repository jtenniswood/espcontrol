# AGENTS.md

## Communication

- The user is technical but not development-oriented. Explain changes in approachable terms and avoid unnecessary implementation detail.
- Keep final updates concise: what changed, how it was checked, and what the user needs to test.

## Development workflow

- Treat `main` as the stable branch.
- For normal code, firmware, configuration, UI, or documentation changes, create a short-lived branch from the latest `main`.
- Use a separate git worktree for feature or fix work so multiple issues can be developed and tested at the same time without changing `main`.
- Use short, descriptive branch names like `fix-display-timeout` or `update-pr-workflow`; do not include `codex` in branch names or PR titles.
- Infer the branch name from the requested outcome unless the task is ambiguous.
- Keep each branch focused on one bug fix, feature, device change, cleanup, or documentation change.
- If a request starts to include unrelated work, keep the extra work out of the branch unless the user explicitly asks to include it.
- Commit completed changes and push the branch.
- Open a pull request marked ready for review so automated checks and review systems run, instead of merging directly to `main`.
- Leave the pull request open until the user confirms they have tested it.
- Do not close related GitHub issues until the user confirms the fix works.
- Only work directly on `main` when the user explicitly asks for it, or for a tiny emergency/documentation-only change where a PR would add no value.
- After a pull request is merged, clean up its local worktree and branch when practical.

## Pull requests

- PR descriptions should explain the purpose of the change, the practical impact, and how it was checked.
- Include clear testing notes so the user can test the branch independently of `main`.
- If firmware needs flashing, name the affected display or device in the PR body.
- Distinguish automated checks from physical device testing. A compile/build pass is not the same as user-confirmed device testing.
- Use the automated PR testing guidance as the starting point for the PR body whenever it is available.
