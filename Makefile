CONFIG ?= config.default
-include $(CONFIG)


BACKENDS      = allegro gdi sdl x11
COLOUR_DEPTHS = 8 16
OSTYPES       = beos cygwin freebsd linux mingw

ifeq ($(findstring $(BACKEND), $(BACKENDS)),)
  $(error Unkown BACKEND "$(BACKEND)", must be one of "$(BACKENDS)")
endif

ifeq ($(findstring $(COLOUR_DEPTH), $(COLOUR_DEPTHS)),)
  $(error Unkown COLOUR_DEPTH "$(COLOUR_DEPTH)", must be one of "$(COLOUR_DEPTHS)")
endif

ifeq ($(findstring $(OSTYPE), $(OSTYPES)),)
  $(error Unkown OSTYPE "$(OSTYPE)", must be one of "$(OSTYPES)")
endif


ifeq ($(BACKEND), x11)
  $(warning ATTENTION: X11 backend is broken)
endif

ifeq ($(COLOUR_DEPTH), 8)
  $(warning ATTENTION: 8bpp is broken)
endif


ifeq ($(OSTYPE),beos)
  ALLEGRO_CONFIG ?= allegro-config
  SDL_CONFIG     ?= sdl-config
  STD_LIBS       ?= -lz
endif

ifeq ($(OSTYPE),cygwin)
  ALLEGRO_CONFIG ?= allegro-config
  OS_INC         ?= -I/usr/include/mingw
  OS_OPT         ?= -mwin32
  SDL_CONFIG     ?= sdl-config
  STD_LIBS       ?= -lgdi32 -lwinmm -lz -mno-cygwin
endif

ifeq ($(OSTYPE),freebsd)
  ALLEGRO_CONFIG ?= allegro-config
  SDL_CONFIG     ?= sdl11-config
  STD_LIBS       ?= -lz
endif

ifeq ($(OSTYPE),mingw)
  ALLEGRO_CONFIG ?= allegro-config
  OS_OPT         ?= -mno-cygwin -DPNG_STATIC -DZLIB_STATIC
  SDL_CONFIG     ?= sdl-config
  STD_LIBS       ?=  -lunicows -lz -lmingw32 -lgdi32 -lwinmm
endif

ifeq ($(OSTYPE),linux)
  ALLEGRO_CONFIG ?= allegro-config
  SDL_CONFIG     ?= sdl-config
  STD_LIBS       ?= -lz
endif


ifneq ($(OPTIMISE),)
  CFLAGS   += -O -fomit-frame-pointer -fschedule-insns2 -fexpensive-optimizations -fgcse -fstrict-aliasing -minline-all-stringops
  CXXFLAGS += -O -fomit-frame-pointer -fschedule-insns2 -fregmove -freorder-blocks -falign-functions
else
  CFLAGS   += -O
  CXXFLAGS += -O
endif

ifneq ($(DEBUG),)
  CFLAGS   += -DDEBUG -g -fno-omit-frame-pointer
  CXXFLAGS += -DDEBUG -g -fno-omit-frame-pointer
endif

ifneq ($(PROFILE),)
  CFLAGS   += -pg
  CXXFLAGS += -pg
  LDFLAGS  += -pg
endif

ifeq ($(OSTYPE),beos)
  CFLAGS += -DUSE_C
endif

CFLAGS   += -Wall -W -Wcast-qual -Wpointer-arith -Wcast-align $(OS_INC) $(OS_OPT) $(FLAGS)
CXXFLAGS += -Wall -W -Wcast-qual -Wpointer-arith -Wcast-align $(OS_INC) $(OS_OPT) $(FLAGS)


