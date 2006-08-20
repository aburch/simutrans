#
# Makefile
#
# Top-Level Makefile for Simutrans
# Copyright (c) 2001 by Hj. Malthaner
# hansjoerg.malthaner@gmx.de
#


OSTYPE=mingw-sdl
#OSTYPE=mingw-gdi
#OSTYPE=mingw-allegro
#OSTYPE=beos
#OSTYPE=linux-gnu


ifeq ($(OSTYPE),)
#OSTYPE=cygwin
#OSTYPE=mingw
#OSTYPE=linux-gnu
endif

export OSTYPE

#OPT=profile
#OPT=debug
#OPT=optimize
OPT=debug_optimize


ifeq ($(OSTYPE),cygwin)
OS_INC=-I/SDL/include -I/usr/include/cygwin
DISPLAY_OBJ= simsys_s16.o
STD_LIBS=
SDLLIBS=  -L/SDL/lib -lz -lSDL -lwinmm -lc /lib/mingw/libstdc++.a -lc /lib/mingw/libmingw32.a -lc /lib/mingw/libmsvcrt.a
#SDLLIBS= -lSDL -lwinmm -lc /lib/libmsvcrt40.a -lc /lib/libstdc++.a -lc /lib/libmingw32.a
#SDLLIBS= -lSDL -lwinmm -lpthread
OS_OPT=-Wbad-function-cast
endif
ifeq ($(OSTYPE),mingw-sdl)
DISPLAY_OBJ= simsys_s16.o
OS_INC=-I /usr/include/mingw
OS_OPT=-mno-cygwin -DPNG_STATIC -DZLIB_STATIC
#OS_OPT=-Wbad-function-cast
STD_LIBS=-lstdc++ -lz
SDLLIBS= -lmingw32 -lSDLmain -lSDL -lwinmm
endif
ifeq ($(OSTYPE),mingw-gdi)
DISPLAY_OBJ= simsys_w16.o
OS_INC=-I /usr/include/mingw
OS_OPT=-mno-cygwin -DPNG_STATIC -DZLIB_STATIC
#OS_OPT=-Wbad-function-cast
STD_LIBS=-lunicows -lstdc++ -lz
SDLLIBS= -lmingw32 -lgdi32 -lwinmm
endif
ifeq ($(OSTYPE),mingw-allegro)
DISPLAY_OBJ= simsys_d16.o
OS_INC=-I /usr/include/mingw
OS_OPT=-mno-cygwin -DPNG_STATIC -DZLIB_STATIC
#OS_OPT=-Wbad-function-cast
STD_LIBS=-lunicows -lstdc++ -lz
SDLLIBS= -lmingw32 -lalleg
endif
ifeq ($(OSTYPE),linux-gnu)
#STD_LIBS= /usr/lib/libstdc++-3-libc6.1-2-2.10.0.a -lz
DISPLAY_OBJ= simsys_s16.o
STD_LIBS= -lstdc++ -lz
SDLLIBS= -lSDL -lpthread
OS_OPT=
endif
ifeq ($(OSTYPE),linux)
STD_LIBS= -lz
SDLLIBS= -lSDL -lpthread
OS_OPT=
endif
ifeq ($(OSTYPE),beos)
DISPLAY_OBJ= simsys_s16.o
STD_LIBS= -lz
SDLLIBS= -lSDL
OS_OPT=
endif

BESDLLIBS= -lSDL -lz

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

# C compiler options
ifeq ($(OSTYPE),beos)
CFLAGS= -DUSE_C -Wall -O -g -fschedule-insns2 -fgcse -fstrict-aliasing -march=i586 -pipe
else
## other systems
ifeq ($(OPT),profile)
CFLAGS= -Wall -pg -O -pipe -fschedule-insns2 -fgcse -fstrict-aliasing -fexpensive-optimizations -march=i586 -g -minline-all-stringops
LDFLAGS= -pg
endif
ifeq ($(OPT),optimize)
CFLAGS= -Wall -O -fschedule-insns2 -fomit-frame-pointer -fgcse -fstrict-aliasing -fexpensive-optimizations -march=i586 -pipe $(OS_INC) -minline-all-stringops
endif
ifeq ($(OPT),debug)
CFLAGS= -Wall -O -g -fschedule-insns2 -fgcse -fstrict-aliasing -fexpensive-optimizations -march=i586 -pipe -minline-all-stringops
endif
ifeq ($(OPT),debug_optimize)
CFLAGS= -DDEBUG -Wall -O -g -fschedule-insns2 -fgcse -fstrict-aliasing -fexpensive-optimizations -march=i586 -pipe -minline-all-stringops
endif
endif


