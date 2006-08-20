CFLAGS = -Wall -O -g -c
CXXFLAGS = -Wall -O -g -c

SOURCES= \
schiene.cc strasse.cc weg.cc dock.cc

OBJECTS= \
schiene.o strasse.o weg.o dock.o


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
