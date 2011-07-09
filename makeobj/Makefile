CFG ?= default
-include ../config.$(CFG)

OSTYPES       = beos cygwin freebsd haiku linux mingw mac

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
CFLAGS += -DMAKEOBJ
CXXFLAGS += -DMAKEOBJ

STD_LIBS += -lz -lbz2 -lpng -lm

ifeq ($(OSTYPE),cygwin)
  OS_INC   ?= -I/usr/include/mingw
  OS_OPT   ?= -mwin32
  STD_LIBS += -lwinmm
endif

ifeq ($(OSTYPE),mingw)
  CC ?= gcc
  OS_OPT   ?= -mno-cygwin -DPNG_STATIC -DZLIB_STATIC -march=pentium
  # we need the libraries EXACTLY in this order to link
  STD_LIBS = -lmingw32 -lpng -lz -lstdc++
endif

ifeq ($(OSTYPE),mac)
  CFLAGS   += -DUSE_HW -DUSE_C  -Os -fast
  CXXFLAGS   += -DUSE_HW -DUSE_C
endif

ifneq ($(OPTIMISE),)
  CFLAGS   += -O3 -fno-schedule-insns
  CXXFLAGS += -O3 -fno-schedule-insns
  ifneq ($(OSTYPE),mac)
    CFLAGS  += -minline-all-stringops
  endif
else
  CFLAGS   += -O
  CXXFLAGS += -O
endif

ifdef DEBUG
  ifeq ($(shell expr $(DEBUG) \>= 1), 1)
    CFLAGS   += -g -DDEBUG
    CXXFLAGS += -g -DDEBUG
  endif
  ifeq ($(shell expr $(DEBUG) \>= 2), 1)
    CFLAGS   += -fno-inline
    CXXFLAGS += -fno-inline
  endif
  ifeq ($(shell expr $(DEBUG) \>= 3), 1)
    CFLAGS   += -O0
    CXXFLAGS += -O0
  endif
else
  CFLAGS += -DNDEBUG
  CXXFLAGS += -DNDEBUG
endif

ifneq ($(PROFILE),)
  CFLAGS   += -pg -DPROFILE -fno-inline
  CXXFLAGS += -pg -DPROFILE -fno-inline
  LDFLAGS += -pg
endif

CFLAGS   += -Wall -W -Wcast-qual -Wpointer-arith -Wcast-align -Wstrict-prototypes $(OS_INC) $(OS_OPT) $(FLAGS)
CXXFLAGS += -Wall -W -Wcast-qual -Wpointer-arith -Wcast-align $(OS_INC) $(OS_OPT) $(FLAGS)

# SOLO_SOURCES contains files which are unique to makeobj;
# SHARED_SOURCES contains those with the exact same object code in makeobj and simutrans;
# VARIANT_SOURCES contains those which need different .o files for makeobj and simutrans.
# At the moment they're all treated identically, of course.
SOLO_SOURCES += makeobj.cc
SOLO_SOURCES += ../besch/writer/bridge_writer.cc
SOLO_SOURCES += ../besch/writer/building_writer.cc
SOLO_SOURCES += ../besch/writer/citycar_writer.cc
SOLO_SOURCES += ../besch/writer/crossing_writer.cc
SOLO_SOURCES += ../besch/writer/factory_writer.cc
SOLO_SOURCES += ../besch/writer/get_climate.cc
SOLO_SOURCES += ../besch/writer/get_waytype.cc
SOLO_SOURCES += ../besch/writer/good_writer.cc
SOLO_SOURCES += ../besch/writer/groundobj_writer.cc
SOLO_SOURCES += ../besch/writer/ground_writer.cc
SOLO_SOURCES += ../besch/writer/image_writer.cc
SOLO_SOURCES += ../besch/writer/imagelist2d_writer.cc
SOLO_SOURCES += ../besch/writer/imagelist3d_writer.cc
SOLO_SOURCES += ../besch/writer/imagelist_writer.cc
SOLO_SOURCES += ../besch/writer/obj_node.cc
SOLO_SOURCES += ../besch/writer/obj_writer.cc
SOLO_SOURCES += ../besch/writer/pedestrian_writer.cc
SOLO_SOURCES += ../besch/writer/roadsign_writer.cc
SOLO_SOURCES += ../besch/writer/root_writer.cc
SOLO_SOURCES += ../besch/writer/sim_writer.cc
SOLO_SOURCES += ../besch/writer/skin_writer.cc
SOLO_SOURCES += ../besch/writer/sound_writer.cc
SOLO_SOURCES += ../besch/writer/text_writer.cc
SOLO_SOURCES += ../besch/writer/tree_writer.cc
SOLO_SOURCES += ../besch/writer/tunnel_writer.cc
SOLO_SOURCES += ../besch/writer/vehicle_writer.cc
SOLO_SOURCES += ../besch/writer/way_writer.cc
SOLO_SOURCES += ../besch/writer/way_obj_writer.cc
SOLO_SOURCES += ../besch/writer/xref_writer.cc
SOLO_SOURCES += ../utils/dr_rdpng.cc
SHARED_SOURCES += ../besch/bild_besch.cc
SHARED_SOURCES += ../dataobj/freelist.cc
SHARED_SOURCES += ../dataobj/tabfile.cc
SHARED_SOURCES += ../simdebug.cc
SHARED_SOURCES += ../simmem.cc
SHARED_SOURCES += ../utils/simstring.cc
SHARED_SOURCES += ../utils/searchfolder.cc
VARIANT_SOURCES += ../simtools.cc
VARIANT_SOURCES += ../utils/log.cc

SOURCES ?= $(SOLO_SOURCES) $(SHARED_SOURCES) $(VARIANT_SOURCES)

BUILDDIR ?= ../build/$(CFG)
TOOL ?= makeobj
PROG ?= makeobj
PROGDIR ?= $(BUILDDIR)/$(TOOL)

include uncommon.mk
