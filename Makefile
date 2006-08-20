#
# Makefile
#
# Top-Level Makefile for Simutrans
# Copyright (c) 2001 by Hj. Malthaner
# hansjoerg.malthaner@gmx.de
#


OSTYPE=mingw
#OSTYPE=linux-gnu


ifeq ($(OSTYPE),)
#OSTYPE=cygwin
#OSTYPE=mingw
OSTYPE=linux-gnu
endif

export OSTYPE

#OPT=profile
#OPT=debug
#OPT=optimize
OPT=debug_optimize


ifeq ($(SIM_OPTIMIZE),true)
OPT=optimize
endif



ifeq ($(OSTYPE),cygwin)
OS_INC=-I/SDL/include -I/usr/include/cygwin
STD_LIBS=
SDLLIBS=  -L/SDL/lib -lz -lSDL -lwinmm -lc /lib/mingw/libstdc++.a -lc /lib/mingw/libmingw32.a -lc /lib/mingw/libmsvcrt.a
#SDLLIBS= -lSDL -lwinmm -lc /lib/libmsvcrt40.a -lc /lib/libstdc++.a -lc /lib/libmingw32.a
#SDLLIBS= -lSDL -lwinmm -lpthread
OS_OPT=-Wbad-function-cast
endif
ifeq ($(OSTYPE),mingw)
OS_INC=-I /usr/include/mingw
OS_OPT=-mno-cygwin -DPNG_STATIC -DZLIB_STATIC
#OS_OPT=-Wbad-function-cast
STD_LIBS=-lstdc++ -lz
SDLLIBS= -lmingw32 -lSDLmain -lSDL -lwinmm
endif
ifeq ($(OSTYPE),linux-gnu)
#STD_LIBS= /usr/lib/libstdc++-3-libc6.1-2-2.10.0.a -lz
STD_LIBS= -lstdc++ -lz
SDLLIBS= -lSDL -lpthread
OS_OPT=
endif
ifeq ($(OSTYPE),linux)
STD_LIBS= -lz
SDLLIBS= -lSDL -lpthread
OS_OPT=
endif

BESDLLIBS= -lSDL -lz

# Compilers, C, C++
CC= gcc
CXX=g++

# cross compilers
#CC=/usr/local/src/gcc-2.95.2/gcc/xgcc -B/usr/local/src/gcc-2.95.2/gcc/ -B/usr/local/powerpc-unknown-linux/bin/  -I/usr/local/powerpc-unknown-linux/include
#CXX=/usr/local/src/gcc-2.95.2/gcc/xgcc -B/usr/local/src/gcc-2.95.2/gcc/ -B/usr/local/powerpc-unknown-linux/bin/ -I/usr/local/powerpc-unknown-linux/include
#CC gcc -static -b powerpc-unknown-linux -Wl,"-melf32ppc"
#CXX gcc -static -b powerpc-unknown-linux -Wl,"-melf32ppc"

# Mingw32 cross compiler

ifeq ($(SIM_CROSS),true)
CC=/usr/local/src/cross-tools/bin/i386-mingw32msvc-gcc
CXX=/usr/local/src/cross-tools/bin/i386-mingw32msvc-g++
endif


export CC
export CXX

MAKE= make

# C compiler options
ifeq ($(OPT),profile)
CFLAGS= -Wall -pg -O -pipe -fschedule-insns2 -fgcse -fstrict-aliasing -fexpensive-optimizations -march=i586 -g
LDFLAGS= -pg
endif
ifeq ($(OPT),optimize)
CFLAGS= -Wall -O -fschedule-insns2 -fomit-frame-pointer -fgcse -fstrict-aliasing -fexpensive-optimizations -march=i586 -pipe $(OS_INC)
endif
ifeq ($(OPT),debug)
CFLAGS= -Wall -O -g -fschedule-insns2 -fgcse -fstrict-aliasing -fexpensive-optimizations -march=i586 -pipe
endif
ifeq ($(OPT),debug_optimize)
CFLAGS= -Wall -O -g -fschedule-insns2 -fgcse -fstrict-aliasing -fexpensive-optimizations -march=i586 -pipe
endif


