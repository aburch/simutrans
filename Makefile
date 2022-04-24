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

DYNAMICSTART =
DYNAMICEND =

CFG ?= default
-include config.$(CFG)


HOSTCC  ?=$(CC)
HOSTCXX ?=$(CXX)

SDL2_CONFIG      ?= pkg-config sdl2
#SDL2_CONFIG     ?= sdl2-config
FREETYPE_CONFIG  ?= pkg-config freetype2
#FREETYPE_CONFIG ?= freetype-config

BACKENDS  := gdi sdl2 mixer_sdl2 posix
OSTYPES   := amiga freebsd haiku linux mac mingw openbsd


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

# since threads are needed to be linked dynamically on most systems
ifdef MULTI_THREAD
  ifeq ($(shell expr $(MULTI_THREAD) \>= 1), 1)
    CFLAGS += -DMULTI_THREAD
    ifneq ($(OSTYPE),mingw)
      ifneq ($(OSTYPE),haiku)
        # mingw has already added pthread statically
        ifneq ($(OSTYPE),mingw)
	  LDFLAGS += -lpthread
        endif
      endif
    endif
  else
    ifneq ($(OSTYPE),mingw)
      ifeq ($(BACKEND),sdl2)
        LDFLAGS += -lpthread
      endif
      ifeq ($(BACKEND),mixer_sdl2)
        LDFLAGS += -lpthread
      endif
    endif
  endif
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
  ifeq ($(shell expr $(STATIC) \>= 1), 1)
    LDFLAGS +=  -Wl,-Bstatic -lm
    DYNAMICSTART = -Wl,-Bdynamic 
    DYNAMICEND = -Wl,-Bstatic 
  endif
else ifeq ($(OSTYPE),mac)
  SOURCES += src/simutrans/OSX/translocation.m
  LDFLAGS += -framework Cocoa
endif

ifeq ($(BACKEND),sdl2)
  SOURCES += src/simutrans/sys/clipboard_s2.cc
else ifeq ($(OSTYPE),mingw)
  SOURCES += src/simutrans/sys/clipboard_w32.cc
else
  SOURCES += src/simutrans/sys/clipboard_internal.cc
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
          LDFLAGS += $(subst -lm ,,$(FTF))
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
    SOURCES += src/simutrans/io/rdwr/zstd_file_rdwr_stream.cc
  endif
endif

ifdef USE_FLUIDSYNTH_MIDI
  ifeq ($(shell expr $(USE_FLUIDSYNTH_MIDI) \>= 1), 1)
    CFLAGS  += -DUSE_FLUIDSYNTH_MIDI
    SOURCES += src/simutrans/music/fluidsynth.cc
    SOURCES += src/simutrans/gui/loadsoundfont_frame.cc
    LDFLAGS += $(DYNAMICSTART)-lfluidsynth $(DYNAMICEND)
    ifeq ($(OSTYPE),mingw)
      # fluidsynth.pc doesn't properly list dependant libraries, unable to use pkg-config. Manually listed below. Only valid for fluidsynth built with options: "-DBUILD_SHARED_LIBS=0 -Denable-aufile=0 -Denable-dbus=0 -Denable-ipv6=0 -Denable-jack=0 -Denable-ladspa=0 -Denable-midishare=0 -Denable-opensles=0 -Denable-oboe=0 -Denable-oss=0 -Denable-readline=0 -Denable-winmidi=0 -Denable-waveout=0 -Denable-libsndfile=0 -Denable-network=0 -Denable-pulseaudio=0 Denable-dsound=1 -Denable-sdl2=0"
      LDFLAGS += -lglib-2.0 -lintl -liconv -ldsound -lole32
    else
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

ifdef WITH_REVISION
  ifeq ($(shell expr $(WITH_REVISION) \>= 1), 1)
    ifeq ($(shell expr $(WITH_REVISION) \>= 2), 1)
      REV := $(WITH_REVISION)
    else
      REV := $(shell tools/get_revision.sh)
      $(info Revision is $(REV))
    endif
  endif
endif

ifneq ($(REV),)
  CFLAGS  += -DREVISION=$(REV)
  DUMMY := $(shell rm -f src/simutrans/revision.h)
