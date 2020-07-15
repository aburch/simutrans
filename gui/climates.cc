/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "climates.h"
#include "minimap.h"
#include "welt.h"

#include "../descriptor/ground_desc.h"

#include "../simdebug.h"
#include "simwin.h"

#include "../dataobj/settings.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"

#include "../display/simgraph.h"

/**
 * set the climate borders
 */
climate_gui_t::climate_gui_t(settings_t* const sets_par) :
	gui_frame_t( translator::translate("Climate Control") )
{
	sets = sets_par;
	set_table_layout(1,0);

	add_table( 2, 0 );
	{
		// Water level
		new_component<gui_label_t>("Water level");
		water_level.init( sets->get_groundwater(), -20*(ground_desc_t::double_grounds?2:1), 20, gui_numberinput_t::AUTOLINEAR, false );
		water_level.add_listener( this );
		add_component( &water_level );

		// Height and roughness
		int mountain_height_start = (int)sets->get_max_mountain_height();
		int mountain_roughness_start = (int)(sets->get_map_roughness()*20.0 + 0.5)-8;

		// Mountain height
		new_component<gui_label_t>("Mountain height");
		mountain_height.init( mountain_height_start, 0, min(1000,100*(11-mountain_roughness_start)), 10, false );
		mountain_height.add_listener( this );
		add_component( &mountain_height );

		// Mountain roughness
		new_component<gui_label_t>("Map roughness");
		mountain_roughness.init( mountain_roughness_start, 0, min(10, 11-((mountain_height_start+99)/100)), gui_numberinput_t::AUTOLINEAR, false );
		mountain_roughness.add_listener( this );
		add_component( &mountain_roughness );

		// summer snowline always starting above highest climate
		new_component<gui_label_t>("Summer snowline");
		summer_snowline.init(SYSCOL_TEXT_HIGHLIGHT, gui_label_t::centered);
		summer_snowline.buf().printf("%d", sets->get_climate_borders(arctic_climate,1) );
		summer_snowline.update();
		add_component( &summer_snowline );

		// Winter snowline
		new_component<gui_label_t>("Winter snowline");
		snowline_winter.init( sets->get_winter_snowline(), sets->get_groundwater(), 127, gui_numberinput_t::AUTOLINEAR, false );
		snowline_winter.add_listener( this );
		add_component( &snowline_winter );
	}
	end_table();

	add_table(3,0);
	{
		// other climate borders ...
		for(  int i=desert_climate-1;  i<MAX_CLIMATES-1;  i++  ) {
			climate_borders_ui[i][0].init( sets->get_climate_borders(i+1,0), sets->get_groundwater(), 127, gui_numberinput_t::AUTOLINEAR, false );
			climate_borders_ui[i][0].add_listener( this );
			add_component( &(climate_borders_ui[i][0]) );
			new_component<gui_label_t>( ground_desc_t::get_climate_name_from_bit((climate)(i+1)) );
			climate_borders_ui[i][1].init( sets->get_climate_borders(i+1,1), sets->get_climate_borders(i+1,0), 127, gui_numberinput_t::AUTOLINEAR, false );
			climate_borders_ui[i][1].add_listener( this );
			add_component( &(climate_borders_ui[i][1]) );
		}
	}
	end_table();

	no_tree.init( button_t::square_state, "no tree" );
	no_tree.pressed = sets->get_no_trees();
	no_tree.add_listener( this );
	add_component( &no_tree );

	lake.init( button_t::square_state, "lake" );
	lake.pressed = sets->get_lake();
	lake.add_listener( this );
	add_component( &lake );

	add_table(2,0);
	{
		new_component<gui_label_t>("Number of rivers");
		river_n.init( sets->get_river_number(), 0, 1024, gui_numberinput_t::POWER2, false );
		river_n.add_listener(this);
		add_component( &river_n );

		new_component<gui_label_t>("minimum length of rivers");
		river_min.init( sets->get_min_river_length(), 0, max(16,sets->get_max_river_length())-16, gui_numberinput_t::AUTOLINEAR, false );
		river_min.add_listener(this);
		add_component( &river_min );

		new_component<gui_label_t>("maximum length of rivers");
		river_max.init( sets->get_max_river_length(), sets->get_min_river_length()+16, 1024, gui_numberinput_t::AUTOLINEAR, false );
		river_max.add_listener(this);
		add_component( &river_max );
	}
	end_table();

	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
}


/**
 * This method is called if an action is triggered
 */
bool climate_gui_t::action_triggered( gui_action_creator_t *comp, value_t v)
{
	welt_gui_t *welt_gui = dynamic_cast<welt_gui_t *>(win_get_magic( magic_welt_gui_t ));
	if(comp==&no_tree) {
		no_tree.pressed ^= 1;
		sets->set_no_trees(no_tree.pressed);
	}
	else if(comp==&lake) {
		lake.pressed ^= 1;
		sets->set_lake(lake.pressed);
	}
	else if(comp==&water_level) {
		sets->groundwater = (sint16)v.i;

		if(  welt_gui  ) {
			welt_gui->update_preview();
		}
	}
	else if(comp==&mountain_height) {
		sets->max_mountain_height = v.i;
		mountain_roughness.set_limits(0,min(10,11-((v.i+99)/100)));
		if(  welt_gui  ) {
			welt_gui->update_preview();
		}
	}
	else if(comp==&mountain_roughness) {
		sets->map_roughness = (double)(v.i+8)/20.0;
		mountain_height.set_limits(0,min(1000,100*(11-v.i)));
		if(  welt_gui  ) {
			welt_gui->update_preview();
		}
	}
	else if(comp==&river_n) {
		sets->river_number = (sint16)v.i;
	}
	else if(comp==&river_min) {
		sets->min_river_length = (sint16)v.i;
		river_max.set_limits(v.i+16,1024);
	}
	else if(comp==&river_max) {
		sets->max_river_length = (sint16)v.i;
		river_min.set_limits(0,max(16,v.i)-16);
	}
	else if(comp==&snowline_winter) {
		sets->winter_snowline = (sint16)v.i;
	}

	// Update borders
	for(  int i=desert_climate-1;  i<=arctic_climate-1;  i++  ) {
		climate_borders_ui[i][0].set_limits( sets->get_groundwater(), 127 );
		climate_borders_ui[i][0].set_value( climate_borders_ui[i][0].get_value() );
		climate_borders_ui[i][1].set_limits( climate_borders_ui[i][0].get_value(), 127 );
		climate_borders_ui[i][1].set_value( climate_borders_ui[i][1].get_value() );
		sets->climate_borders[i+1][0] = climate_borders_ui[i][0].get_value();
		sets->climate_borders[i+1][1] = climate_borders_ui[i][1].get_value();
	}

	summer_snowline.buf().printf("%d", sets->get_climate_borders(arctic_climate,1) );
	summer_snowline.update();
	return true;
}


void climate_gui_t::update_river_number( sint16 new_river_number )
{
	river_n.set_value( new_river_number );
}
