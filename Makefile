CFG ?= default
-include config.$(CFG)


BACKENDS      = allegro gdi sdl mixer_sdl x11 posix
COLOUR_DEPTHS = 0 8 16
OSTYPES       = amiga beos cygwin freebsd haiku linux mingw mac

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


ifeq ($(OSTYPE),amiga)
  STD_LIBS ?= -lz -lbz2 -lunix -lpthread -lSDL_mixer -lsmpeg -lvorbisfile -lvorbis -logg
  CFLAGS += -mcrt=newlib -DUSE_C -DBIG_ENDIAN -gstabs+
  LDFLAGS += -Bstatic -non_shared
endif

ifeq ($(OSTYPE),beos)
  LIBS += -lz -lnet -lbz2
endif

ifeq ($(OSTYPE),haiku)
  LIBS += -lz -lnetwork -lbz2
endif

ifeq ($(OSTYPE),freebsd)
  LIBS += -lz -lbz2
endif

ifeq ($(OSTYPE),mac)
  CFLAGS  += -DUSE_HW
  CCFLAGS += -Os -fast
  LIBS    += -lz -lbz2
endif

ifeq ($(OSTYPE),linux)
  LIBS += -lz -lbz2
endif


ifeq ($(OSTYPE),cygwin)
  CFLAGS += -I/usr/include/mingw -mwin32
  LIBS   += -lgdi32 -lwinmm -lz -lbz2
endif

ifeq ($(OSTYPE),mingw)
  CC ?= gcc
  SOURCES += simsys_w32_png.cc
  CFLAGS  += -mno-cygwin -DPNG_STATIC -DZLIB_STATIC -march=pentium
  LDFLAGS += -static-libgcc -static-libstdc++
  ifeq ($(BACKEND),gdi)
    LIBS += -lunicows
  endif
  LIBS += -lmingw32 -lgdi32 -lwinmm -lwsock32 -lz -lbz2
endif

ifeq ($(OSTYPE),mingw)
  SOURCES += clipboard_w32.cc
else
  SOURCES += clipboard_internal.cc
endif

ALLEGRO_CONFIG ?= allegro-config
SDL_CONFIG     ?= sdl-config


ifneq ($(OPTIMISE),)
    CFLAGS += -O3 -fno-schedule-insns
  ifneq ($(OSTYPE),mac)
    ifneq ($(OSTYPE),haiku)
      CFLAGS += -minline-all-stringops
    endif
  endif
else
  CFLAGS += -O
endif

ifdef DEBUG
  ifeq ($(shell expr $(DEBUG) \>= 1), 1)
    CFLAGS += -g -DDEBUG
  endif
  ifeq ($(shell expr $(DEBUG) \>= 2), 1)
    ifneq ($(PROFILE), 2)
      CFLAGS += -fno-inline
    endif
  endif
  ifeq ($(shell expr $(DEBUG) \>= 3), 1)
    ifneq ($(PROFILE), 2)
      CFLAGS += -O0
    endif
  endif
else
  CFLAGS += -DNDEBUG
endif

ifneq ($(PROFILE),)
  CFLAGS  += -pg -DPROFILE
  ifneq ($(PROFILE), 2)
    CFLAGS  += -fno-inline -fno-schedule-insns
  endif
  LDFLAGS += -pg
endif

ifneq ($(WITH_REVISION),)
  REV = $(shell git log|head -1|tail -c +8|cksum| awk '{print $1}')
  ifneq ($(REV),)
    CFLAGS  += -DREVISION="$(REV)"
  endif
endif

CFLAGS   += -Wall -W -Wcast-qual -Wpointer-arith -Wcast-align $(FLAGS)
CCFLAGS  += -Wstrict-prototypes