# C++ compiler options
ifeq ($(OPT),profile)
CXXFLAGS= -Wall -pg -pipe -O -march=i586 $(OS_OPT)
LDFLAGS= -pg
endif
ifeq ($(OPT),optimize)
CXXFLAGS= -Wall -W -Wcast-align -Wcast-qual -Wpointer-arith -O -fomit-frame-pointer -march=i586 -pipe $(OS_OPT)
endif
ifeq ($(OPT),debug)
CXXFLAGS= -Wall -W -Wcast-align -Wcast-qual -Wpointer-arith -march=i586 -g -pipe $(OS_OPT)
endif
ifeq ($(OPT),debug_optimize)
CXXFLAGS= -Wall -W -Wcast-align -march=i586 -O -g -pipe $(OS_OPT)
endif


export CFLAGS
export CXXFLAGS


# Link-time cc options:
#
# Profiler
# LDFLAGS= -pg -L/usr/X11/lib/
#
# Standard
##LDFLAGS= -L/usr/X11/lib/




# To link any special libraries, add the necessary -l commands here.
SUB_OBJS=\
 utils/writepcx.o utils/log.o utils/image_encoder.o utils/simstring.o\
 utils/cstring_t.o utils/tocstring.o utils/searchfolder.o\
 utils/cbuffer_t.o\
 dataobj/nodelist_t.o dataobj/nodes_12.o\
 dataobj/powernet.o\
 dataobj/ribi.o dataobj/tabfile.o dataobj/linie.o \
 dataobj/fahrplan.o dataobj/koord.o dataobj/koord3d.o dataobj/route.o \
 dataobj/einstellungen.o dataobj/translator.o \
 dataobj/warenziel.o dataobj/umgebung.o\
 dataobj/marker.o dataobj/dingliste.o dataobj/loadsave.o \
 bauer/warenbauer.o\
 bauer/vehikelbauer.o\
 bauer/tunnelbauer.o\
 bauer/brueckenbauer.o\
 bauer/fabrikbauer.o\
 bauer/hausbauer.o\
 bauer/wegbauer.o\
 sucher/platzsucher.o\
 dings/oberleitung.o\
 dings/wolke.o dings/raucher.o dings/zeiger.o dings/baum.o dings/bruecke.o\
 dings/tunnel.o dings/gebaeudefundament.o dings/gebaeude.o\
 dings/signal.o dings/leitung2.o dings/lagerhaus.o\
 boden/boden.o  boden/fundament.o  boden/grund.o  boden/wasser.o\
 boden/brueckenboden.o  boden/tunnelboden.o \
 boden/wege/dock.o boden/wege/strasse.o \
 boden/wege/schiene.o boden/wege/weg.o \
 gui/components/gui_textinput.o gui/components/gui_resizer.o \
 gui/components/gui_chart.o \
 gui/components/gui_speedbar.o \
 gui/components/gui_textarea.o \
 gui/components/gui_divider.o \
 gui/components/gui_flowtext.o \
 gui/components/gui_combobox.o \
 gui/help_frame.o gui/citylist_frame_t.o gui/citylist_stats_t.o\
 gui/colors.o gui/welt.o gui/werkzeug_waehler.o\
 gui/werkzeug_parameter_waehler.o\
 gui/ticker_view_t.o gui/goods_frame_t.o gui/goods_stats_t.o\
 gui/halt_info.o gui/halt_detail.o gui/depot_frame.o\
 gui/halt_list_frame.o gui/halt_list_item.o gui/halt_list_filter_frame.o \
 gui/karte.o gui/stadt_info.o gui/fabrik_info.o \
 gui/fahrplan_gui.o gui/line_management_gui.o gui/kennfarbe.o \
 gui/sprachen.o gui/button.o gui/optionen.o gui/spieler.o gui/infowin.o \
 gui/scrolled_list.o gui/schedule_list.o gui/schedule_gui.o gui/scrollbar.o \
 gui/messagebox.o gui/tab_panel.o gui/image_list.o gui/map_frame.o \
 gui/gui_frame.o gui/gui_scrollpane.o gui/gui_container.o \
 gui/gui_label.o gui/gui_convoiinfo.o gui/world_view_t.o \
 gui/sound_frame.o gui/savegame_frame.o \
 gui/load_relief_frame.o gui/loadsave_frame.o \
 gui/money_frame.o gui/convoi_frame.o gui/convoi_filter_frame.o gui/convoi_info_t.o gui/label_frame.o \
 mm/mempool.o mm/memblock.o \
 besch/reader/obj_reader.o besch/reader/root_reader.o besch/reader/xref_reader.o \
 besch/reader/building_reader.o besch/reader/good_reader.o besch/reader/tree_reader.o \
 besch/reader/skin_reader.o besch/reader/image_reader.o besch/reader/factory_reader.o \
 besch/reader/bridge_reader.o besch/reader/vehicle_reader.o besch/reader/tunnel_reader.o \
 besch/reader/way_reader.o besch/reader/ground_reader.o besch/reader/crossing_reader.o \
 besch/reader/citycar_reader.o  besch/reader/pedestrian_reader.o \
 besch/ware_besch.o\
 besch/haus_besch.o\
 besch/bruecke_besch.o\
 besch/grund_besch.o\
 besch/tunnel_besch.o\
 besch/reader/sim_reader.o\
 drivables/car_group_t.o\
 drivables/car_t.o




