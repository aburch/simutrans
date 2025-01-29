#!/bin/bash
#
packen()
{
	# pack all files of the current release
	FILELISTE=`find . -type f "(" -name "*.c" -o -name "*.h" -o -name "*.cc" -o -name "*.mm" ")" -not -path */.vs/* -not -path */vcpkg_installed/*`
	FILELISTF=`find . -type f "(" -name "*.sh" -o -name "*.mk"  -o -name "*.nut" ")" -not -path */.vs/* -not -path */vcpkg_installed/*`
	FILELISTG=`find . -type f "(" -name "*.txt" -o -name "*.bdf"  -o -name "*.fnt"  -o -name "*.mid"  -o -name "*.tab" ")" -not -path */.vs/* -not -path */vcpkg_installed/*`
	FILELISTH=`find . -type f "(" -name "*.tab" -o -name "*.dat"  -o -name "*.png" -o -name "*.svg" -o -name "*.desktop" ")" -not -path */.vs/* -not -path */vcpkg_installed/*`
	FILELISTI=`find . -type f "(" -name "*.vcxproj" -o -name "*.vcxproj.filters"  -o -name "*.sln" -o -name "*.vcxitems" -o -name "*.json" ")" -not -path */.vs/* -not -path */vcpkg_installed/*`
	rm ../simutrans-src.zip
	zip ../simutrans-src.zip $FILELISTE
	zip ../simutrans-src.zip $FILELISTF
	zip ../simutrans-src.zip $FILELISTG
	zip ../simutrans-src.zip $FILELISTH
	zip ../simutrans-src.zip $FILELISTI
	zip ../simutrans-src.zip Makefile src/makeobj/Makefile src/nettool/Makefile
	zip ../simutrans-src.zip cmake/*.cmake
	zip ../simutrans-src.zip configure.ac config.default.in config.template src/OSX/simutrans.icns
	zip -r ../simutrans-src.zip src/Windows
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
cd ..
autoconf
packen
