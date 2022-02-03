#!/bin/bash
#
# Script to update squirrel source code from SQUIRREL3
#
# Call with ``update_squirrel PATH_TO_SQUIRREL'', where
# PATH_TO_SQUIRREL is path of local clone of https://github.com/albertodemichelis/squirrel
#
# Call it within a git repo of simutrans.
#
# Still needs manual review, to make sure none of our modifications are reverted.
SQPATH=$1
# copy and rename files
echo $SQPATH/COPYRIGHT ../simutrans/license_squirrel.txt
cp $SQPATH/COPYRIGHT ../simutrans/license_squirrel.txt
cp $SQPATH/include/* .
cp $SQPATH/squirrel/*.h squirrel/

cp $SQPATH/squirrel/*.h   squirrel/
cp $SQPATH/squirrel/*.cpp squirrel/


for i in *.h; do gawk -f update_squirrel.awk "$i" > temp; mv temp "$i"; done

cd squirrel
# rename
for i in *.cpp; do mv "$i" "${i/.cpp}".cc; done
# replace include <sq*.h> by include "../sq*.h"
for i in *.cc; do gawk -f ../update_squirrel.awk "$i" > temp; mv temp "$i"; done
for i in *.h; do gawk -f ../update_squirrel.awk "$i" > temp; mv temp "$i"; done
cd ..

cp $SQPATH/sqstdlib/*.h   sqstdlib/
cp $SQPATH/sqstdlib/*.cpp sqstdlib/

cd sqstdlib
# rename
for i in *.cpp; do mv "$i" "${i/.cpp}".cc; done
# replace include <sq*.h> by include "../sq*.h"
for i in *.cc; do gawk -f ../update_squirrel.awk "$i" > temp; mv temp "$i"; done
for i in *.h; do gawk -f ../update_squirrel.awk "$i" > temp; mv temp "$i"; done
cd ..

cd ..
git diff -w --ignore-blank-lines --ignore-space-at-eol -b > update_squirrel.diff

git checkout -- squirrel/squirrel
git checkout -- squirrel/sqstdlib
git checkout -- squirrel/sq*.h
git checkout -- simutrans/license_squirrel.txt

patch --ignore-whitespace -p3 < update_squirrel.diff
