CFLAGS = -Wall -O -g -c
CXXFLAGS = -Wall -O -g -c

SOURCES= boden.cc  fundament.cc  grund.cc  wasser.cc \
 brueckenboden.cc tunnelboden.cc monorailboden.cc

OBJECTS= boden.o  fundament.o  grund.o  wasser.o \
 brueckenboden.o tunnelboden.o monorailboden.o


all:   wege_sub $(OBJECTS)


wege_sub:
	$(MAKE) -e -C wege


depend:
	makedepend $(SOURCES)
	cd wege; make depend; cd ..



dep: depdep
	cd wege; make dep; cd ..


depdep: $(SOURCES)
	gcc -M $^ > .depend


ifeq (.depend,$(wildcard .depend))
    # include .depend only if it exists
include .depend
endif


# DO NOT DELETE