# C++ compiler options
ifeq ($(OPT),profile)
CXXFLAGS= -Wall -pg -pipe -O -march=i586 $(OS_OPT)
LDFLAGS= -pg
endif
ifeq ($(OPT),optimize)
CXXFLAGS= -Wall -W -Wcast-align -Wcast-qual -Wpointer-arith -O -fomit-frame-pointer -fregmove -fschedule-insns2 -fmove-all-movables -freorder-blocks -falign-functions  -march=i586 -pipe $(OS_OPT)
endif
ifeq ($(OPT),debug)
CXXFLAGS=  -DDEBUG -Wall -W -Wcast-align -Wcast-qual -Wpointer-arith -march=i586 -g -pipe $(OS_OPT)
endif
ifeq ($(OPT),debug_optimize)
CXXFLAGS=  -DDEBUG -Wall -W -Wcast-align -Wcast-qual -Wpointer-arith -march=i586 -O -fregmove -fschedule-insns2 -fmove-all-movables -freorder-blocks -falign-functions -g -pipe $(OS_OPT)
endif


export CFLAGS
export CXXFLAGS




# To link any special libraries, add the necessary -l commands here.
SUB_OBJS=\
 utils/log.o utils/simstring.o utils/cstring_t.o utils/tocstring.o utils/searchfolder.o\
 utils/cbuffer_t.o\
 dataobj/freelist.o\
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
 dings/wolke.o dings/raucher.o dings/zeiger.o dings/baum.o dings/bruecke.o dings/pillar.o \
 dings/tunnel.o dings/gebaeude.o \
 dings/signal.o dings/leitung2.o dings/roadsign.o dings/dummy.o dings/lagerhaus.o\
 boden/boden.o  boden/fundament.o  boden/grund.o  boden/wasser.o\
 boden/brueckenboden.o  boden/tunnelboden.o boden/monorailboden.o \
 boden/wege/kanal.o \
 boden/wege/runway.o \
 boden/wege/strasse.o \
 boden/wege/schiene.o boden/wege/weg.o \
 gui/components/gui_textinput.o gui/components/gui_resizer.o \
 gui/components/gui_chart.o \
 gui/components/gui_speedbar.o \
 gui/components/gui_textarea.o \
 gui/components/gui_divider.o \
 gui/components/gui_flowtext.o \
 gui/components/gui_combobox.o \
 gui/help_frame.o gui/citylist_frame_t.o gui/citylist_stats_t.o\
 gui/message_frame_t.o gui/message_stats_t.o gui/message_option_t.o  gui/message_info_t.o\
 gui/colors.o gui/welt.o\
 gui/werkzeug_parameter_waehler.o\
 gui/goods_frame_t.o gui/goods_stats_t.o\
 gui/halt_info.o gui/halt_detail.o gui/depot_frame.o\
 gui/halt_list_frame.o gui/halt_list_item.o gui/halt_list_filter_frame.o \
 gui/karte.o gui/stadt_info.o gui/fabrik_info.o \
 gui/fahrplan_gui.o gui/line_management_gui.o gui/kennfarbe.o \
 gui/sprachen.o gui/button.o gui/optionen.o gui/spieler.o gui/infowin.o \
 gui/scrolled_list.o gui/schedule_list.o gui/schedule_gui.o gui/scrollbar.o \
 gui/messagebox.o gui/tab_panel.o gui/image_list.o gui/map_frame.o gui/map_legend.o \
 gui/gui_frame.o gui/gui_scrollpane.o gui/gui_container.o \
 gui/gui_label.o gui/gui_convoiinfo.o gui/world_view_t.o \
 gui/sound_frame.o gui/savegame_frame.o \
 gui/load_relief_frame.o gui/loadsave_frame.o \
 gui/money_frame.o gui/convoi_frame.o gui/convoi_filter_frame.o gui/convoi_info_t.o gui/label_frame.o \
 gui/factorylist_frame_t.o gui/factorylist_stats_t.o \
 gui/curiositylist_frame_t.o gui/curiositylist_stats_t.o \
 besch/reader/obj_reader.o besch/reader/root_reader.o besch/reader/xref_reader.o \
 besch/reader/building_reader.o besch/reader/good_reader.o besch/reader/tree_reader.o \
 besch/reader/skin_reader.o besch/reader/image_reader.o besch/reader/factory_reader.o \
 besch/reader/bridge_reader.o besch/reader/vehicle_reader.o besch/reader/tunnel_reader.o \
 besch/reader/way_reader.o besch/reader/ground_reader.o besch/reader/crossing_reader.o \
 besch/reader/citycar_reader.o  besch/reader/pedestrian_reader.o besch/reader/roadsign_reader.o \
 besch/ware_besch.o\
 besch/haus_besch.o\
 besch/bruecke_besch.o\
 besch/grund_besch.o\
 besch/tunnel_besch.o\
 besch/reader/sim_reader.o


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
SOURCES += blockmanager.cc
SOURCES += railblocks.cc
SOURCES += simcity.cc
SOURCES += simconvoi.cc
SOURCES += simdebug.cc
SOURCES += simdepot.cc
SOURCES += simdings.cc
SOURCES += simdisplay.c
SOURCES += simevent.c
SOURCES += simfab.cc
#SOURCES += simgraph16.c # seems to be unused
SOURCES += simhalt.cc
SOURCES += simintr.cc
SOURCES += simio.cc
SOURCES += simline.cc
SOURCES += simlinemgmt.cc
SOURCES += simmain.cc
SOURCES += simmem.cc
SOURCES += simmesg.cc
SOURCES += simpeople.cc
SOURCES += simplan.cc
SOURCES += simplay.cc
SOURCES += simskin.cc
SOURCES += simsound.cc
SOURCES += simticker.cc
SOURCES += simtime.cc
SOURCES += simtools.c
SOURCES += simvehikel.cc
SOURCES += simverkehr.cc
SOURCES += simview.cc
SOURCES += simware.cc
SOURCES += simwerkz.cc
SOURCES += simwin.cc
SOURCES += simworld.cc
SOURCES += simworldview.cc
SOURCES += tpl/debug_helper.c
SOURCES += tpl/no_such_element_exception.cc