SOURCES += bauer/brueckenbauer.cc
SOURCES += bauer/fabrikbauer.cc
SOURCES += bauer/hausbauer.cc
SOURCES += bauer/tunnelbauer.cc
SOURCES += bauer/vehikelbauer.cc
SOURCES += bauer/warenbauer.cc
SOURCES += bauer/wegbauer.cc
SOURCES += besch/bild_besch.cc
SOURCES += besch/bruecke_besch.cc
SOURCES += besch/fabrik_besch.cc
SOURCES += besch/grund_besch.cc
SOURCES += besch/haus_besch.cc
SOURCES += besch/reader/bridge_reader.cc
SOURCES += besch/reader/building_reader.cc
SOURCES += besch/reader/citycar_reader.cc
SOURCES += besch/reader/crossing_reader.cc
SOURCES += besch/reader/factory_reader.cc
SOURCES += besch/reader/good_reader.cc
SOURCES += besch/reader/ground_reader.cc
SOURCES += besch/reader/groundobj_reader.cc
SOURCES += besch/reader/image_reader.cc
SOURCES += besch/reader/imagelist2d_reader.cc
SOURCES += besch/reader/imagelist3d_reader.cc
SOURCES += besch/reader/imagelist_reader.cc
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
SOURCES += besch/vehikel_besch.cc
SOURCES += besch/ware_besch.cc
SOURCES += boden/boden.cc
SOURCES += boden/brueckenboden.cc
SOURCES += boden/fundament.cc
SOURCES += boden/grund.cc
SOURCES += boden/monorailboden.cc
SOURCES += boden/tunnelboden.cc
SOURCES += boden/wasser.cc
SOURCES += boden/wege/kanal.cc
SOURCES += boden/wege/maglev.cc
SOURCES += boden/wege/monorail.cc
SOURCES += boden/wege/narrowgauge.cc
SOURCES += boden/wege/runway.cc
SOURCES += boden/wege/schiene.cc
SOURCES += boden/wege/strasse.cc
SOURCES += boden/wege/weg.cc
SOURCES += dataobj/crossing_logic.cc
SOURCES += dataobj/dingliste.cc
SOURCES += dataobj/einstellungen.cc
SOURCES += dataobj/fahrplan.cc
SOURCES += dataobj/freelist.cc
SOURCES += dataobj/gameinfo.cc
SOURCES += dataobj/koord.cc
SOURCES += dataobj/koord3d.cc
SOURCES += dataobj/livery_scheme.cc
SOURCES += dataobj/loadsave.cc
SOURCES += dataobj/marker.cc
SOURCES += dataobj/network.cc
SOURCES += dataobj/network_address.cc
SOURCES += dataobj/network_cmd.cc
SOURCES += dataobj/network_cmd_ingame.cc
SOURCES += dataobj/network_cmp_pakset.cc
SOURCES += dataobj/network_file_transfer.cc
SOURCES += dataobj/network_packet.cc
SOURCES += dataobj/network_socket_list.cc
SOURCES += dataobj/pakset_info.cc
SOURCES += dataobj/powernet.cc
SOURCES += dataobj/replace_data.cc
SOURCES += dataobj/ribi.cc
SOURCES += dataobj/route.cc
SOURCES += dataobj/scenario.cc
SOURCES += dataobj/tabfile.cc
SOURCES += dataobj/translator.cc
SOURCES += dataobj/umgebung.cc
SOURCES += dataobj/warenziel.cc
SOURCES += dings/baum.cc
SOURCES += dings/bruecke.cc
SOURCES += dings/crossing.cc
SOURCES += dings/field.cc
SOURCES += dings/gebaeude.cc
SOURCES += dings/groundobj.cc
SOURCES += dings/label.cc
SOURCES += dings/leitung2.cc
SOURCES += dings/pillar.cc
SOURCES += dings/roadsign.cc
SOURCES += dings/signal.cc
SOURCES += dings/tunnel.cc
SOURCES += dings/wayobj.cc
SOURCES += dings/wolke.cc
SOURCES += dings/zeiger.cc
SOURCES += font.cc
SOURCES += freight_list_sorter.cc
SOURCES += gui/banner.cc
SOURCES += gui/baum_edit.cc
SOURCES += gui/citybuilding_edit.cc
SOURCES += gui/citylist_frame_t.cc
SOURCES += gui/citylist_stats_t.cc
SOURCES += gui/climates.cc
SOURCES += gui/colors.cc
SOURCES += gui/components/gui_button.cc
SOURCES += gui/components/gui_chart.cc
SOURCES += gui/components/gui_combobox.cc
SOURCES += gui/components/gui_component_table.cc
SOURCES += gui/components/gui_convoy_assembler.cc
SOURCES += gui/components/gui_convoy_label.cc
SOURCES += gui/components/gui_fixedwidth_textarea.cc
SOURCES += gui/components/gui_flowtext.cc
SOURCES += gui/components/gui_image_list.cc
SOURCES += gui/components/gui_label.cc
SOURCES += gui/components/gui_numberinput.cc
SOURCES += gui/components/gui_scrollbar.cc
SOURCES += gui/components/gui_scrolled_list.cc
SOURCES += gui/components/gui_scrollpane.cc
SOURCES += gui/components/gui_speedbar.cc
SOURCES += gui/components/gui_tab_panel.cc
SOURCES += gui/components/gui_table.cc
SOURCES += gui/components/gui_textarea.cc
SOURCES += gui/components/gui_textinput.cc
SOURCES += gui/components/gui_world_view_t.cc
SOURCES += gui/convoi_detail_t.cc
SOURCES += gui/convoi_filter_frame.cc
SOURCES += gui/convoi_frame.cc
SOURCES += gui/convoi_info_t.cc
SOURCES += gui/curiosity_edit.cc
SOURCES += gui/curiositylist_frame_t.cc
SOURCES += gui/curiositylist_stats_t.cc
SOURCES += gui/depot_frame.cc
SOURCES += gui/enlarge_map_frame_t.cc
SOURCES += gui/extend_edit.cc
SOURCES += gui/fabrik_info.cc
SOURCES += gui/factory_chart.cc
SOURCES += gui/factory_edit.cc
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
SOURCES += gui/halt_list_stats.cc
SOURCES += gui/help_frame.cc
SOURCES += gui/jump_frame.cc
SOURCES += gui/karte.cc
SOURCES += gui/kennfarbe.cc
SOURCES += gui/label_info.cc
SOURCES += gui/labellist_frame_t.cc
SOURCES += gui/labellist_stats_t.cc
SOURCES += gui/line_item.cc
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
SOURCES += gui/pakselector.cc
SOURCES += gui/password_frame.cc
SOURCES += gui/player_frame_t.cc
SOURCES += gui/replace_frame.cc
SOURCES += gui/savegame_frame.cc
SOURCES += gui/scenario_frame.cc
SOURCES += gui/schedule_list.cc
SOURCES += gui/server_frame.cc
SOURCES += gui/settings_frame.cc
SOURCES += gui/settings_stats.cc
SOURCES += gui/signal_spacing.cc
SOURCES += gui/sound_frame.cc
SOURCES += gui/sprachen.cc
SOURCES += gui/stadt_info.cc
SOURCES += gui/station_building_select.cc
SOURCES += gui/thing_info.cc
SOURCES += gui/trafficlight_info.cc
SOURCES += gui/welt.cc
SOURCES += gui/werkzeug_waehler.cc
SOURCES += old_blockmanager.cc
SOURCES += player/ai.cc
SOURCES += player/ai_goods.cc
SOURCES += player/ai_passenger.cc
SOURCES += player/simplay.cc
SOURCES += simcity.cc
SOURCES += convoy.cc
SOURCES += simconvoi.cc
SOURCES += simdebug.cc
SOURCES += simdepot.cc
SOURCES += simdings.cc
SOURCES += simevent.cc
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
SOURCES += simplan.cc
SOURCES += simskin.cc
SOURCES += simsound.cc
SOURCES += simsys.cc
SOURCES += simticker.cc
SOURCES += simtools.cc
SOURCES += simview.cc
SOURCES += simware.cc
SOURCES += simwerkz.cc
SOURCES += simwin.cc
SOURCES += simworld.cc
SOURCES += path_explorer.cc
SOURCES += sucher/platzsucher.cc
SOURCES += unicode.cc
SOURCES += utils/cbuffer_t.cc
SOURCES += utils/checksum.cc
SOURCES += utils/float32e8_t.cc
SOURCES += utils/log.cc
SOURCES += utils/memory_rw.cc
SOURCES += utils/searchfolder.cc
SOURCES += utils/sha1.cc
SOURCES += utils/simstring.cc
SOURCES += vehicle/movingobj.cc
SOURCES += vehicle/simpeople.cc
SOURCES += vehicle/simvehikel.cc
SOURCES += vehicle/simverkehr.cc

