#!/bin/sh
# builds for macOS

# remove dynamic libraries to force static linking
rm /opt/homebrew/opt/freetype/lib/libfreetype.dylib
rm /opt/homebrew/opt/libpng/lib/libpng16.dylib
rm /opt/homebrew/opt/zstd/lib/libzstd.dylib
rm /opt/homebrew/opt/sdl2/lib/libSDL2.dylib 

# normal build
echo "BACKEND = sdl2" >config.default
echo "OSTYPE = mac" >>config.default
echo "COLOUR_DEPTH =16" >>config.default
echo "DEBUG = 0" >>config.default
echo "MSG_LEVEL = 3" >>config.default
echo "OPTIMISE = 1" >>config.default
echo "STATIC = 1" >>config.default
echo "MULTI_THREAD = 1" >>config.default
echo "USE_ZSTD = 1" >>config.default
echo "USE_FREETYPE = 1" >>config.default
echo "WITH_REVISION = 9281" >>config.default
echo "AV_FOUNDATION = 1" >>config.default
echo "FLAGS = -std=c++20" >>config.default
make -j4
mv build/default/sim sim-mac-OTRP
