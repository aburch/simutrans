CFLAGS = -Wall -O -g -c
CXXFLAGS = -Wall -O -g -c

SOURCES= \
schiene.cc strasse.cc weg.cc kanal.cc runway.cc

OBJECTS= \
schiene.o strasse.o weg.o kanal.o runway.o


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
