#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

CFG ?= default
-include ../../config.$(CFG)

OSTYPES       = beos cygwin freebsd haiku linux mingw mac

ifeq ($(findstring $(OSTYPE), $(OSTYPES)),)
  $(error Unkown OSTYPE "$(OSTYPE)", must be one of "$(OSTYPES)")
endif

# Absolutely essential
CXXFLAGS += -DNETTOOL

ifeq ($(OSTYPE),cygwin)
  OS_INC   ?= -I/usr/include/mingw
  OS_OPT   ?= -mwin32
  STD_LIBS += -lwinmm -lcurses
endif

ifeq ($(OSTYPE),mingw)
  CC ?= gcc
  OS_OPT   ?= -march=pentium
  # we need the libraries EXACTLY in this order to link
  STD_LIBS = -lmingw32 -lstdc++ -lws2_32
endif

ifeq ($(OSTYPE),mac)
  CXXFLAGS   += -DUSE_HW -DUSE_C
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

# SOLO_SOURCES contains files which are unique to nettool;
# SHARED_SOURCES contains those with the exact same object code in nettool and simutrans;
# VARIANT_SOURCES contains those which need different .o files for nettool and simutrans.
# At the moment they're all treated identically, of course.
SOLO_SOURCES += nettool.cc
SHARED_SOURCES += ../simutrans/dataobj/freelist.cc
SHARED_SOURCES += ../simutrans/network/memory_rw.cc
SHARED_SOURCES += ../simutrans/network/network_address.cc
SHARED_SOURCES += ../simutrans/network/network_cmd.cc
SHARED_SOURCES += ../simutrans/network/network_packet.cc
SHARED_SOURCES += ../simutrans/network/network_socket_list.cc
SHARED_SOURCES += ../simutrans/simdebug.cc
SHARED_SOURCES += ../simutrans/simmem.cc
SHARED_SOURCES += ../simutrans/utils/simstring.cc
SHARED_SOURCES += ../simutrans/utils/fetchopt.cc
SHARED_SOURCES += ../simutrans/utils/sha1.cc
SHARED_SOURCES += ../simutrans/utils/sha1_hash.cc
VARIANT_SOURCES += ../simutrans/utils/log.cc
VARIANT_SOURCES += ../simutrans/network/network.cc
VARIANT_SOURCES += ../simutrans/network/network_file_transfer.cc

SOURCES ?= $(SOLO_SOURCES) $(SHARED_SOURCES) $(VARIANT_SOURCES)

BUILDDIR ?= build/$(CFG)
TOOL  = nettool
PROG ?= nettool

ifeq ($(origin NETTOOL_PROGDIR), undefined)
  NETTOOL_PROGDIR := ../../$(BUILDDIR)/$(TOOL)
endif


BUILDDIR := ../../$(BUILDDIR)

TOOL_PROGDIR = $(NETTOOL_PROGDIR)

include ../../uncommon.mk
