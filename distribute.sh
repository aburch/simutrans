#!/bin/bash
#
# parameters (in this order):
# "-no-lang" prevents downloading the translations
# "-no-rev" do not include revision number in zip file name
# "-rev=###" overide SDL revision with ## (number)


# get pthreads DLL
getDLL()
{
	# Use curl if available, else use wget
	echo "Downloading pthreadGC2.dll"
	curl -q -h > /dev/null
	if [ $? -eq 0 ]; then
		curl -q ftp://sourceware.org/pub/pthreads-win32/dll-latest/dll/x86/pthreadGC2.dll > pthreadGC2.dll || {
		echo "Error: download of PthreadGC2.dll failed (curl returned $?)" >&2
		exit 4
	}
	else
		wget -q --help > /dev/null
		if [ $? -eq 0 ]; then
			wget -q -N ftp://sourceware.org/pub/pthreads-win32/dll-latest/dll/x86/pthreadGC2.dll || {
			echo "Error: download of PthreadGC2.dll failed (wget returned $?)" >&2
			exit 4
		}
		else
			echo "Error: Neither curl or wget are available on your system, please install either and try again!" >&2
			exit 6
		fi
	fi
}

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
  BUILDDIR=../sim
else
  BUILDDIR=../build/default/sim
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
  cd simutrans

  if [ "$PGC" -ne 0	]; then
    getDLL
  fi
  cd ..
  updatepath="/nsis/"
  updater="download-paksets.exe"
  cd nsis
  makensis onlineupgrade.nsi
  cd ..
elif [ "$OST" = "linux" ]; then
 simarchivbase=simulinux
elif [ "$OST" = "freebsd" ]; then
 simarchivbase=simubsd
elif [ "$OST" = "amiga" ]; then
 simarchivbase=simuamiga
fi

# Test if there is something to distribute ...
if [ ! -f ./simutrans/$BUILDDIR$simexe ]; then
  echo "No simutrans executable found! Aborted!"
  exit 1
fi

# now add revision number without any modificators
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
	FILELISTE=`find simutrans -type f "(" -name "*.tab" -o -name "*.mid" -o -name "*.bdf" -o -name "*.fnt" -o -name "*.txt"  -o -name "*.dll" -o -name "*.pak" -o -name "*.nut" -o -name "*.dll" ")"`
	zip -9 $simarchiv.zip $FILELISTE simutrans/simutrans$simexe simutrans/$updater
}

buildOSX()
{
	# builds a bundle for MAC OS
	mkdir -p "simutrans.app/Contents/MacOS"
	mkdir -p "simutrans.app/Contents/Resources"
	cp $BUILDDIRsim.exe   "simutrans.app/Contents/MacOS/simutrans"
	strip "simutrans.app/Contents/MacOS/simutrans"
	cp "../OSX/simutrans.icns" "simutrans.app/Contents/Resources/simutrans.icns"
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
else
  cp $BUILDDIR$simexe ./simutrans$simexe
  strip simutrans$simexe
fi
cp ..$updatepath$updater $updater
cd ..
distribute

# .. finally delete executable and language files
rm simutrans/simutrans$simexe

# cleanup dll's
if [ "$PGC" -ne 0 ]; then
  rm simutrans/pthreadGC2.dll
fi

# swallow any error values, return success in any case
exit 0
