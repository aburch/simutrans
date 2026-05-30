---
name: simutrans-pr-review
description: Review a Simutrans OTRP pull request by analyzing the diff in full codebase context, then post inline GitHub review comments.
argument-hint: "<PR URL or PR number>"
allowed-tools: Bash(gh *), Bash(git *)
---

# Simutrans PR Review

You are reviewing a pull request for the Simutrans OTRP project. Assume the repo is located in the current working directory.

## Step 1: Parse the PR reference

The argument is `$ARGUMENTS`. Extract owner, repo name, and PR number:
- If it looks like `https://github.com/OWNER/REPO/pull/NUMBER` or `.../pulls/NUMBER/...`, extract accordingly.
- If it's just a number, assume `teamhimeh/simutrans`.

## Step 2: Fetch PR data

Run these in parallel:
```bash
gh pr view NUMBER --repo OWNER/REPO
gh pr diff NUMBER --repo OWNER/REPO
gh api repos/OWNER/REPO/pulls/NUMBER/reviews
gh api repos/OWNER/REPO/pulls/NUMBER/comments
```

Note: `gh pr diff` outputs a unified diff. Parse the diff to identify:
- Which files are changed
- Which line numbers are affected (both old and new sides)
- The nature of each change

## Step 2b: Check for existing automated review

Inspect the reviews fetched above. Look for any review whose `body` contains "Automated code review by" or "による自動レビュー".

**If an existing automated review is found:**
- Note its `submitted_at` timestamp and `commit_id`.
- Fetch the commits added to the PR after that review:
  ```bash
  gh api repos/OWNER/REPO/pulls/NUMBER/commits
  ```
  Identify commits whose `committer.date` is later than the review's `submitted_at`.
- Also check the existing inline review comments (from the `/comments` call above) — note which files/lines already have comments from an automated review.
- Set a flag `ALREADY_REVIEWED=true` and proceed with the full diff review, but keep in mind:
  - The primary goal is to check whether the PR is now **approve-ready** (all prior issues resolved, no new problems introduced).
  - In Step 4, flag issues that appear in commits **after** the existing automated review as higher priority — these are changes the earlier review did not cover.
  - Do **not** re-post comments on lines already covered by the existing automated review unless the issue persists or has changed.

**If no existing automated review is found:**
- Set `ALREADY_REVIEWED=false` and proceed normally.

## Step 3: Read codebase context

For each changed file, read the surrounding context in the local repo (current working directory). Focus on:
- The full function containing the change (not just the diff hunk)
- Related functions that interact with the changed code
- Header files (.h) for types and declarations
- Any comments or invariants described in the surrounding code

Also check git log for related recent changes:
```bash
git log --oneline -20 -- <changed_file>
```

## Step 3b: Upstream Port Verification

If the PR states it is a port from the standard Simutrans (`aburch/simutrans`), you MUST check if the implementation matches `aburch/simutrans`.
- Clone `aburch/simutrans` into a temporary directory if needed.
- Find and examine the relevant upstream commits that originally implemented the feature.
- Identify any implementation differences between this PR and the upstream standard repository (e.g., missing fixes, divergence in logic).

## Step 4: Analyze and list ALL potential issues

Produce a detailed list of potential issues. For each issue, record:
- **File**: exact file path (relative to repo root, e.g. `simconvoi.cc`)
- **Line**: the NEW file line number where the issue appears
- **Severity**: `must-fix` / `should-fix` / `borderline` / `nit`
- **Description**: what is wrong or potentially wrong
- **Evidence**: why you think this is an issue (concrete reasoning from the code)
- **Confidence**: how confident you are (0–100%)
- **Post-review**: `true` if this issue appears in a commit added **after** the existing automated review (only relevant when `ALREADY_REVIEWED=true`)

Be thorough — list everything, even minor style issues or possible edge cases.

When `ALREADY_REVIEWED=true`, also assess overall approve-readiness:
- Are all issues raised in the prior automated review resolved?
- Are there any remaining `must-fix` or `should-fix` issues across the entire diff?
- Record a top-level verdict: `APPROVE_READY: true/false` with a one-sentence reason.

