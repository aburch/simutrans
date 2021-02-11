#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

OBJS := $(patsubst %, $(BUILDDIR)/%.o, $(basename $(SOURCES)))
DEPS := $(OBJS:%.o=%.d)
DIRS := $(sort $(dir $(OBJS)))

# Make build directories
DUMMY := $(shell mkdir -p $(DIRS))

BUILDCONFIG_FILES := common.mk Makefile config.$(CFG)

.PHONY: all clean

.SUFFIXES: .rc

ifeq ($(VERBOSE),)
  Q := @
else
  Q :=
endif

simutrans: $(PROGDIR)/$(PROG)

$(PROGDIR)/$(PROG): $(OBJS)
	@echo "===> LD  $@"
	$(Q)$(HOSTCXX) $(OBJS) $(LDFLAGS) $(LIBS) -o $(PROGDIR)/$(PROG)

-include $(DEPS)

# Silence stale header dependency errors
%.h:
	@true

$(BUILDDIR)/%.o: %.mm $(BUILDCONFIG_FILES)
	@echo "===> Obj-c OSX $<"
	$(Q)$(HOSTCXX) $(CXXFLAGS) $(OBJCFLAGS) -c -MMD -o $@ $<

$(BUILDDIR)/%.o: %.m $(BUILDCONFIG_FILES)
	@echo "===> Obj-c OSX $<"
	$(Q)$(HOSTCXX) $(CXXFLAGS) $(OBJCFLAGS) -c -MMD -o $@ $<

$(BUILDDIR)/%.o: %.c $(BUILDCONFIG_FILES)
	@echo "===> HOSTCC  $<"
	$(Q)$(HOSTCC) $(CCFLAGS) -c -MMD -o $@ $<

$(BUILDDIR)/%.o: %.cc $(BUILDCONFIG_FILES)
	@echo "===> HOSTCXX $<"
	$(Q)$(HOSTCXX) $(CXXFLAGS) -c -MMD -o $@ $<

$(BUILDDIR)/%.o: %.rc $(BUILDCONFIG_FILES)
	@echo "===> RES $<"
#	$(Q)$(WINDRES) --preprocessor "$(HOSTCXX) -E -xc -DRC_INVOKED -MMD -MF $(@:%.o=%.d) -MT $@" -O COFF $< $@
	$(Q)$(WINDRES) --preprocessor "$(HOSTCXX) -E -xc -DRC_INVOKED -MMD -MT $@" -O COFF $< $@
