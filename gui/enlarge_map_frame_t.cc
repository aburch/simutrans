/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string.h>

#include "enlarge_map_frame_t.h"
#include "minimap.h"
#include "welt.h"
#include "components/gui_divider.h"
#include "load_relief_frame.h"
#include "messagebox.h"

#include "../simdebug.h"
#include "../simworld.h"
#include "simwin.h"
#include "../display/simimg.h"

#include "../dataobj/height_map_loader.h"
#include "../dataobj/settings.h"
#include "../dataobj/translator.h"

#include "../simcolor.h"

#include "../display/simgraph.h"

#include "../utils/simrandom.h"


koord enlarge_map_frame_t::koord_from_rotation(settings_t const* const sets, sint16 const x, sint16 const y, sint16 const w, sint16 const h)
{
	koord offset( sets->get_origin_x(), sets->get_origin_y() );
	switch( sets->get_rotation() ) {
		default:
		case 0: return offset+koord(x,y);
		case 1: return offset+koord(y,w-x);
		case 2: return offset+koord(w-x,h-y);
		case 3: return offset+koord(h-y,x);
	}
}

bool enlarge_map_frame_t::update_from_heightfield(const char *filename)
{
	dbg->message("gui_t::update_from_heightfield()", "%s", filename);

	const sint8 min_h = env_t::default_settings.get_minimumheight();
	const sint8 max_h = env_t::default_settings.get_maximumheight();

	height_map_loader_t hml(min_h, max_h, env_t::height_conv_mode);
	sint16 w, h;
	sint8 *h_field=NULL;

	if(hml.get_height_data_from_file(filename, (sint8)sets->get_groundwater(), h_field, w, h, false )) {
		sets->set_size_x(w);
		sets->set_size_y(h);
		uint16 old_x = welt->get_size().x;
		uint16 old_y = welt->get_size().y;
		if(old_x>w||old_y>h) {
			create_win( new news_img("\nThe size of heightfield file must be larger than this map\n") , w_info, magic_none );
			return false;
		}
		uint16 pre_x = min(sets->get_size_x(), map.get_width());
		uint16 pre_y = min(sets->get_size_y(), map.get_height());

		inp_x_size.set_value(sets->get_size_x());
		inp_y_size.set_value(sets->get_size_y());

		const int mx = sets->get_size_x()/pre_x;
		const int my = sets->get_size_y()/pre_y;
		for(  int y=0;  y<pre_y;  y++  ) {
			for(  int x=0;  x<pre_x;  x++  ) {
				if(  pos.x<=old_x  &&  pos.y<=old_y  ){
					if(  x==(old_x/mx)  ||  y==(old_y/my)  ){
						// border
						map.at(x,y) = color_idx_to_rgb(COL_WHITE);
					}
					else {
						const koord pos(x*mx,y*my);
						const sint16 height = welt->lookup_hgt( pos );
						map.at(x,y) = minimap_t::calc_height_color(height, sets->get_groundwater());
					}
				}
				else {
					map.at(x,y) = minimap_t::calc_height_color( h_field[x*mx+y*my*w], sets->get_groundwater()-1 );
				}
			}
		}
		map_preview.set_map_data(&map);
		free(h_field);
		loaded_heightfield = true;
		return true;
	}
	loaded_heightfield = false;
	return false;
}


enlarge_map_frame_t::enlarge_map_frame_t() :
	gui_frame_t( translator::translate("enlarge map") ),
	sets(new settings_t(welt->get_settings())), // Make a copy.
	map(MAP_PREVIEW_SIZE_X-2, MAP_PREVIEW_SIZE_Y-2)
{
	sets->set_size_x(welt->get_size().x);
	sets->set_size_y(welt->get_size().y);

	changed_number_of_towns = false;

	// Component creation
	set_table_layout(1,0);
	// top part: preview, maps size
	add_table(2,1);
	{
		// input fields
		add_table(2,2);
		{
			// map seed number label
			map_number_label.init();
			map_number_label.buf().printf(translator::translate("Map number: %d"), welt->get_settings().get_map_number());
			map_number_label.update();
			add_component(&map_number_label);

			// Map X size edit
			inp_x_size.init( sets->get_size_x(), welt->get_size().x, 32766, sets->get_size_x()>=512 ? 128 : 64, false );
			inp_x_size.add_listener(this);
			add_component( &inp_x_size );

			// Map size label
			size_label.init();
			size_label.buf().printf(translator::translate("Size (%d MB):"), 9999);
			size_label.update();
			add_component( &size_label );

			// Map size Y edit
			inp_y_size.init( sets->get_size_y(), sets->get_size_y(), 32766, sets->get_size_y()>=512 ? 128 : 64, false );
			inp_y_size.add_listener(this);
			add_component( &inp_y_size );
		}
		end_table();

		// Map preview (will be initialized in update_preview
		add_component( &map_preview );
	}
	end_table();

	// specify map parameters
	add_table(2,0);
	{
		// Number of towns
		new_component<gui_label_t>("5WORLD_CHOOSE");
		inp_number_of_towns.add_listener(this);
		inp_number_of_towns.init(abs(sets->get_city_count()), 0, 999);
		add_component( &inp_number_of_towns );

		// Town size
		new_component<gui_label_t>("Median Citizen per town");
		inp_town_size.add_listener(this);
		inp_town_size.set_limits(0,999999);
		inp_town_size.set_increment_mode(50);
		inp_town_size.set_value( sets->get_mean_citizen_count() );
		add_component( &inp_town_size );
	}
	end_table();

	new_component<gui_divider_t>();

	// read hightfield data
	load_map.init(button_t::roundbox | button_t::flexible, "load map");
	load_map.add_listener( this );
	add_component( &load_map );
	// start game
	start_button.init( button_t::roundbox | button_t::flexible, "enlarge map");
	start_button.add_listener( this );
	add_component( &start_button );

	welt_gui_t::update_memory(&size_label, sets);
	update_preview();

	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
}


