#!/bin/bash
#
packen()
{
	# pack all files of the current release
	FILELISTE=`find . -type f "(" -name "*.c" -o -name "*.h" -o -name "*.cc" -o -name "*.mm" ")"`
	FILELISTF=`find . -type f "(" -name "*.sh" -o -name "*.mk"  -o -name "*.nut" ")"`
	FILELISTG=`find . -type f "(" -name "*.txt" -o -name "*.bdf"  -o -name "*.fnt"  -o -name "*.mid"  -o -name "*.tab" ")"`
	FILELISTH=`find . -type f "(" -name "*.rtf" -o -name "*.bmp"  -o -name "*.nsi"  -o -name "*.nsh" ")"`
	FILELISTI=`find . -type f "(" -name "*.tab" -o -name "*.dat"  -o -name "*.png" -o -name "*.svg" ")"`
	rm ../simutrans-src.zip
	zip ../simutrans-src.zip $FILELISTE
	zip ../simutrans-src.zip $FILELISTF
	zip ../simutrans-src.zip $FILELISTG
	zip ../simutrans-src.zip $FILELISTH
	zip ../simutrans-src.zip $FILELISTI
	zip ../simutrans-src.zip Makefile makeobj/Makefile nettools/Makefile
	zip ../simutrans-src.zip cmake/*.cmake
	zip ../simutrans-src.zip Simutrans.manifest configure.ac config.default.in config.template simutrans.ico old.ico stormoog.ico simres.rc OSX/simutrans.icns
	zip ../simutrans-src.zip documentation/*.pal simutrans/themes/*.pak
}

# not used (stumbles in private: in class definition ... :( )
beautify()
{
  # convert all source files and reformats them
  FILELISTE=`find . -type f "(" -name "*.c" -o -name "*.h" -o -name "*.cc" ")"`
  for file in $FILELISTE; do
    bcpp -f 2 -t -i 2 -ylcnc -yq bcl -yo -qb 1000 < $file > _tmpfile
    touch -r $file _tmpfile
    echo "bcpp $file"
    cp -f _tmpfile $file
    rm -f _tmpfile
  done
}

# take action!
cd trunk
autoconf
packen