else
  ifeq ("$(wildcard src/simutrans/revision.h)","")
    DUMMY := $(shell echo '\#define REVISION' > src/simutrans/revision.h)
  endif
endif

GIT_HASH := $(shell git rev-parse --short=7 HEAD)
ifeq ($(.SHELLSTATUS),0)
  $(info Git hash is 0x$(GIT_HASH))
  CFLAGS  += -DGIT_HASH=0x$(GIT_HASH)
endif


CFLAGS   += -Wall -Wextra -Wcast-qual -Wpointer-arith -Wcast-align $(FLAGS)
CCFLAGS  += -ansi -Wstrict-prototypes -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64


# sources (in alphabetic order)
SOURCES += src/simutrans/builder/brueckenbauer.cc
SOURCES += src/simutrans/builder/fabrikbauer.cc
SOURCES += src/simutrans/builder/goods_manager.cc
SOURCES += src/simutrans/builder/hausbauer.cc
SOURCES += src/simutrans/builder/tree_builder.cc
SOURCES += src/simutrans/builder/tunnelbauer.cc
SOURCES += src/simutrans/builder/vehikelbauer.cc
SOURCES += src/simutrans/builder/wegbauer.cc
SOURCES += src/simutrans/dataobj/crossing_logic.cc
SOURCES += src/simutrans/dataobj/environment.cc
SOURCES += src/simutrans/dataobj/freelist.cc
SOURCES += src/simutrans/dataobj/gameinfo.cc
SOURCES += src/simutrans/dataobj/height_map_loader.cc
SOURCES += src/simutrans/dataobj/koord.cc
SOURCES += src/simutrans/dataobj/koord3d.cc
SOURCES += src/simutrans/dataobj/loadsave.cc
SOURCES += src/simutrans/dataobj/marker.cc
SOURCES += src/simutrans/dataobj/objlist.cc
SOURCES += src/simutrans/dataobj/pakset_manager.cc
SOURCES += src/simutrans/dataobj/powernet.cc
SOURCES += src/simutrans/dataobj/records.cc
SOURCES += src/simutrans/dataobj/rect.cc
SOURCES += src/simutrans/dataobj/ribi.cc
SOURCES += src/simutrans/dataobj/route.cc
SOURCES += src/simutrans/dataobj/scenario.cc
SOURCES += src/simutrans/dataobj/schedule.cc
SOURCES += src/simutrans/dataobj/settings.cc
SOURCES += src/simutrans/dataobj/tabfile.cc
SOURCES += src/simutrans/dataobj/translator.cc
SOURCES += src/simutrans/descriptor/bridge_desc.cc
SOURCES += src/simutrans/descriptor/building_desc.cc
SOURCES += src/simutrans/descriptor/factory_desc.cc
SOURCES += src/simutrans/descriptor/goods_desc.cc
SOURCES += src/simutrans/descriptor/ground_desc.cc
SOURCES += src/simutrans/descriptor/image.cc
SOURCES += src/simutrans/descriptor/obj_base_desc.cc
SOURCES += src/simutrans/descriptor/reader/bridge_reader.cc
SOURCES += src/simutrans/descriptor/reader/building_reader.cc
SOURCES += src/simutrans/descriptor/reader/citycar_reader.cc
SOURCES += src/simutrans/descriptor/reader/crossing_reader.cc
SOURCES += src/simutrans/descriptor/reader/factory_reader.cc
SOURCES += src/simutrans/descriptor/reader/good_reader.cc
SOURCES += src/simutrans/descriptor/reader/ground_reader.cc
SOURCES += src/simutrans/descriptor/reader/groundobj_reader.cc
SOURCES += src/simutrans/descriptor/reader/image_reader.cc
SOURCES += src/simutrans/descriptor/reader/imagelist2d_reader.cc
SOURCES += src/simutrans/descriptor/reader/imagelist_reader.cc
SOURCES += src/simutrans/descriptor/reader/obj_reader.cc
SOURCES += src/simutrans/descriptor/reader/pedestrian_reader.cc
SOURCES += src/simutrans/descriptor/reader/roadsign_reader.cc
SOURCES += src/simutrans/descriptor/reader/root_reader.cc
SOURCES += src/simutrans/descriptor/reader/sim_reader.cc
SOURCES += src/simutrans/descriptor/reader/skin_reader.cc
SOURCES += src/simutrans/descriptor/reader/sound_reader.cc
SOURCES += src/simutrans/descriptor/reader/text_reader.cc
SOURCES += src/simutrans/descriptor/reader/tree_reader.cc
SOURCES += src/simutrans/descriptor/reader/tunnel_reader.cc
SOURCES += src/simutrans/descriptor/reader/vehicle_reader.cc
SOURCES += src/simutrans/descriptor/reader/way_obj_reader.cc
SOURCES += src/simutrans/descriptor/reader/way_reader.cc
SOURCES += src/simutrans/descriptor/reader/xref_reader.cc
SOURCES += src/simutrans/descriptor/sound_desc.cc
SOURCES += src/simutrans/descriptor/tunnel_desc.cc
SOURCES += src/simutrans/descriptor/vehicle_desc.cc
SOURCES += src/simutrans/descriptor/way_desc.cc
SOURCES += src/simutrans/display/font.cc
SOURCES += src/simutrans/display/simgraph$(COLOUR_DEPTH).cc
SOURCES += src/simutrans/display/simview.cc
SOURCES += src/simutrans/display/viewport.cc
SOURCES += src/simutrans/freight_list_sorter.cc
SOURCES += src/simutrans/ground/boden.cc
SOURCES += src/simutrans/ground/brueckenboden.cc
SOURCES += src/simutrans/ground/fundament.cc
SOURCES += src/simutrans/ground/grund.cc
SOURCES += src/simutrans/ground/monorailboden.cc
SOURCES += src/simutrans/ground/tunnelboden.cc
SOURCES += src/simutrans/ground/wasser.cc
SOURCES += src/simutrans/gui/ai_option.cc
SOURCES += src/simutrans/gui/ai_selector.cc
SOURCES += src/simutrans/gui/banner.cc
SOURCES += src/simutrans/gui/base_info.cc
SOURCES += src/simutrans/gui/baum_edit.cc
SOURCES += src/simutrans/gui/city_info.cc
SOURCES += src/simutrans/gui/citybuilding_edit.cc
SOURCES += src/simutrans/gui/citylist_frame.cc
SOURCES += src/simutrans/gui/citylist_stats.cc
SOURCES += src/simutrans/gui/climates.cc
SOURCES += src/simutrans/gui/components/gui_aligned_container.cc
SOURCES += src/simutrans/gui/components/gui_building.cc
SOURCES += src/simutrans/gui/components/gui_button.cc
SOURCES += src/simutrans/gui/components/gui_button_to_chart.cc
SOURCES += src/simutrans/gui/components/gui_chart.cc
SOURCES += src/simutrans/gui/components/gui_colorbox.cc
SOURCES += src/simutrans/gui/components/gui_combobox.cc
SOURCES += src/simutrans/gui/components/gui_component.cc
SOURCES += src/simutrans/gui/components/gui_container.cc
SOURCES += src/simutrans/gui/components/gui_convoiinfo.cc
SOURCES += src/simutrans/gui/components/gui_divider.cc
SOURCES += src/simutrans/gui/components/gui_fixedwidth_textarea.cc
SOURCES += src/simutrans/gui/components/gui_flowtext.cc
SOURCES += src/simutrans/gui/components/gui_image.cc
SOURCES += src/simutrans/gui/components/gui_image_list.cc
SOURCES += src/simutrans/gui/components/gui_label.cc
SOURCES += src/simutrans/gui/components/gui_map_preview.cc
SOURCES += src/simutrans/gui/components/gui_numberinput.cc
SOURCES += src/simutrans/gui/components/gui_obj_view.cc
SOURCES += src/simutrans/gui/components/gui_schedule.cc
SOURCES += src/simutrans/gui/components/gui_scrollbar.cc
SOURCES += src/simutrans/gui/components/gui_scrolled_list.cc
SOURCES += src/simutrans/gui/components/gui_scrollpane.cc
SOURCES += src/simutrans/gui/components/gui_speedbar.cc
SOURCES += src/simutrans/gui/components/gui_tab_panel.cc
SOURCES += src/simutrans/gui/components/gui_textarea.cc
SOURCES += src/simutrans/gui/components/gui_textinput.cc
SOURCES += src/simutrans/gui/components/gui_timeinput.cc
SOURCES += src/simutrans/gui/components/gui_waytype_tab_panel.cc
SOURCES += src/simutrans/gui/components/gui_world_view.cc
SOURCES += src/simutrans/gui/convoi_detail.cc
SOURCES += src/simutrans/gui/convoi_filter_frame.cc
SOURCES += src/simutrans/gui/convoi_frame.cc
SOURCES += src/simutrans/gui/convoi_info.cc
SOURCES += src/simutrans/gui/convoy_item.cc
SOURCES += src/simutrans/gui/curiosity_edit.cc
SOURCES += src/simutrans/gui/curiositylist_frame.cc
SOURCES += src/simutrans/gui/curiositylist_stats.cc
SOURCES += src/simutrans/gui/depot_frame.cc
SOURCES += src/simutrans/gui/depotlist_frame.cc
SOURCES += src/simutrans/gui/display_settings.cc
SOURCES += src/simutrans/gui/enlarge_map_frame.cc
SOURCES += src/simutrans/gui/extend_edit.cc
SOURCES += src/simutrans/gui/fabrik_info.cc
SOURCES += src/simutrans/gui/factory_chart.cc
SOURCES += src/simutrans/gui/factory_edit.cc
SOURCES += src/simutrans/gui/factorylist_frame.cc
SOURCES += src/simutrans/gui/factorylist_stats.cc
SOURCES += src/simutrans/gui/goods_frame.cc
SOURCES += src/simutrans/gui/goods_stats.cc
SOURCES += src/simutrans/gui/ground_info.cc
SOURCES += src/simutrans/gui/groundobj_edit.cc
SOURCES += src/simutrans/gui/gui_frame.cc
SOURCES += src/simutrans/gui/gui_theme.cc
SOURCES += src/simutrans/gui/halt_info.cc
SOURCES += src/simutrans/gui/halt_list_filter_frame.cc
SOURCES += src/simutrans/gui/halt_list_frame.cc
SOURCES += src/simutrans/gui/halt_list_stats.cc
SOURCES += src/simutrans/gui/headquarter_info.cc
SOURCES += src/simutrans/gui/help_frame.cc
SOURCES += src/simutrans/gui/jump_frame.cc
SOURCES += src/simutrans/gui/kennfarbe.cc
SOURCES += src/simutrans/gui/label_info.cc
SOURCES += src/simutrans/gui/labellist_frame.cc
SOURCES += src/simutrans/gui/labellist_stats.cc
SOURCES += src/simutrans/gui/line_item.cc
SOURCES += src/simutrans/gui/line_management_gui.cc
SOURCES += src/simutrans/gui/load_relief_frame.cc
SOURCES += src/simutrans/gui/loadfont_frame.cc
SOURCES += src/simutrans/gui/loadsave_frame.cc
SOURCES += src/simutrans/gui/map_frame.cc
SOURCES += src/simutrans/gui/message_frame.cc
SOURCES += src/simutrans/gui/message_option.cc
SOURCES += src/simutrans/gui/message_stats.cc
SOURCES += src/simutrans/gui/messagebox.cc
SOURCES += src/simutrans/gui/minimap.cc
SOURCES += src/simutrans/gui/money_frame.cc
SOURCES += src/simutrans/gui/obj_info.cc
SOURCES += src/simutrans/gui/optionen.cc
SOURCES += src/simutrans/gui/pakinstaller.cc
SOURCES += src/simutrans/gui/pakselector.cc
SOURCES += src/simutrans/gui/password_frame.cc
SOURCES += src/simutrans/gui/player_frame.cc
SOURCES += src/simutrans/gui/privatesign_info.cc
SOURCES += src/simutrans/gui/savegame_frame.cc
SOURCES += src/simutrans/gui/scenario_frame.cc
SOURCES += src/simutrans/gui/scenario_info.cc
SOURCES += src/simutrans/gui/schedule_list.cc
SOURCES += src/simutrans/gui/script_tool_frame.cc
SOURCES += src/simutrans/gui/server_frame.cc
SOURCES += src/simutrans/gui/settings_frame.cc
SOURCES += src/simutrans/gui/settings_stats.cc
SOURCES += src/simutrans/gui/signal_info.cc
SOURCES += src/simutrans/gui/signal_spacing.cc
SOURCES += src/simutrans/gui/simwin.cc
SOURCES += src/simutrans/gui/sound_frame.cc
SOURCES += src/simutrans/gui/sprachen.cc
SOURCES += src/simutrans/gui/station_building_select.cc
SOURCES += src/simutrans/gui/themeselector.cc
SOURCES += src/simutrans/gui/tool_selector.cc
SOURCES += src/simutrans/gui/trafficlight_info.cc
SOURCES += src/simutrans/gui/vehiclelist_frame.cc
SOURCES += src/simutrans/gui/welt.cc
SOURCES += src/simutrans/io/classify_file.cc
SOURCES += src/simutrans/io/raw_image.cc
SOURCES += src/simutrans/io/raw_image_bmp.cc
SOURCES += src/simutrans/io/raw_image_png.cc
SOURCES += src/simutrans/io/raw_image_ppm.cc
SOURCES += src/simutrans/io/rdwr/adler32_stream.cc
SOURCES += src/simutrans/io/rdwr/bzip2_file_rdwr_stream.cc
SOURCES += src/simutrans/io/rdwr/compare_file_rd_stream.cc
SOURCES += src/simutrans/io/rdwr/raw_file_rdwr_stream.cc
SOURCES += src/simutrans/io/rdwr/rdwr_stream.cc
SOURCES += src/simutrans/io/rdwr/zlib_file_rdwr_stream.cc
SOURCES += src/simutrans/network/checksum.cc
SOURCES += src/simutrans/network/memory_rw.cc
SOURCES += src/simutrans/network/network.cc
SOURCES += src/simutrans/network/network_address.cc
SOURCES += src/simutrans/network/network_cmd.cc
SOURCES += src/simutrans/network/network_cmd_ingame.cc
SOURCES += src/simutrans/network/network_cmd_scenario.cc
SOURCES += src/simutrans/network/network_cmp_pakset.cc
SOURCES += src/simutrans/network/network_file_transfer.cc
SOURCES += src/simutrans/network/network_packet.cc
SOURCES += src/simutrans/network/network_socket_list.cc
SOURCES += src/simutrans/network/pakset_info.cc
SOURCES += src/simutrans/obj/baum.cc
SOURCES += src/simutrans/obj/bruecke.cc
SOURCES += src/simutrans/obj/crossing.cc
SOURCES += src/simutrans/obj/depot.cc
SOURCES += src/simutrans/obj/field.cc
SOURCES += src/simutrans/obj/gebaeude.cc
SOURCES += src/simutrans/obj/groundobj.cc
SOURCES += src/simutrans/obj/label.cc
SOURCES += src/simutrans/obj/leitung2.cc
SOURCES += src/simutrans/obj/pillar.cc
SOURCES += src/simutrans/obj/roadsign.cc
SOURCES += src/simutrans/obj/signal.cc
SOURCES += src/simutrans/obj/simobj.cc
SOURCES += src/simutrans/obj/tunnel.cc
SOURCES += src/simutrans/obj/way/kanal.cc
SOURCES += src/simutrans/obj/way/maglev.cc
SOURCES += src/simutrans/obj/way/monorail.cc
SOURCES += src/simutrans/obj/way/narrowgauge.cc
SOURCES += src/simutrans/obj/way/runway.cc
SOURCES += src/simutrans/obj/way/schiene.cc
SOURCES += src/simutrans/obj/way/strasse.cc
SOURCES += src/simutrans/obj/way/weg.cc
SOURCES += src/simutrans/obj/wayobj.cc
SOURCES += src/simutrans/obj/wolke.cc
SOURCES += src/simutrans/obj/zeiger.cc
SOURCES += src/simutrans/old_blockmanager.cc
SOURCES += src/simutrans/player/ai.cc
SOURCES += src/simutrans/player/ai_goods.cc
SOURCES += src/simutrans/player/ai_passenger.cc
SOURCES += src/simutrans/player/ai_scripted.cc
SOURCES += src/simutrans/player/finance.cc
SOURCES += src/simutrans/player/simplay.cc
SOURCES += src/simutrans/script/api/api_city.cc
SOURCES += src/simutrans/script/api/api_command.cc
SOURCES += src/simutrans/script/api/api_const.cc
SOURCES += src/simutrans/script/api/api_control.cc
SOURCES += src/simutrans/script/api/api_convoy.cc
SOURCES += src/simutrans/script/api/api_factory.cc
SOURCES += src/simutrans/script/api/api_gui.cc
SOURCES += src/simutrans/script/api/api_halt.cc
SOURCES += src/simutrans/script/api/api_include.cc
SOURCES += src/simutrans/script/api/api_line.cc
SOURCES += src/simutrans/script/api/api_map_objects.cc
SOURCES += src/simutrans/script/api/api_obj_desc.cc
SOURCES += src/simutrans/script/api/api_obj_desc_base.cc
SOURCES += src/simutrans/script/api/api_pathfinding.cc
SOURCES += src/simutrans/script/api/api_player.cc
SOURCES += src/simutrans/script/api/api_scenario.cc
SOURCES += src/simutrans/script/api/api_schedule.cc
SOURCES += src/simutrans/script/api/api_settings.cc
SOURCES += src/simutrans/script/api/api_simple.cc
SOURCES += src/simutrans/script/api/api_tiles.cc
SOURCES += src/simutrans/script/api/api_world.cc
SOURCES += src/simutrans/script/api/export_desc.cc
SOURCES += src/simutrans/script/api/get_next.cc
SOURCES += src/simutrans/script/api_class.cc
SOURCES += src/simutrans/script/api_function.cc
SOURCES += src/simutrans/script/api_param.cc
SOURCES += src/simutrans/script/dynamic_string.cc
SOURCES += src/simutrans/script/export_objs.cc
SOURCES += src/simutrans/script/script.cc
SOURCES += src/simutrans/script/script_loader.cc
SOURCES += src/simutrans/script/script_tool_manager.cc
SOURCES += src/simutrans/simconvoi.cc
SOURCES += src/simutrans/simdebug.cc
SOURCES += src/simutrans/simevent.cc
SOURCES += src/simutrans/simfab.cc
SOURCES += src/simutrans/simhalt.cc
SOURCES += src/simutrans/siminteraction.cc
SOURCES += src/simutrans/simintr.cc
SOURCES += src/simutrans/simio.cc
SOURCES += src/simutrans/simline.cc
SOURCES += src/simutrans/simlinemgmt.cc
SOURCES += src/simutrans/simloadingscreen.cc
SOURCES += src/simutrans/simmain.cc
SOURCES += src/simutrans/simmem.cc
SOURCES += src/simutrans/simmesg.cc
SOURCES += src/simutrans/simskin.cc
SOURCES += src/simutrans/simsound.cc
SOURCES += src/simutrans/simticker.cc
SOURCES += src/simutrans/simware.cc
SOURCES += src/simutrans/sys/simsys.cc
SOURCES += src/simutrans/tool/simmenu.cc
SOURCES += src/simutrans/tool/simtool-scripted.cc
SOURCES += src/simutrans/tool/simtool.cc
SOURCES += src/simutrans/utils/cbuffer.cc
SOURCES += src/simutrans/utils/checklist.cc
SOURCES += src/simutrans/utils/csv.cc
SOURCES += src/simutrans/utils/log.cc
SOURCES += src/simutrans/utils/searchfolder.cc
SOURCES += src/simutrans/utils/sha1.cc
SOURCES += src/simutrans/utils/sha1_hash.cc
SOURCES += src/simutrans/utils/simrandom.cc
SOURCES += src/simutrans/utils/simstring.cc
SOURCES += src/simutrans/utils/simthread.cc
SOURCES += src/simutrans/utils/unicode.cc
SOURCES += src/simutrans/vehicle/air_vehicle.cc
SOURCES += src/simutrans/vehicle/movingobj.cc
SOURCES += src/simutrans/vehicle/pedestrian.cc
SOURCES += src/simutrans/vehicle/rail_vehicle.cc
SOURCES += src/simutrans/vehicle/road_vehicle.cc
SOURCES += src/simutrans/vehicle/simroadtraffic.cc
SOURCES += src/simutrans/vehicle/vehicle.cc
SOURCES += src/simutrans/vehicle/vehicle_base.cc
SOURCES += src/simutrans/vehicle/water_vehicle.cc
SOURCES += src/simutrans/world/placefinder.cc
SOURCES += src/simutrans/world/simcity.cc
SOURCES += src/simutrans/world/simplan.cc
SOURCES += src/simutrans/world/simworld.cc
SOURCES += src/simutrans/world/surface.cc
SOURCES += src/simutrans/world/terraformer.cc
SOURCES += src/squirrel/sq_extensions.cc
SOURCES += src/squirrel/sqstdlib/sqstdaux.cc
SOURCES += src/squirrel/sqstdlib/sqstdblob.cc
SOURCES += src/squirrel/sqstdlib/sqstdio.cc
SOURCES += src/squirrel/sqstdlib/sqstdmath.cc
SOURCES += src/squirrel/sqstdlib/sqstdrex.cc
SOURCES += src/squirrel/sqstdlib/sqstdstream.cc
SOURCES += src/squirrel/sqstdlib/sqstdstring.cc
SOURCES += src/squirrel/sqstdlib/sqstdsystem.cc
SOURCES += src/squirrel/squirrel/sqapi.cc
SOURCES += src/squirrel/squirrel/sqbaselib.cc
SOURCES += src/squirrel/squirrel/sqclass.cc
SOURCES += src/squirrel/squirrel/sqcompiler.cc
SOURCES += src/squirrel/squirrel/sqdebug.cc
SOURCES += src/squirrel/squirrel/sqfuncstate.cc
SOURCES += src/squirrel/squirrel/sqlexer.cc
SOURCES += src/squirrel/squirrel/sqmem.cc
SOURCES += src/squirrel/squirrel/sqobject.cc
SOURCES += src/squirrel/squirrel/sqstate.cc
SOURCES += src/squirrel/squirrel/sqtable.cc
SOURCES += src/squirrel/squirrel/sqvm.cc

