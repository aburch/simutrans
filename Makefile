CFG ?= default
-include config.$(CFG)


BACKENDS      = allegro gdi opengl sdl sdl2 mixer_sdl posix
COLOUR_DEPTHS = 0 16
OSTYPES       = amiga beos cygwin freebsd haiku linux mingw32 mingw64 mac openbsd

ifeq ($(findstring $(BACKEND), $(BACKENDS)),)
  $(error Unkown BACKEND "$(BACKEND)", must be one of "$(BACKENDS)")
endif

ifeq ($(findstring $(COLOUR_DEPTH), $(COLOUR_DEPTHS)),)
  $(error Unkown COLOUR_DEPTH "$(COLOUR_DEPTH)", must be one of "$(COLOUR_DEPTHS)")
endif

ifeq ($(findstring $(OSTYPE), $(OSTYPES)),)
  $(error Unkown OSTYPE "$(OSTYPE)", must be one of "$(OSTYPES)")
endif

ifeq ($(OSTYPE),amiga)
  STD_LIBS ?= -lunix -lSDL_mixer -lsmpeg -lvorbisfile -lvorbis -logg
  CFLAGS += -mcrt=newlib -DSIM_BIG_ENDIAN -gstabs+
  LDFLAGS += -Bstatic -non_shared
else
# BeOS (obsolete)
  ifeq ($(OSTYPE),beos)
    LIBS += -lnet
  else
    ifneq ($(findstring $(OSTYPE), cygwin mingw32 mingw64),)
      ifeq ($(OSTYPE),cygwin)
        CFLAGS  += -I/usr/include/mingw -mwin32
      else
        ifeq ($(OSTYPE), mingw32)
          CFLAGS  += -DPNG_STATIC -DZLIB_STATIC -static
          LDFLAGS += -static-libgcc -static-libstdc++ -Wl,--large-address-aware -static
          LIBS += -lmingw32
        endif
        ifeq ($(OSTYPE), mingw64)
          CFLAGS  += -DPNG_STATIC -DZLIB_STATIC -static
          LDFLAGS += -static-libgcc -static-libstdc++ -static
          LIBS += -lmingw32
        endif
      endif
      SOURCES += simsys_w32_png.cc
      CFLAGS  += -DNOMINMAX -DWIN32_LEAN_AND_MEAN -DWINVER=0x0501 -D_WIN32_IE=0x0500
      LIBS    += -lgdi32 -lwinmm -lws2_32 -limm32
      # Disable the console on Windows unless WIN32_CONSOLE is set or graphics are disabled
      ifneq ($(WIN32_CONSOLE),)
        LDFLAGS += -mconsole
      else
        ifeq ($(BACKEND),posix)
          LDFLAGS += -mconsole
        else
          LDFLAGS += -mwindows
        endif
      endif
    else
# Haiku (needs to activate the GCC 4x)
      ifeq ($(OSTYPE),haiku)
        LIBS += -lnetwork -lbe
      endif
    endif
  endif
endif

ifeq ($(OSTYPE),mac)
  CFLAGS +=  -std=c++11 -stdlib=libstdc++
  LDFLAGS += -stdlib=libstdc++
endif

ifeq ($(OSTYPE), mingw64)
  SOURCES += clipboard_w32.cc
else
	ifeq ($(OSTYPE),mingw32)
	  SOURCES += clipboard_w32.cc
	else
	  SOURCES += clipboard_internal.cc
  endif
endif

ifeq ($(OSTYPE),openbsd)
  CXXFLAGS +=  -std=c++11
endif

LIBS += -lbz2 -lz

CXXFLAGS +=  -std=gnu++11

ALLEGRO_CONFIG ?= allegro-config
SDL_CONFIG     ?= sdl-config
SDL2_CONFIG    ?= sdl2-config

ifneq ($(OPTIMISE),)
  CFLAGS += -O3
  ifeq ($(findstring $(OSTYPE), amiga),)
	ifneq ($(findstring $(CXX), clang),)
		CFLAGS += -minline-all-stringops
	endif
  endif
else
  CFLAGS += -O
endif

ifdef DEBUG
	ifndef MSG_LEVEL
		MSG_LEVEL = 3
	endif
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
  ifdef MSG_LEVEL
	CFLAGS += -DMSG_LEVEL=$(MSG_LEVEL)
	endif
  ifneq ($(PROFILE), 2)
    CFLAGS  += -fno-inline -fno-schedule-insns
  endif
  LDFLAGS += -pg
