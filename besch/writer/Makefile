CFLAGS = -Wall -O -g -c
CXXFLAGS = -Wall -O -g -c

SOURCES=building_writer.cc good_writer.cc skin_writer.cc \
 image_writer.cc imagelist2d_writer.cc imagelist_writer.cc \
 obj_node.cc obj_writer.cc root_writer.cc factory_writer.cc \
 text_writer.cc tree_writer.cc xref_writer.cc bridge_writer.cc \
 sim_writer.cc \
 vehicle_writer.cc tunnel_writer.cc citycar_writer.cc pedestrian_writer.cc

OBJECTS=building_writer.o good_writer.o skin_writer.o \
 image_writer.o imagelist2d_writer.o imagelist_writer.o \
 obj_node.o obj_writer.o root_writer.o xref_writer.o \
 text_writer.o tree_writer.o factory_writer.o bridge_writer.o \
 sim_writer.o \
 vehicle_writer.o tunnel_writer.o citycar_writer.o pedestrian_writer.o

all:   $(OBJECTS)

depend:
	makedepend $(SOURCES)

dep: depdep


depdep: $(SOURCES)
	gcc -M $^ > .depend


ifeq (.depend,$(wildcard .depend))
    # include .depend only if it exists
include .depend
endif
