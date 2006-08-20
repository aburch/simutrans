#CXX = g++
CXXFLAGS = -O

SOURCES=worldtest.cc testtool.cc
#buildings_frame_t.cc buildings_stats_t.cc

OBJECTS=worldtest.o testtool.o
#buildings_frame_t.o buildings_stats_t.o


all: $(OBJECTS)

memtest:
	g++ -o memtest memtest.cc ../simmem.o ../simdebug.o ../utils/log.o

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
