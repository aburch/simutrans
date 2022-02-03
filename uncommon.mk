#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

# Source files located in ../
OBJS := $(patsubst %, $(BUILDDIR)/%-$(TOOL).o, $(basename $(patsubst ../simutrans/%, %,$(filter ../simutrans/%,$(SOURCES)))))
# Source files located in current directory
OBJS += $(patsubst %, $(BUILDDIR)/$(TOOL)/%-$(TOOL).o, $(basename $(filter-out ../simutrans/%,$(SOURCES))))
DEPS := $(patsubst %.o, %.d, $(OBJS))
DIRS := $(sort $(dir $(OBJS)))

BUILDCONFIG_FILES := ../../uncommon.mk Makefile ../../config.$(CFG)

# Make build directories
DUMMY := $(shell mkdir -p $(DIRS))

.PHONY: all clean clean-prog

ifeq ($(VERBOSE),)
  Q = @
else
  Q =
endif

all: $(TOOL_PROGDIR)/$(PROG)

$(TOOL_PROGDIR)/$(PROG): $(OBJS)
	@echo "===> LD  $@"
	$(Q)$(CXX) $(OBJS) $(LDFLAGS) $(STD_LIBS) $(LIBS) -o $(TOOL_PROGDIR)/$(PROG)

clean-prog:
	$(Q)rm -f $(TOOL_PROGDIR)/$(PROG)

clean:
	@echo "===> Cleaning up"
	$(Q)rm -f $(OBJS)
	$(Q)rm -f $(DEPS)
	$(Q)rm -f $(TOOL_PROGDIR)/$(PROG)

-include $(DEPS)

# Silence stale header dependency errors
%.h:
	@true

# Source files located in ../
$(BUILDDIR)/%-$(TOOL).o: ../simutrans/%.cc $(BUILDCONFIG_FILES)
	@echo "===> CC  $<"
	$(Q)$(CC) $(CCFLAGS) -c -MMD -o $@ $<

$(BUILDDIR)/%-$(TOOL).o: ../simutrans/%.cc $(BUILDCONFIG_FILES)
	@echo "===> CXX $<"
	$(Q)$(CXX) $(CXXFLAGS) -c -MMD -o $@ $<

# Source files located in current directory
$(BUILDDIR)/$(TOOL)/%-$(TOOL).o: %.c $(BUILDCONFIG_FILES)
	@echo "===> CC  $<"
	$(Q)$(CC) $(CCFLAGS) -c -MMD -o $@ $<

$(BUILDDIR)/$(TOOL)/%-$(TOOL).o: %.cc $(BUILDCONFIG_FILES)
	@echo "===> CXX $<"
	$(Q)$(CXX) $(CXXFLAGS) -c -MMD -o $@ $<
