#!/bin/bash
set -e


pushd simutrans
../sim -use_workdir -objects pak -lang en -scenario automated-tests -debug 2 2>&1 | ts -s | tee output.log &
pid=$!

result=1

while :
do
	sleep 1

	if [[ ! -d /proc/$pid/ ]]
	then
		# process crashed etc.
		echo "Process crashed (test failed)"
		result=1
		break
	fi

	if [[ -n "$(grep 'Tests completed successfully.' output.log)" ]]
	then
		# kill
		echo "Killing process (test succeeded)"
		kill %1
		result=0
		break
	fi

	if [[ -n "$(grep 'error \[Call function failed\] calling' output.log)" ||
	      -n "$(grep 'error \[Reading / compiling script failed] calling' output.log)" ||
	      -n "$(grep '</error>' output.log)" ]]
	then
		# kill
		echo "Killing process (test failed)"
		kill %1
		result=1
		break
	fi
done

popd

exit $result