enlarge_map_frame_t::~enlarge_map_frame_t()
{
	delete sets;
}


/**
 * This method is called if an action is triggered
 */
bool enlarge_map_frame_t::action_triggered( gui_action_creator_t *comp,value_t v)
{
	if(comp==&inp_x_size) {
		sets->set_size_x( v.i );
		inp_x_size.set_increment_mode( v.i>=64 ? (v.i>=512 ? 128 : 64) : 8 );
		update_preview();
	}
	else if(comp==&inp_y_size) {
		sets->set_size_y( v.i );
		inp_y_size.set_increment_mode( v.i>=64 ? (v.i>=512 ? 128 : 64) : 8 );
		update_preview();
	}
	else if(comp==&inp_number_of_towns) {
		sets->set_city_count( v.i );
	}
	else if(comp==&inp_town_size) {
		sets->set_mean_citizen_count( v.i );
	}
	else if(comp==&start_button) {
		destroy_all_win( true );
		dbg->message("enlarge_map_frame_t::action_triggered()","enlarge with loaded heightfield? %s",loaded_heightfield?"true":"false");
		if(loaded_heightfield) {
			welt->load_heightfield(sets, false);
		} else {
			welt->enlarge_map(sets, NULL);
		}
	}
	else if(comp==&load_map) {
		if(!loaded_heightfield){
			inp_x_size.disable();
			inp_y_size.disable();
			sets->heightfield = "";
			load_relief_frame_t* lrf = new load_relief_frame_t(sets);
			create_win(lrf, w_info, magic_load_t );
			win_set_pos(lrf, (display_get_width() - lrf->get_windowsize().w-10), env_t::iconsize.h);
			loaded_heightfield = true;
		} else {
			inp_x_size.enable();
			inp_y_size.enable();
			sets->heightfield = "";
			loaded_heightfield = false;
		}

	}
	else {
		return false;
	}
	return true;
}


void enlarge_map_frame_t::draw(scr_coord pos, scr_size size)
{
	while (welt->get_settings().get_rotation() != sets->get_rotation()) {
		// map was rotated while we are active ... => rotate too!
		sets->rotate90();
		sets->set_size( sets->get_size_y(), sets->get_size_x() );
		if( sets->heightfield.size()==0 || !loaded_heightfield ) {
			loaded_heightfield = false;
			inp_x_size.enable();
			inp_y_size.enable();
			sets->heightfield = "";
		}
	}
		update_preview();
	load_map.pressed = loaded_heightfield;

	gui_frame_t::draw(pos, size);
}


/**
 * Calculate the new Map-Preview. Initialize the new RNG!
 */
void enlarge_map_frame_t::update_preview()
{
	uint16 old_x = welt->get_size().x;
	uint16 old_y = welt->get_size().y;
	dbg->message("enlarge_map_frame_t::update_preview()", "update enlarge map preview, %s", sets->heightfield.c_str());
	if( sets->heightfield.size()==0 || !update_from_heightfield(sets->heightfield.c_str()) ) {
		// reset noise seed
		setsimrand(0xFFFFFFFF, welt->get_settings().get_map_number());

		// "welt" still knows the old size. The new size is saved in "sets".
		uint16 pre_x = min(sets->get_size_x(), map.get_width());
		uint16 pre_y = min(sets->get_size_y(), map.get_height());

		const int mx = sets->get_size_x()/pre_x;
		const int my = sets->get_size_y()/pre_y;

		for(  int j=0;  j<pre_y;  j++  ) {
			for(  int i=0;  i<pre_x;  i++  ) {
				PIXVAL color;
				koord pos(i*mx,j*my);

				if(  pos.x<=old_x  &&  pos.y<=old_y  ){
					if(  i==(old_x/mx)  ||  j==(old_y/my)  ){
						// border
						color = color_idx_to_rgb(COL_WHITE);
					}
					else {
						const sint16 height = welt->lookup_hgt( pos );
						color = minimap_t::calc_height_color(height, sets->get_groundwater());
					}
				}
				else {
					// new part
					const sint16 height = karte_t::perlin_hoehe(sets, pos, koord(old_x,old_y) );
					color = minimap_t::calc_height_color(height, sets->get_groundwater());
				}
				map.at(i,j) = color;
			}
		}
		for(  uint j=0;  j<map.get_height();  j++  ) {
			for(  uint i=(j<pre_y ? pre_x : 0);  i<map.get_width();   i++  ) {
				map.at(i,j) = color_idx_to_rgb(COL_GREY1);
			}
		}
		loaded_heightfield = false;
		sets->heightfield = "";
		map_preview.set_map_data(&map);
	}

	if(!changed_number_of_towns){// Interpolate number of towns.
		sint32 new_area = sets->get_size_x() * sets->get_size_y();
		sint32 old_area = old_x * old_y;
		sint32 const towns = welt->get_settings().get_city_count();
		sets->set_city_count( towns * new_area / old_area - towns );
		inp_number_of_towns.set_value(abs(sets->get_city_count()) );
	}

	// guess the new memory needed
	welt_gui_t::update_memory(&size_label, sets);
}
