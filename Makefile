#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

# Define variables here to force them as simple flavor. -> Faster parallel builds.
FLAGS :=
CFLAGS ?=
LDFLAGS ?=
LIBS :=
SOURCES :=
STATIC := 0


CFG ?= default
-include config.$(CFG)


HOSTCC  ?=$(CC)
HOSTCXX ?=$(CXX)

CFLAGS += -std=c++14

SDL_CONFIG       ?= sdl-config
SDL2_CONFIG      ?= pkg-config sdl2
#SDL2_CONFIG     ?= sdl2-config
FREETYPE_CONFIG  ?= pkg-config freetype2
#FREETYPE_CONFIG ?= freetype-config

BACKENDS  := gdi sdl sdl2 mixer_sdl mixer_sdl2 posix
OSTYPES   := amiga beos freebsd haiku linux mac mingw openbsd


ifeq ($(findstring $(BACKEND), $(BACKENDS)),)
  $(error Unkown BACKEND "$(BACKEND)", must be one of "$(BACKENDS)")
endif

ifeq ($(findstring $(OSTYPE), $(OSTYPES)),)
  $(error Unkown OSTYPE "$(OSTYPE)", must be one of "$(OSTYPES)")
endif

ifeq ($(BACKEND),posix)
  COLOUR_DEPTH := 0
else
  COLOUR_DEPTH := 16
endif

ifeq ($(OSTYPE),amiga)
  STD_LIBS ?= -lunix -lSDL_mixer -lsmpeg -lvorbisfile -lvorbis -logg
  CFLAGS   += -mcrt=newlib -DSIM_BIG_ENDIAN -gstabs+
  LDFLAGS  += -Bstatic -non_shared
else ifeq ($(OSTYPE),beos)
  # BeOS (obsolete)
  LIBS += -lnet
else ifeq ($(OSTYPE),freebsd)
  CFLAGS  += -I/usr/local/include
else ifeq ($(OSTYPE),haiku)
  # Haiku (needs to activate the GCC 4x)
  LIBS += -lnetwork -lbe
else ifeq ($(OSTYPE),mingw)
  CFLAGS    += -DPNG_STATIC -DZLIB_STATIC
  ifeq ($(shell expr $(STATIC) \>= 1), 1)
    CFLAGS  += -static
    LDFLAGS += -static-libgcc -static-libstdc++ -static
  endif
  ifeq ($(MINGW_PACKAGE_PREFIX),mingw-w64-i686)
    LDFLAGS   += -Wl,--large-address-aware
  endif
  LDFLAGS   += -pthread
  CFLAGS    += -Wno-deprecated-copy -DNOMINMAX -DWIN32_LEAN_AND_MEAN -DWINVER=0x0501 -D_WIN32_IE=0x0500
  LIBS      += -lmingw32 -lgdi32 -lwinmm -lws2_32 -limm32

  # Disable the console on Windows unless WIN32_CONSOLE is set or graphics are disabled
  ifdef WIN32_CONSOLE
    ifeq ($(shell expr $(WIN32_CONSOLE) \>= 1), 1)
      LDFLAGS += -mconsole
    endif
  else ifeq ($(BACKEND),posix)
    LDFLAGS += -mconsole
  else
    LDFLAGS += -mwindows
  endif
else ifeq ($(OSTYPE),linux)
  LD_FLAGS += "-Wl,-Bstatic"
else ifeq ($(OSTYPE),mac)
  SOURCES += OSX/translocation.m
  LDFLAGS += -framework Cocoa
endif

ifeq ($(BACKEND),sdl2)
  SOURCES += sys/clipboard_s2.cc
else ifeq ($(OSTYPE),mingw)
  SOURCES += sys/clipboard_w32.cc
else
  SOURCES += sys/clipboard_internal.cc
endif

LIBS += -lbz2 -lz -lpng

ifdef OPTIMISE
  ifeq ($(shell expr $(OPTIMISE) \>= 1), 1)
    CFLAGS += -O3
    ifeq ($(findstring amiga, $(OSTYPE)),)
      ifneq ($(findstring clang, $(CXX)),)
        CFLAGS += -minline-all-stringops
      endif
    endif
  endif
else
  CFLAGS += -O1
endif

ifneq ($(LTO),)
  CFLAGS += -flto
  LDFLAGS += -flto
endif

