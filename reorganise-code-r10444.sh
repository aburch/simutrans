#!/bin/bash

if [[ -n "$(ls | grep -e '.editorconfig')" ]]; then
	echo "Wrong directory!" >&2
	exit 1
fi

rm -rf revision.h
mkdir -p src/simutrans
mkdir -p src/simutrans/tool
mkdir -p src/Windows

svn add src

declare -a PATHREPLACE=("s|\.\./squirrel/|\.\./\.\./squirrel/|")
sed -i 's|#include "\.\./|#include "\.\./simutrans/|' squirrel/*.{h,cc}
svn mv  squirrel src/

sed -i 's|#include "\.\.|#include "\.\./simutrans|' makeobj/*.{h,cc}
svn mv  makeobj src/

svn mv gui src/simutrans
svn mv obj src/simutrans

svn mv  dataobj descriptor display io music network player script sound sys tpl utils vehicle world src/simutrans/

PATHREPLACE+=("s|boden/wege/|obj/way/|")
sed -i -E 's|#include \"\.\./([^/]+\")|#include \"\.\./\.\./ground/\1|' boden/wege/*.{h,cc}
svn mv  boden/wege/ src/simutrans/obj/way

PATHREPLACE+=("s|bauer/|builder/|")
svn mv  bauer src/simutrans/builder

PATHREPLACE+=("s|boden/|ground/|")
sed -i 's|wege/|../obj/way/|' boden/*.{h,cc}
svn mv  boden src/simutrans/ground

PATHREPLACE+=("s|nettools/|nettool/|")
sed -i 's|#include "..|#include "../simutrans|' nettools/*.{h,cc}
svn mv nettools src/nettool

# Additional reorganizations

PATHREPLACE+=("s|unicode\.cc|utils/unicode\.cc|" "s|unicode\.h|utils/unicode\.h|")
sed -i 's|#include "|#include "../|' unicode.{h,cc}
sed -i 's|#include "../utils/|#include "|' unicode.{h,cc}
svn mv unicode.{h,cc} src/simutrans/utils

PATHREPLACE+=("s|simdepot\.cc|obj/depot\.cc|" "s|simdepot\.h|obj/depot\.h|")
sed -i 's|#include "|#include "../|' simdepot.{h,cc}
sed -i 's|#include "../obj/|#include "|' simdepot.{h,cc}
svn mv simdepot.cc src/simutrans/obj/depot.cc
svn mv simdepot.h src/simutrans/obj/depot.h

PATHREPLACE+=("s|simmenu\.cc|tool/simmenu\.cc|" "s|simmenu\.h|tool/simmenu\.h|")
sed -i 's|#include "|#include "../|' simmenu.{h,cc}
svn mv  simmenu.{h,cc} src/simutrans/tool/

PATHREPLACE+=("s|simtool|tool/simtool|")
sed -i 's|#include "|#include "../|' simtool*
svn mv simtool* src/simutrans/tool/

PATHREPLACE+=("s|simworld\.cc|world/simworld\.cc|" "s|simworld\.h|world/simworld\.h|")
sed -i 's|#include "|#include "../|' simworld.{h,cc}
svn mv simworld.{h,cc} src/simutrans/world/

PATHREPLACE+=("s|simplan\.cc|world/simplan\.cc|" "s|simplan\.h|world/simplan\.h|")
sed -i 's|#include "|#include "../|' simplan.{h,cc}
svn mv simplan.{h,cc} src/simutrans/world/

PATHREPLACE+=("s|simcity\.cc|world/simcity\.cc|" "s|simcity\.h|world/simcity\.h|")
sed -i 's|#include "|#include "../|' simcity.{h,cc}
svn mv simcity.{h,cc} src/simutrans/world/

find . -maxdepth 1 -type f -name "*.cc" -exec svn mv {} src/simutrans \;
find . -maxdepth 1 -type f -name "*.h" -exec svn mv {} src/simutrans \;

PATHREPLACE+=("s|finder/|world/|")
svn mv finder/*.{h,cc} src/simutrans/world
svn rm --force finder

PATHREPLACE+=("s|ifc/simtestdriver\.h|vehicle/simtestdriver\.h|" "s|ifc/sync_steppable\.h|obj/sync_steppable\.h|")
svn mv ifc/simtestdriver.h src/simutrans/vehicle/
svn mv ifc/sync_steppable.h src/simutrans/obj/
svn rm --force ifc

svn mv  simres.rc src/Windows
svn mv  Simutrans.manifest src/Windows
svn mv  nsis src/Windows

svn mv  OSX src
svn mv  android src

svn mv scripts tools
svn mv distribute.sh tools
svn mv findversion.sh tools
svn mv get_lang_files.* tools
svn mv get_pak.* tools
svn mv get_revision.* tools
svn mv install-building-libs* tools
svn mv revision.jse tools
svn mv ../base.tab src
svn mv ../zipsrc.sh tools

svn mv ../patches src/

svn mv  *.ico src/Windows/
svn mv  simutrans.svg src/simutrans/
svn mv  .desktop src/simutrans/

find . -name '*_t.cc' -exec bash -c 'svn mv $0 ${0/_t\.cc/\.cc}' {} \;
find . -name '*_t.h' -exec bash -c 'svn mv $0 ${0/_t\.h/\.h}' {} \;

echo "Remove _t suffix from files"
# remove _t also in the include files
sed -i 's|_t\.|\.|' cmake/SimutransSourceList.cmake
sed -i 's|_t\.|\.|' Simutrans-Main.vcxitems
find . -type f -name "Makefile" -exec  sed -E -i 's|_t\.|\.|' {} \;

includereplace="-e s|_t\.h\\\"|\.h\\\"|"
# process accumulated path and name changes in include files
for pattern in "${PATHREPLACE[@]}"
do
	echo "Replacing $pattern"
	if ! [[ $pattern == *".cc|"* ]]; then
		includereplace="${includereplace} -e ${pattern} "
	fi
	sed -i "$pattern"  cmake/SimutransSourceList.cmake
	frontpattern=`echo $pattern | sed -E 's-(s\|.+\|).+\|-\1-'`
	backpattern=`echo $pattern | sed -E 's-s\|.+\|(.+\|)-\1-' | sed 's|/|\\\\\\\\|g'`
	mspattern=`echo $pattern | sed 's|/|\\\\\\\\|g'`
	sed -i "$mspattern" Simutrans-Main.vcxitems
	find . -type f -name "Makefile" -exec  sed -E -i "$pattern" {} \;
done

#set -x
echo "Now replace in include files"
find . -type f "(" -name "*.cc" -o -name "*.h" -o -name "*mm" ")" -exec sed -i ${includereplace} {} \;
sed -i 's|../world/||' src/simutrans/world/*.{h,cc}

echo "Final cleaning up build system"
sed -i -E 's|^\t\t([^\\\$]+)|\t\tsrc/simutrans/\1|' cmake/SimutransSourceList.cmake
sed -i 's|makeboj/|add_subdirectory(src/makeobj EXCLUDE_FROM_ALL)|' cmake/SimutransSourceList.cmake
sed -i 's|add_subdirectory(makeobj EXCLUDE_FROM_ALL)|add_subdirectory(src/makeobj EXCLUDE_FROM_ALL)|' CMakeLists.txt
sed -i 's|add_subdirectory(nettools EXCLUDE_FROM_ALL)|add_subdirectory(src/nettool EXCLUDE_FROM_ALL)|' CMakeLists.txt

sed -i 's|Directory)|Directory)src\\simutrans\\|' Simutrans-Main.vcxitems
sed -i 's|ClCompile Include=\"|ClCompile Include=\"src\\simutrans\\|' Simutrans*.vcxproj
sed -i 's|cscript.exe //Nologo revision.jse|cscript.exe //Nologo tools\\revision.jse|' Simutrans*.vcxproj
sed -i 's|revision\.h\"|src\\\\simutrans\\\\revision\.h\"|' tools/revision.jse

sed -i 's|SOURCES *+= |SOURCES += src/simutrans/|' Makefile
sed -i 's|./get_revision.sh|tools/get_revision.sh|' Makefile

sed -i 's| sys/| src/simutrans/sys/|' CMakeLists.txt
sed -i 's| display/| src/simutrans/display/|' CMakeLists.txt
sed -i 's| music/| src/simutrans/music/|' CMakeLists.txt
sed -i 's| sound/| src/simutrans/sound/|' CMakeLists.txt
sed -i 's| io/| src/simutrans/io/|' CMakeLists.txt
sed -i 's| gui/| src/simutrans/gui/|' CMakeLists.txt
sed -i 's|${SOURCE_DIR}/revision.h|${SOURCE_DIR}/src/simutrans/revision.h|' CMakeLists.txt
sed -i 's|${SOURCE_DIR}/revision.h|${SOURCE_DIR}/src/simutrans/revision.h|' cmake/SimutransRevision.cmake
sed -i 's|/nsis|/src/Windows/nsis|' CMakeLists.txt
sed -i 's|get_pak.sh|${CMAKE_SOURCE_DIR}/tools/get_pak.sh|' CMakeLists.txt
sed -i 's|get_pak.ps1|tools/get_pak.ps1|' CMakeLists.txt
sed -i 's|get_lang_files.ps1|tools/get_lang_files.ps1|' CMakeLists.txt
sed -i 's|../get_lang_files.sh|${CMAKE_SOURCE_DIR}/tools/get_lang_files.sh|' CMakeLists.txt
sed -i 's|paksetinfo.h|src/simutrans/paksetinfo.h|' CMakeLists.txt


sed -i 's|#include \"simversion.h\"|#include \"../simutrans/simversion.h\"|' src/Windows/simres.rc
sed -i 's| simres.rc| src/Windows/simres.rc|' CMakeLists.txt
sed -i 's|simutrans\\simres.rc|Windows\\simres.rc|' Simutrans-Main.vcxitems
sed -i 's|src/simutrans/simres.rc|src/Windows/simres.rc|' Makefile

# since squirrel moved up
sed -i 's|simutrans/squirrel|squirrel|' cmake/SimutransSourceList.cmake
sed -i 's|src\\simutrans\\squirrel|src\\squirrel|' Simutrans-Main.vcxitems
find . -type f -name "Makefile" -exec  sed -E -i 's|simutrans/squirrel/|squirrel/|' {} \;

# Fix makeobj/nettool CMakeLists.txt
sed -i 's|../|../simutrans/|' src/makeobj/CMakeLists.txt
sed -i 's|../|../simutrans/|' src/nettool/CMakeLists.txt

# Fix makeobj/nettool Makefile build
sed -i 's|\$(MAKE) -e -C makeobj|$(MAKE) -e -C src/makeobj|' Makefile
sed -i 's|\$(MAKE) -e -C nettools|$(MAKE) -e -C src/nettool|' Makefile
sed -i 's|-include |-include ../|' src/{makeobj,nettool}/Makefile
sed -i 's|\.\./uncommon\.mk|../../uncommon.mk|' src/{makeobj,nettool}/Makefile uncommon.mk
sed -i 's|+= \.\./|+= ../simutrans/|' src/{makeobj,nettool}/Makefile
sed -i 's|_PROGDIR := \.\./|_PROGDIR := ../../|' src/{makeobj,nettool}/Makefile
sed -i 's|BUILDDIR := \.\./|BUILDDIR := ../../|' src/{makeobj,nettool}/Makefile
sed -i 's|\.\./config\.|../../config.|' uncommon.mk
sed -i 's|OBJS := $(patsubst %, $(BUILDDIR)/%-$(TOOL).o, $(basename $(patsubst ../%, %,$(filter ../%,$(SOURCES)))))|OBJS := $(patsubst %, $(BUILDDIR)/%-$(TOOL).o, $(basename $(patsubst ../simutrans/%, %,$(filter ../simutrans/%,$(SOURCES)))))|' uncommon.mk
sed -i 's|OBJS += $(patsubst %, $(BUILDDIR)/$(TOOL)/%-$(TOOL).o, $(basename $(filter-out ../%,$(SOURCES))))|OBJS += $(patsubst %, $(BUILDDIR)/$(TOOL)/%-$(TOOL).o, $(basename $(filter-out ../simutrans/%,$(SOURCES))))|' uncommon.mk
sed -i 's|\.\./%\.c \$(BUILDCONFIG_FILES)|../simutrans/%.cc $(BUILDCONFIG_FILES)|' uncommon.mk
sed -i 's|\.\./%\.cc \$(BUILDCONFIG_FILES)|../simutrans/%.cc $(BUILDCONFIG_FILES)|' uncommon.mk

# TODO Update location of paksetinfo.h in get_pak.sh

# TODO fix distribution and github actions

# prissi prefernces
# TODO add file prefixes to some files (like obj_ for objects)
# TODO rename xxx_vehicle.cc to vehicle_xxx.cc and so on
