#!/bin/sh
wget "https://github.com/kuba--/zip/zipball/master/" -O master.zip
subdir="$(unzip -lqq master.zip | grep -o -e "kuba---zip-[^/]*" | uniq)"
unzip -j master.zip "$subdir/src/*"
rm -f master.zip
