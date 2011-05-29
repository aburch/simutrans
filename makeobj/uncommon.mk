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

all: $(BUILDDIR)/$(TOOL)/$(PROG)

$(BUILDDIR)/$(TOOL)/$(PROG): $(OBJS)
	@echo "===> LD  $@"
	$(Q)$(CXX) $(OBJS) $(LDFLAGS) $(STD_LIBS) $(LIBS) -o $@

clean:
	@echo "===> Cleaning up"
	$(Q)rm -fr $(BUILDDIR)

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