ifdef DEBUG
  MSG_LEVEL ?= 3
  PROFILE ?= 0

  ifeq ($(shell expr $(DEBUG) \>= 1), 1)
    CFLAGS   += -g -DDEBUG
  endif

  ifeq ($(shell expr $(DEBUG) \>= 2), 1)
    ifeq ($(shell expr $(PROFILE) \>= 2), 1)
      CFLAGS += -fno-inline
    endif
  endif

  ifeq ($(shell expr $(DEBUG) \>= 3), 1)
    ifeq ($(shell expr $(PROFILE) \>= 2), 1)
      CFLAGS += -O0
    endif
  endif
else
  CFLAGS += -DNDEBUG
endif

ifdef MSG_LEVEL
  CFLAGS += -DMSG_LEVEL=$(MSG_LEVEL)
endif

ifdef USE_FREETYPE
  ifeq ($(shell expr $(USE_FREETYPE) \>= 1), 1)
    CFLAGS   += -DUSE_FREETYPE
    ifneq ($(FREETYPE_CONFIG),)
      CFLAGS += $(shell $(FREETYPE_CONFIG) --cflags)
      ifeq ($(shell expr $(STATIC) \>= 1), 1)
        # since static is not supported by slightly old freetype versions
        FTF := $(shell $(FREETYPE_CONFIG) --libs --static)
        ifneq ($(FTF),)
          LDFLAGS += $(FTF)
        else
          LDFLAGS += $(shell $(FREETYPE_CONFIG) --libs)
        endif
      else
        LDFLAGS   += $(shell $(FREETYPE_CONFIG) --libs)
      endif
    else
      LDFLAGS += -lfreetype
      ifeq ($(OSTYPE),mingw)
        LDFLAGS += -lpng -lharfbuzz
      endif
    endif

    ifeq ($(OSTYPE),mingw)
      LDFLAGS += -lfreetype
    endif
  endif
endif

ifdef USE_UPNP
  ifeq ($(shell expr $(USE_UPNP) \>= 1), 1)
    CFLAGS      += -DUSE_UPNP
    LDFLAGS     += -lminiupnpc
    ifeq ($(OSTYPE),mingw)
      ifeq ($(shell expr $(STATIC) \>= 1), 1)
        LDFLAGS += -Wl,-Bdynamic
      endif
      LDFLAGS   += -liphlpapi
      ifeq ($(shell expr $(STATIC) \>= 1), 1)
        LDFLAGS += -Wl,-Bstatic
      endif
    endif
  endif
endif

ifdef USE_ZSTD
  ifeq ($(shell expr $(USE_ZSTD) \>= 1), 1)
    CFLAGS  += -DUSE_ZSTD
    LDFLAGS += -lzstd
    SOURCES += io/rdwr/zstd_file_rdwr_stream.cc
  endif
endif

ifdef USE_FLUIDSYNTH_MIDI
  ifeq ($(shell expr $(USE_FLUIDSYNTH_MIDI) \>= 1), 1)
    CFLAGS  += -DUSE_FLUIDSYNTH_MIDI
    SOURCES += music/fluidsynth.cc
    SOURCES += gui/loadsoundfont_frame.cc
    LDFLAGS += -lfluidsynth
    ifeq ($(OSTYPE),mingw)
      # fluidsynth.pc doesn't properly list dependant libraries, unable to use pkg-config. Manually listed below. Only valid for fluidsynth built with options: "-DBUILD_SHARED_LIBS=0 -Denable-aufile=0 -Denable-dbus=0 -Denable-ipv6=0 -Denable-jack=0 -Denable-ladspa=0 -Denable-midishare=0 -Denable-opensles=0 -Denable-oboe=0 -Denable-oss=0 -Denable-readline=0 -Denable-winmidi=0 -Denable-waveout=0 -Denable-libsndfile=0 -Denable-network=0 -Denable-pulseaudio=0 Denable-dsound=1 -Denable-sdl2=0"
      LDFLAGS += -lglib-2.0 -lintl -liconv -ldsound -lole32
    endif
  endif
else
  USE_FLUIDSYNTH_MIDI := 0
endif

ifdef PROFILE
  ifeq ($(shell expr $(PROFILE) \>= 1), 1)
    CFLAGS   += -pg -DPROFILE
    ifeq ($(shell expr $(PROFILE) \>= 2), 1)
      CFLAGS += -fno-inline

      ifeq ($(findstring clang, $(CXX)),)
        CFLAGS += -fno-schedule-insns
      endif
    endif
    LDFLAGS  += -pg
  endif
