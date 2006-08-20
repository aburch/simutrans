
#OSTYPE = beos
#OSTYPE = cygwin
#OSTYPE = freebsd
#OSTYPE = linux
OSTYPE = mingw

LN= $(CC)

CFLAGS=-O
CXXFLAGS=-O

ifeq ($(OSTYPE),mingw)
LDLIBS=-lmingw32 -lpng -lz -lstdc++
endif

ifeq ($(OSTYPE),linux)
LDLIBS=-lpng -lz -lstdc++ -lm
endif

ifeq ($(OSTYPE),beos)
LDLIBS=-lpng -lz -lstdc++
endif


SOURCES=makeobj.cc

OBJECTS=makeobj.o ../besch/writer/obj_node.o \
 ../simmem.o \
 ../dataobj/freelist.o \
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
 ../besch/writer/sim_writer.o ../besch/writer/roadsign_writer.o \
 ../besch/writer/get_waytype.o \
 ../simdebug.o ../utils/log.o ../simtools.o

all: makeobj

makeobj: $(OBJECTS)
	$(LN) -s $(CXXFLAGS) $(LDFLAGS) -o makeobj $(OBJECTS) $(LDLIBS)

clean:
	rm -f $(OBJECTS)

depend:
	makedepend $(SOURCES)

dep: depdep


depdep: $(SOURCES)
	$(CC) -M $^ > .depend


ifeq (.depend,$(wildcard .depend))
    # include .depend only if it exists
include .depend
endif
