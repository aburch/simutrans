#!/bin/sh
# builds 32bit GDI for windows

# libbrotli static is broken in MinGW for freetype2
for f in libbrotlidec libbrotlienc libbrotlicommon; do
	mv "/mingw32/lib/$f.dll.a" "/mingw32/lib/$f.dll._"
	cp "/mingw32/lib/$f-static.a" "/mingw32/lib/$f.a"
done

# normal build
echo "BACKEND = gdi" >config.default
echo "OSTYPE = mingw" >>config.default
echo "COLOUR_DEPTH =16" >>config.default
echo "DEBUG = 0" >>config.default
echo "MSG_LEVEL = 3" >>config.default
echo "OPTIMISE = 1" >>config.default
echo "MULTI_THREAD = 1" >>config.default
echo "WIN32_CONSOLE = 1" >>config.default
echo "USE_ZSTD = 1" >>config.default
echo "USE_FREETYPE = 1" >>config.default
echo "WITH_REVISION = $(svn info --show-item revision svn://servers.simutrans.org/simutrans)" >>config.default
echo "STATIC = 1" >>config.default
make -j
mv build/default/sim.exe sim-WinGDI-OTRP.exe

