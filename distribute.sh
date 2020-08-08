#!/bin/bash

#
# This file is part of the Simutrans-Extended project under the Artistic License.
# (see LICENSE.txt)
#

#
# parameters (in this order):
# "-no-lang" prevents downloading the translations
# "-no-rev" do not include revision number in zip file name
# "-rev=###" overide SDL revision with ## (number)


# first assume unix name defaults ...
simexe=
updatepath="/"
updater="get_pak.sh"

OST=unknown
# now get the OSTYPE from config.default and remove all spaces around
OST=`grep "^OSTYPE" config.default | sed "s/OSTYPE[ ]*=[ ]*//" | sed "s/[ ]*\#.*//"`

PGC=0
# now get the OSTYPE from config.default and remove all spaces around
PGC=`grep "^BUNDLE_PTHREADGC2" config.default | sed "s/BUNDLE_PTHREADGC2[ ]*=[ ]*//" | sed "s/[ ]*\#.*//"`

BUILDDIR=`grep "^PROGDIR" config.default | sed "s/PROGDIR[ ]*=[ ]*//" | sed "s/[ ]*\#.*//"`
if [ -n "$BUILDDIR" ]; then
  BUILDDIR=../simutrans-extended
else
  BUILDDIR=../build/default/simutrans-extended
fi

# now make the correct archive name
simexe=
if [ "$OST" = "mac" ]; then
  simarchivbase=simumac
elif [ "$OST" = "haiku" ]; then
 simarchivbase=simuhaiku
elif [ "$OST" = "mingw" ]; then
  simexe=.exe
  SDLTEST=`grep "^BACKEND =" config.default | sed "s/BACKEND[ ]*=[ ]*//" | sed "s/[ ]*\#.*//"`
  if [ "$SDLTEST" = "sdl" ]  ||  [ "$SDLTEST" = "sdl2" ]; then
    simarchivbase=simuwin-sdl
  else
    simarchivbase=simuwin
# Missing: Copy matching SDL dll!
  fi
  updatepath="/nsis/"
  updater="download-paksets.exe"
elif [ "$OST" = "linux" ]; then
 simarchivbase=simulinux
elif [ "$OST" = "freebsd" ]; then
 simarchivbase=simubsd
elif [ "$OST" = "amiga" ]; then
 simarchivbase=simuamiga
fi


# now add revesion number without any modificators
# fetch language files
if [ `expr match "$*" ".*-rev="` != "0" ]; then
  REV_NR=$(echo $* | sed "s/.*-rev=[ ]*//" | sed "s/[^0-9]*//")
  simarchiv=$simarchivbase-$REV_NR
elif [ "$#" = "0"  ]  ||  [ `expr match "$*" ".*-no-rev"` = "0" ]; then
  REV_NR=`svnversion | sed "s/[0-9]*://" | sed "s/M.*//"`
  simarchiv=$simarchivbase-$REV_NR
else
  echo "No revision given!"
  simarchiv=$simarchivbase
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
		hdiutil eject /Volumes/SDL2 >>/dev/stderr
	echo "APPL????" > "simutrans.app/Contents/PkgInfo"
	sh ../OSX/plistgen.sh "simutrans.app" "simutrans"
}

# fetch language files
if [ "$#" = "0"  ]  ||  [ `expr match "$*" "-no-lang"` = "0" ]; then
  sh ./get_lang_files.sh
fi

# now built the archive for distribution
cd simutrans
if [ $OSTYPE = darwin* ]; then
  buildOSX
  ls
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