endif

ifdef MULTI_THREAD
  ifeq ($(shell expr $(MULTI_THREAD) \>= 1), 1)
    CFLAGS += -DMULTI_THREAD
    ifneq ($(OSTYPE),haiku)
      # mingw has already added pthread statically
      ifneq ($(OSTYPE),mingw)
        LDFLAGS += -lpthread
      endif
    endif
  endif
endif

ifdef WITH_REVISION
  ifeq ($(shell expr $(WITH_REVISION) \>= 1), 1)
    ifeq ($(shell expr $(WITH_REVISION) \>= 2), 1)
      REV := $(WITH_REVISION)
    else
      REV := $(shell ./get_revision.sh)
      $(info Revision is $(REV))
    endif
  endif
endif

ifneq ($(REV),)
  CFLAGS  += -DREVISION=$(REV)
  DUMMY := $(shell rm -f revision.h)
else
  ifeq ("$(wildcard revision.h)","")
    DUMMY := $(shell echo '\#define REVISION' > revision.h)
  endif
endif

GIT_HASH := $(shell git rev-parse --short=7 HEAD 2>/dev/null 1>/dev/null; echo $$?)
ifneq ($(GIT_HASH),)
  GIT_HASH := $(shell git rev-parse --short=7 HEAD)
  $(info Git hash is 0x$(GIT_HASH))
  CFLAGS  += -DGIT_HASH=0x$(GIT_HASH)
endif


CFLAGS   += -Wall -Wextra -Wcast-qual -Wpointer-arith -Wcast-align $(FLAGS)
CCFLAGS  += -ansi -Wstrict-prototypes -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64