SOURCES += simgraph$(COLOUR_DEPTH).cc

ifdef DEBUG_WEIGHTMAPS
  SOURCES += utils/dbg_weightmap.cc
  CFLAGS += -DDEBUG_WEIGHTMAPS
endif

ifeq ($(BACKEND),allegro)
  SOURCES  += simsys_d.cc
  SOURCES += sound/allegro_sound.cc
  SOURCES += music/allegro_midi.cc
  ifeq ($(ALLEGRO_CONFIG),)
    ALLEGRO_CFLAGS  :=
    ALLEGRO_LDFLAGS := -lalleg
  else
    ALLEGRO_CFLAGS  := $(shell $(ALLEGRO_CONFIG) --cflags)
    ALLEGRO_LDFLAGS := $(shell $(ALLEGRO_CONFIG) --libs)
  endif
  CFLAGS += $(ALLEGRO_CFLAGS) -DUSE_SOFTPOINTER
  LIBS   += $(ALLEGRO_LDFLAGS)
endif


ifeq ($(BACKEND),gdi)
  SOURCES += simsys_w.cc
  SOURCES += music/w32_midi.cc
  SOURCES += sound/win32_sound.cc
endif


ifeq ($(BACKEND),sdl)
  SOURCES += simsys_s.cc
  CFLAGS  += -DUSE_16BIT_DIB
  ifeq ($(OSTYPE),mac)
    # Core Audio (Quicktime) base sound system routines
    SOURCES += sound/core-audio_sound.mm
    SOURCES += music/core-audio_midi.mm
    LIBS    += -framework Foundation -framework QTKit
  else
    SOURCES  += sound/sdl_sound.cc
    ifeq ($(findstring $(OSTYPE), cygwin mingw),)
	    SOURCES += music/no_midi.cc
    else
      SOURCES += music/w32_midi.cc
    endif
  endif
  ifeq ($(SDL_CONFIG),)
    ifeq ($(OSTYPE),mac)
      SDL_CFLAGS  := -I/System/Libraries/Frameworks/SDL/Headers -Dmain=SDL_main
      SDL_LDFLAGS := -framework SDL -framework Cocoa -I/System/Libraries/Frameworks/SDL/Headers SDLMain.m
    else
      SDL_CFLAGS  := -I$(MINGDIR)/include/SDL -Dmain=SDL_main
      SDL_LDFLAGS := -lSDLmain -lSDL -mwindows
    endif
  else
    SDL_CFLAGS  := $(shell $(SDL_CONFIG) --cflags)
    SDL_LDFLAGS := $(shell $(SDL_CONFIG) --libs)
  endif

  CFLAGS += $(SDL_CFLAGS)
  LIBS   += $(SDL_LDFLAGS)
