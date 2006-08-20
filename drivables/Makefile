#CXX = g++
CXXFLAGS = -O

SOURCES=\
car_t.cc car_group_t.cc


OBJECTS=\
car_t.o car_group_t.o


all: $(OBJECTS)


dep: depdep


depdep: $(SOURCES)
	gcc -M $^ > .depend


ifeq (.depend,$(wildcard .depend))
    # include .depend only if it exists
include .depend
endif
