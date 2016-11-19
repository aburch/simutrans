#/bin/bash
while IFS=, read from to
do
	echo "Replace $from by $to"
	REPLACE="s/$from/$to/g"
	find ../ -name "*.h"  -exec sed -i ${REPLACE} {} \;
	find ../ -name "*.cc" -exec sed -i ${REPLACE} {} \;
done < $1
