#!/bin/bash

#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

# assumes makeobj is in the trunk directory

cd aero
rm -rf *.pak
../makeobj pak aerotheme.pak skins_aero.dat
mv *.pak ../../simutrans/themes
cp *.tab ../../simutrans/themes

cd ../flat
rm -rf *.pak
../makeobj pak flat.pak flat-skin.dat
../makeobj pak flat-touch.pak flat-skin-touch.dat
mv *.pak ../../simutrans/themes
cp *.tab ../../simutrans/themes

cd ../standard
rm -rf *.pak
../makeobj pak classic.pak standard.dat
mv *.pak ../../simutrans/themes
cp *.tab ../../simutrans/themes

cd ../highcontrast
rm -rf *.pak
../makeobj pak highcontrast.pak theme.dat
mv *.pak ../../simutrans/themes
cp *.tab ../../simutrans/themes

cd ../highcontrast-large
rm -rf *.pak
../makeobj pak highcontrast-large.pak theme.dat
mv *.pak ../../simutrans/themes
cp *.tab ../../simutrans/themes

cd ../modern
rm -rf *.pak
../makeobj pak modern.pak standard.dat
../makeobj pak modern-large.pak standard-large.dat
mv *.pak ../../simutrans/themes
cp *.tab ../../simutrans/themes

cd ../pak64german/files
../../makeobj pak
cd ../file-large
../../makeobj pak
../../makeobj pak128 ./ ./dat_128.txt
cd ..
..\makeobj merge ./menu.pak64german.pak ./files/*.pak
..\makeobj merge ./menu.pak64german_large.pak ./files_large/*.pak
mv *.pak ../../simutrans/themes
cp *.tab ../../simutrans/themes

cd ../pak192comic/pak192comic
../../makeobj pak menu.pak192comic.pak theme.dat
cd ../pak192comicxxl
../../makeobj pak menu.pak192comicxxl.pak theme.dat
cd ..
mv pak192comic/menu.pak192comic.pak ../../simutrans/themes
mv pak192comicxxl/menu.pak192comicxxl.pak ../../simutrans/themes
cp *.tab ../../simutrans/themes