SOURCES += bauer/brueckenbauer.cc
SOURCES += bauer/fabrikbauer.cc
SOURCES += bauer/hausbauer.cc
SOURCES += bauer/tunnelbauer.cc
SOURCES += bauer/vehikelbauer.cc
SOURCES += bauer/warenbauer.cc
SOURCES += bauer/wegbauer.cc
SOURCES += besch/bruecke_besch.cc
SOURCES += besch/grund_besch.cc
SOURCES += besch/haus_besch.cc
SOURCES += besch/reader/bridge_reader.cc
SOURCES += besch/reader/building_reader.cc
SOURCES += besch/reader/citycar_reader.cc
SOURCES += besch/reader/crossing_reader.cc
SOURCES += besch/reader/factory_reader.cc
SOURCES += besch/reader/good_reader.cc
SOURCES += besch/reader/ground_reader.cc
SOURCES += besch/reader/image_reader.cc
SOURCES += besch/reader/imagelist2d_reader.cc
SOURCES += besch/reader/imagelist_reader.cc
SOURCES += besch/reader/intlist_reader.cc
SOURCES += besch/reader/obj_reader.cc
SOURCES += besch/reader/pedestrian_reader.cc
SOURCES += besch/reader/roadsign_reader.cc
SOURCES += besch/reader/root_reader.cc
SOURCES += besch/reader/sim_reader.cc
SOURCES += besch/reader/skin_reader.cc
SOURCES += besch/reader/sound_reader.cc
SOURCES += besch/reader/text_reader.cc
SOURCES += besch/reader/tree_reader.cc
SOURCES += besch/reader/tunnel_reader.cc
SOURCES += besch/reader/vehicle_reader.cc
SOURCES += besch/reader/way_obj_reader.cc
SOURCES += besch/reader/way_reader.cc
SOURCES += besch/reader/xref_reader.cc
SOURCES += besch/sound_besch.cc
SOURCES += besch/tunnel_besch.cc
SOURCES += besch/ware_besch.cc
SOURCES += boden/boden.cc
SOURCES += boden/brueckenboden.cc
SOURCES += boden/fundament.cc
SOURCES += boden/grund.cc
SOURCES += boden/monorailboden.cc
SOURCES += boden/tunnelboden.cc
SOURCES += boden/wasser.cc
SOURCES += boden/wege/kanal.cc
SOURCES += boden/wege/monorail.cc
SOURCES += boden/wege/runway.cc
SOURCES += boden/wege/schiene.cc
SOURCES += boden/wege/strasse.cc
SOURCES += boden/wege/weg.cc
SOURCES += dataobj/dingliste.cc
SOURCES += dataobj/einstellungen.cc
SOURCES += dataobj/fahrplan.cc
SOURCES += dataobj/freelist.cc
SOURCES += dataobj/koord.cc
SOURCES += dataobj/koord3d.cc
SOURCES += dataobj/linie.cc
SOURCES += dataobj/loadsave.cc
SOURCES += dataobj/marker.cc
SOURCES += dataobj/powernet.cc
SOURCES += dataobj/ribi.cc
SOURCES += dataobj/route.cc
SOURCES += dataobj/tabfile.cc
SOURCES += dataobj/translator.cc
SOURCES += dataobj/umgebung.cc
SOURCES += dataobj/warenziel.cc
SOURCES += dings/baum.cc
SOURCES += dings/bruecke.cc
SOURCES += dings/dummy.cc
SOURCES += dings/gebaeude.cc
SOURCES += dings/lagerhaus.cc
SOURCES += dings/leitung2.cc
SOURCES += dings/pillar.cc
SOURCES += dings/raucher.cc
SOURCES += dings/roadsign.cc
SOURCES += dings/signal.cc
SOURCES += dings/tunnel.cc
SOURCES += dings/wayobj.cc
SOURCES += dings/wolke.cc
SOURCES += dings/zeiger.cc
SOURCES += freight_list_sorter.cc
SOURCES += gui/citylist_frame_t.cc
SOURCES += gui/citylist_stats_t.cc
SOURCES += gui/colors.cc
SOURCES += gui/components/gui_button.cc
SOURCES += gui/components/gui_chart.cc
SOURCES += gui/components/gui_combobox.cc
SOURCES += gui/components/gui_flowtext.cc
SOURCES += gui/components/gui_image_list.cc
SOURCES += gui/components/gui_label.cc
SOURCES += gui/components/gui_resizer.cc
SOURCES += gui/components/gui_scrollbar.cc
SOURCES += gui/components/gui_scrolled_list.cc
SOURCES += gui/components/gui_scrollpane.cc
SOURCES += gui/components/gui_speedbar.cc
SOURCES += gui/components/gui_tab_panel.cc
SOURCES += gui/components/gui_textarea.cc
SOURCES += gui/components/gui_textinput.cc
SOURCES += gui/components/gui_world_view_t.cc
SOURCES += gui/convoi_detail_t.cc
SOURCES += gui/convoi_filter_frame.cc
SOURCES += gui/convoi_frame.cc
SOURCES += gui/convoi_info_t.cc
SOURCES += gui/curiositylist_frame_t.cc
SOURCES += gui/curiositylist_stats_t.cc
SOURCES += gui/depot_frame.cc
SOURCES += gui/fabrik_info.cc
SOURCES += gui/factorylist_frame_t.cc
SOURCES += gui/factorylist_stats_t.cc
SOURCES += gui/fahrplan_gui.cc
SOURCES += gui/goods_frame_t.cc
SOURCES += gui/goods_stats_t.cc
SOURCES += gui/ground_info.cc
SOURCES += gui/gui_container.cc
SOURCES += gui/gui_convoiinfo.cc
SOURCES += gui/gui_frame.cc
SOURCES += gui/halt_detail.cc
SOURCES += gui/halt_info.cc
SOURCES += gui/halt_list_filter_frame.cc
SOURCES += gui/halt_list_frame.cc
SOURCES += gui/halt_list_item.cc
SOURCES += gui/help_frame.cc
SOURCES += gui/karte.cc
SOURCES += gui/kennfarbe.cc
SOURCES += gui/label_frame.cc
SOURCES += gui/line_management_gui.cc
SOURCES += gui/load_relief_frame.cc
SOURCES += gui/loadsave_frame.cc
SOURCES += gui/map_frame.cc
SOURCES += gui/message_frame_t.cc
SOURCES += gui/message_option_t.cc
SOURCES += gui/message_stats_t.cc
SOURCES += gui/messagebox.cc
SOURCES += gui/money_frame.cc
SOURCES += gui/optionen.cc
SOURCES += gui/player_frame_t.cc
SOURCES += gui/savegame_frame.cc
SOURCES += gui/schedule_list.cc
SOURCES += gui/sound_frame.cc
SOURCES += gui/sprachen.cc
SOURCES += gui/stadt_info.cc
SOURCES += gui/thing_info.cc
SOURCES += gui/welt.cc
SOURCES += gui/werkzeug_parameter_waehler.cc
SOURCES += old_blockmanager.cc
SOURCES += simcity.cc
SOURCES += simconvoi.cc
SOURCES += simdebug.cc
SOURCES += simdepot.cc
SOURCES += simdings.cc
SOURCES += simdisplay.c
SOURCES += simevent.c
SOURCES += simfab.cc
SOURCES += simhalt.cc
SOURCES += simintr.cc
SOURCES += simio.cc
SOURCES += simline.cc
SOURCES += simlinemgmt.cc
SOURCES += simmain.cc
SOURCES += simmem.cc
SOURCES += simmenu.cc
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
SOURCES += sucher/platzsucher.cc
SOURCES += tpl/debug_helper.cc
SOURCES += tpl/no_such_element_exception.cc
SOURCES += unicode.c
SOURCES += utils/cbuffer_t.cc
SOURCES += utils/cstring_t.cc
SOURCES += utils/log.cc
SOURCES += utils/searchfolder.cc
SOURCES += utils/simstring.c
SOURCES += utils/tocstring.cc


