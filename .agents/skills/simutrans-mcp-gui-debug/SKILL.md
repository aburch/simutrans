---
name: simutrans-mcp-gui-debug
description: Start Simutrans test saves with -mcp-port, execute Squirrel scripts through the built-in MCP server, capture screenshots, and verify GUI/dialog behavior. Use for GUI-oriented debugging or when a test needs visual confirmation rather than the normal automated test runner.
argument-hint: "[save_name] [squirrel_code]"
allowed-tools: Bash(make *), Bash(./sim *), Bash(timeout *), Bash(python3 .agents/skills/simutrans-mcp-gui-debug/scripts/mcp_client.py *), Bash(lsof *), Bash(ps *), Bash(kill *), Bash(rg *), Bash(sed *), Read
---

# Simutrans MCP GUI Debug

Use this skill when GUI state must be inspected visually. This is separate from the normal `run-automated-tests` workflow: start Simutrans with `-load` and `-mcp-port`, drive it through MCP `run_squirrel`, then capture the screen with MCP `capture_screen`. For GUI/debug work, also use the Squirrel GUI accessibility APIs (`gui.get_windows()` and `gui.get_window_components(window_id)`) to inspect the visible window and component tree.

Do not hardcode local absolute paths, personal config paths, or machine-specific ports in repository files. Use environment variables and placeholders in notes or scripts.

## Core Facts

- Simutrans starts its built-in MCP server with `-mcp-port PORT`.
- The automated-test scenario declares its base save in `tests/automated-tests/scenario.nut` as `map.file`.
- For this workflow, load that save directly with `-load SAVE_NAME`, not with `-scenario`.
- The `-load` argument is the save name without the `.sve` extension.
- MCP exposes at least `run_squirrel` and `capture_screen`.
- MCP `run_squirrel` currently runs in an AI-style VM. APIs needed only for MCP driving must be visible outside scenario-only bindings unless the MCP implementation changes.
- The MCP listener may bind IPv6 loopback (`::1`) instead of IPv4 loopback. The helper script tries both by default.
- The helper script requires Python 3.10 or newer.
- `gui.get_windows()` returns open-window snapshots. Pass a returned window `id` to `gui.get_window_components(window_id)` to inspect that window's component tree.

## Procedure

1. Identify the save name:
   - Read `tests/automated-tests/scenario.nut`.
   - Use `map.file` without `.sve`, for example `empty-16x16`.

2. Build if code changed:
   ```bash
   make -j8
   ```

3. Choose a temporary MCP port:
   - Prefer an explicit `MCP_PORT` environment variable when the user provides one.
   - Otherwise choose a high, task-local port and verify it is free if needed.

4. Start Simutrans in a long-running shell session:
   ```bash
   ./sim -set_workdir "${SIMUTRANS_TEST_BASE}" -objects pak \
     -load "${SAVE_NAME}" -mcp-port "${MCP_PORT}" \
     -debug 2 -lang en -fps 20
   ```
   Keep the process running until all MCP calls and screenshots are complete. If local TCP or the external test base is blocked by sandboxing, request escalation.

5. Call MCP through the helper:
   ```bash
   python3 .agents/skills/simutrans-mcp-gui-debug/scripts/mcp_client.py \
     --port "${MCP_PORT}" \
     --run-squirrel 'return "ok";'
   ```

6. Drive the GUI with Squirrel:
   Set `MCP_SCREENSHOT` to a writable PNG path first. Examples:
   - POSIX shell: `MCP_SCREENSHOT="${TMPDIR:-/tmp}/simutrans-gui-debug.png"`
   - PowerShell: `$env:MCP_SCREENSHOT = Join-Path $env:TEMP "simutrans-gui-debug.png"`

   ```bash
   python3 .agents/skills/simutrans-mcp-gui-debug/scripts/mcp_client.py \
     --port "${MCP_PORT}" \
     --run-squirrel 'gui.close_all_windows(); return gui.open_dialog_tool(2);' \
     --capture "${MCP_SCREENSHOT}"
   ```

7. Capture the component tree:
   - Use `gui.get_windows()` to find the target window, usually the top window after opening the dialog.
   - Use `gui.get_window_components(window.id)` to retrieve component snapshots.
   - When printing a component tree for human inspection, filter out components whose `role` is `"unknown"` unless their bounds or existence are directly relevant. These are often layout helpers or zero-size implementation details, and hiding them makes screenshot comparison easier.

   Example:
   ```bash
   python3 .agents/skills/simutrans-mcp-gui-debug/scripts/mcp_client.py \
     --port "${MCP_PORT}" \
     --run-squirrel 'function b(s) { return "(" + s.x + "," + s.y + "," + s.w + "," + s.h + ")"; } function dump(c, d) { local out = ""; if (c.role != "unknown") { local indent = ""; for (local i = 0; i < d; i++) indent += "  "; out += indent + c.role + "#" + c.id + " screen=" + b(c.screen_bounds); if ("text" in c) out += " text=[" + c.text + "]"; out += "\n"; } foreach (child in c.children) out += dump(child, d + 1); return out; } local top = null; foreach (w in gui.get_windows()) if (w.top) top = w; local out = "window title=[" + top.title + "] bounds=" + b(top.bounds) + "\n"; foreach (c in gui.get_window_components(top.id)) out += dump(c, 0); return out;'
   ```

8. Inspect the screenshot and component tree:
   - Use the image viewing tool on the captured PNG.
   - Confirm the expected window, dialog title, selected control, or visual state is actually visible.
   - Compare the component tree's title, text labels, roles, and screen bounds against the screenshot.
   - Treat a true Squirrel return value as insufficient for GUI bugs unless the screenshot and component tree also match.

9. Clean up:
   - Stop the Simutrans process you started.
   - Confirm the MCP port is no longer listening if there is any doubt.

## Troubleshooting

- `connect EPERM`: local TCP is sandbox-blocked; rerun the MCP client with escalation.
- `Connection refused`: Simutrans is not running, the port is wrong, or the listener bound only to another loopback address. Check the running process and try `::1` / `127.0.0.1`.
- `the index '...' does not exist`: the Squirrel API is not exported to the VM used by MCP. Check whether it is scenario-only.
- `Call function failed`: inspect `script-mcp.log` under the active Simutrans workdir for the Squirrel error.
- Blank or stale screenshot: wait briefly after the GUI action, then call `capture_screen` again.