# sources (in alphabetic order)
SOURCES += bauer/brueckenbauer.cc
SOURCES += bauer/fabrikbauer.cc
SOURCES += bauer/goods_manager.cc
SOURCES += bauer/hausbauer.cc
SOURCES += bauer/tunnelbauer.cc
SOURCES += bauer/tree_builder.cc
SOURCES += bauer/vehikelbauer.cc
SOURCES += bauer/wegbauer.cc
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
SOURCES += dataobj/environment.cc
SOURCES += dataobj/freelist.cc
SOURCES += dataobj/gameinfo.cc
SOURCES += dataobj/height_map_loader.cc
SOURCES += dataobj/koord.cc
SOURCES += dataobj/koord3d.cc
SOURCES += dataobj/loadsave.cc
SOURCES += dataobj/marker.cc
SOURCES += dataobj/objlist.cc
SOURCES += dataobj/powernet.cc
SOURCES += dataobj/records.cc
SOURCES += dataobj/rect.cc
SOURCES += dataobj/ribi.cc
SOURCES += dataobj/route.cc
SOURCES += dataobj/scenario.cc
SOURCES += dataobj/schedule.cc
SOURCES += dataobj/settings.cc
SOURCES += dataobj/tabfile.cc
SOURCES += dataobj/translator.cc
SOURCES += descriptor/bridge_desc.cc
SOURCES += descriptor/building_desc.cc
SOURCES += descriptor/factory_desc.cc
SOURCES += descriptor/goods_desc.cc
SOURCES += descriptor/ground_desc.cc
SOURCES += descriptor/image.cc
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
SOURCES += descriptor/way_desc.cc
SOURCES += display/font.cc
SOURCES += display/simgraph$(COLOUR_DEPTH).cc
SOURCES += display/simview.cc
SOURCES += display/viewport.cc
SOURCES += finder/placefinder.cc
SOURCES += freight_list_sorter.cc
SOURCES += gui/ai_option_t.cc
SOURCES += gui/ai_selector.cc
SOURCES += gui/banner.cc
SOURCES += gui/base_info.cc
SOURCES += gui/baum_edit.cc
SOURCES += gui/city_info.cc
SOURCES += gui/citybuilding_edit.cc
SOURCES += gui/citylist_frame_t.cc
SOURCES += gui/citylist_stats_t.cc
SOURCES += gui/climates.cc
SOURCES += gui/components/gui_aligned_container.cc
SOURCES += gui/components/gui_building.cc
SOURCES += gui/components/gui_button.cc
SOURCES += gui/components/gui_button_to_chart.cc
SOURCES += gui/components/gui_chart.cc
SOURCES += gui/components/gui_colorbox.cc
SOURCES += gui/components/gui_combobox.cc
SOURCES += gui/components/gui_container.cc
SOURCES += gui/components/gui_convoiinfo.cc
SOURCES += gui/components/gui_divider.cc
SOURCES += gui/components/gui_fixedwidth_textarea.cc
SOURCES += gui/components/gui_flowtext.cc
SOURCES += gui/components/gui_image.cc
SOURCES += gui/components/gui_image_list.cc
SOURCES += gui/components/gui_component.cc
SOURCES += gui/components/gui_label.cc
SOURCES += gui/components/gui_map_preview.cc
SOURCES += gui/components/gui_numberinput.cc
SOURCES += gui/components/gui_obj_view_t.cc
SOURCES += gui/components/gui_schedule.cc
SOURCES += gui/components/gui_scrollbar.cc
SOURCES += gui/components/gui_scrolled_list.cc
SOURCES += gui/components/gui_scrollpane.cc
SOURCES += gui/components/gui_speedbar.cc
SOURCES += gui/components/gui_tab_panel.cc
SOURCES += gui/components/gui_textarea.cc
SOURCES += gui/components/gui_textinput.cc
SOURCES += gui/components/gui_timeinput.cc
SOURCES += gui/components/gui_waytype_tab_panel.cc
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
SOURCES += gui/depotlist_frame.cc
SOURCES += gui/display_settings.cc
SOURCES += gui/enlarge_map_frame_t.cc
SOURCES += gui/extend_edit.cc
SOURCES += gui/fabrik_info.cc
SOURCES += gui/factory_chart.cc
SOURCES += gui/factory_edit.cc
SOURCES += gui/factorylist_frame_t.cc
SOURCES += gui/factorylist_stats_t.cc
SOURCES += gui/goods_frame_t.cc
SOURCES += gui/goods_stats_t.cc
SOURCES += gui/ground_info.cc
SOURCES += gui/groundobj_edit.cc
SOURCES += gui/gui_frame.cc
SOURCES += gui/gui_theme.cc
SOURCES += gui/halt_info.cc
SOURCES += gui/halt_list_filter_frame.cc
SOURCES += gui/halt_list_frame.cc
SOURCES += gui/halt_list_stats.cc
SOURCES += gui/headquarter_info.cc
SOURCES += gui/help_frame.cc
SOURCES += gui/jump_frame.cc
SOURCES += gui/minimap.cc
SOURCES += gui/kennfarbe.cc
SOURCES += gui/label_info.cc
SOURCES += gui/labellist_frame_t.cc
SOURCES += gui/labellist_stats_t.cc
SOURCES += gui/line_item.cc
SOURCES += gui/line_management_gui.cc
SOURCES += gui/load_relief_frame.cc
SOURCES += gui/loadfont_frame.cc
SOURCES += gui/loadsave_frame.cc
SOURCES += gui/map_frame.cc
SOURCES += gui/message_frame_t.cc
SOURCES += gui/message_option_t.cc
SOURCES += gui/message_stats_t.cc
SOURCES += gui/messagebox.cc
SOURCES += gui/money_frame.cc
SOURCES += gui/obj_info.cc
SOURCES += gui/optionen.cc
SOURCES += gui/pakselector.cc
SOURCES += gui/pakinstaller.cc
SOURCES += gui/password_frame.cc
SOURCES += gui/player_frame_t.cc
SOURCES += gui/privatesign_info.cc
SOURCES += gui/savegame_frame.cc
SOURCES += gui/scenario_frame.cc
SOURCES += gui/scenario_info.cc
SOURCES += gui/schedule_list.cc
SOURCES += gui/script_tool_frame.cc
SOURCES += gui/server_frame.cc
SOURCES += gui/settings_frame.cc
SOURCES += gui/settings_stats.cc
SOURCES += gui/signal_spacing.cc
SOURCES += gui/simwin.cc
SOURCES += gui/sound_frame.cc
SOURCES += gui/sprachen.cc
SOURCES += gui/station_building_select.cc
SOURCES += gui/themeselector.cc
SOURCES += gui/tool_selector.cc
SOURCES += gui/trafficlight_info.cc
SOURCES += gui/vehiclelist_frame.cc
SOURCES += gui/welt.cc
SOURCES += io/classify_file.cc
SOURCES += io/raw_image.cc
SOURCES += io/raw_image_bmp.cc
SOURCES += io/raw_image_png.cc
SOURCES += io/raw_image_ppm.cc
SOURCES += io/rdwr/adler32_stream.cc
SOURCES += io/rdwr/compare_file_rd_stream.cc
SOURCES += io/rdwr/bzip2_file_rdwr_stream.cc
SOURCES += io/rdwr/raw_file_rdwr_stream.cc
SOURCES += io/rdwr/rdwr_stream.cc
SOURCES += io/rdwr/zlib_file_rdwr_stream.cc
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
SOURCES += obj/simobj.cc
SOURCES += obj/tunnel.cc
SOURCES += obj/wayobj.cc
SOURCES += obj/wolke.cc
SOURCES += obj/zeiger.cc
SOURCES += old_blockmanager.cc
SOURCES += player/ai.cc
SOURCES += player/ai_goods.cc
SOURCES += player/ai_passenger.cc
SOURCES += player/ai_scripted.cc
SOURCES += player/finance.cc
SOURCES += player/simplay.cc
SOURCES += script/api/api_city.cc
SOURCES += script/api/api_command.cc
SOURCES += script/api/api_const.cc
SOURCES += script/api/api_control.cc
SOURCES += script/api/api_convoy.cc
SOURCES += script/api/api_factory.cc
SOURCES += script/api/api_gui.cc
SOURCES += script/api/api_halt.cc
SOURCES += script/api/api_include.cc
SOURCES += script/api/api_line.cc
SOURCES += script/api/api_map_objects.cc
SOURCES += script/api/api_obj_desc.cc
SOURCES += script/api/api_obj_desc_base.cc
SOURCES += script/api/api_pathfinding.cc
SOURCES += script/api/api_player.cc
SOURCES += script/api/api_scenario.cc
SOURCES += script/api/api_schedule.cc
SOURCES += script/api/api_settings.cc
SOURCES += script/api/api_simple.cc
SOURCES += script/api/api_tiles.cc
SOURCES += script/api/api_world.cc
SOURCES += script/api/export_desc.cc
SOURCES += script/api/get_next.cc
SOURCES += script/api_class.cc
SOURCES += script/api_function.cc
SOURCES += script/api_param.cc
SOURCES += script/dynamic_string.cc
SOURCES += script/export_objs.cc
SOURCES += script/script.cc
SOURCES += script/script_loader.cc
SOURCES += script/script_tool_manager.cc
SOURCES += simcity.cc
SOURCES += simconvoi.cc
SOURCES += simdebug.cc
SOURCES += simdepot.cc
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
SOURCES += simskin.cc
SOURCES += simsound.cc
SOURCES += simticker.cc
SOURCES += simtool.cc
SOURCES += simtool-scripted.cc
SOURCES += simware.cc
SOURCES += simworld.cc
SOURCES += squirrel/sq_extensions.cc
SOURCES += squirrel/sqstdlib/sqstdaux.cc
SOURCES += squirrel/sqstdlib/sqstdblob.cc
SOURCES += squirrel/sqstdlib/sqstdio.cc
SOURCES += squirrel/sqstdlib/sqstdmath.cc
SOURCES += squirrel/sqstdlib/sqstdrex.cc
SOURCES += squirrel/sqstdlib/sqstdstream.cc
SOURCES += squirrel/sqstdlib/sqstdstring.cc
SOURCES += squirrel/sqstdlib/sqstdsystem.cc
SOURCES += squirrel/squirrel/sqapi.cc
SOURCES += squirrel/squirrel/sqbaselib.cc
SOURCES += squirrel/squirrel/sqclass.cc
SOURCES += squirrel/squirrel/sqcompiler.cc
SOURCES += squirrel/squirrel/sqdebug.cc
SOURCES += squirrel/squirrel/sqfuncstate.cc
SOURCES += squirrel/squirrel/sqlexer.cc
SOURCES += squirrel/squirrel/sqmem.cc
SOURCES += squirrel/squirrel/sqobject.cc
SOURCES += squirrel/squirrel/sqstate.cc
SOURCES += squirrel/squirrel/sqtable.cc
SOURCES += squirrel/squirrel/sqvm.cc
SOURCES += sys/simsys.cc
SOURCES += unicode.cc
SOURCES += utils/cbuffer_t.cc
SOURCES += utils/checklist.cc
SOURCES += utils/csv.cc
SOURCES += utils/log.cc
SOURCES += utils/searchfolder.cc
SOURCES += utils/sha1.cc
SOURCES += utils/sha1_hash.cc
SOURCES += utils/simrandom.cc
SOURCES += utils/simstring.cc
SOURCES += utils/simthread.cc
SOURCES += vehicle/air_vehicle.cc
SOURCES += vehicle/movingobj.cc
SOURCES += vehicle/pedestrian.cc
SOURCES += vehicle/rail_vehicle.cc
SOURCES += vehicle/road_vehicle.cc
SOURCES += vehicle/simroadtraffic.cc
SOURCES += vehicle/vehicle.cc
SOURCES += vehicle/vehicle_base.cc
SOURCES += vehicle/water_vehicle.cc


