/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_SETTINGS_STATS_H
#define GUI_SETTINGS_STATS_H


#include <math.h>

#include "../tpl/vector_tpl.h"
#include "../utils/cbuffer.h"

#include "components/gui_container.h"
#include "components/gui_component.h"
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
	new_world = (win_get_magic( magic_welt_gui_t )!=NULL);\
	numinp.clear(); \
	button.clear(); \
	remove_all(); \
	set_table_layout(1,0); \
	add_table(2,0);

// at the end of init
#define INIT_END \
	end_table();\
	set_size( get_min_size());


#define INIT_NUM(t,a,b,c,d,e) \
{\
	gui_numberinput_t *ni = new_component<gui_numberinput_t>();\
	ni->init( (sint32)(a), (b), (c), (d), (e) );\
	numinp.append( ni );\
	gui_label_t *lb = new_component<gui_label_t>();\
	lb->set_text_pointer(t);\
}\

#define INIT_NUM_NEW(t,a,b,c,d,e) if(  new_world  ) INIT_NUM( (t), (a), (b), (c), (d) , (e) )

#define INIT_COST(t,a,b,c,d,e) \
{\
	gui_numberinput_t *ni = new_component<gui_numberinput_t>();\
	ni->init( (sint32)( (a)/(sint64)100 ), (b), (c), (d), (e) );\
	numinp.append( ni );\
	gui_label_t *lb = new_component<gui_label_t>();\
	lb->set_text_pointer(t);\
}\

#define INIT_COST_NEW(t,a,b,c,d,e) if(  new_world  ) INIT_COST( (t), (a), (b), (c), (d) , (e) )

#define INIT_LB(t) \
{\
	gui_label_t *lb = new_component<gui_label_t>();\
	lb->set_text_pointer(t);\
}\

#define INIT_LB_NEW(t) if(  new_world  ) INIT_LB( (t) )

#define INIT_BOOL(t,a) \
{\
	button_t *bt = new_component_span<button_t>(2);\
	bt->init(button_t::square_automatic, (t));\
	button.append( bt );\
	bt->pressed = (a);\
}\

#define INIT_BOOL_NEW(t,a) if(  new_world  ) INIT_BOOL( (t), (a) )

#define SEPERATOR \
	new_component_span<gui_divider_t>(2);


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



/**
 * Settings for property lists
 */
class settings_stats_t
{
protected:
	bool new_world;
	// since the copy constructor will not copy the right action listener => pointer
	slist_tpl<gui_numberinput_t *> numinp;
	slist_tpl<button_t *> button;

public:
	settings_stats_t() {}

	void init(settings_t const*);
	void read(settings_t const*);
};



// the only task left are the respective init/reading routines
class settings_general_stats_t : protected settings_stats_t, public gui_aligned_container_t, public action_listener_t
{
	gui_combobox_t savegame;
public:
	// needed for savegame combobox
	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
	void init(settings_t const*);
	void read(settings_t*);
};

class settings_display_stats_t : protected settings_stats_t, public gui_aligned_container_t
{
public:
	void init(settings_t const*);
	void read(settings_t*);
};

class settings_routing_stats_t : protected settings_stats_t, public gui_aligned_container_t
{
public:
	void init(settings_t const*);
	void read(settings_t*);
};

class settings_economy_stats_t : protected settings_stats_t, public gui_aligned_container_t
{
public:
	void init(settings_t const*);
	void read(settings_t*);
};

class settings_costs_stats_t : protected settings_stats_t, public gui_aligned_container_t
{
public:
	void init(settings_t const*);
	void read(settings_t*);
};

class settings_climates_stats_t : protected settings_stats_t, public gui_aligned_container_t, public action_listener_t
{
private:
	gui_combobox_t climate_generate;
	gui_combobox_t tree_generate;

	settings_t* local_sets;
public:
	void init(settings_t*);
	void read(settings_t*);
	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
