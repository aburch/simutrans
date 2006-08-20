#CXX = g++
CXXFLAGS = -O

SOURCES=memblock.cc mempool.cc

OBJECTS=memblock.o mempool.o


all: $(OBJECTS)


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
