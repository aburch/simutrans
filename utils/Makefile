CROSSCC=/usr/local/src/cross-tools/bin/i386-mingw32msvc-gcc

LN=$(CC)


MP_LDLIBS=-lpng -lz
LDFLAGS=

SOURCES += cbuffer_t.cc
SOURCES += cstring_t.cc
SOURCES += log.cc
SOURCES += searchfolder.cc
SOURCES += simstring.c
SOURCES += tocstring.cc

include ../common.mk


MP_SOURCES += dr_rdpng.c
MP_SOURCES += makepak.c

MP_OBJECTS = $(MP_SOURCES:%.c=%.o)


makepak:   $(MP_OBJECTS)
	$(LN) -static $(LDFLAGS) -o makepak $(MP_OBJECTS) $(MP_LDLIBS)
	strip makepak

cross:
	$(CROSSCC) -O2 -o makepak.exe -fomit-frame-pointer dr_rdpng.c makepak.c $(MP_LDLIBS)
