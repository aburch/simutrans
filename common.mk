OBJECTS = $(filter %.o, $(SOURCES:%.cc=%.o) $(SOURCES:%.c=%.o) $(SOURCES:%.rc=%.o))

.PHONY: $(SUBDIRS) $(SUBDIRS:%=%_clean) clean $(SUBDIRS:%=%_depend) depend

.SUFFIXES: .rc

all: $(SUBDIRS) $(OBJECTS)

$(SUBDIRS):
	@$(MAKE) -C $@

clean: $(SUBDIRS:%=%_clean)
	@echo "===> Cleaning up"
	@rm -f $(OBJECTS) .depend *~ *.bak

$(SUBDIRS:%=%_clean):
	@$(MAKE) -C $(@:%_clean=%) clean


depend: $(SUBDIRS:%=%_depend)
	@echo "===> DEP"
	@$(CC) -M $(filter %.c, $(SOURCES)) $(filter %.cc, $(SOURCES)) > .depend

$(SUBDIRS:%=%_depend):
	$(MAKE) -C $(@:%_depend=%) depend

-include .depend


%.o: %.rc
	@echo "===> RES $<"
	@windres -O COFF $< $@