ifeq ($(BACKEND),gdi)
  SOURCES += src/simutrans/sys/simsys_w.cc
  SOURCES += src/simutrans/sound/win32_sound_xa.cc
  LDFLAGS += -lxaudio2_8
  ifneq ($(shell expr $(USE_FLUIDSYNTH_MIDI) \>= 1), 1)
    SOURCES += src/simutrans/music/w32_midi.cc
  endif
endif

ifeq ($(BACKEND),sdl2)
  SOURCES += src/simutrans/sys/simsys_s2.cc
  ifeq ($(OSTYPE),mac)
    AV_FOUNDATION ?= 0
    ifeq ($(shell expr $(AV_FOUNDATION) \>= 1), 1)
      # Core Audio (AVFoundation) base sound system routines
      SOURCES += src/simutrans/sound/AVF_core-audio_sound.mm
      LIBS    += -framework Foundation -framework AVFoundation
      ifneq ($(shell expr $(USE_FLUIDSYNTH_MIDI) \>= 1), 1)
        SOURCES += src/simutrans/music/AVF_core-audio_midi.mm
      endif
    else
      # Core Audio (Quicktime) base sound system routines
      SOURCES += src/simutrans/sound/core-audio_sound.mm
      LIBS    += -framework Foundation -framework QTKit
      ifneq ($(shell expr $(USE_FLUIDSYNTH_MIDI) \>= 1), 1)
        SOURCES += src/simutrans/music/core-audio_midi.mm
      endif
    endif
  else
    SOURCES += src/simutrans/sound/sdl2_sound.cc
    ifneq ($(shell expr $(USE_FLUIDSYNTH_MIDI) \>= 1), 1)
      ifneq ($(OSTYPE),mingw)
        SOURCES += src/simutrans/music/no_midi.cc
      else
        SOURCES += src/simutrans/music/w32_midi.cc
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
    SDL_LDFLAGS   := $(DYNAMICSTART) $(shell $(SDL2_CONFIG) --libs) $(DYNAMICEND)
    ifeq ($(shell expr $(STATIC) \>= 1), 1)
      ifeq ($(OSTYPE),mingw)
        SDL_LDFLAGS = $(shell $(SDL2_CONFIG) --static --libs)
      endif
    endif
  endif
  CFLAGS += $(SDL_CFLAGS)
  LIBS   += $(SDL_LDFLAGS)
