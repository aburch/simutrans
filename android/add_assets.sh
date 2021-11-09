#!/bin/bash

# This script adds assets to embed with the Android build
# It is called by AndroidPreBuild.sh
# Working folder is expected to be this file location (<repo root>/simutrans/android/
# Assets are downloaded to this folder before being copied to the target location

echo "Adding assets"

download_with_retry() {
  echo 'START ' $1
  RESULT=-1
  RETRY=5
  while [ $RESULT -ne 0 ] && [ $RETRY -ne 0 ]
  do
    wget --show-progress $1 -O $2
    RESULT=$?
    RETRY=$((RETRY-1))
  done
  if [ $RETRY -eq 0 ]
    then 
      exit 1
  fi
}

[ -e ../simutrans/pak ] || (download_with_retry http://downloads.sourceforge.net/project/simutrans/pak64/122-0/simupak64-122-0.zip simupak64-122-0.zip && unzip -n simupak64-122-0.zip -d ..) || exit 1
[ -e ../simutrans/pak ] || (download_with_retry http://downloads.sourceforge.net/project/simutrans/pak64.japan/120-0/simupak64.japan-120-0-1.zip simupak64.japan-120-0-1.zip && unzip -n simupak64.japan-120-0-1.zip -d ..) || exit 1
#[ -e ../simutrans/pak64.german ] || (download_with_retry http://simutrans-germany.com/pak.german/pak64.german_0-122-0-0-2_full.zip pak64.german_0-122-0-0-2_full.zip && unzip -n pak64.german_0-122-0-0-2_full.zip -d ..) || exit 1
[ -e ../simutrans/music/TimGM6mb.sf2 ] || (download_with_retry https://sourceforge.net/p/mscore/code/HEAD/tree/trunk/mscore/share/sound/TimGM6mb.sf2?format=raw TimGM6mb.sf2 && cp ./TimGM6mb.sf2 ../simutrans/music/TimGM6mb.sf2) || exit 1
[ -e ../simutrans/font/RobotoCondensed-Regular.ttf ] || (download_with_retry https://fonts.google.com/download?family=Roboto%20Condensed Roboto_Condensed.zip && unzip -n Roboto_Condensed.zip -d ../simutrans/font) || exit 1
[ -e ../simutrans/font/Roboto-Regular.ttf ] || (download_with_retry https://fonts.google.com/download?family=Roboto Roboto.zip && unzip -n Roboto.zip -d ../simutrans/font) || exit 1
[ -e ../simutrans/cacert.pem ] || cp ./cacert.pem ../simutrans/cacert.pem || exit 1
[ -e ../simutrans/get_pak.sh ] || cp ./unpak.sh ../simutrans/get_pak.sh; chmod 755 ../simutrans/get_pak.sh  || exit 1


echo 'Done adding assets'
