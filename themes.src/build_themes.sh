#!/bin/bash

#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

# assumes makeobj is in the trunk directory

cd aero
rm -rf *.pak
../../src/makeobj/makeobj pak aerotheme.pak skins_aero.dat
mv *.pak ../../simutrans/themes
cp *.tab ../../simutrans/themes

cd ../flat
rm -rf *.pak
../../src/makeobj/makeobj pak flat.pak flat-skin.dat
../../src/makeobj/makeobj pak flat-touch.pak flat-skin-touch.dat
mv *.pak ../../simutrans/themes
cp *.tab ../../simutrans/themes

cd ../standard
rm -rf *.pak
../../src/makeobj/makeobj pak classic.pak standard.dat
mv *.pak ../../simutrans/themes
cp *.tab ../../simutrans/themes

cd ../highcontrast
rm -rf *.pak
../../src/makeobj/makeobj pak highcontrast.pak theme.dat
mv *.pak ../../simutrans/themes
cp *.tab ../../simutrans/themes

cd ../highcontrast-large
rm -rf *.pak
../../src/makeobj/makeobj pak highcontrast-large.pak theme.dat
mv *.pak ../../simutrans/themes
cp *.tab ../../simutrans/themes

cd ../modern
rm -rf *.pak
../../src/makeobj/makeobj pak modern.pak standard.dat
../../src/makeobj/makeobj pak modern-large.pak standard-large.dat
mv *.pak ../../simutrans/themes
cp *.tab ../../simutrans/themes

cd ../pak64german/files
../../../src/makeobj/makeobj pak
cd ../files_large
../../../src/makeobj/makeobj pak
../../../src/makeobj/makeobj pak128 ./ ./dat_128.txt
cd ..
../../src/makeobj/makeobj merge ./menu.pak64german.pak ./files/*.pak
../../src/makeobj/makeobj merge ./menu.pak64german_large.pak ./files_large/*.pak
mv *.pak ../../simutrans/themes
cp *.tab ../../simutrans/themes

cd ../pak192comic/pak192comic
../../../src/makeobj/makeobj pak menu.pak192comic.pak theme.dat
cd ../pak192comicxxl
../../../src/makeobj/makeobj pak menu.pak192comicxxl.pak theme.dat
cd ..
mv pak192comic/menu.pak192comic.pak ../../simutrans/themes
mv pak192comicxxl/menu.pak192comicxxl.pak ../../simutrans/themes
cp *.tab ../../simutrans/themes
