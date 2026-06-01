---
name: simutrans-pr-manual-tests
description: Propose manual test cases to verify a PR's changes. Checks PR description, diff, and affected code. Posts a comment with checkboxes, checking those already executed. Uses an independent agent to filter tests.
argument-hint: "<PR URL or PR number>"
allowed-tools: Bash(gh *), Bash(git *)
---

# Simutrans PR Manual Test Cases

You are assisting with the Simutrans OTRP project. Your task is to propose manual test cases required to verify that the changes in a given Pull Request work correctly.

## Step 1: Parse the PR reference

The argument is `$ARGUMENTS`. Extract owner, repo name, and PR number:
- If it looks like `https://github.com/OWNER/REPO/pull/NUMBER`, extract accordingly.
- If it's just a number, assume `teamhimeh/simutrans`.

## Step 2: Fetch PR data and codebase context

Run these in parallel to gather information:
```bash
gh pr view NUMBER --repo OWNER/REPO
gh pr diff NUMBER --repo OWNER/REPO
gh api repos/OWNER/REPO/pulls/NUMBER/comments
gh api repos/OWNER/REPO/issues/NUMBER/comments
```

Also, read the surrounding context of the affected code in the local repository using file viewing tools to fully understand the impact of the changes.

## Step 3: Analyze changes and determine test cases

Analyze the PR description, the diff, existing PR comments, and the affected code locations.
Identify what manual testing a human must perform to verify the correctness of the PR's changes.
List the proposed test cases. **Each test case MUST be written as a detailed, step-by-step procedure to make it easy for a human tester to execute.** Specify "who" does "what" and "what the expected result is" in clearly numbered steps.
If the PR description or any PR comments indicate that a specific test case has **already been executed** and verified, note this down so its checkbox can be checked initially.

## Step 4: Validate test case necessity using an independent agent

Launch a **separate independent agent** (using the invoke_subagent tool) with the following prompt:

```
You are a QA reviewer for the Simutrans OTRP project.

You have been given a list of proposed manual test cases for PR #NUMBER in OWNER/REPO.
Your job is to determine if each test case is TRULY NECESSARY to verify the PR's changes.

For each test case:
- Is it directly related to the changes in the PR?
- Is it necessary to confirm the changes work correctly?

Output your evaluation in JSON:
{
  "test_cases": [
    {
      "description": "<The test case description>",
      "already_executed": <true/false>,
      "status": "<keep | discard | ask_user>",
      "reasoning": "<Why you chose this status>"
    }
  ]
}

Use "ask_user" if you are unsure whether the test case is necessary for confirming the operation.
```

Provide the agent with the PR description, diff summary, and your proposed test cases. Wait for its response.

## Step 5: Handle ambiguous cases

If the independent agent marked any test case as `ask_user`:
Present it to the user:
> **Question on test case necessity**:
> Test case: [description]
> Reasoning: [reasoning]
> Should I include this test case in the PR comment? (yes/no)

Use the `ask_question` tool or simply output the question and wait for the user's reply.

## Step 6: Format and post the PR comment

Compile the final list of approved test cases (`keep` and user-approved `ask_user` cases).
Format them as a GitHub markdown checklist. The comment body should be written in **Japanese**. Each test case title MUST be prefixed with its sequence number, e.g., "テストケース1：".
- `- [ ] **テストケース1：Test case title**\n  1. Step 1\n  2. Step 2` for unexecuted tests.
- `- [x] **テストケース2：Test case title**\n  1. Step 1\n  2. Step 2` for tests that are already shown as executed.

Example format:
```markdown
コードの変更内容を分析し、変更を網羅するための手動テストケースを作成しました。
予期せぬバグや互換性問題が潜んでいないか確認するため、以下のテストの実施を推奨します。

### 提案する手動テストケース

- [x] **テストケース1：○○の確認**
  1. プレイヤーAでXXを建設する。
  2. YYの操作を行い、ZZになることを確認する。

- [ ] **テストケース2：△△の確認**
  1. 条件を○○に設定する。
  2. エラーにならずに△△が実行されることを確認する。

*(Automated PR test case generation by AI agent)*
```

Post the comment to the PR using `gh`:
```bash
gh pr comment NUMBER --repo OWNER/REPO --body "..."
```
