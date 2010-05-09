/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef settings_passenger_stats_h
#define settings_passenger_stats_h

#include <math.h>

#include "../tpl/vector_tpl.h"
#include "../tpl/array_tpl.h"

#include "../ifc/gui_komponente.h"
#include "gui_container.h"
#include "components/gui_numberinput.h"
#include "components/gui_component_table.h"
#include "components/gui_label.h"
#include "components/gui_textarea.h"
#include "components/list_button.h"

class einstellungen_t;

/* With the following macros, elements could be added to the property lists.
 * ATTENTION: In the init and read preocedures, the order of the item MUST be identical!
 */

// call this befor any init is done ...
#define INIT_INIT \
	width = 16;\
	sint16 ypos = 4;\
	remove_all();\
	free_all();\
	seperator = 0;\

#define INIT_NUM(t,a,b,c,d,e) \
{\
	width = max(width, proportional_string_width(t)+66);\
	gui_numberinput_t *ni = new gui_numberinput_t();\
	ni->init( (a), (b), (c), (d), (e) );\
	ni->set_pos( koord( 2, ypos ) );\
	ni->set_groesse( koord( 37+7*max(1,(sint16)(log10((double)(c)+1.0)+0.5)), BUTTON_HEIGHT ) );\
	numinp.append( ni );\
	add_komponente( ni );\
	gui_label_t *lb = new gui_label_t();\
	lb->set_text_pointer(t);\
	lb->set_pos( koord( ni->get_groesse().x+6, ypos ) );\
	label.append( lb );\
	add_komponente( lb );\
	ypos += BUTTON_HEIGHT;\
}\

#define INIT_COST(t,a,b,c,d,e) \
{\
	width = max(width, proportional_string_width(t)+66);\
	gui_numberinput_t *ni = new gui_numberinput_t();\
	ni->init( (a)/(sint64)100, (b), (c), (d), (e) );\
	ni->set_pos( koord( 2, ypos ) );\
	ni->set_groesse( koord( 37+7*max(1,(sint16)(log10((double)(c)+1.0)+0.5)), BUTTON_HEIGHT ) );\
	numinp.append( ni );\
	add_komponente( ni );\
	gui_label_t *lb = new gui_label_t();\
	lb->set_text_pointer(t);\
	lb->set_pos( koord( ni->get_groesse().x+6, ypos ) );\
	label.append( lb );\
	add_komponente( lb );\
	ypos += BUTTON_HEIGHT;\
}\

#define INIT_LB(t) \
{\
	width = max(width, proportional_string_width(t)+4);\
	gui_label_t *lb = new gui_label_t();\
	lb->set_text_pointer(t);\
	lb->set_pos( koord( 4, ypos ) );\
	label.append( lb );\
	add_komponente( lb );\
	ypos += BUTTON_HEIGHT;\
}\

#define INIT_BOOL(t,a) \
{\
	width = max(width, proportional_string_width(t)+20);\
	button_t *bt = new button_t();\
	bt->init( button_t::square_automatic, (t), koord( 2, ypos ) );\
	bt->pressed = (a);\
	button.append( bt );\
	add_komponente( bt );\
	ypos += BUTTON_HEIGHT;\
}\

#define SEPERATOR \
	ypos += 7;\
	seperator += 1;\


// call this before and EXIT_...
#define EXIT_INIT \
	uint32 read_numinp = 0;\
	uint32 read_button = 0;\

#define EXIT_NUM(t) (t)( numinp.at(read_numinp++)->get_value() )
#define EXIT_NUM2(t,expr) (t)( numinp.at(read_numinp++)->get_value() expr)
#define EXIT_COST(t) (t)( (sint64)(numinp.at(read_numinp++)->get_value())*100 )
#define EXIT_NUM_ARRAY(t, i) (t)((i), numinp.at(read_numinp++)->get_value() )
#define EXIT_NUM_VALUE(t) (t) = numinp.at(read_numinp++)->get_value()
#define EXIT_COST_VALUE(t) (t) = (sint64)(numinp.at(read_numinp++)->get_value())*100
#define EXIT_BOOL(t) (t)( button.at(read_button++)->pressed )
#define EXIT_BOOL_VALUE(t) (t) = button.at(read_button++)->pressed


/**
 * Settings for property lists ...
 * @author Hj. Malthaner
 */
class settings_stats_t : public gui_container_t
{
protected:
	sint16 width, seperator;
	// since the copy constructor will no copy the right action listener => pointer
	slist_tpl<gui_label_t *> label;
	slist_tpl<gui_numberinput_t *> numinp;
	slist_tpl<button_t *> button;
	slist_tpl<gui_component_table_t *> table;
	list_tpl<gui_komponente_t> others;

	gui_label_t& new_label(koord pos, const char *text);
	gui_textarea_t& new_textarea(koord pos, const char *text);
	gui_numberinput_t& new_numinp(koord pos, sint32 value, sint32 min, sint32 max, sint32 mode = gui_numberinput_t::AUTOLINEAR, bool wrap = false);
	button_t& new_button(koord pos, const char *text, bool pressed);
	gui_component_table_t& new_table(koord pos, coordinate_t columns, coordinate_t rows);
	void set_cell_component(gui_component_table_t &tbl, gui_komponente_t &c, coordinate_t x, coordinate_t y);

	void free_all();

	void init( einstellungen_t *sets );
	void read( einstellungen_t *sets );

	koord get_groesse() const {
		return koord(width,(numinp.get_count()+button.get_count()+label.get_count())*BUTTON_HEIGHT+seperator*7+6);
	}
public:
	settings_stats_t() { width = 16; }
	~settings_stats_t() { free_all(); }
};



// the only task left are the respective init/reading routines
class settings_general_stats_t : public settings_stats_t
{
public:
	void init( einstellungen_t *sets );
	void read( einstellungen_t *sets );
};

class settings_routing_stats_t : public settings_stats_t
{
public:
	void init( einstellungen_t *sets );
	void read( einstellungen_t *sets );
};

class settings_economy_stats_t : public settings_stats_t
{
public:
	void init( einstellungen_t *sets );
	void read( einstellungen_t *sets );
};

class settings_experimental_general_stats_t : public settings_stats_t
{
public:
	void init( einstellungen_t *sets );
	void read( einstellungen_t *sets );
};

class settings_experimental_revenue_stats_t : public settings_stats_t
{
public:
	void init( einstellungen_t *sets );
	void read( einstellungen_t *sets );
};

class settings_costs_stats_t : public settings_stats_t
{
public:
	void init( einstellungen_t *sets );
	void read( einstellungen_t *sets );
};



#endif
