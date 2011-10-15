# Source files located in ../
OBJS := $(patsubst %, $(BUILDDIR)/%-$(TOOL).o, $(basename $(patsubst ../%, %,$(filter ../%,$(SOURCES)))))
# Source files located in current directory
OBJS += $(patsubst %, $(BUILDDIR)/$(TOOL)/%-$(TOOL).o, $(basename $(filter-out ../%,$(SOURCES))))
DEPS := $(patsubst %.o, %.d, $(OBJS))
DIRS := $(sort $(dir $(OBJS)))

# Make build directories
DUMMY := $(shell mkdir -p $(DIRS))

.PHONY: clean

ifeq ($(VERBOSE),)
  Q = @
else
  Q =
endif

all: $(MAKEOBJ_PROGDIR)/$(PROG)

$(MAKEOBJ_PROGDIR)/$(PROG): $(OBJS)
	@echo "===> LD  $@"
	$(Q)$(CXX) $(OBJS) $(LDFLAGS) $(STD_LIBS) $(LIBS) -o $(MAKEOBJ_PROGDIR)/$(PROG)

clean:
	@echo "===> Cleaning up"
	$(Q)rm -f $(OBJS)
	$(Q)rm -f $(DEPS)
	$(Q)rm -f $(MAKEOBJ_PROGDIR)/$(PROG)

-include $(DEPS)

# Silence stale header dependency errors
%.h:
	@true

# Source files located in ../
$(BUILDDIR)/%-$(TOOL).o: ../%.c
	@echo "===> CC  $<"
	$(Q)$(CC) $(CCFLAGS) -c -MMD -o $@ $<

$(BUILDDIR)/%-$(TOOL).o: ../%.cc
	@echo "===> CXX $<"
	$(Q)$(CXX) $(CXXFLAGS) -c -MMD -o $@ $<

# Source files located in current directory
$(BUILDDIR)/$(TOOL)/%-$(TOOL).o: %.c
	@echo "===> CC  $<"
	$(Q)$(CC) $(CCFLAGS) -c -MMD -o $@ $<

$(BUILDDIR)/$(TOOL)/%-$(TOOL).o: %.cc
	@echo "===> CXX $<"
	$(Q)$(CXX) $(CXXFLAGS) -c -MMD -o $@ $<
