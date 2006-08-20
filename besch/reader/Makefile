CFLAGS = -Wall -O -g -c
CXXFLAGS = -Wall -O -g -c

SOURCES=obj_reader.cc root_reader.cc xref_reader.cc \
 building_reader.cc good_reader.cc tree_reader.cc factory_reader.cc skin_reader.cc \
 image_reader.cc sim_reader.cc \
 bridge_reader.cc tunnel_reader.cc vehicle_reader.cc \
 way_reader.cc ground_reader.cc crossing_reader.cc \
 citycar_reader.cc pedestrian_reader.cc roadsign_reader.cc


OBJECTS=obj_reader.o root_reader.o xref_reader.o \
 building_reader.o good_reader.o tree_reader.o factory_reader.o skin_reader.o \
 image_reader.o sim_reader.o \
 bridge_reader.o tunnel_reader.o vehicle_reader.o \
 way_reader.o ground_reader.o crossing_reader.o \
 citycar_reader.o pedestrian_reader.o roadsign_reader.o


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
