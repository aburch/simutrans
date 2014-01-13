CFG ?= default
-include ../config.$(CFG)

OSTYPES       = beos cygwin freebsd haiku linux mingw mac

ifeq ($(findstring $(OSTYPE), $(OSTYPES)),)
  $(error Unkown OSTYPE "$(OSTYPE)", must be one of "$(OSTYPES)")
endif

# Absolutely essential
CFLAGS += -DNETTOOL
CXXFLAGS += -DNETTOOL

ifeq ($(OSTYPE),cygwin)
  OS_INC   ?= -I/usr/include/mingw
  OS_OPT   ?= -mwin32
  STD_LIBS += -lwinmm -lcurses
endif

ifeq ($(OSTYPE),mingw)
  CC ?= gcc
  OS_OPT   ?= -mno-cygwin -march=pentium
  # we need the libraries EXACTLY in this order to link
  STD_LIBS = -lmingw32 -lstdc++ -lwsock32
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

# SOLO_SOURCES contains files which are unique to nettool;
# SHARED_SOURCES contains those with the exact same object code in nettool and simutrans;
# VARIANT_SOURCES contains those which need different .o files for nettool and simutrans.
# At the moment they're all treated identically, of course.
SOLO_SOURCES += nettool.cc
SHARED_SOURCES += ../dataobj/freelist.cc
SHARED_SOURCES += ../network/memory_rw.cc
SHARED_SOURCES += ../network/network_address.cc
SHARED_SOURCES += ../network/network_cmd.cc
SHARED_SOURCES += ../network/network_packet.cc
SHARED_SOURCES += ../network/network_socket_list.cc
SHARED_SOURCES += ../network/pwd_hash.cc
SHARED_SOURCES += ../simdebug.cc
SHARED_SOURCES += ../simmem.cc
SHARED_SOURCES += ../utils/simstring.cc
SHARED_SOURCES += ../utils/fetchopt.cc
SHARED_SOURCES += ../utils/sha1.cc
VARIANT_SOURCES += ../utils/log.cc
VARIANT_SOURCES += ../network/network.cc
VARIANT_SOURCES += ../network/network_file_transfer.cc

SOURCES ?= $(SOLO_SOURCES) $(SHARED_SOURCES) $(VARIANT_SOURCES)

BUILDDIR ?= build/$(CFG)
TOOL  = nettool
PROG ?= nettool

ifeq ($(origin NETTOOL_PROGDIR), undefined)
  NETTOOL_PROGDIR := ../$(BUILDDIR)/$(TOOL)
endif


BUILDDIR := ../$(BUILDDIR)

TOOL_PROGDIR = $(NETTOOL_PROGDIR)

include ../uncommon.mk
