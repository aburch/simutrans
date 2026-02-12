# Automated Tests for Simutrans OTRP

This directory contains automated test scripts for Simutrans OTRP.

## Prerequisites

1. Build the Simutrans executable (`sim` or `sim.exe`)
2. Extract the test base environment:
   ```
   unzip simutrans-test-base.zip -d /path/to/test-base
   ```

## Running Tests Locally

### macOS / Linux

```bash
# Run all tests
WORKDIR=/path/to/simutrans-test-base SIM_BINARY=./sim tests/run-automated-tests.sh

# Run a single test function
WORKDIR=/path/to/simutrans-test-base SIM_BINARY=./sim tests/run-automated-tests.sh test_label
WORKDIR=/path/to/simutrans-test-base SIM_BINARY=./sim tests/run-automated-tests.sh test_building_build_house_random
```

The script automatically copies the latest test files from `tests/automated-tests/`
to the WORKDIR scenario directory before running.

### Environment Variables

- `WORKDIR`: Path to the extracted simutrans-test-base directory (required)
- `SIM_BINARY`: Path to the Simutrans executable (default: ../sim)

### Example

```bash
# Extract test base to your home directory
unzip simutrans-test-base.zip -d ~/

# Run all tests
WORKDIR=~/simutrans-test-base SIM_BINARY=./sim tests/run-automated-tests.sh

# Run a single test function
WORKDIR=~/simutrans-test-base SIM_BINARY=./sim tests/run-automated-tests.sh test_label
```

## Expected Output

The test runner will:
- Start Simutrans with the automated test scenario
- Monitor test execution with progress updates
- Display test results (SUCCESS or FAILURE)
- Exit with code 0 on success, 1 on failure

Success output:
```
✓ Tests completed successfully!
Result: SUCCESS
```

Failure output:
```
✗ Test execution failed (script error detected)
Result: FAILURE
Last 30 lines of output:
[error details]
```

## CI/CD Integration

Tests are automatically run on GitHub Actions for all pull requests.

### How CI Tests Work

1. **Build Job**: Compiles the Simutrans binary and bundles dependencies
2. **List Tests Job**: Discovers all test functions and groups them into batches of 10
3. **Test Jobs**: Each batch runs in a separate parallel job
   - Within each batch, tests run sequentially but in isolated game instances
   - This reduces CI resource usage while maintaining test independence

Example: If there are 35 tests, they are split into:
- Batch 1: Tests 1-10
- Batch 2: Tests 11-20
- Batch 3: Tests 21-30
- Batch 4: Tests 31-35

This batching strategy balances parallel execution with GitHub Actions resource limits.

See `.github/workflows/otrp-automated-tests.yml` for the workflow configuration.

## Test Structure

- `automated-tests/` - Test scenarios and scripts
  - `scenario.nut` - Main test scenario script
  - `all_tests.nut` - Test suite definitions
  - `test_helpers.nut` - Helper functions
  - `tests/` - Individual test cases
- `run-automated-tests.sh` - Test runner script
- `simutrans-test-base.zip` - Test environment (pak files, fonts, etc.)