ifeq ($(OSTYPE),mingw-sdl)
  SOURCES += simres.rc
endif
ifeq ($(OSTYPE),mingw-gdi)
  SOURCES += simres.rc
endif

SUBDIRS = bauer besch boden dataobj dings gui sucher utils

include common.mk


all:    16

8:      subs normal

16:     subs normal16

cross:  subs wincross
	mv simwin.exe ../simutrans/simutrans.exe


subs: $(SUBDIRS)

normal:	$(OBJECTS) simsys_d.o simgraph.o
	$(LN) $(LDFLAGS) -o sim $(OBJECTS) simsys_d.o simgraph.o /usr/local/lib/liballeg.a $(SUB_OBJS) $(STD_LIBS)


normal16: $(OBJECTS) $(DISPLAY_OBJ) simgraph16.o
	$(LN) $(LDFLAGS) -o sim $(OBJECTS) $(DISPLAY_OBJ) simgraph16.o $(SUB_OBJS) $(STD_LIBS) $(SDLLIBS)

allegro16:	$(OBJECTS) simsys_d16.o simgraph16.o
	$(LN) $(LDFLAGS) -o sim $(OBJECTS) simsys_d16.o simgraph16.o $(SUB_OBJS) -lalleg.a $(STD_LIBS)

allegro_dyn: $(OBJECTS) simsys_d.o
	$(CC) $(CFLAGS) -c -D"MSDOS" simgraph.c
	$(LN) $(LDFLAGS) -o sim simsys_d.o $(OBJECTS) $(SUB_OBJS) -lm -lalleg-3.9.34 -lalleg_unsharable

allegro: $(OBJECTS) simsys_d.o
	$(CC) $(CFLAGS) -c -D"MSDOS" simgraph.c
	$(LN) $(LDFLAGS) -o sim simsys_d.o $(OBJECTS) /usr/local/lib/liballeg.a $(SUB_OBJS) -lX11 -lXext -lesd

newcars: subs car_sub $(OBJECTS)
	$(LN) $(LDFLAGS) -o sim $(OBJECTS) simsys_s16.o simgraph16.o $(SUB_OBJS) drivables/*.o $(STD_LIBS) $(SDLLIBS)

makeobj_prog:
	$(MAKE) -e -C makeobj


autotest:
	$(MAKE) CXXFLAGS='$(CXXFLAGS) -DAUTOTEST' testfiles

testfiles: dataobj_sub dings_sub gui_sub utils_sub test_sub $(OBJECTS) simsys_s16.o simgraph16.o
	$(LN) $(LDFLAGS) -o sim $(OBJECTS) simsys_s16.o simgraph16.o asm/display_img16w.o test/worldtest.o test/testtool.o $(SUB_OBJS) $(STD_LIBS)  $(SDLLIBS)

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


doc:
	kdoc -d html -p Simutrans *.h */*.h *.h */*/*.h

lib:    $(OBJECTS)
	ar r libsim.a $(OBJECTS) $(SUB_OBJS)
	ar d libsim.a simmain.o
	ranlib libsim.a


# some dependencies that make dep can't generate

simsys_s16.o: simversion.h
