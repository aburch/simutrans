#!/bin/bash
set -e

# Automated test runner for Simutrans OTRP (macOS/Linux compatible)
# This script runs automated tests and monitors for completion or failure

# Check required environment variables
if [ -z "$WORKDIR" ]; then
	echo "Error: WORKDIR environment variable must be set"
	echo "Usage: WORKDIR=/path/to/simutrans-test-base ./run-automated-tests-macos.sh"
	exit 1
fi

SIM_BINARY="${SIM_BINARY:-../sim}"

# Use temporary file for monitoring
TEMP_LOG=$(mktemp)
trap "rm -f $TEMP_LOG" EXIT

echo "Running automated tests..."
echo "Working directory: $WORKDIR"
echo "Sim binary: $SIM_BINARY"
echo ""

# Start sim in background and redirect output
$SIM_BINARY -set_workdir "$WORKDIR" -objects pak -scenario automated-tests -debug 2 -lang en -fps 100 > "$TEMP_LOG" 2>&1 &
pid=$!

echo "Simutrans process started (PID: $pid)"
echo "Monitoring test execution..."
echo ""

result=1
timeout=300  # 5 minutes timeout
elapsed=0
last_line=""

while [ $elapsed -lt $timeout ]; do
	sleep 1
	elapsed=$((elapsed + 1))

	# Check if process is still running (macOS compatible)
	if ! ps -p $pid > /dev/null 2>&1; then
		# Check if tests completed successfully before process ended
		if grep -q 'Tests completed successfully.' "$TEMP_LOG" 2>/dev/null; then
			echo ""
			echo "✓ Tests completed successfully!"
			result=0
		else
			echo ""
			echo "✗ Process terminated unexpectedly (test failed or crashed)"
			result=1
		fi
		break
	fi

	# Check for successful completion
	if grep -q 'Tests completed successfully.' "$TEMP_LOG" 2>/dev/null; then
		echo ""
		echo "✓ Tests completed successfully!"
		kill $pid 2>/dev/null || true
		result=0
		break
	fi

	# Check for errors
	if grep -q 'error \[Call function failed\] calling' "$TEMP_LOG" 2>/dev/null || \
	   grep -q 'error \[Reading / compiling script failed\] calling' "$TEMP_LOG" 2>/dev/null || \
	   grep -q '</error>' "$TEMP_LOG" 2>/dev/null; then
		echo ""
		echo "✗ Test execution failed (script error detected)"
		kill $pid 2>/dev/null || true
		result=1
		break
	fi

	# Show progress every second
	if [ -f "$TEMP_LOG" ]; then
		current_line=$(grep -E '\[[0-9]+/[0-9]+\]' "$TEMP_LOG" 2>/dev/null | tail -1 | sed 's/^.*Print:[[:space:]]*//')
		if [ -n "$current_line" ] && [ "$current_line" != "$last_line" ]; then
			echo "$current_line"
			last_line="$current_line"
		fi
	fi
done

# Timeout check
if [ $elapsed -ge $timeout ]; then
	echo ""
	echo "✗ Test execution timed out after ${timeout}s"
	kill $pid 2>/dev/null || true
	result=1
fi

echo ""
echo "========================================"
if [ $result -eq 0 ]; then
	echo "Result: SUCCESS"
else
	echo "Result: FAILURE"
	echo ""
	echo "Last 30 lines of output:"
	echo "----------------------------------------"
	tail -30 "$TEMP_LOG" 2>/dev/null || true
fi
echo "========================================"

exit $result
