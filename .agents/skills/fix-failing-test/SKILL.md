---
name: fix-failing-test
description: Pick a failing test from the simutrans automated test suite, run it, analyze why it fails, and fix the C++ or Squirrel API code to make it pass.
argument-hint: "[test_name] (optional - if omitted, pick one automatically)"
allowed-tools: Bash(make *), Bash(WORKDIR=* SIM_BINARY=* tests/run-automated-tests.sh *), Bash(WORKDIR=* SIM_BINARY=* ./sim *), Bash(git *)
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
- Run all tests: `WORKDIR=${SIMUTRANS_TEST_BASE} SIM_BINARY=./sim tests/run-automated-tests.sh`
- Run single test: `WORKDIR=${SIMUTRANS_TEST_BASE} SIM_BINARY=./sim tests/run-automated-tests.sh <test_func_name>`
- Build: `make -j8`

## Procedure

### 1. Select a test

If a test name was provided as $ARGUMENTS, use that. Otherwise:
- Read `tests/automated-tests/all_tests.nut`
- Pick a test from the `failing_tests` array (prefer simpler/shorter tests first)
- Read the test implementation file to understand what it tests

### 2. Run the test

- Move the selected test(s) from `failing_tests` to `all_tests` in `all_tests.nut`
- Run only the target test: `WORKDIR=${SIMUTRANS_TEST_BASE} SIM_BINARY=./sim tests/run-automated-tests.sh <test_func_name>`
- If you need more detailed output, run the sim binary directly and grep for FAILED lines

### 2.5. Look up available pak assets when needed

When writing or fixing tests that reference vehicle/way/building names, you often need to know exact names available in the test pak. **Do NOT guess names** — instead, write a temporary Squirrel script that queries the API and run it:

```squirrel
// tests/automated-tests/tests/test_list_assets.nut
function test_list_assets()
{
    // List available vehicles by waytype
    foreach (v in vehicle_desc_x.get_available_vehicles(wt_road)) {
        print("road: " + v.name + "  electric=" + v.needs_electrification())
    }
    foreach (v in vehicle_desc_x.get_available_vehicles(wt_rail)) {
        print("rail: " + v.name + "  electric=" + v.needs_electrification())
    }
    // List rail depots
    foreach (b in building_desc_x.get_building_list(building_desc_x.depot)) {
        if (b.get_type() == building_desc_x.depot) {
            print("depot: " + b.name + "  wt=" + b.get_waytype())
        }
    }
    // List rail stations
    foreach (s in building_desc_x.get_available_stations(building_desc_x.station, wt_rail, good_desc_x.passenger)) {
        print("rail_station: " + s.name)
    }
    // List overhead lines
    foreach (w in wayobj_desc_x.get_available_wayobjs(wt_rail)) {
        if (w.is_overhead_line()) { print("overhead: " + w.name) }
    }
    // List tram ways
    foreach (w in way_desc_x.get_available_ways(wt_rail, st_tram)) {
        print("tram_way: " + w.name)
    }
    ASSERT_TRUE(true)
}
```

Run it directly via the sim binary (not the test runner, since `print` output is suppressed on success):
```bash
# Copy test file, set minimal all_tests.nut, run sim for ~10s
cp tests/automated-tests/tests/test_list_assets.nut ${SIMUTRANS_TEST_BASE}/pak/scenario/automated-tests/tests/
cat > ${SIMUTRANS_TEST_BASE}/pak/scenario/automated-tests/all_tests.nut <<'EOF'
include("tests/test_list_assets")
all_tests <- [test_list_assets]
failing_tests <- []
EOF
timeout 12 ./sim -set_workdir ${SIMUTRANS_TEST_BASE} -objects pak \
  -scenario automated-tests -debug 2 -lang en -fps 100 2>&1 | grep "Script: Print:"
```

**Known pak assets** (test pak, stable — no need to query each time):

| Category | Names |
|----------|-------|
| Road vehicles (diesel) | `Buessig`, `Bus`, `Posttransporter`, `Kohletransporter`, … |
| Rail vehicles (electric) | `E44` (speed=90), `1Ellok`, `2Ellok`, `BB200`, `Be5-7`, `NS1000`, `NS1600`, … |
| Rail vehicles (diesel) | `1Diesellokomotive`, `2Diesellokomotive`, `3Diesellokomotive`, `TPSchienenbus`, … |
| Tram vehicles (electric) | `Lowa`, `Tatra-T4`, `T13`, `T24`, `Tram_GT6_Kopf`, `Tram_GT6_Ende`, … |
| Tram vehicles (non-electric) | `TB13`, `TB24`, `Pferdebahn`, … |
| Road stops | `BusStop`, `PostStop` |
| Rail stations | `TrainStop`, `GCGTrainStop`, `PSHall1TrainStop`, … |
| Rail ways | `sand_track` (55 km/h), `wooden_sleeper_track` (120), `concrete_sleeper_track` (200) |
| Tram ways | `Rillenschienen` (90 km/h) — use `way_desc_x.get_available_ways(wt_rail, st_tram)[0]` |
| Rail overhead lines | `SlowOverheadpower`, `HighSpeedOverheadpower` |
| Road | `cobblestone_road` |
| Depots | `CarDepot` (road), `TrainDepot` (rail), `TramDepot` (tram), `ShipDepot` (water), `AirDepot` (air) |

**Important notes for test authoring:**
- Bus stops cannot be placed on road intersections (tiles with 3+ connections) — place stops on through-road tiles only
- Tram way removal uses `"" + wt_rail`, not `"" + wt_tram`
- `RESET_ALL_PLAYER_FUNDS()` asserts maintenance==0 — skip it if not cleaning up infrastructure; each test runs in its own game instance so cleanup is optional
- When running single tests, `test_helpers.nut` is NOT included — do not call `get_depot_by_wt()` directly; inline the logic instead

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

- Build: `make -j8`
- Run the target test and any other tests that could be affected by your changes: `WORKDIR=${SIMUTRANS_TEST_BASE} SIM_BINARY=./sim tests/run-automated-tests.sh <test_func_name>`
- Do NOT run the full test suite (no arguments) locally — it takes too long. All tests are independent, so only run the ones relevant to your changes.
- Full test suite verification is handled by CI when a PR is created.
- If the test still fails, iterate on the fix

### 6. Finalize

- Update `all_tests.nut`: the fixed test should be in `all_tests`, not in `failing_tests`
- Do NOT commit automatically - let the user decide when to commit
- Report what was fixed and why the test was failing