endif

ifneq ($(MULTI_THREAD),)
  ifeq ($(shell expr $(MULTI_THREAD) \>= 1), 1)
    CFLAGS += -DMULTI_THREAD
    ifeq ($(OSTYPE),mingw32 mingw64)
#use lpthreadGC2d for debug alternatively
#		Disabled, as this does not work for cross-compiling
#      LDFLAGS += -lpthreadGC2
	   LDFLAGS += -static -lpthread
    else
      ifneq ($(OSTYPE),haiku)
        LDFLAGS += -lpthread
      endif
    endif
  endif
endif

ifneq ($(WITH_REVISION),)
  ifeq ($(shell expr $(WITH_REVISION) \>= 1), 1)
    ifeq ($(shell expr $(WITH_REVISION) \>= 2), 1)
      REV = $(WITH_REVISION)
    else
      REV = $(shell git rev-parse --short HEAD)
    endif
    ifneq ($(REV),)
      CFLAGS  += -DREVISION="$(REV)"
    endif
  endif
endif

CFLAGS   += -Wall -W -Wcast-qual -Wpointer-arith -Wcast-align $(FLAGS)
CCFLAGS  += -ansi -Wstrict-prototypes -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

SOURCES += bauer/brueckenbauer.cc
SOURCES += bauer/fabrikbauer.cc
SOURCES += bauer/hausbauer.cc
SOURCES += bauer/tunnelbauer.cc
SOURCES += bauer/vehikelbauer.cc
SOURCES += bauer/goods_manager.cc
SOURCES += bauer/wegbauer.cc
SOURCES += descriptor/image.cc
SOURCES += descriptor/bridge_desc.cc
SOURCES += descriptor/factory_desc.cc
SOURCES += descriptor/ground_desc.cc
SOURCES += descriptor/building_desc.cc
SOURCES += descriptor/obj_base_desc.cc
SOURCES += descriptor/reader/bridge_reader.cc
SOURCES += descriptor/reader/building_reader.cc
SOURCES += descriptor/reader/citycar_reader.cc
SOURCES += descriptor/reader/crossing_reader.cc
SOURCES += descriptor/reader/factory_reader.cc
SOURCES += descriptor/reader/good_reader.cc
SOURCES += descriptor/reader/ground_reader.cc
SOURCES += descriptor/reader/groundobj_reader.cc
SOURCES += descriptor/reader/image_reader.cc
SOURCES += descriptor/reader/imagelist2d_reader.cc
SOURCES += descriptor/reader/imagelist3d_reader.cc
SOURCES += descriptor/reader/imagelist_reader.cc
SOURCES += descriptor/reader/obj_reader.cc
SOURCES += descriptor/reader/pedestrian_reader.cc
SOURCES += descriptor/reader/roadsign_reader.cc
SOURCES += descriptor/reader/root_reader.cc
SOURCES += descriptor/reader/sim_reader.cc
SOURCES += descriptor/reader/skin_reader.cc
SOURCES += descriptor/reader/sound_reader.cc
SOURCES += descriptor/reader/text_reader.cc
SOURCES += descriptor/reader/tree_reader.cc
SOURCES += descriptor/reader/tunnel_reader.cc
SOURCES += descriptor/reader/vehicle_reader.cc
SOURCES += descriptor/reader/way_obj_reader.cc
SOURCES += descriptor/reader/way_reader.cc
SOURCES += descriptor/reader/xref_reader.cc
SOURCES += descriptor/sound_desc.cc
SOURCES += descriptor/tunnel_desc.cc
SOURCES += descriptor/vehicle_desc.cc
SOURCES += descriptor/goods_desc.cc
SOURCES += descriptor/way_desc.cc
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
SOURCES += dataobj/objlist.cc
SOURCES += dataobj/settings.cc
SOURCES += dataobj/schedule.cc
SOURCES += dataobj/freelist.cc
SOURCES += dataobj/gameinfo.cc
SOURCES += dataobj/height_map_loader.cc
SOURCES += dataobj/koord.cc
SOURCES += dataobj/koord3d.cc
SOURCES += dataobj/loadsave.cc
SOURCES += dataobj/marker.cc
SOURCES += dataobj/powernet.cc
SOURCES += dataobj/records.cc
SOURCES += dataobj/ribi.cc
SOURCES += dataobj/route.cc
SOURCES += dataobj/scenario.cc
SOURCES += dataobj/tabfile.cc
SOURCES += dataobj/translator.cc
SOURCES += dataobj/environment.cc
SOURCES += obj/baum.cc
SOURCES += obj/bruecke.cc
SOURCES += obj/crossing.cc
SOURCES += obj/field.cc
SOURCES += obj/gebaeude.cc
SOURCES += obj/groundobj.cc
SOURCES += obj/label.cc
SOURCES += obj/leitung2.cc
SOURCES += obj/pillar.cc
SOURCES += obj/roadsign.cc
SOURCES += obj/signal.cc
SOURCES += obj/tunnel.cc
SOURCES += obj/wayobj.cc
SOURCES += obj/wolke.cc
SOURCES += obj/zeiger.cc
SOURCES += display/font.cc
SOURCES += display/simgraph$(COLOUR_DEPTH).cc
SOURCES += display/simview.cc
SOURCES += display/viewport.cc
SOURCES += freight_list_sorter.cc
SOURCES += gui/ai_option_t.cc
SOURCES += gui/banner.cc
SOURCES += gui/baum_edit.cc
SOURCES += gui/base_info.cc
SOURCES += gui/citybuilding_edit.cc
SOURCES += gui/citylist_frame_t.cc
SOURCES += gui/citylist_stats_t.cc
SOURCES += gui/climates.cc
SOURCES += gui/display_settings.cc
SOURCES += gui/components/gui_button.cc
SOURCES += gui/components/gui_chart.cc
SOURCES += gui/components/gui_combobox.cc
SOURCES += gui/components/gui_container.cc
SOURCES += gui/components/gui_convoiinfo.cc
SOURCES += gui/components/gui_obj_view_t.cc
SOURCES += gui/components/gui_fixedwidth_textarea.cc
SOURCES += gui/components/gui_flowtext.cc
SOURCES += gui/components/gui_image.cc
SOURCES += gui/components/gui_image_list.cc
SOURCES += gui/components/gui_component.cc
SOURCES += gui/components/gui_label.cc
SOURCES += gui/components/gui_map_preview.cc
SOURCES += gui/components/gui_numberinput.cc
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
SOURCES += gui/convoy_item.cc
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
SOURCES += gui/schedule_gui.cc
SOURCES += gui/goods_frame_t.cc
SOURCES += gui/goods_stats_t.cc
SOURCES += gui/ground_info.cc
SOURCES += gui/gui_frame.cc
SOURCES += gui/gui_theme.cc
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
SOURCES += gui/line_class_manager.cc
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
SOURCES += gui/onewaysign_info.cc
SOURCES += gui/optionen.cc
SOURCES += gui/overtaking_mode.cc
SOURCES += gui/pakselector.cc
SOURCES += gui/password_frame.cc
SOURCES += gui/player_frame_t.cc
SOURCES += gui/privatesign_info.cc
SOURCES += gui/savegame_frame.cc
SOURCES += gui/scenario_frame.cc
SOURCES += gui/scenario_info.cc
SOURCES += gui/schedule_list.cc
SOURCES += gui/schiene_info.cc
SOURCES += gui/server_frame.cc
SOURCES += gui/settings_frame.cc
SOURCES += gui/settings_stats.cc
SOURCES += gui/signal_info.cc
SOURCES += gui/signal_spacing.cc
SOURCES += gui/simwin.cc
SOURCES += gui/sound_frame.cc
SOURCES += gui/sprachen.cc
SOURCES += gui/times_history.cc
SOURCES += gui/times_history_container.cc
SOURCES += gui/times_history_entry.cc
SOURCES += gui/city_info.cc
SOURCES += gui/station_building_select.cc
SOURCES += gui/themeselector.cc
SOURCES += gui/obj_info.cc
SOURCES += gui/trafficlight_info.cc
SOURCES += gui/vehicle_class_manager.cc
SOURCES += gui/welt.cc
SOURCES += gui/tool_selector
SOURCES += network/checksum.cc
SOURCES += network/memory_rw.cc
SOURCES += network/network.cc
SOURCES += network/network_address.cc
SOURCES += network/network_cmd.cc
SOURCES += network/network_cmd_ingame.cc
SOURCES += network/network_cmd_scenario.cc
SOURCES += network/network_cmp_pakset.cc
SOURCES += network/network_file_transfer.cc
SOURCES += network/network_packet.cc
SOURCES += network/network_socket_list.cc
SOURCES += network/pakset_info.cc
SOURCES += network/pwd_hash.cc
SOURCES += old_blockmanager.cc
SOURCES += player/ai.cc
SOURCES += player/ai_goods.cc
SOURCES += player/ai_passenger.cc
SOURCES += player/finance.cc
SOURCES += player/simplay.cc
SOURCES += script/api_class.cc
SOURCES += script/api_function.cc
SOURCES += script/api_param.cc
SOURCES += script/api/api_city.cc
SOURCES += script/api/api_const.cc
SOURCES += script/api/api_convoy.cc
SOURCES += script/api/api_gui.cc
SOURCES += script/api/api_factory.cc
SOURCES += script/api/api_halt.cc
SOURCES += script/api/api_include.cc
SOURCES += script/api/api_line.cc
SOURCES += script/api/api_map_objects.cc
SOURCES += script/api/api_obj_desc.cc
SOURCES += script/api/api_obj_desc_base.cc
SOURCES += script/api/api_player.cc
SOURCES += script/api/api_scenario.cc
SOURCES += script/api/api_schedule.cc
SOURCES += script/api/api_settings.cc
SOURCES += script/api/api_simple.cc
SOURCES += script/api/api_tiles.cc
SOURCES += script/api/api_world.cc
SOURCES += script/api/export_desc.cc
SOURCES += script/api/get_next.cc
SOURCES += script/dynamic_string.cc
SOURCES += script/export_objs.cc
SOURCES += script/script.cc
SOURCES += squirrel/sq_extensions.cc
SOURCES += squirrel/squirrel/sqapi.cc
SOURCES += squirrel/squirrel/sqclass.cc
SOURCES += squirrel/squirrel/sqdebug.cc
SOURCES += squirrel/squirrel/sqlexer.cc
SOURCES += squirrel/squirrel/sqobject.cc
SOURCES += squirrel/squirrel/sqtable.cc
SOURCES += squirrel/squirrel/sqbaselib.cc
SOURCES += squirrel/squirrel/sqcompiler.cc
SOURCES += squirrel/squirrel/sqfuncstate.cc
SOURCES += squirrel/squirrel/sqmem.cc
SOURCES += squirrel/squirrel/sqstate.cc
SOURCES += squirrel/squirrel/sqvm.cc
SOURCES += squirrel/sqstdlib/sqstdaux.cc
SOURCES += squirrel/sqstdlib/sqstdio.cc
SOURCES += squirrel/sqstdlib/sqstdrex.cc
SOURCES += squirrel/sqstdlib/sqstdstring.cc
SOURCES += squirrel/sqstdlib/sqstdblob.cc
SOURCES += squirrel/sqstdlib/sqstdmath.cc
SOURCES += squirrel/sqstdlib/sqstdstream.cc
SOURCES += squirrel/sqstdlib/sqstdsystem.cc
SOURCES += simcity.cc
SOURCES += simconvoi.cc
SOURCES += simdebug.cc
SOURCES += simdepot.cc
SOURCES += simobj.cc
SOURCES += simevent.cc
SOURCES += simfab.cc
SOURCES += simhalt.cc
SOURCES += siminteraction.cc
SOURCES += simintr.cc
SOURCES += simio.cc
SOURCES += simline.cc
SOURCES += simlinemgmt.cc
SOURCES += simloadingscreen.cc
SOURCES += simmain.cc
SOURCES += simmem.cc
SOURCES += simmenu.cc
SOURCES += simmesg.cc
SOURCES += simplan.cc
SOURCES += simsignalbox.cc
SOURCES += simskin.cc
SOURCES += simsound.cc
SOURCES += simsys.cc
SOURCES += simticker.cc
SOURCES += simtool.cc
SOURCES += simware.cc
SOURCES += simworld.cc
SOURCES += finder/placefinder.cc
SOURCES += unicode.cc
SOURCES += utils/cbuffer_t.cc
SOURCES += utils/csv.cc
SOURCES += utils/log.cc
SOURCES += utils/searchfolder.cc
SOURCES += utils/sha1.cc
SOURCES += utils/simrandom.cc
SOURCES += vehicle/simroadtraffic.cc
SOURCES += utils/simstring.cc
SOURCES += utils/simthread.cc
SOURCES += vehicle/movingobj.cc
SOURCES += vehicle/simpeople.cc
SOURCES += vehicle/simvehicle.cc
SOURCES += simunits.cc
SOURCES += convoy.cc
SOURCES += utils/float32e8_t.cc
SOURCES += path_explorer.cc
SOURCES += gui/components/gui_component_table.cc
SOURCES += gui/components/gui_table.cc
SOURCES += gui/components/gui_convoy_assembler.cc
SOURCES += gui/components/gui_convoy_label.cc
SOURCES += gui/replace_frame.cc
SOURCES += dataobj/livery_scheme.cc
SOURCES += dataobj/replace_data.cc

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
  CFLAGS += -DGDI_SOUND
