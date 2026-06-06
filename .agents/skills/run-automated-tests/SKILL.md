---
name: run-automated-tests
description: Run automated tests for Simutrans OTRP, build the project, and look up pak assets.
argument-hint: "[test_name] (optional - specific test function to run)"
allowed-tools: Bash(make *), Bash(WORKDIR=* SIM_BINARY=* tests/run-automated-tests.sh *), Bash(WORKDIR=* SIM_BINARY=* ./sim *), Bash(timeout *), Bash(cp *), Bash(cat *)
---

# Run Automated Tests

You run automated tests for Simutrans OTRP.

## Basic Commands

- **Build**: `make -j8`
- **Run single test**: `WORKDIR=${SIMUTRANS_TEST_BASE} SIM_BINARY=./sim tests/run-automated-tests.sh <test_func_name>`
- **Run all tests**: `WORKDIR=${SIMUTRANS_TEST_BASE} SIM_BINARY=./sim tests/run-automated-tests.sh`

**Important**: Do NOT run the full test suite (no arguments) locally — it takes too long. All tests are independent, so only run the ones relevant to your changes. Full test suite verification is handled by CI when a PR is created.

If you need more detailed output, run the sim binary directly and grep for FAILED lines.

## Look up available pak assets when needed

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
