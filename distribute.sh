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
	FILELISTE=`find simutrans -type f "(" -name "*.tab" -o -name "*.mid" -o -name "*.bdf" -o -name "*.fnt" -o -name "*.txt"  -o -name "*.dll" ")"`
	zip -9 $simarchiv.zip $FILELISTE simutrans/simutrans$simexe
}

# now built the archive for distribution
cd simutrans
cp ../sim$simexe ./simutrans$simexe
strip simutrans$simexe
cd text
wget http://translator.simutrans.com/tmp/language_pack-Base+texts.zip
unzip language_pack-Base+texts.zip
rm language_pack-Base+texts.zip
rm ce.tab
cd ../..
distribute
rm simutrans/simutrans$simexe
rm simutrans/text/*.tab