SOURCES += simgraph$(COLOUR_DEPTH).c


ifeq ($(BACKEND),allegro)
  SOURCES  += simsys_d$(COLOUR_DEPTH).c
  ifeq ($(ALLEGRO_CONFIG),)
    ALLEGRO_CFLAGS  :=
    ALLEGRO_LDFLAGS := -lalleg
  else
    ALLEGRO_CFLAGS  := $(shell $(ALLEGRO_CONFIG) --cflags)
    ALLEGRO_LDFLAGS := $(shell $(ALLEGRO_CONFIG) --libs)
  endif
  ALLEGRO_CFLAGS    += -DUSE_SOFTPOINTER
  CFLAGS   += $(ALLEGRO_CFLAGS)
  CXXFLAGS += $(ALLEGRO_CFLAGS)
  LIBS     += $(ALLEGRO_LDFLAGS)
endif

ifeq ($(BACKEND),gdi)
  SOURCES += simsys_w$(COLOUR_DEPTH).c
endif

ifeq ($(BACKEND),sdl)
  SOURCES  += simsys_s$(COLOUR_DEPTH).c
  ifeq ($(SDL_CONFIG),)
    SDL_CFLAGS  := -I$(MINGDIR)/include/SDL -Dmain=SDL_main
    SDL_LDFLAGS := -lmingw32 -lSDLmain -lSDL -mwindows
  else
    SDL_CFLAGS  := $(shell $(SDL_CONFIG) --cflags)
    SDL_LDFLAGS := $(shell $(SDL_CONFIG) --libs)
  endif
  CFLAGS   += $(SDL_CFLAGS)
  CXXFLAGS += $(SDL_CFLAGS)
  LIBS     += $(SDL_LDFLAGS)
endif

ifeq ($(BACKEND),x11)
  SOURCES  += simsys_x$(COLOUR_DEPTH).c
  CFLAGS   += -I/usr/X11R6/include
  CXXFLAGS += -I/usr/X11R6/include
  LIBS     += -L/usr/X11R6/lib/ -lX11 -lXext
endif


ifneq ($(findstring $(OSTYPE), cygwin mingw),)
  SOURCES += simres.rc
endif


ifeq ($(findstring $(OSTYPE), cygwin mingw),)
  ifeq ($(BACKEND), allegro)
    SOURCES += music/allegro_midi.c
  else
    SOURCES += music/no_midi.c
  endif
else
  SOURCES += music/w32_midi.c
endif


ifeq ($(BACKEND), allegro)
  SOURCES += sound/allegro_sound.c
endif

ifeq ($(BACKEND), gdi)
  SOURCES += sound/win32_sound.c
endif

ifeq ($(BACKEND), sdl)
  SOURCES += sound/sdl_sound.c
endif

ifeq ($(BACKEND), x11)
  SOURCES += sound/no_sound.c
endif


PROG = sim


include common.mk


makeobj_prog:
	$(MAKE) -e -C makeobj