endif

ifeq ($(BACKEND),mixer_sdl2)
  SOURCES += src/simutrans/sys/simsys_s2.cc
  ifeq ($(SDL2_CONFIG),)
    ifeq ($(OSTYPE),mac)
      SDL_CFLAGS  := -F /Library/Frameworks -I/Library/Frameworks/SDL2.framework/Headers
      SDL_LDFLAGS := -framework SDL2 -F /Library/Frameworks -I /Library/Frameworks/SDL2.framework/Headers
    else
      SDL_CFLAGS  := -I$(MINGDIR)/include/SDL2 -Dmain=SDL_main
      SDL_LDFLAGS := -lSDL2main -lSDL2
    endif
  else
    SOURCES += src/simutrans/sound/sdl_mixer_sound.cc
    SOURCES += src/simutrans/music/sdl_midi.cc
    SDL_CFLAGS    := $(shell $(SDL2_CONFIG) --cflags)
    SDL_LDFLAGS   := $(DYMANICSTART) $(shell $(SDL2_CONFIG) --libs) -lSDL2_mixer $(DYNAMICEND)
    ifeq ($(shell expr $(STATIC) \>= 1), 1)
      ifeq ($(OSTYPE),mingw)
        SDL_LDFLAGS = $(shell $(SDL2_CONFIG) --static --libs) -lSDL2_mixer
      endif
    endif
  endif
  CFLAGS += $(SDL_CFLAGS)
  LIBS   += $(SDL_LDFLAGS)
