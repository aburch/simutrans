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

static const char *wind_dir_text[4] = { "North", "East", "South", "West" };

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

		new_component<gui_label_t>( "Winter snowline" );
		snowline_winter.init( sets->get_winter_snowline(), sets->get_groundwater(), 127, gui_numberinput_t::AUTOLINEAR, false );
		snowline_winter.add_listener( this );
		add_component( &snowline_winter );

		new_component<gui_label_t>( "Wind direction" );
		wind_dir.set_focusable( false );
		for( int i = 0; i < 4; i++ ) {
			wind_dir.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate( wind_dir_text[i] ), SYSCOL_TEXT );
			if ((1 << i) == sets->wind_direction) {
				wind_dir.set_selection(i);
			}
		}
		add_component( &wind_dir );
		wind_dir.add_listener( this );
	}
	end_table();

	climate_generator.add_tab( &height_climate, translator::translate( "height based" ), NULL, translator::translate( "Generate climate from height" ) );
	climate_generator.add_tab( &humidity_climate, translator::translate( "temperature-humidity based" ), NULL, translator::translate( "Generate climate from humidity" ) );
	add_component( &climate_generator );

	height_climate.set_table_layout(3,0);
	{
		// other climate borders ...
		for( int i = desert_climate - 1; i < MAX_CLIMATES - 1; i++ ) {
			climate_borders_ui[i][0].init( sets->get_climate_borders( i + 1, 0 ), sets->get_groundwater(), 127, gui_numberinput_t::AUTOLINEAR, false );
			climate_borders_ui[i][0].add_listener( this );
			height_climate.add_component( &(climate_borders_ui[i][0]) );
			height_climate.new_component<gui_label_t>( ground_desc_t::get_climate_name_from_bit( (climate)(i + 1) ) );
			climate_borders_ui[i][1].init( sets->get_climate_borders( i + 1, 1 ), sets->get_climate_borders( i + 1, 0 ), 127, gui_numberinput_t::AUTOLINEAR, false );
			climate_borders_ui[i][1].add_listener( this );
			height_climate.add_component( &(climate_borders_ui[i][1]) );
		}
		height_climate.new_component_span<gui_label_t>( "climate area percentage", 2 );
		patch_size.init( sets->get_patch_size_percentage(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
		patch_size.add_listener( this );
		height_climate.add_component( &patch_size );
	}

	humidity_climate.set_table_layout(3,0);
	{
		humidity_climate.new_component_span<gui_label_t>( "moisture land", 2 );
		moistering.init( 1, 0, 10, gui_numberinput_t::AUTOLINEAR, false );
		moistering.add_listener( this );
		humidity_climate.add_component( &moistering );

		humidity_climate.new_component_span<gui_label_t>( "moisture water", 2 );
		moistering_water.init( 1, 0, 10, gui_numberinput_t::AUTOLINEAR, false );
		moistering_water.add_listener( this );
		humidity_climate.add_component( &moistering_water );

		humidity_climate.new_component<gui_label_t>( "humidities" );
		humidities[0].init( sets->get_desert_humidity(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
		humidities[0].add_listener( this );
		humidity_climate.add_component( &humidities[0] );
		humidities[1].init( sets->get_tropic_humidity(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
		humidities[1].add_listener( this );
		humidity_climate.add_component( &humidities[1] );

		humidity_climate.new_component_span<gui_label_t>( "temperature borders", 3 );

		// other climate borders ...
		for(  int i=0;  i<5;  i++  ) {
			temperatures[i].init( sets->get_climate_temperature_borders(i), -30, 30, gui_numberinput_t::AUTOLINEAR, false );
			temperatures[i].add_listener( this );
			humidity_climate.add_component( &temperatures[i] );
		}
		humidity_climate.new_component<gui_empty_t>();

		// summer snowline (actually setting arctic broders)
		humidity_climate.new_component_span<gui_label_t>( "Summer snowline", 2 );
		snowline_summer.init( sets->get_climate_borders(arctic_climate,1), sets->get_groundwater(), 127, gui_numberinput_t::AUTOLINEAR, false );
		snowline_summer.add_listener( this );
		humidity_climate.add_component( &snowline_summer );
	}

	add_table(2,0);
	{
		new_component<gui_label_t>("Trees");
		tree.set_focusable( false );
		tree.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate( "none" ), SYSCOL_TEXT );
		tree.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate( "random" ), SYSCOL_TEXT );
		tree.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate( "rainfall" ), SYSCOL_TEXT );
		tree.set_selection( sets->get_tree_distribution() );
		add_component( &tree );
		tree.add_listener( this );

		new_component<gui_label_t>("Lake height");
		lake.init( sets->lake_height, sets->get_groundwater(), 127, gui_numberinput_t::AUTOLINEAR, false );
		lake.add_listener( this );
		add_component( &lake );

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

	climate_generator.set_active_tab_index( sets->get_climate_generator() );
	climate_generator.add_listener( this );

	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
}


/**
 * This method is called if an action is triggered
 */
bool climate_gui_t::action_triggered( gui_action_creator_t *comp, value_t v)
{
	welt_gui_t *welt_gui = dynamic_cast<welt_gui_t *>(win_get_magic( magic_welt_gui_t ));
	if(comp==&tree) {
		const settings_t::tree_distribution_t value = (settings_t::tree_distribution_t)clamp(v.i, (long)settings_t::TREE_DIST_NONE, (long)settings_t::TREE_DIST_COUNT-1);
		sets->set_tree_distribution(value);
	}
	if(comp==&wind_dir) {
		sets->wind_direction = (ribi_t::ribi)(1 << v.i);
	}
	else if(comp==&lake) {
		sets->lake_height = (sint8)v.i;
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
	else if(comp==&climate_generator) {
		sets->set_climate_generator( (settings_t::climate_generate_t)v.i );
		if( sets->get_climate_generator()==settings_t::HEIGHT_BASED ) {

			// disable rainfall tree generation
			if( sets->get_tree_distribution()==settings_t::TREE_DIST_RAINFALL ) {
				sets->set_tree_distribution(settings_t::TREE_DIST_RANDOM);
			}
			tree.clear_elements();
			tree.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate( "none" ), SYSCOL_TEXT );
			tree.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate( "random" ), SYSCOL_TEXT );
			tree.set_selection( sets->get_tree_distribution() );

			// enable climate height selection
			for(  int i=desert_climate-1;  i<arctic_climate-1;  i++  ) {
				climate_borders_ui[i][0].enable();
				climate_borders_ui[i][1].enable();
			}

		}
		else {
			// enable rainfall tree generation
			tree.clear_elements();
			tree.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate( "none" ), SYSCOL_TEXT );
			tree.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate( "random" ), SYSCOL_TEXT );
			tree.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate( "rainfall" ), SYSCOL_TEXT );
			tree.set_selection( sets->get_tree_distribution() );

			// disable climate height selection (except arctic which is used for snowline)
			for(  int i=desert_climate-1;  i<arctic_climate-1;  i++  ) {
				climate_borders_ui[i][0].disable();
				climate_borders_ui[i][1].disable();
			}
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
	else if(comp==&moistering) {
		sets->moisture = (sint8)v.i;
	}
	else if(comp==&moistering_water) {
		sets->moisture_water = (sint8)v.i;
	}
	else if(comp==&humidities[0]) {
		sets->desert_humidity = (sint8)v.i;
	}
	else if(comp==&humidities[1]) {
		sets->tropic_humidity = (sint8)v.i;
	}
	else if(comp==&snowline_winter) {
		sets->winter_snowline = (sint16)v.i;
	}
	else if(comp==&snowline_summer) {
		// set artic borders
		sets->patch_size_percentage = (sint8)v.i;
	}

	// Update temperature borders
	for(  int i=0;  i<5;  i++  ) {
		sets->climate_temperature_borders[i] = (sint8)temperatures[i].get_value();
	}

	// Update height borders
	for(  int i=desert_climate-1;  i<=arctic_climate-1;  i++  ) {
		climate_borders_ui[i][0].set_limits( sets->get_groundwater(), 127 );
		climate_borders_ui[i][0].set_value( climate_borders_ui[i][0].get_value() );
		climate_borders_ui[i][1].set_limits( climate_borders_ui[i][0].get_value(), 127 );
		climate_borders_ui[i][1].set_value( climate_borders_ui[i][1].get_value() );
		sets->climate_borders[i+1][0] = (sint8)climate_borders_ui[i][0].get_value();
		sets->climate_borders[i+1][1] = (sint8)climate_borders_ui[i][1].get_value();
	}

	return true;
}


void climate_gui_t::update_river_number( sint16 new_river_number )
{
	river_n.set_value( new_river_number );
}
