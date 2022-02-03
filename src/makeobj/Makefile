#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

CFG ?= default
-include ../../config.$(CFG)

OSTYPES = beos cygwin freebsd haiku linux mingw mac

ifeq ($(findstring $(OSTYPE), $(OSTYPES)),)
  $(error Unkown OSTYPE "$(OSTYPE)", must be one of "$(OSTYPES)")
endif

PNG_CONFIG ?= pkg-config libpng

ifneq ($(PNG_CONFIG),)
	PNG_CFLAGS  := $(shell $(PNG_CONFIG) --cflags)
	PNG_LDFLAGS := $(shell $(PNG_CONFIG) --libs)
endif

CFLAGS  += $(PNG_CFLAGS)
LDFLAGS += $(PNG_LDFLAGS)

# Absolutely essential
CXXFLAGS += -DMAKEOBJ

STD_LIBS += -lz -lbz2 -lpng

ifeq ($(OSTYPE),cygwin)
  OS_INC   ?= -I/usr/include/mingw
  OS_OPT   ?= -mwin32
  STD_LIBS += -lwinmm
endif

ifeq ($(OSTYPE),mingw)
  OS_OPT   ?= -DPNG_STATIC -DZLIB_STATIC -march=pentium
  LDFLAGS += -static-libgcc -static-libstdc++ -static
  # we need the libraries EXACTLY in this order to link
  STD_LIBS = -lmingw32 -lpng -lz
endif

ifeq ($(OSTYPE),mac)
  CXXFLAGS   += -DUSE_HW -DUSE_C
endif

ifeq ($(OSTYPE),freebsd)
  CXXFLAGS += -I/usr/local/include
endif

ifdef OPTIMISE
  ifeq ($(shell expr $(OPTIMISE) \>= 1), 1)
    CXXFLAGS += -O3

    # clang does not support fno-schedule-insns
    ifeq ($(findstring clang, $(CXX)),)
      CXXFLAGS += -fno-schedule-insns
    endif
  endif
else
  CXXFLAGS += -O
endif

ifdef DEBUG
  ifeq ($(shell expr $(DEBUG) \>= 1), 1)
    CXXFLAGS += -g -DDEBUG
  endif
  ifeq ($(shell expr $(DEBUG) \>= 2), 1)
    CXXFLAGS += -fno-inline
  endif
  ifeq ($(shell expr $(DEBUG) \>= 3), 1)
    CXXFLAGS += -O0
  endif
else
  CXXFLAGS += -DNDEBUG
endif

ifdef PROFILE
  ifeq ($(shell expr $(PROFILE) \>= 1), 1)
    CXXFLAGS += -pg -DPROFILE -fno-inline
    LDFLAGS += -pg
  endif
endif

CXXFLAGS += -DREVISION -Wall -Wextra -Wcast-qual -Wpointer-arith -Wcast-align $(OS_INC) $(OS_OPT) $(FLAGS)

# SOLO_SOURCES contains files which are unique to makeobj;
# SHARED_SOURCES contains those with the exact same object code in makeobj and simutrans;
# VARIANT_SOURCES contains those which need different .o files for makeobj and simutrans.
# At the moment they're all treated identically, of course.
SOLO_SOURCES += makeobj.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/bridge_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/building_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/citycar_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/crossing_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/factory_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/get_climate.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/get_waytype.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/good_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/groundobj_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/ground_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/image_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/imagelist2d_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/imagelist_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/obj_node.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/obj_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/pedestrian_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/roadsign_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/root_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/sim_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/skin_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/sound_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/text_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/tree_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/tunnel_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/vehicle_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/way_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/way_obj_writer.cc
SOLO_SOURCES += ../simutrans/descriptor/writer/xref_writer.cc
SHARED_SOURCES += ../simutrans/descriptor/image.cc
SHARED_SOURCES += ../simutrans/dataobj/freelist.cc
SHARED_SOURCES += ../simutrans/io/raw_image.cc
SHARED_SOURCES += ../simutrans/simio.h
SHARED_SOURCES += ../simutrans/simdebug.cc
SHARED_SOURCES += ../simutrans/simmem.cc
SHARED_SOURCES += ../simutrans/utils/simstring.cc
SHARED_SOURCES += ../simutrans/utils/searchfolder.cc
VARIANT_SOURCES += ../simutrans/dataobj/tabfile.cc
VARIANT_SOURCES += ../simutrans/io/classify_file.cc
VARIANT_SOURCES += ../simutrans/io/raw_image_bmp.cc
VARIANT_SOURCES += ../simutrans/io/raw_image_png.cc
VARIANT_SOURCES += ../simutrans/io/raw_image_ppm.cc
VARIANT_SOURCES += ../simutrans/utils/log.cc

SOURCES ?= $(SOLO_SOURCES) $(SHARED_SOURCES) $(VARIANT_SOURCES)

BUILDDIR ?= build/$(CFG)
TOOL  = makeobj
PROG ?= makeobj

ifeq ($(origin MAKEOBJ_PROGDIR), undefined)
  MAKEOBJ_PROGDIR := ../../$(BUILDDIR)/$(TOOL)
endif

BUILDDIR := ../../$(BUILDDIR)

TOOL_PROGDIR = $(MAKEOBJ_PROGDIR)

include ../../uncommon.mk
