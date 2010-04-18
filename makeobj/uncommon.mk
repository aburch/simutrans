DEPS    = $(filter %.d, $(SOURCES:%.cc=%-makeobj.d) $(SOURCES:%.c=%-makeobj.d) $(SOURCES:%.m=%-makeobj.d) $(SOURCES:%.mm=%-makeobj.d))
OBJECTS = $(filter %.o, $(SOURCES:%.cc=%-makeobj.o) $(SOURCES:%.c=%-makeobj.o) $(SOURCES:%.m=%-makeobj.o) $(SOURCES:%.mm=%-makeobj.o) $(SOURCES:%.rc=%-makeobj.o))

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

ifndef NO_DEPS
depend: $(DEPS)

ifeq ($(findstring $(MAKECMDGOALS), clean depend),)
-include $(DEPS)
endif
endif

# Silence stale header dependency errors
%.h:
	@true

%-makeobj.d: %.mm
	@echo "===> DEP OSX $<"
	$(Q)$(CXX) $(OBJCFLAGS) -MT `echo $< | sed 's/\.m/\-makeobj.o/'` -MM $< | sed 's#^$(@F:%.d=%.o):#$@ $(@:%.d=%.o):#' > $@

%-makeobj.d: %.m
	@echo "===> DEP OSX $<"
	$(Q)$(CXX) $(OBJCFLAGS) -MT `echo $< | sed 's/\.mm/\-makeobj.o/'` -MM $< | sed 's#^$(@F:%.d=%.o):#$@ $(@:%.d=%.o):#' > $@

%-makeobj.d: %.c
	@echo "===> DEP $<"
	$(Q)$(CC) $(CFLAGS) -MT `echo $< | sed 's/\.c/\-makeobj.o/'` -MM $< | sed 's#^$(@F:%.d=%.o):#$@ $(@:%.d=%.o):#' > $@

%-makeobj.d: %.cc
	@echo "===> DEP $<"
	$(Q)$(CXX) $(CXXFLAGS) -MT `echo $< | sed 's/\.cc/\-makeobj.o/'` -MM $< | sed 's#^$(@F:%.d=%.o):#$@ $(@:%.d=%.o):#' > $@

%-makeobj.o: %.mm
	@echo "===> Obj-c OSX $<"
	$(Q)$(CXX) $(CXXFLAGS) $(OBJCFLAGS)  -o $@ -c $<

%-makeobj.o: %.m
	@echo "===> Obj-c OSX $<"
	$(Q)$(CXX) $(CXXFLAGS) $(OBJCFLAGS)  -o $@ -c $<

%-makeobj.o: %.c
	@echo "===> CC  $<"
	$(Q)$(CC) $(CFLAGS) -o $@ -c $<

%-makeobj.o: %.cc
	@echo "===> CXX $<"
	$(Q)$(CXX) $(CXXFLAGS) -o $@ -c $<

%-makeobj.o: %.rc
	@echo "===> RES $<"
	$(Q)$(WINDRES) -O COFF $< $@
