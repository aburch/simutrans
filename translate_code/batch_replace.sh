#/bin/bash
while IFS=, read from to
do
	echo "Replace $from by $to"
	REPLACE="s/$from/$to/g"

	find ../ -name "*.cc" -or -name "*.h" | while read fname; do

		sed "${REPLACE}" $fname > tmp.sed
		cmp -s tmp.sed $fname
		res=$?

		if [ $res -eq 1 ]
		then
			echo "Replace in $fname"
			mv tmp.sed $fname
		else
			rm tmp.sed
		fi
	done
done < $1
