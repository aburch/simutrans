#!/bin/bash
#

# first assum unix ...
simexe=
updatepath="/"
updater="get_pak.sh"

if [[ $OSTYPE = darwin* ]]; then
  simarchiv=simumac
elif [[ $OSTYPE = beos* ]]; then
 simarchiv=simuhaiku
elif [[ $OSTYPE = msys* ]]; then
  simexe=.exe
  simarchiv=simuwin
  echo "Windows"
  updatepath="/nsis/"
  updater="download-paksets.exe"
else
 simarchiv=simulinux
 echo "$OSTYPE: I assume linux distribution is ok for it"
fi


# (otherwise there will be many .svn included under windows)
distribute()
{
	# pack all files of the current release
	FILELISTE=`find simutrans -type f "(" -name "*.tab" -o -name "*.mid" -o -name "*.bdf" -o -name "*.fnt" -o -name "*.txt"  -o -name "*.dll"  -o -name "*.pak" -o  -name "*.nut" ")"`
	zip -9 $simarchiv.zip $FILELISTE simutrans/simutrans$simexe simutrans/$updater
}

buildOSX()
{
	# builds a bundle for MAC OS
	mkdir -p "simutrans.app/Contents/MacOS"
	mkdir -p "simutrans.app/Contents/Resources"
	cp ../sim.exe   "simutrans.app/Contents/MacOS/simutrans"
	strip "simutrans.app/Contents/MacOS/simutrans"
	cp "../OSX/simutrans.icns" "simutrans.app/Contents/Resources/simutrans.icns"
	echo "APPL????" > "simutrans.app/Contents/PkgInfo"
	sh ../OSX/plistgen.sh "simutrans.app" "simutrans"
}

# fetch language files
sh ./get_lang_files.sh

# now built the archive for distribution
cd simutrans
if [[ $OSTYPE = darwin* ]]; then
  buildOSX
else
  cp ../sim$simexe ./simutrans$simexe
  strip simutrans$simexe
fi
cp ..$updatepath$updater $updater
cd ..
distribute

# .. finally delete executable and language files
rm simutrans/simutrans$simexe
#rm simutrans/text/*.tab
