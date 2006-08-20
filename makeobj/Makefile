
#OSTYPE=mingw
OSTYPE=linux-gnu


# Compilers, C, C++
CC= gcc
CXX=g++


# Mingw32 cross compiler

ifeq ($(SIM_CROSS),true)
CC=/usr/local/src/cross-tools/bin/i386-mingw32msvc-gcc
CXX=/usr/local/src/cross-tools/bin/i386-mingw32msvc-g++
endif

LN= $(CC)

CFLAGS=-O
CXXFLAGS=-O

#CFLAGS=-g
#CXXFLAGS=-g

ifeq ($(OSTYPE),mingw)
#-lSDLmain -lSDL -lwinmm
LDLIBS=-lmingw32 -lpng -lz -lmingwex
else
LDLIBS=-lpng -lz -lm -lstdc++
endif
LDFLAGS=

SOURCES=makeobj.cc

OBJECTS=makeobj.o ../besch/writer/obj_node.o \
 ../simmem.o \
 ../dataobj/nodelist_t.o ../dataobj/nodes_12.o \
 ../dataobj/tabfile.o ../utils/cstring_t.o ../utils/searchfolder.o \
 ../tpl/debug_helper.o ../tpl/no_such_element_exception.o \
 ../makepak16/dr_rdpng.o \
 ../besch/writer/building_writer.o ../besch/writer/good_writer.o \
 ../besch/writer/skin_writer.o ../besch/writer/image_writer.o \
 ../besch/writer/imagelist2d_writer.o ../besch/writer/imagelist_writer.o \
 ../besch/writer/obj_writer.o ../besch/writer/root_writer.o \
 ../besch/writer/xref_writer.o ../besch/writer/text_writer.o \
 ../besch/writer/tree_writer.o ../besch/writer/factory_writer.o \
 ../besch/writer/bridge_writer.o ../besch/writer/vehicle_writer.o \
 ../besch/writer/way_writer.o ../besch/writer/ground_writer.o \
 ../besch/writer/crossing_writer.o  ../besch/writer/tunnel_writer.o \
 ../besch/writer/citycar_writer.o  ../besch/writer/pedestrian_writer.o \
 ../besch/writer/sim_writer.o\
 ../simdebug.o ../utils/log.o

all: makeobj

makeobj: $(OBJECTS)
	$(LN) -static $(CXXFLAGS) $(LDFLAGS) -o makeobj $(OBJECTS) $(LDLIBS)
	strip makeobj

win32: $(OBJECTS)
	$(LN) -static $(CXXFLAGS) $(LDFLAGS) -o makeobj.exe $(OBJECTS) $(LDLIBS)


clean:
	rm -f $(OBJECTS)

depend:
	makedepend $(SOURCES)


dep: depdep


depdep: $(SOURCES)
	gcc -M $^ > .depend


ifeq (.depend,$(wildcard .depend))
    # include .depend only if it exists
include .depend
endif
