#CC=gcc
CXX=g++

CROSSCC=/usr/local/src/cross-tools/bin/i386-mingw32msvc-gcc

LN=$(CC)


CFLAGS=-O2 -fomit-frame-pointer

MP_LDLIBS=-lpng -lz
LDFLAGS=

SOURCES=dr_rdpng.c makepak.c image_encoder.c\
simstring.c writepcx.c log.cc cstring_t.cc tocstring.cc\
cbuffer_t.cc searchfolder.cc

MP_OBJECTS=dr_rdpng.o makepak.o
OBJECTS=writepcx.o image_encoder.o\
simstring.o log.o cstring_t.o tocstring.o\
cbuffer_t.o searchfolder.o

all: sim_obj makepak


sim_obj: $(OBJECTS)


makepak:   $(MP_OBJECTS)
	$(LN) -static $(LDFLAGS) -o makepak $(MP_OBJECTS) $(MP_LDLIBS)
	strip makepak

cross:
	$(CROSSCC) -O2 -o makepak.exe -fomit-frame-pointer dr_rdpng.c makepak.c $(MP_LDLIBS)


depend:
	makedepend $(SOURCES)


dep: depdep


depdep: $(SOURCES)
	gcc -M $^ > .depend


ifeq (.depend,$(wildcard .depend))
    # include .depend only if it exists
include .depend
endif

# DO NOT DELETE
