---
name: fix-failing-test
description: Pick a failing test from the simutrans automated test suite, analyze why it fails, and fix the C++ or Squirrel API code to make it pass. Use run-automated-tests to run the test.
argument-hint: "[test_name] (optional - if omitted, pick one automatically)"
allowed-tools: Bash(git *)
---

# Fix Failing Test

You are fixing automated tests in the Simutrans OTRP project. Follow these steps precisely.

## Context

- Test definitions: `tests/automated-tests/all_tests.nut`
  - `all_tests` array: tests that currently pass
  - `failing_tests` array: tests that are known to fail
- Test implementations: `tests/automated-tests/tests/test_*.nut`
- Test helpers: `tests/automated-tests/test_helpers.nut`
- Squirrel API bindings: `script/api/` directory
- C++ tool implementations: `simtool.cc`, `simtool.h`, and related files
- Upstream reference: `upstream/master` branch (aburch/simutrans), source at `src/simutrans/`

## Procedure

### 1. Select a test

If a test name was provided as $ARGUMENTS, use that. Otherwise:
- Read `tests/automated-tests/all_tests.nut`
- Pick a test from the `failing_tests` array (prefer simpler/shorter tests first)
- Read the test implementation file to understand what it tests

### 2. Run the test

Use the **`run-automated-tests`** skill to run the target test. Move the selected test(s) from `failing_tests` to `all_tests` in `all_tests.nut` before running if you want to verify it passes.

### 3. Analyze the failure

Common failure patterns:
- **`the index 'xxx' does not exist`**: A Squirrel API function/constant is not defined. Check `script/api/` for missing bindings. Compare with upstream `src/simutrans/script/api/`.
- **`"" == null`**: A tool call returned empty string (error) instead of null (success). The C++ tool implementation is returning an error when it shouldn't, or vice versa.
- **`Error during initializing tool`**: `tool->init()` returned false. Check the tool's `init()` method.
- **`Tool has no effects`**: The tool's `is_work_network_safe()` or `is_work_here_network_safe()` returned true when it shouldn't.
- **Assertion value mismatches**: Compare actual vs expected to understand the semantic difference (e.g., maintenance costs, positions, etc.)

### 4. Fix the code

- For missing Squirrel API: Add bindings in `script/api/api_*.cc` and type definitions in `squirrel_types_ai.awk` / `squirrel_types_scenario.awk`
- For C++ tool bugs: Fix the tool implementation in `simtool.cc` or related files
- For test expectation issues: Only modify the test if the test expectation is clearly wrong
- Reference the upstream aburch/simutrans repository for how things should work (e.g., `git fetch https://github.com/aburch/simutrans.git master && git show FETCH_HEAD:src/simutrans/...`)

### 5. Build and verify

- Use the **`run-automated-tests`** skill to build and verify your fix.
- If the test still fails, iterate on the fix.

### 6. Finalize

- Update `all_tests.nut`: the fixed test should be in `all_tests`, not in `failing_tests`
- Do NOT commit automatically - let the user decide when to commit
- Report what was fixed and why the test was failing
