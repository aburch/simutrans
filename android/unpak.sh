#!/bin/sh

URL=$1
pakzippath="${URL##http*\/}"
rm -rf $pakzippath
wget $URL
unzip -l "$pakzippath" | grep "simutrans/"
if [ $? -eq 1 ]; then
	# Not matched - We only have a simutrans/ directory
	unzip -qo "$pakzippath"
else
	unzip -qo "$pakzippath"
	cp -RF simutrans/* .
	rm -rf simutrans
fi
rm -rf "$pakzippath"


