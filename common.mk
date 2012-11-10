OBJS := $(patsubst %, $(BUILDDIR)/%.o, $(basename $(SOURCES)))
DEPS := $(OBJS:%.o=%.d)
DIRS := $(sort $(dir $(OBJS)))

# Make build directories
DUMMY := $(shell mkdir -p $(DIRS))

.PHONY: all clean

.SUFFIXES: .rc

ifeq ($(VERBOSE),)
  Q = @
else
  Q =
endif

all: $(PROGDIR)/$(PROG)

$(PROGDIR)/$(PROG): $(OBJS)
	@echo "===> LD  $@"
	$(Q)$(CXX) $(OBJS) $(LDFLAGS) $(LIBS) -o $(PROGDIR)/$(PROG)

clean:
	@echo "===> Cleaning up"
	$(Q)rm -f $(OBJS)
	$(Q)rm -f $(DEPS)
	$(Q)rm -f $(PROGDIR)/$(PROG)

-include $(DEPS)

# Silence stale header dependency errors
%.h:
	@true

$(BUILDDIR)/%.o: %.mm
	@echo "===> Obj-c OSX $<"
	$(Q)$(CXX) $(CXXFLAGS) $(OBJCFLAGS) -c -MMD -o $@ $<

$(BUILDDIR)/%.o: %.m
	@echo "===> Obj-c OSX $<"
	$(Q)$(CXX) $(CXXFLAGS) $(OBJCFLAGS) -c -MMD -o $@ $<

$(BUILDDIR)/%.o: %.c
	@echo "===> CC  $<"
	$(Q)$(CC) $(CCFLAGS) -c -MMD -o $@ $<

$(BUILDDIR)/%.o: %.cc
	@echo "===> CXX $<"
	$(Q)$(CXX) $(CXXFLAGS) -c -MMD -o $@ $<

$(BUILDDIR)/%.o: %.rc
	@echo "===> RES $<"
	$(Q)$(WINDRES) --preprocessor="$(CXX) -E -xc -DRC_INVOKED -MMD -MT $@" -O COFF $< $@
#	$(Q)$(WINDRES) --preprocessor "$(CXX) -E -xc -DRC_INVOKED -MMD -MF $(@:%.o=%.d) -MT $@" -O COFF $< $@
