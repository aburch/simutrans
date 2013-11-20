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

#include "components/gui_container.h"
#include "components/gui_komponente.h"
#include "components/gui_numberinput.h"
#include "components/gui_label.h"

#include "components/action_listener.h"
#include "components/gui_combobox.h"

class settings_t;

/* With the following macros, elements could be added to the property lists.
 * ATTENTION: In the init and read procedures, the order of the items MUST be identical!
 */

// call this before any init is done ...
#define INIT_INIT \
	width = 16;\
	height = 0;\
	sint16 ypos = 0;\
	remove_all();\
	free_all();\
	seperator = 0;\
	new_world = (win_get_magic( magic_welt_gui_t )!=NULL);\


#define INIT_NUM(t,a,b,c,d,e) \
{\
	width = max(width, proportional_string_width(t)+66);\
	gui_numberinput_t *ni = new gui_numberinput_t();\
	ni->init( (sint32)(a), (b), (c), (d), (e) );\
	ni->set_pos( scr_coord( 0, ypos ) );\
	ni->set_size( scr_size( 37+7*max(1,(sint16)(log10((double)(c)+1.0)+0.5)), D_EDIT_HEIGHT ) );\
	numinp.append( ni );\
	add_komponente( ni );\
	gui_label_t *lb = new gui_label_t();\
	lb->set_text_pointer(t);\
	lb->align_to(ni, ALIGN_CENTER_V + ALIGN_EXTERIOR_H + ALIGN_LEFT, scr_coord(D_H_SPACE,0) );\
	label.append( lb );\
	add_komponente( lb );\
	ypos += D_EDIT_HEIGHT;\
}\

#define INIT_NUM_NEW(t,a,b,c,d,e) if(  new_world  ) INIT_NUM( (t), (a), (b), (c), (d) , (e) )

#define INIT_COST(t,a,b,c,d,e) \
{\
	width = max(width, proportional_string_width(t)+66);\
	gui_numberinput_t *ni = new gui_numberinput_t();\
	ni->init( (sint32)( (a)/(sint64)100 ), (b), (c), (d), (e) );\
	ni->set_pos( scr_coord( 0, ypos ) );\
	ni->set_size( scr_size( 37+7*max(1,(sint16)(log10((double)(c)+1.0)+0.5)), D_EDIT_HEIGHT ) );\
	numinp.append( ni );\
	add_komponente( ni );\
	gui_label_t *lb = new gui_label_t();\
	lb->set_text_pointer(t);\
	lb->align_to(ni, ALIGN_CENTER_V + ALIGN_EXTERIOR_H + ALIGN_LEFT, scr_coord(D_H_SPACE,0) );\
	label.append( lb );\
	add_komponente( lb );\
	ypos += D_EDIT_HEIGHT;\
}\

#define INIT_COST_NEW(t,a,b,c,d,e) if(  new_world  ) INIT_COST( (t), (a), (b), (c), (d) , (e) )

#define INIT_LB(t) \
{\
	width = max(width, proportional_string_width(t)+4);\
	gui_label_t *lb = new gui_label_t();\
	lb->set_text_pointer(t);\
	lb->set_pos( scr_coord( 0, ypos ) );\
	label.append( lb );\
	add_komponente( lb );\
	ypos += LINESPACE;\
}\

#define INIT_LB_NEW(t) if(  new_world  ) INIT_LB( (t) )

#define INIT_BOOL(t,a) \
{\
	width = max(width, proportional_string_width(t)+20);\
	button_t *bt = new button_t();\
	bt->init( button_t::square_automatic, (t), scr_coord( 0, ypos ) );\
	bt->pressed = (a);\
	button.append( bt );\
	add_komponente( bt );\
	ypos += D_CHECKBOX_HEIGHT;\
}\

#define INIT_BOOL_NEW(t,a) if(  new_world  ) INIT_BOOL( (t), (a) )

#define SEPERATOR \
	ypos += D_V_SPACE;\
	seperator += 1;\


// call this before and READ_...
#define READ_INIT \
	slist_tpl<gui_numberinput_t*>::const_iterator numiter  = numinp.begin(); \
	slist_tpl<button_t*>::const_iterator          booliter = button.begin();

#define READ_NUM(t)            (t)((*numiter++)->get_value())
#define READ_NUM_NEW(t)        if(new_world) { READ_NUM(t); }
#define READ_COST(t)           (t)((sint64)((*numiter++)->get_value()) * 100)
#define READ_NUM_VALUE(t)      (t) = (*numiter++)->get_value()
#define READ_NUM_VALUE_NEW(t)  if(new_world) { READ_NUM_VALUE(t); }
#define READ_COST_VALUE(t)     (t) = (sint64)((*numiter++)->get_value()) * 100
#define READ_COST_VALUE_NEW(t) if(new_world) { READ_COST_VALUE(t); }
#define READ_BOOL(t)           (t)((*booliter++)->pressed)
#define READ_BOOL_NEW(t)       if(new_world) { READ_BOOL(t); }
#define READ_BOOL_VALUE(t)     (t) = (*booliter++)->pressed
#define READ_BOOL_VALUE_NEW(t) if(new_world) { READ_BOOL_VALUE(t); }

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
	sint16 width, height, seperator;
	bool new_world;
	// since the copy constructor will no copy the right action listener => pointer
	slist_tpl<gui_label_t *> label;
	slist_tpl<gui_numberinput_t *> numinp;
	slist_tpl<button_t *> button;

	void free_all();

public:
	settings_stats_t() { width = 16; }
	~settings_stats_t() { free_all(); }

	void init(settings_t const*);
	void read(settings_t const*);

	scr_size get_size() const {
		//return scr_size(width,(button.get_count()*D_BUTTON_HEIGHT+label.get_count())*LINESPACE+seperator*D_V_SPACE+6);
		return scr_size(width,height);
	}
};



// the only task left are the respective init/reading routines
class settings_general_stats_t : protected settings_stats_t, public gui_container_t, public action_listener_t
{
	gui_combobox_t savegame;
public:
	// needed for savegame combobox
	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
	void init(settings_t const*);
	void read(settings_t*);
};

class settings_display_stats_t : protected settings_stats_t, public gui_container_t
{
public:
	void init(settings_t const*);
	void read(settings_t*);
};

class settings_routing_stats_t : protected settings_stats_t, public gui_container_t
{
public:
	void init(settings_t const*);
	void read(settings_t*);
};

class settings_economy_stats_t : protected settings_stats_t, public gui_container_t
{
public:
	void init(settings_t const*);
	void read(settings_t*);
};

class settings_costs_stats_t : protected settings_stats_t, public gui_container_t
{
public:
	void init(settings_t const*);
	void read(settings_t*);
};

class settings_climates_stats_t : protected settings_stats_t, public gui_container_t, public action_listener_t
{
private:
	cbuffer_t buf;
	settings_t* local_sets;
public:
	void init(settings_t*);
	void read(settings_t*);
	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
