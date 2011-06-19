DEPS := $(patsubst %, $(BUILDDIR)/%.d, $(basename $(filter-out %.rc, $(SOURCES))))
OBJS := $(patsubst %, $(BUILDDIR)/%.o, $(basename $(SOURCES)))
DIRS := $(sort $(dir $(OBJS)))

# Make build directories
DUMMY := $(shell mkdir -p $(DIRS))

.PHONY: clean depend

.SUFFIXES: .rc

ifeq ($(VERBOSE),)
  Q = @
else
  Q =
endif

all: $(BUILDDIR)/$(PROG)

$(BUILDDIR)/$(PROG): $(OBJS)
	@echo "===> LD  $@"
	$(Q)$(CXX) $(OBJS) $(LDFLAGS) $(LIBS) -o $(PROG)

clean:
	@echo "===> Cleaning up"
	$(Q)rm -fr $(BUILDDIR)

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
	$(Q)$(WINDRES) -O COFF $< $@
