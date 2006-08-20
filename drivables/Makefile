#CXX = g++
CXXFLAGS = -O

SOURCES=\
car_t.cpp car_group_t.cpp


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