# SDL Include under X
SDLINC=-I /usr/local/include -I /boot/develop/tools/gnupro/include




# miscellaneous OS-dependent stuff
# linker
LN= $(CC)
# file deletion command
RM= rm
# library (.a) file creation command
AR= ar rc
# second step in .a creation (use "touch" if not needed)
AR2= ranlib


# source files (independently compilable files)
SOURCES= \
 tpl/debug_helper.c tpl/no_such_element_exception.cc\
 simgraph16.c simevent.c simdisplay.c\
 simdebug.cc simmem.cc simio.cc\
 simticker.cc simview.cc simworldview.cc\
 simtime.cc simtools.cc\
 railblocks.cc blockmanager.cc simwin.cc simdepot.cc \
 simvehikel.cc simverkehr.cc simpeople.cc \
 simdings.cc simware.cc simfab.cc simplan.cc\
 simhalt.cc simconvoi.cc\
 simcity.cc simwerkz.cc simworld.cc simplay.cc simsound.cc simintr.cc \
 simmain.cc simskin.cc simlinemgmt.cc simline.cc

ASM_SOURCES= \
 asm/pixcopy.s asm/colorpixcopy.s asm/display_img.s asm/display_img16.s asm/display_img16w.s

# objectfiles
OBJECTS= \
 tpl/debug_helper.o tpl/no_such_element_exception.o\
 simevent.o simdisplay.o\
 simdebug.o simmem.o simio.o\
 simticker.o simview.o simworldview.o\
 simtime.o simtools.o\
 railblocks.o blockmanager.o simwin.o simdepot.o \
 simvehikel.o simverkehr.o simpeople.o \
 simdings.o simware.o simfab.o simplan.o\
 simhalt.o simconvoi.o\
 simcity.o simwerkz.o simworld.o simplay.o simsound.o simintr.o \
 simmain.o  simskin.o simlinemgmt.o simline.o

ifeq ($(OSTYPE),mingw)
ASM_DISPLAY_IMG= asm/display_img16w.o
else
ASM_DISPLAY_IMG= asm/display_img16.o
endif

ASM_OBJECTS= \
 asm/pixcopy.o asm/colorpixcopy.o asm/display_img.o asm/display_img16.o asm/display_img16w.o


