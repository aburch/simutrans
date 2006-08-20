#CXX = g++
CXXFLAGS = -O -Wall -g -c

SOURCES=\
 gui_chart.cc\
 gui_resizer.cc\
 gui_divider.cc\
 gui_speedbar.cc\
 gui_textinput.cc\
 gui_flowtext.cc\
 gui_textarea.cc\
 gui_combobox.cc


OBJECTS=\
 gui_chart.o\
 gui_resizer.o\
 gui_divider.o\
 gui_speedbar.o\
 gui_textinput.o\
 gui_flowtext.o\
 gui_textarea.o\
 gui_combobox.o


all:   $(OBJECTS)


dep: depdep


depdep: $(SOURCES)
	gcc -M $^ > .depend


ifeq (.depend,$(wildcard .depend))
    # include .depend only if it exists
include .depend
endif