ifeq ($(BACKEND),gdi)
  SOURCES += sys/simsys_w.cc
  SOURCES += sound/win32_sound_xa.cc
  LDFLAGS += -lxaudio2_8
  ifneq ($(shell expr $(USE_FLUIDSYNTH_MIDI) \>= 1), 1)
    SOURCES += music/w32_midi.cc
  endif
endif

ifeq ($(BACKEND),sdl)
  SOURCES += sys/simsys_s.cc
  ifeq ($(OSTYPE),mac)
    AV_FOUNDATION ?= 0
    ifeq ($(shell expr $(AV_FOUNDATION) \>= 1), 1)
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
    SOURCES   += sound/sdl_sound.cc
    ifneq ($(OSTYPE),mingw)
      ifeq ($(USE_FLUIDSYNTH_MIDI), 0)
        SOURCES += music/no_midi.cc
      endif
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
    SDL_CFLAGS    := $(shell $(SDL_CONFIG) --cflags)
    ifeq ($(shell expr $(STATIC) \>= 1), 1)
      SDL_LDFLAGS := $(shell $(SDL_CONFIG) --static-libs)
    else
      SDL_LDFLAGS := $(shell $(SDL_CONFIG) --libs)
    endif
  endif
  CFLAGS += $(SDL_CFLAGS)
  LIBS   += $(SDL_LDFLAGS)
