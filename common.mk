DEPS    = $(filter %.d, $(SOURCES:%.cc=%.d) $(SOURCES:%.c=%.d))
OBJECTS = $(filter %.o, $(SOURCES:%.cc=%.o) $(SOURCES:%.c=%.o) $(SOURCES:%.rc=%.o))

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
	$(Q)rm -f $(OBJECTS) $(DEPS)

ifndef NO_DEPS
depend: $(DEPS)

ifeq ($(findstring $(MAKECMDGOALS), clean depend),)
-include $(DEPS)
endif
endif

# Silence stale header dependency errors
%.h:
	@true

%.d: %.c
	@echo "===> DEP $<"
	$(Q)$(CC) $(CFLAGS) -MM $< | sed 's#^$(@F:%.d=%.o):#$@ $(@:%.d=%.o):#' > $@

%.d: %.cc
	@echo "===> DEP $<"
	$(Q)$(CXX) $(CXXFLAGS) -MM $< | sed 's#^$(@F:%.d=%.o):#$@ $(@:%.d=%.o):#' > $@

%.o: %.c
	@echo "===> CC  $<"
	$(Q)$(CC) $(CFLAGS) -o $@ -c $<

%.o: %.cc
	@echo "===> CXX $<"
	$(Q)$(CXX) $(CXXFLAGS) -o $@ -c $<

%.o: %.rc
	@echo "===> RES $<"
	$(Q)$(WINDRES) -O COFF $< $@