endif

ifeq ($(BACKEND),posix)
  SOURCES += src/simutrans/sys/simsys_posix.cc
  SOURCES += src/simutrans/music/no_midi.cc
  SOURCES += src/simutrans/sound/no_sound.cc
endif

CFLAGS += -DCOLOUR_DEPTH=$(COLOUR_DEPTH)

ifeq ($(OSTYPE),mingw)
  SOURCES += src/Windows/simres.rc
  ifneq ($(REV),)
    WINDRES ?= windres -DREVISION=$(REV)
  else
    WINDRES ?= windres -DREVISION
  endif
endif

CCFLAGS  += $(CFLAGS)
CXXFLAGS += $(CFLAGS)

# static linking everything but SDL2 works is the best one can do on Linux ...
ifeq ($(OSTYPE),linux)
  ifeq ($(shell expr $(STATIC) \>= 1), 1)
    LIBS += -static-libgcc -static-libstdc++
    ifeq ($(BACKEND),posix)
      LIBS += -static
    endif
  endif
endif

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
	$(Q)$(MAKE) -e -C src/makeobj FLAGS="$(FLAGS)"

nettool:
	@echo "Building nettool"
	$(Q)$(MAKE) -e -C src/nettool FLAGS="$(FLAGS)"

test: simutrans
	$(PROGDIR)/$(PROG) -set_basedir $(shell pwd)/simutrans -objects pak -scenario automated-tests -debug 2 -lang en -fps 100

clean:
	@echo "===> Cleaning up"
	$(Q)rm -f $(OBJS)
	$(Q)rm -f $(DEPS)
	$(Q)rm -f $(PROGDIR)/$(PROG)
	$(Q)rm -fr $(PROGDIR)/$(PROG).app
	$(Q)$(MAKE) -e -C src/makeobj clean
	$(Q)$(MAKE) -e -C src/nettool clean