endif

ifeq ($(BACKEND),sdl)
  SOURCES += simsys_s.cc
  ifeq ($(OSTYPE),mac)
		ifeq ($(AV_FOUNDATION),1)
			# Core Audio (AVFoundation) base sound system routines
			SOURCES += sound/AVF_core-audio_sound.mm
			SOURCES += music/AVF_core-audio_midi.mm
			LIBS    += -framework Foundation -framework AVFoundation
		else
			# Core Audio (Quicktime) base sound system routines
			SOURCES += sound/core-audio_sound.mm
			SOURCES += music/core-audio_midi.mm
			LIBS    += -framework Foundation -framework QTKit
		endif
  else
    SOURCES  += sound/sdl_sound.cc
    ifeq ($(findstring $(OSTYPE), cygwin mingw32 mingw64),)
      SOURCES += music/no_midi.cc
    else
      SOURCES += music/w32_midi.cc
    endif
  endif
  ifeq ($(SDL_CONFIG),)
    ifeq ($(OSTYPE),mac)
      SDL_CFLAGS  := -I/Library/Frameworks/SDL.framework/Headers -Dmain=SDL_main
      SDL_LDFLAGS := -framework SDL -framework Cocoa -I/Library/Frameworks/SDL.framework/Headers OSX/SDLMain.m
    else
      SDL_CFLAGS  := -I$(MINGDIR)/include/SDL -Dmain=SDL_main
      SDL_LDFLAGS := -lSDLmain -lSDL
    endif
  else
    SDL_CFLAGS  := $(shell $(SDL_CONFIG) --cflags)
    ifeq ($(OSTYPE),mingw32 mingw64)
		SDL_LDFLAGS := $(shell $(SDL_CONFIG) --static-libs)
	else
	   SDL_LDFLAGS := $(shell $(SDL_CONFIG) --libs)
	endif
  endif
  CFLAGS += $(SDL_CFLAGS)
  LIBS   += $(SDL_LDFLAGS)
