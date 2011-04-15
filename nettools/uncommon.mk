DEPS    = $(filter %.d, $(SOURCES:%.cc=%-nettool.d) $(SOURCES:%.c=%-nettool.d) $(SOURCES:%.m=%-nettool.d) $(SOURCES:%.mm=%-nettool.d))
OBJECTS = $(filter %.o, $(SOURCES:%.cc=%-nettool.o) $(SOURCES:%.c=%-nettool.o) $(SOURCES:%.m=%-nettool.o) $(SOURCES:%.mm=%-nettool.o) $(SOURCES:%.rc=%-nettool.o))

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

%-nettool.d: %.mm
	@echo "===> DEP OSX $<"
	$(Q)$(CXX) $(OBJCFLAGS) -MT `echo $< | sed 's/\.m/\-nettool.o/'` -MM $< | sed 's#^$(@F:%.d=%.o):#$@ $(@:%.d=%.o):#' > $@

%-nettool.d: %.m
	@echo "===> DEP OSX $<"
	$(Q)$(CXX) $(OBJCFLAGS) -MT `echo $< | sed 's/\.mm/\-nettool.o/'` -MM $< | sed 's#^$(@F:%.d=%.o):#$@ $(@:%.d=%.o):#' > $@

%-nettool.d: %.c
	@echo "===> DEP $<"
	$(Q)$(CC) $(CFLAGS) -MT `echo $< | sed 's/\.c/\-nettool.o/'` -MM $< | sed 's#^$(@F:%.d=%.o):#$@ $(@:%.d=%.o):#' > $@

%-nettool.d: %.cc
	@echo "===> DEP $<"
	$(Q)$(CXX) $(CXXFLAGS) -MT `echo $< | sed 's/\.cc/\-nettool.o/'` -MM $< | sed 's#^$(@F:%.d=%.o):#$@ $(@:%.d=%.o):#' > $@

%-nettool.o: %.mm
	@echo "===> Obj-c OSX $<"
	$(Q)$(CXX) $(CXXFLAGS) $(OBJCFLAGS)  -o $@ -c $<

%-nettool.o: %.m
	@echo "===> Obj-c OSX $<"
	$(Q)$(CXX) $(CXXFLAGS) $(OBJCFLAGS)  -o $@ -c $<

%-nettool.o: %.c
	@echo "===> CC  $<"
	$(Q)$(CC) $(CFLAGS) -o $@ -c $<

%-nettool.o: %.cc
	@echo "===> CXX $<"
	$(Q)$(CXX) $(CXXFLAGS) -o $@ -c $<

%-nettool.o: %.rc
	@echo "===> RES $<"
	$(Q)$(WINDRES) -O COFF $< $@
