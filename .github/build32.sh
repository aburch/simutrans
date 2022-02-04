#!/bin/sh
# builds 32bit GDI for windows

# libbrotli static is broken in MinGW for freetype2
for f in libbrotlidec libbrotlienc libbrotlicommon; do
	mv "/mingw32/lib/$f.dll.a" "/mingw32/lib/$f.dll._"
	cp "/mingw32/lib/$f-static.a" "/mingw32/lib/$f.a"
done

# normal build
autoconf
make clean
sh ./configure
echo "FLAGS = -DREVISION=$(svn info --show-item revision svn://servers.simutrans.org/simutrans) " >>config.default
echo "STATIC = 1" >>config.default
%echo "VERBOSE = 1" >>config.default
cat config.default >2
make
sh tools/distribute.sh
mv simu*.zip simuwin-gdi32-nightly.zip