endif

ifeq ($(BACKEND),sdl2)
  SOURCES += simsys_s2.cc
  ifeq ($(OSTYPE),mac)
		ifeq ($(AV_FOUNDATION),1)
			# Core Audio (AVFoundation) base sound system routines
			SOURCES += sound/AVF_core-audio_sound.mm
			SOURCES += music/AVF_core-audio_midi.mm
			LIBS    += -framework Foundation -framework AVFoundation
		else
			# Core Audio (Quicktime) base sound system routines
			SOURCES += sound/core-audio_sound.mm
			SOURCES += music/core-audio_midi.mm
			LIBS    += -framework Foundation -framework QTKit
		endif
  else
    SOURCES  += sound/sdl_sound.cc
    ifeq ($(findstring $(OSTYPE), cygwin mingw32 mingw64),)
      SOURCES += music/no_midi.cc
    else
      SOURCES += music/w32_midi.cc
    endif
  endif
  ifeq ($(SDL2_CONFIG),)
    ifeq ($(OSTYPE),mac)
      SDL_CFLAGS  := -I/Library/Frameworks/SDL2.framework/Headers
      SDL_LDFLAGS := -framework SDL2
    else
      SDL_CFLAGS  := -I$(MINGDIR)/include/SDL2 -Dmain=SDL_main
      SDL_LDFLAGS := -lSDL2main -lSDL2
    endif
  else
    SDL_CFLAGS  := $(shell $(SDL2_CONFIG) --cflags)
    SDL_LDFLAGS := $(shell $(SDL2_CONFIG) --libs)
  endif
  CFLAGS += $(SDL_CFLAGS)
  LIBS   += $(SDL_LDFLAGS)
