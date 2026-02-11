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
cd tests
WORKDIR=/path/to/simutrans-test-base SIM_BINARY=../sim ./run-automated-tests.sh
```

### Environment Variables

- `WORKDIR`: Path to the extracted simutrans-test-base directory (required)
- `SIM_BINARY`: Path to the Simutrans executable (default: ../sim)

### Example

```bash
# Extract test base to your home directory
unzip simutrans-test-base.zip -d ~/

# Run tests
cd tests
WORKDIR=~/simutrans-test-base ./run-automated-tests.sh
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
See `.github/workflows/otrp-automated-tests.yml` for the workflow configuration.

## Test Structure

- `automated-tests/` - Test scenarios and scripts
  - `scenario.nut` - Main test scenario script
  - `all_tests.nut` - Test suite definitions
  - `test_helpers.nut` - Helper functions
  - `tests/` - Individual test cases
- `run-automated-tests.sh` - Test runner script
- `simutrans-test-base.zip` - Test environment (pak files, fonts, etc.)