endif

ifeq ($(BACKEND),sdl2)
  SOURCES += sys/simsys_s2.cc
  ifeq ($(OSTYPE),mac)
    AV_FOUNDATION ?= 0
    ifeq ($(shell expr $(AV_FOUNDATION) \>= 1), 1)
      # Core Audio (AVFoundation) base sound system routines
      SOURCES += sound/AVF_core-audio_sound.mm
      LIBS    += -framework Foundation -framework AVFoundation
      ifneq ($(shell expr $(USE_FLUIDSYNTH_MIDI) \>= 1), 1)
        SOURCES += music/AVF_core-audio_midi.mm
      endif
    else
      # Core Audio (Quicktime) base sound system routines
      SOURCES += sound/core-audio_sound.mm
      LIBS    += -framework Foundation -framework QTKit
      ifneq ($(shell expr $(USE_FLUIDSYNTH_MIDI) \>= 1), 1)
        SOURCES += music/core-audio_midi.mm
      endif
    endif
  else
    SOURCES   += sound/sdl2_sound.cc
    ifneq ($(shell expr $(USE_FLUIDSYNTH_MIDI) \>= 1), 1)
      ifneq ($(OSTYPE),mingw)
        SOURCES += music/no_midi.cc
      else
        SOURCES += music/w32_midi.cc
      endif
    endif
  endif

  ifeq ($(SDL2_CONFIG),)
    ifeq ($(OSTYPE),mac)
      SDL_CFLAGS  := -F /Library/Frameworks -I/Library/Frameworks/SDL2.framework/Headers
      SDL_LDFLAGS := -framework SDL2 -F /Library/Frameworks -I /Library/Frameworks/SDL2.framework/Headers
    else
      SDL_CFLAGS  := -I$(MINGDIR)/include/SDL2 -Dmain=SDL_main
      SDL_LDFLAGS := -lSDL2main -lSDL2
    endif
  else
    SDL_CFLAGS    := $(shell $(SDL2_CONFIG) --cflags)
    ifeq ($(shell expr $(STATIC) \>= 1), 1)
      SDL_LDFLAGS := $(shell $(SDL2_CONFIG) --static --libs)
    else
      SDL_LDFLAGS := $(shell $(SDL2_CONFIG) --libs)
    endif
  endif
  CFLAGS += $(SDL_CFLAGS)
  LIBS   += $(SDL_LDFLAGS)
