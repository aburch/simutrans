SOURCES=\
platzsucher.cc

OBJECTS=\
platzsucher.o


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
