#!/bin/sh

cd simutrans || exit 1

echo "Updating translations"
tools/get_lang_files.sh || exit 1

cd simutrans

echo "Get pak64"
../tools/get_pak.sh pak64 || exit 1

echo "Get pak64.german"
../tools/get_pak.sh pak.german || exit 1

echo "Get pak64.japan"
../tools/get_pak.sh pak64.japan || exit 1

cd ..


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

echo "`pwd`"
echo "`ls -l`"
echo "`ls -l *`"

mkdir simutrans/music
[ -e simutrans/music/TimGM6mb.sf2 ] || (download_with_retry https://sourceforge.net/p/mscore/code/HEAD/tree/trunk/mscore/share/sound/TimGM6mb.sf2?format=raw TimGM6mb.sf2 && cp ./TimGM6mb.sf2 simutrans/music/TimGM6mb.sf2) || exit 1
#[ -e ../simutrans/font/RobotoCondensed-Regular.ttf ] || (download_with_retry https://fonts.google.com/download?family=Roboto%20Condensed Roboto_Condensed.zip && unzip -n Roboto_Condensed.zip -d ../simutrans/font) || exit 1
[ -e simutrans/font/Roboto-Regular.ttf ] || (download_with_retry https://fonts.google.com/download?family=Roboto Roboto.zip && unzip -n Roboto.zip Roboto-Regular.ttf -d simutrans/font) || exit 1
#[ -e simutrans/cacert.pem ] || cp src/android/cacert.pem simutrans/cacert.pem || exit 1
[ -e simutrans/get_pak.sh ] || cp src/android/unpak.sh simutrans/get_pak.sh; chmod 755 simutrans/get_pak.sh  || exit 1



echo 'Done adding assets'
cd ..

mkdir -p AndroidData
echo Generating data.zip
cd simutrans/simutrans

rm -f ../../AndroidData/data.zip
zip -r -0 ../../AndroidData/data.zip * >/dev/null
cd ../..
echo Generating data.zip done

[ -e AndroidData/TimGM6mb.sf2 ] || cp simutrans/TimGM6mb.sf2 AndroidData/ || exit 1
