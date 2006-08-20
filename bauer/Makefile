SOURCES=\
warenbauer.cc\
fabrikbauer.cc\
tunnelbauer.cc\
hausbauer.cc\
wegbauer.cc\
brueckenbauer.cc\
vehikelbauer.cc


OBJECTS=\
warenbauer.o\
fabrikbauer.o\
tunnelbauer.o\
hausbauer.o\
wegbauer.o\
brueckenbauer.o\
vehikelbauer.o


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