endif


ifeq ($(BACKEND),mixer_sdl)
  SOURCES += simsys_s.cc
  SOURCES += sound/sdl_mixer_sound.cc
  SOURCES += music/sdl_midi.cc
  CFLAGS  += -DUSE_16BIT_DIB
  ifeq ($(SDL_CONFIG),)
    SDL_CFLAGS  := -I$(MINGDIR)/include/SDL -Dmain=SDL_main
    SDL_LDFLAGS := -lmingw32 -lSDLmain -lSDL -mwindows
  else
    SDL_CFLAGS  := $(shell $(SDL_CONFIG) --cflags)
    SDL_LDFLAGS := $(shell $(SDL_CONFIG) --libs)
  endif
  CFLAGS += $(SDL_CFLAGS)
  LIBS   += $(SDL_LDFLAGS) -lSDL_mixer
endif

ifeq ($(BACKEND),x11)
  SOURCES  += simsys_x$(COLOUR_DEPTH).c
  SOURCES += sound/no_sound.cc
  SOURCES += music/no_midi.cc
  CFLAGS  += -I/usr/X11R6/include
  LIBS    += -L/usr/X11R6/lib/ -lX11 -lXext
endif

ifeq ($(BACKEND),posix)
  SOURCES += simsys_posix.cc
  SOURCES += music/no_midi.cc
  SOURCES += sound/no_sound.cc
endif

CFLAGS += -DCOLOUR_DEPTH=$(COLOUR_DEPTH)

ifneq ($(findstring $(OSTYPE), cygwin mingw),)
  SOURCES += simres.rc
  WINDRES ?= windres
endif

CCFLAGS += -DUSE_INDEPENDENT_PATH_POOL -DDEBUG_SIMRAND_CALLS

CCFLAGS  += $(CFLAGS)
CXXFLAGS += $(CFLAGS)

BUILDDIR ?= build/$(CFG)

PROGDIR  ?= $(BUILDDIR)
PROG     ?= simutrans-experimental

include common.mk


makeobj_prog:
	$(Q)$(MAKE) -e -C makeobj FLAGS="$(FLAGS)"
