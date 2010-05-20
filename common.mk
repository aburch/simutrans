DEPS    = $(filter %.d, $(SOURCES:%.cc=%.d) $(SOURCES:%.c=%.d) $(SOURCES:%.m=%.d) $(SOURCES:%.mm=%.d))
OBJECTS = $(filter %.o, $(SOURCES:%.cc=%.o) $(SOURCES:%.c=%.o) $(SOURCES:%.m=%.o) $(SOURCES:%.mm=%.o) $(SOURCES:%.rc=%.o))

.PHONY: clean depend

.SUFFIXES: .rc

ifeq ($(VERBOSE),)
  Q = @
else
  Q =
endif

all: $(PROG)

$(PROG): $(OBJECTS)
	@echo "===> LD  $@"
	$(Q)$(CXX) $(OBJECTS) $(LDFLAGS) $(STD_LIBS) $(LIBS) -o $@

clean:
	@echo "===> Cleaning up"
	$(Q)rm -f $(PROG) $(OBJECTS) $(DEPS)

-include $(DEPS)

# Silence stale header dependency errors
%.h:
	@true

%.o: %.mm
	@echo "===> Obj-c OSX $<"
	$(Q)$(CXX) $(CXXFLAGS) $(OBJCFLAGS) -c -MMD -o $@ $<

%.o: %.m
	@echo "===> Obj-c OSX $<"
	$(Q)$(CXX) $(CXXFLAGS) $(OBJCFLAGS) -c -MMD -o $@ $<

%.o: %.c
	@echo "===> CC  $<"
	$(Q)$(CC) $(CCFLAGS) -c -MMD -o $@ $<

%.o: %.cc
	@echo "===> CXX $<"
	$(Q)$(CXX) $(CXXFLAGS) -c -MMD -o $@ $<

%.o: %.rc
	@echo "===> RES $<"
	$(Q)$(WINDRES) -O COFF $< $@
