#CXX = g++
CXXFLAGS = -O -Wall -g -c

SOURCES= button.cc\
 infowin.cc colors.cc karte.cc welt.cc\
 werkzeug_parameter_waehler.cc\
 goods_frame_t.cc goods_stats_t.cc\
 kennfarbe.cc halt_info.cc halt_detail.cc\
 halt_list_frame.cc halt_list_item.cc\
 help_frame.cc depot_frame.cc label_frame.cc\
 citylist_frame_t.cc citylist_stats_t.cc\
 message_frame_t.cc message_stats_t.cc message_option_t.cc message_info_t.cc\
 stadt_info.cc fabrik_info.cc fahrplan_gui.cc\
 messagebox.cc sprachen.cc optionen.cc spieler.cc\
 schedule_list.cc schedule_gui.cc scrolled_list.cc scrollbar.cc\
 tab_panel.cc image_list.cc gui_container.cc gui_frame.cc gui_scrollpane.cc\
 gui_label.cc gui_convoiinfo.cc world_view_t.cc\
 map_frame.cc map_legend.cc \
 sound_frame.cc savegame_frame.cc \
 savegame_frame.cc load_relief_frame.cc \
 loadsave_frame.cc money_frame.cc convoi_frame.cc convoi_info_t.cc \
 convoi_filter_frame.cc halt_list_frame.cc halt_list_item.cc halt_list_filter_frame.cc \
 line_management_gui.cc \
 factorylist_frame_t.cc factorylist_stats_t.cc  \
 curiositylist_frame_t.cc curiositylist_stats_t.cc


OBJECTS=button.o\
 infowin.o colors.o karte.o welt.o\
 werkzeug_parameter_waehler.o\
 goods_frame_t.o goods_stats_t.o\
 kennfarbe.o halt_info.o  halt_detail.o\
 halt_list_frame.o halt_list_item.o\
 help_frame.o depot_frame.o label_frame.o\
 citylist_frame_t.o citylist_stats_t.o\
 message_frame_t.o message_stats_t.o message_option_t.o message_info_t.o\
 stadt_info.o fabrik_info.o fahrplan_gui.o\
 messagebox.o sprachen.o optionen.o spieler.o \
 schedule_list.o schedule_gui.o scrolled_list.o scrollbar.o\
 tab_panel.o image_list.o gui_container.o gui_frame.o gui_scrollpane.o\
 gui_label.o gui_convoiinfo.o world_view_t.o\
 map_frame.o map_legend.o \
 sound_frame.o \
 savegame_frame.o load_relief_frame.o \
 loadsave_frame.o money_frame.o convoi_frame.o convoi_info_t.o \
 convoi_filter_frame.o halt_list_frame.o halt_list_item.o halt_list_filter_frame.o \
 line_management_gui.o \
 factorylist_frame_t.o factorylist_stats_t.o \
 curiositylist_frame_t.o curiositylist_stats_t.o


all:   component_sub $(OBJECTS)

component_sub:
	$(MAKE) -e -C components


depend:
	makedepend $(SOURCES)


dep: depdep
	cd components; make dep; cd ..

depdep: $(SOURCES)
	gcc -M $^ > .depend


ifeq (.depend,$(wildcard .depend))
    # include .depend only if it exists
include .depend
endif

# DO NOT DELETE