endif

ifeq ($(BACKEND),mixer_sdl2)
  SOURCES += sys/simsys_s2.cc
  ifeq ($(SDL2_CONFIG),)
    ifeq ($(OSTYPE),mac)
      SDL_CFLAGS  := -F /Library/Frameworks -I/Library/Frameworks/SDL2.framework/Headers
      SDL_LDFLAGS := -framework SDL2 -F /Library/Frameworks -I /Library/Frameworks/SDL2.framework/Headers
    else
      SDL_CFLAGS  := -I$(MINGDIR)/include/SDL2 -Dmain=SDL_main
      SDL_LDFLAGS := -lSDL2main -lSDL2
    endif
  else
    SOURCES += sound/sdl_mixer_sound.cc
    SOURCES += music/sdl_midi.cc
    SDL_CFLAGS    := $(shell $(SDL2_CONFIG) --cflags)
    ifeq ($(shell expr $(STATIC) \>= 1), 1)
      SDL_LDFLAGS := $(shell $(SDL2_CONFIG) --static-libs)
    else
      SDL_LDFLAGS := $(shell $(SDL2_CONFIG) --libs)
    endif
  endif
  CFLAGS += $(SDL_CFLAGS)
  LIBS   += $(SDL_LDFLAGS) -lSDL2_mixer
endif

ifeq ($(BACKEND),mixer_sdl)
  SOURCES += sys/simsys_s.cc
  SOURCES += sound/sdl_mixer_sound.cc
  SOURCES += music/sdl_midi.cc
  ifeq ($(SDL_CONFIG),)
    SDL_CFLAGS  := -I$(MINGDIR)/include/SDL -Dmain=SDL_main
    SDL_LDFLAGS := -lmingw32 -lSDLmain -lSDL
  else
    SDL_CFLAGS  := $(shell $(SDL_CONFIG) --cflags)
    SDL_LDFLAGS := $(shell $(SDL_CONFIG) --libs)
  endif
  CFLAGS += $(SDL_CFLAGS)
  LIBS   += $(SDL_LDFLAGS) -lSDL_mixer
endif

ifeq ($(BACKEND),posix)
  SOURCES += sys/simsys_posix.cc
  SOURCES += music/no_midi.cc
  SOURCES += sound/no_sound.cc
endif

CFLAGS += -DCOLOUR_DEPTH=$(COLOUR_DEPTH)

ifeq ($(OSTYPE),mingw)
  SOURCES += simres.rc
  ifneq ($(REV),)
    WINDRES ?= windres -DREVISION=$(REV)
  else
    WINDRES ?= windres -DREVISION
  endif
endif

CCFLAGS  += $(CFLAGS)
CXXFLAGS += $(CFLAGS)

BUILDDIR ?= build/$(CFG)
PROGDIR  ?= $(BUILDDIR)
PROG     ?= sim


.DEFAULT_GOAL := simutrans
.PHONY: simutrans makeobj nettool

include common.mk

ifeq ($(OSTYPE),mac)
  include OSX/osx.mk
endif

all: simutrans makeobj nettool

makeobj:
	@echo "Building makeobj"
	$(Q)$(MAKE) -e -C makeobj FLAGS="$(FLAGS)"

nettool:
	@echo "Building nettool"
	$(Q)$(MAKE) -e -C nettools FLAGS="$(FLAGS)"

test: simutrans
	$(PROGDIR)/$(PROG) -set_workdir $(shell pwd)/simutrans -objects pak -scenario automated-tests -debug 2 -lang en -fps 100

clean:
	@echo "===> Cleaning up"
	$(Q)rm -f $(OBJS)
	$(Q)rm -f $(DEPS)
	$(Q)rm -f $(PROGDIR)/$(PROG)
	$(Q)rm -fr $(PROGDIR)/$(PROG).app
	$(Q)$(MAKE) -e -C makeobj clean
	$(Q)$(MAKE) -e -C nettools clean
