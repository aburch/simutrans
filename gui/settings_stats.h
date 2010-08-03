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
#include "../utils/cbuffer_t.h"

#include "components/gui_komponente.h"
#include "gui_container.h"
#include "components/gui_numberinput.h"
#include "components/gui_label.h"
#include "components/list_button.h"
#include "components/action_listener.h"

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


// call this before and READ_...
#define READ_INIT \
	slist_iterator_tpl<gui_numberinput_t *>numiter(numinp);\
	slist_iterator_tpl<button_t *>booliter(button);

#define READ_NUM(t) numiter.next(); (t)( numiter.get_current()->get_value() )
#define READ_COST(t) numiter.next(); (t)( (sint64)(numiter.get_current()->get_value())*100 )
#define READ_NUM_VALUE(t) numiter.next(); (t) = numiter.get_current()->get_value()
#define READ_COST_VALUE(t) numiter.next(); (t) = (sint64)(numiter.get_current()->get_value())*100
#define READ_BOOL(t) booliter.next(); (t)( booliter.get_current()->pressed )
#define READ_BOOL_VALUE(t) booliter.next(); (t) = booliter.get_current()->pressed

/*
	uint32 read_numinp = 0;\
	uint32 read_button = 0;\

#define READ_NUM(t) (t)( numinp.at(read_numinp++)->get_value() )
#define READ_COST(t) (t)( (sint64)(numinp.at(read_numinp++)->get_value())*100 )
#define READ_NUM_VALUE(t) (t) = numinp.at(read_numinp++)->get_value()
#define READ_COST_VALUE(t) (t) = (sint64)(numinp.at(read_numinp++)->get_value())*100
#define READ_BOOL(t) (t)( button.at(read_button++)->pressed )
#define READ_BOOL_VALUE(t) (t) = button.at(read_button++)->pressed
*/


/**
 * Settings for property lists ...
 * @author Hj. Malthaner
 */
class settings_stats_t
{
protected:
	sint16 width, seperator;
	// since the copy constructor will no copy the right action listener => pointer
	slist_tpl<gui_label_t *> label;
	slist_tpl<gui_numberinput_t *> numinp;
	slist_tpl<button_t *> button;

	void free_all();

public:
	settings_stats_t() { width = 16; }
	~settings_stats_t() { free_all(); }

	void init( einstellungen_t *sets );
	void read( einstellungen_t *sets );

	koord get_groesse() const {
		return koord(width,(button.get_count()+label.get_count())*BUTTON_HEIGHT+seperator*7+6);
	}
};



// the only task left are the respective init/reading routines
class settings_general_stats_t : protected settings_stats_t, public gui_container_t
{
public:
	void init( einstellungen_t *sets );
	void read( einstellungen_t *sets );
};

class settings_routing_stats_t : protected settings_stats_t, public gui_container_t
{
public:
	void init( einstellungen_t *sets );
	void read( einstellungen_t *sets );
};

class settings_economy_stats_t : protected settings_stats_t, public gui_container_t
{
public:
	void init( einstellungen_t *sets );
	void read( einstellungen_t *sets );
};

class settings_costs_stats_t : protected settings_stats_t, public gui_container_t
{
public:
	void init( einstellungen_t *sets );
	void read( einstellungen_t *sets );
};

class settings_climates_stats_t : protected settings_stats_t, public gui_container_t, public action_listener_t
{
private:
	cbuffer_t buf;
	einstellungen_t *local_sets;
public:
	settings_climates_stats_t() : buf( 128 ) {}
	void init( einstellungen_t *sets );
	void read( einstellungen_t *sets );
	bool action_triggered(gui_action_creator_t *komp, value_t extra);
};

#endif