all:    16

8:      subs normal

16:     subs normal16

cross:  subs wincross
	mv simwin.exe ../simutrans/simutrans.exe


subs:   gui_sub dataobj_sub dings_sub bauer_sub sucher_sub boden_sub mm_sub utils_sub besch_sub car_sub

clean:
	rm -f *.o */*.o */*/*.o
	rm -f *.bak */*.bak */*/*.bak
	rm -f *~ */*~ */*/*~

normal:	$(OBJECTS) simsys_d.o simgraph.o
	$(LN) $(LDFLAGS) -o sim $(OBJECTS) simsys_d.o simgraph.o /usr/local/lib/liballeg.a $(SUB_OBJS) $(STD_LIBS)

normal16:	$(OBJECTS) simsys_s16.o simgraph16.o $(ASM_DISPLAY_IMG)
	$(LN) $(LDFLAGS) -o sim $(OBJECTS) simsys_s16.o simgraph16.o $(ASM_DISPLAY_IMG) $(SUB_OBJS) $(STD_LIBS) $(SDLLIBS)

allegro16:	$(OBJECTS) simsys_d16.o simgraph16.o
	$(LN) $(LDFLAGS) -o sim $(OBJECTS) simsys_d16.o simgraph16.o $(SUB_OBJS) -lefence /usr/local/lib/liballeg.a $(STD_LIBS)

allegro_dyn: $(OBJECTS) simsys_d.o
	gcc $(CFLAGS) -c -D"MSDOS" simgraph.c
	$(LN) $(LDFLAGS) -o sim simsys_d.o $(OBJECTS) $(SUB_OBJS) -lm -lalleg-3.9.34 -lalleg_unsharable

allegro: $(OBJECTS) simsys_d.o
	gcc $(CFLAGS) -c -D"MSDOS" simgraph.c
	$(LN) $(LDFLAGS) -o sim simsys_d.o $(OBJECTS) /usr/local/lib/liballeg.a $(SUB_OBJS) -lX11 -lXext -lesd


newcars: subs car_sub $(OBJECTS)
	$(LN) $(LDFLAGS) -o sim $(OBJECTS) simsys_s16.o simgraph16.o $(SUB_OBJS) drivables/*.o $(STD_LIBS) $(SDLLIBS)

makeobj_prog:
	$(MAKE) -e -C makeobj


autotest:
	$(MAKE) CXXFLAGS='$(CXXFLAGS) -DAUTOTEST' testfiles

testfiles: dataobj_sub dings_sub gui_sub utils_sub test_sub $(OBJECTS) simsys_s16.o simgraph16.o
	$(LN) $(LDFLAGS) -o sim $(OBJECTS) simsys_s16.o simgraph16.o asm/display_img16w.o test/worldtest.o test/testtool.o $(SUB_OBJS) $(STD_LIBS)  $(SDLLIBS)

# test/buildings_frame_t.o test/buildings_stats_t.o

pak:
	utils/makepak


wincross: $(OBJECTS) simsys_s16.o simgraph16.o asm/display_img16w.o
	/usr/local/src/cross-tools/bin/i386-mingw32msvc-windres -O COFF simwin.rc simres.o
	$(LN) -o simwin.exe $(OBJECTS) $(SUB_OBJS) simsys_s16.o simgraph16.o asm/display_img16w.o simres.o -lmingw32 -lz -lSDLmain -lSDL -lwinmm
	/usr/local/src/cross-tools/bin/i386-mingw32msvc-strip simwin.exe



xsdl:   subs $(OBJECTS) simsys_s.o simgraph.o
	$(LN) $(LDFLAGS) -o sim $(OBJECTS) simsys_s16.o simgraph16.o $(SUB_OBJS) $(SDLLIBS)

besdl:  subs $(OBJECTS) simsys_s16.o simgraph16.o
	$(LN) $(LDFLAGS) -o sim $(OBJECTS) simsys_s16.o simgraph16.o $(SUB_OBJS) $(BESDLLIBS)

windsl: subs $(OBJECTS) simsys_s.o simgraph.o
	$(LN) $(LDFLAGS) -o sim $(OBJECTS) simsys_s.o $(SUB_OBJS) $(WINSDLLIBS)


demo:
	cp *.tab COPYRIGHT.txt readme.txt thanks.txt problem_report.txt change_request.txt ../simutrans
	cp -r history ../simutrans/history.txt
	cp -r config ../simutrans/
	rm -rf ../simutrans/config/CVS
	cp -r font ../simutrans/
	rm -rf ../simutrans/font/CVS
	cp -r palette ../simutrans/
	rm -rf ../simutrans/palette/CVS
	cp -r text ../simutrans/
	rm -rf ../simutrans/pak/*
	mkdir ../simutrans/pak/text
	cp scenarios/default/pak/*.pak ../simutrans/pak/
	cp scenarios/default/pak/text/*.tab ../simutrans/pak/text/
	strip sim
	mv sim ../simutrans/simutrans

static:    $(OBJECTS) simsys_x.o
	$(LN) -static $(LDFLAGS) -o simstat $(OBJECTS) simsys_x.o $(SUB_OBJS)

# always use the "dep" target if possible!

depend:
	cd gui; make depend; cd ..
	cd dings; make depend; cd ..
	cd dataobj; make depend; cd ..
	cd mm; make depend; cd ..
	cd utils; make depend; cd ..
	cd boden; make depend; cd ..
	cd test; make depend; cd ..
	makedepend $(SOURCES)

# dependencies without makedepend

dep: depdep
	cd gui; make dep; cd ..
	cd dings; make dep; cd ..
	cd dataobj; make dep; cd ..
	cd bauer; make dep; cd ..
	cd sucher; make dep; cd ..
	cd mm; make dep; cd ..
	cd utils; make dep; cd ..
	cd boden; make dep; cd ..


depdep: $(SOURCES)
	gcc -M $^ > .depend


doc:
	kdoc -d html -p Simutrans *.h */*.h *.h */*/*.h

lib:    $(OBJECTS)
	ar r libsim.a $(OBJECTS) $(SUB_OBJS)
	ar d libsim.a simmain.o
	ranlib libsim.a

mm_sub:
	$(MAKE) -e -C mm

utils_sub:
	$(MAKE) -e -C utils sim_obj

besch_sub:
	$(MAKE) -e -C besch

bauer_sub:
	$(MAKE) -e -C bauer

sucher_sub:
	$(MAKE) -e -C sucher

adt_sub:
	$(MAKE) -e -C adt

gui_sub:
	$(MAKE) -e -C gui

dataobj_sub:
	$(MAKE) -e -C dataobj

dings_sub:
	$(MAKE) -e -C dings

boden_sub:
	$(MAKE) -e -C boden

car_sub:
	$(MAKE) -e -C drivables

test_sub:
	$(MAKE) -e -C test



asm/pixcopy.o: asm/pixcopy.s
	$(CC) -c asm/pixcopy.s -o asm/pixcopy.o

asm/colorpixcopy.o: asm/colorpixcopy.s
	$(CC) -c asm/colorpixcopy.s -o asm/colorpixcopy.o

asm/display_img.o: asm/display_img.s
	$(CC) -c asm/display_img.s -o asm/display_img.o

asm/display_img16.o: asm/display_img16.s
	$(CC) -c asm/display_img16.s -o asm/display_img16.o

asm/display_img16w.o: asm/display_img16w.s
	$(CC) -c asm/display_img16w.s -o asm/display_img16w.o



ifeq (.depend,$(wildcard .depend))
    # include .depend only if it exists
include .depend
endif


win:
	./fix_vc.sh win

unix:
	./fix_vc.sh unix



# some dependencies that make dep can't generate

simsys_s16.o: simversion.h

# DO NOT DELETE