endif

ifeq ($(BACKEND),mixer_sdl)
  SOURCES += simsys_s.cc
  SOURCES += sound/sdl_mixer_sound.cc
  SOURCES += music/sdl_midi.cc
  ifeq ($(SDL_CONFIG),)
    SDL_CFLAGS  := -I$(MINGDIR)/include/SDL -Dmain=SDL_main
    SDL_LDFLAGS := -lmingw32 -lSDLmain -lSDL
  else
    SDL_CFLAGS  := $(shell $(SDL_CONFIG) --cflags)
	ifeq ($(OSTYPE),mingw32 mingw64)
		SDL_LDFLAGS := $(shell $(SDL_CONFIG) --static-libs)
	else
	   SDL_LDFLAGS := $(shell $(SDL_CONFIG) --libs)
	endif

  endif
  CFLAGS += $(SDL_CFLAGS)
  LIBS   += $(SDL_LDFLAGS) -lSDL_mixer
endif

ifeq ($(BACKEND),opengl)
  SOURCES += simsys_opengl.cc
  ifeq ($(OSTYPE),mac)
		ifeq ($(AV_FOUNDATION),1)
			# Core Audio (AVFoundation) base sound system routines
			SOURCES += sound/AVF_core-audio_sound.mm
			SOURCES += music/AVF_core-audio_midi.mm
			LIBS    += -framework Foundation -framework AVFoundation
		else
			# Core Audio (Quicktime) base sound system routines
			SOURCES += sound/core-audio_sound.mm
			SOURCES += music/core-audio_midi.mm
			LIBS    += -framework Foundation -framework QTKit
		endif
  else
    SOURCES  += sound/sdl_sound.cc
    ifeq ($(findstring $(OSTYPE), cygwin mingw32 mingw64),)
      SOURCES += music/no_midi.cc
    else
      SOURCES += music/w32_midi.cc
    endif
  endif
  ifeq ($(SDL_CONFIG),)
    SDL_CFLAGS  := -I$(MINGDIR)/include/SDL -Dmain=SDL_main
    SDL_LDFLAGS := -lmingw32 -lSDLmain -lSDL
  else
    SDL_CFLAGS  := $(shell $(SDL_CONFIG) --cflags)
    ifeq ($(OSTYPE),mingw32 mingw64)
		SDL_LDFLAGS := $(shell $(SDL_CONFIG) --static-libs)
	else
	   SDL_LDFLAGS := $(shell $(SDL_CONFIG) --libs)
	endif
  endif
  CFLAGS += $(SDL_CFLAGS)
  LIBS   += $(SDL_LDFLAGS)
  ifeq ($(OSTYPE),mac)
    LIBS += -framework OpenGL
  else
    ifeq ($(OSTYPE),mingw32 mingw64)
      LIBS += -lglew32 -lopengl32
    else
      LIBS += -lGLEW -lGL
    endif
  endif