Severity definitions:
- `must-fix`: correctness bug, crash risk, data corruption, regression
- `should-fix`: logic concern, unclear invariant violation, potential bug
- `borderline`: uncertain — could be fine, could be a problem. **CRITICAL**: If the PR is a port from standard Simutrans (aburch/simutrans) and a potential issue is suspected to originate from the upstream standard implementation itself, it MUST be classified as a `borderline` item so the user can decide whether to post it.
- `nit`: style, naming, minor code quality

## Step 5: Launch a filtering agent

After compiling the full list, launch a **separate independent agent** using the Agent tool with the following prompt (fill in the placeholders):

```
You are a strict code reviewer for the Simutrans OTRP project (a train simulation game).

You have been given a list of potential issues found in PR #NUMBER in OWNER/REPO.
Your job: determine which issues should be posted as inline GitHub review comments.

Rules for inclusion:
- Include `must-fix` items always.
- Include `should-fix` items unless the concern is clearly unfounded given the full codebase context.
- For `borderline` items: DO NOT decide yourself. Return them separately as "ask_user" items.
- Exclude `nit` items unless they are genuinely important for this codebase.

For each included issue, produce a JSON object:
{
  "path": "relative/file/path.cc",
  "line": <new file line number as integer>,
  "side": "RIGHT",
  "body": "<comment text — professional, concise, technical. Mention this is an automated review by your AI agent's name (e.g. Gemini) at the end.>"
}

For borderline items, produce:
{
  "ask_user": true,
  "summary": "<one sentence description>",
  "path": "...",
  "line": ...,
  "body": "<proposed comment if user says yes>"
}

Output a single JSON object:
{
  "post": [ ... ],
  "ask": [ ... ]
}

Here is the full issue list:

[INSERT ISSUE LIST HERE]

Here is the PR diff for reference:

[INSERT DIFF HERE]
```

Wait for the agent to return its JSON output.

## Step 6: Handle borderline items

For each item in `ask`, present it to the user:
> **Borderline item**: [summary]  
> File: `path:line`  
> Proposed comment: "[body]"  
> Should I post this? (yes/no)

Use `AskUserQuestion` tool if available, otherwise output the question and wait for the user's reply.
Add user-approved borderline items to the `post` list.

## Step 7: Post inline comments

Build the review using the GitHub API. Post all approved comments as a **single review** (not individual comments) so they appear together:

```bash
gh api repos/OWNER/REPO/pulls/NUMBER/reviews \
  --method POST \
  --input - <<'EOF'
{
  "body": "Automated code review by your AI agent's name (e.g. Gemini).\n\nThis review was generated by analyzing the diff in the context of the full codebase.",
  "event": "COMMENT",
  "comments": [
    ... array of {path, line, side, body} objects ...
  ]
}
EOF
```

If there are no comments to post, report that no actionable issues were found and do not create an empty review.

## Step 8: Report

Tell the user:
- How many issues were found total
- How many were posted as inline comments
- The URL to the review on GitHub (if a review was posted)
- If `ALREADY_REVIEWED=true`: whether the PR is **approve-ready** (`APPROVE_READY: true/false`) and the one-sentence reason
- If the PR is a port from `aburch/simutrans`, include a detailed report of your upstream implementation comparison (what differences were found, if any, and any analysis of those differences).

## Important notes

- The repo on disk is the current working directory
- The default remote repo for `gh` commands is `teamhimeh/simutrans`
- `OTRP_VERSION_MAJOR` in `simversion.h` is intentionally incremented right before release, not per-feature PR. Do not flag a missing version bump as a review issue.
- Line numbers in the GitHub review API use the **new file** line numbers (after the change), with `"side": "RIGHT"`
- If a comment is on a deleted line (old side only), use `"side": "LEFT"` and the old line number
- Comments must be on lines that appear in the diff — you cannot comment on unchanged lines that are not part of a diff hunk; if the relevant line isn't in the diff, find the closest changed line in the same hunk
- The inline comment body should be in Japanese if it's a natural explanation, but English for technical terms. Mirror the language of the PR description.
- Always identify comments as automated: end each comment body with `\n\n*(your AI agent's name (e.g. Gemini) による自動レビュー)*`