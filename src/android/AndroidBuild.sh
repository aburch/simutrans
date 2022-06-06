#!/bin/sh

cd simutrans

ln -sf libbzip2.so ../../../../obj/local/$1/libbz2.so

rm -f config.$1.txt
echo VERBOSE=1 >> config.$1.txt
echo OPTIMIZE=1 >> config.$1.txt
echo OSTYPE=linux >> config.$1.txt
echo COLOUR_DEPTH=16 >> config.$1.txt
echo BACKEND=sdl2 >> config.$1.txt
echo USE_SOFTPOINTER=1 >> config.$1.txt
echo USE_FREETYPE=1 >> config.$1.txt
echo USE_FLUIDSYNTH_MIDI=1 >> config.$1.txt
echo WITH_REVISION=1 >> config.$1.txt
echo MSG_LEVEL=3 >> config.$1.txt
echo DEBUG=1 >> config.$1.txt
echo FLAGS ?= -DUSE_OWN_PAKINSTALL

cmake -E copy_if_different config.$1.txt config.$1

echo "#define REVISION `sh tools/get_revision.sh`" > revision.h.txt
cmake -E copy_if_different revision.h.txt revision.h

env CFLAGS="-fpermissive -fvisibility=hidden -fvisibility-inlines-hidden -ffunction-sections -fdata-sections" \
	LDFLAGS="-L`pwd`/../../../../obj/local/$1 -Wl,--gc-sections -s " \
	PATH=`pwd`/../..:$PATH \
	../../setEnvironment-$1.sh sh -c " \
		make -j8 CFG=$1 && \
		cp -f build/$1/sim ../libapplication-$1.so" || exit 1
