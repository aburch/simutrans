#CXX = g++
CFLAGS = -Wall -O -g -c
CXXFLAGS = -Wall -O -g -c

SOURCES=wolke.cc raucher.cc zeiger.cc baum.cc bruecke.cc tunnel.cc\
gebaeude.cc signal.cc\
leitung2.cc lagerhaus.cc oberleitung.cc roadsign.cc dummy.cc

OBJECTS=wolke.o raucher.o zeiger.o baum.o bruecke.o tunnel.o\
gebaeude.o signal.o\
leitung2.o lagerhaus.o oberleitung.o roadsign.o dummy.o


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

# DO NOT DELETE