endif

ifeq ($(BACKEND),posix)
  SOURCES += simsys_posix.cc
  SOURCES += music/no_midi.cc
  SOURCES += sound/no_sound.cc
endif

CFLAGS += -DCOLOUR_DEPTH=$(COLOUR_DEPTH)

ifneq ($(findstring $(OSTYPE), cygwin mingw32 mingw64),)
  SOURCES += simres.rc
  # See https://sourceforge.net/p/mingw-w64/discussion/723798/thread/bf2a464d/
  ifeq ($(OSTYPE), mingw32)
    WINDRES ?= windres -F pe-i386
  else
    ifeq ($(OSTYPE), mingw64)
      WINDRES ?= x86_64-w64-mingw32-windres
    endif
  endif
endif

CCFLAGS  += $(CFLAGS)
CXXFLAGS += $(CFLAGS)

BUILDDIR ?= build/$(CFG)
PROGDIR  ?= $(BUILDDIR)
ifneq ($(findstring $(OSTYPE), cygwin mingw32 mingw64),)
  PROG     ?= Simutrans-Extended.exe
else
  PROG     ?= simutrans-extended
endif

include common.mk

ifeq ($(OSTYPE),mac)
  include OSX/osx.mk
endif


.PHONY: makeobj-extended

makeobj:
	$(Q)$(MAKE) -e -C makeobj FLAGS="$(FLAGS)"
