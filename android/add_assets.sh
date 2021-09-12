#!/bin/bash

# This script adds assets to embed with the Android build
# It is called by AndroidPreBuild.sh
# Working folder is expected to be this file location (<repo root>/simutrans/android/
# Assets are downloaded to this folder before being copied to the target location

echo "Adding assets"

[ -e ../simutrans/pak ] || (wget --retry-connrefused --show-progress -nc https://downloads.sourceforge.net/project/simutrans/pak64/122-0/simupak64-122-0.zip && unzip -n simupak64-122-0.zip -d ..)
[ -e ../simutrans/pak64.german ] || (wget --retry-connrefused --show-progress -nc http://simutrans-germany.com/pak.german/pak64.german_0-122-0-0-2_full.zip && unzip -n pak64.german_0-122-0-0-2_full.zip -d ..)
[ -e ../simutrans/music/TimGM6mb.sf2 ] || (wget --retry-connrefused --show-progress -nc https://sourceforge.net/p/mscore/code/HEAD/tree/trunk/mscore/share/sound/TimGM6mb.sf2?format=raw -O TimGM6mb.sf2 && cp ./TimGM6mb.sf2 ../simutrans/music/TimGM6mb.sf2)
[ -e ../simutrans/font/RobotoCondensed-Regular.ttf ] || (wget --retry-connrefused --show-progress -nc https://fonts.google.com/download?family=Roboto%20Condensed -O Roboto_Condensed.zip && unzip -n Roboto_Condensed.zip -d ../simutrans/font)
[ -e ../simutrans/font/Roboto-Regular.ttf ] || (wget --retry-connrefused --show-progress -nc https://fonts.google.com/download?family=Roboto -O Roboto.zip && unzip -n Roboto.zip -d ../simutrans/font)
[ -e ../simutrans/cacert.pem ] || cp ./cacert.pem ../simutrans/cacert.pem
