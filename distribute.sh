#!/bin/bash
#
# CHANGE THIS FOR BEOS, CYGWIB, ETC!
if [ $MSYSTEM = MINGW32 ]
  then
  simexe=.exe
  simarchiv=simuwin
  echo "Windows"
  else
  simexe=
  simarchiv=simulinux
  echo "Linux"
fi



# (otherwise there will be many .svn included under windows)
distribute()
{
	# pack all files of the current release
	FILELISTE=`find simutrans -type f "(" -name "*.tab" -o -name "*.mid" -o -name "*.bdf" -o -name "*.fnt" -o -name "*.txt"  -o -name "*.dll"  -o -name "*.pak" ")"`
	zip -9 $simarchiv.zip $FILELISTE simutrans/simutrans$simexe
}

# now built the archive for distribution
cd simutrans
cp ../sim$simexe ./simutrans$simexe
strip simutrans$simexe
cd text
# get the translations for basis
# The first file is longer, but only because it contains SQL error messages
# - discard it after complete download (although parsing it would give us the archive's name):
wget --post-data "version=0&choice=all&submit=Export!" --delete-after http://simutrans-germany.com/translator/script/wrap.php || {
  echo "Error: generating file language_pack-Base+texts.zip failed (wget returned $?)" >&2;
  exit 3;
}
wget -N http://simutrans-germany.com/translator/data/tab/language_pack-Base+texts.zip || {
  echo "Error: download of file language_pack-Base+texts.zip failed (wget returned $?)" >&2
  rm -f "language_pack-Base+texts.zip"
  exit 4
}
unzip -tv "language_pack-Base+texts.zip" || {
   echo "Error: file language_pack-Base+texts.zip seems to be defective" >&2
   rm -f "language_pack-Base+texts.zip"
   exit 5
}
unzip "language_pack-Base+texts.zip"
rm language_pack-Base+texts.zip
# remove Chris English (may become british ... )
rm ce.tab
cd ../..
distribute
rm simutrans/simutrans$simexe
rm simutrans/text/*.tab
