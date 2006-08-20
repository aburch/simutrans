CFLAGS = -Wall -O -g -c
CXXFLAGS = -Wall -O -g -c

SOURCES=ware_besch.cc haus_besch.cc bruecke_besch.cc grund_besch.cc tunnel_besch.cc


OBJECTS=ware_besch.o haus_besch.o bruecke_besch.o grund_besch.o tunnel_besch.o


# all:   reader_sub writer_sub $(OBJECTS)

all:   reader_sub $(OBJECTS)

reader_sub:
	$(MAKE) -e -C reader

writer_sub:
	$(MAKE) -e -C writer

dep: depdep
	cd reader; make dep; cd ..
	cd writer; make dep; cd ..

depdep: $(SOURCES)
	gcc -M $^ > .depend


ifeq (.depend,$(wildcard .depend))
    # include .depend only if it exists
include .depend
endif
