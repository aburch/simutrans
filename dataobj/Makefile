#CXX = g++
CXXFLAGS = -O

SOURCES=\
freelist.cc\
translator.cc ribi.cc koord.cc koord3d.cc\
linie.cc fahrplan.cc route.cc einstellungen.cc\
warenziel.cc umgebung.cc koord3d.cc dingliste.cc marker.cc\
loadsave.cc tabfile.cc powernet.cc

OBJECTS=\
freelist.o\
translator.o ribi.o koord.o koord3d.o\
linie.o fahrplan.o route.o einstellungen.o\
warenziel.o umgebung.o koord3d.o dingliste.o marker.o\
loadsave.o tabfile.o powernet.o


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
