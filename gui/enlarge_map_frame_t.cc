/*
 * Dialogue to increase map size.
 *
 * Gerd Wachsmuth
 *
 * October 2008
 */

#include <string.h>

#include "enlarge_map_frame_t.h"
#include "karte.h"
#include "messagebox.h"

#include "../simdebug.h"
#include "../simworld.h"
#include "../gui/simwin.h"
#include "../display/simimg.h"
#include "../simtools.h"
#include "../simintr.h"

#include "../dataobj/settings.h"
#include "../dataobj/translator.h"

// just for their structure size ...
#include "../boden/wege/schiene.h"
#include "../obj/baum.h"
#include "../simcity.h"
#include "../vehicle/simvehikel.h"
#include "../player/simplay.h"

#include "../simcolor.h"

#include "../display/simgraph.h"

#include "../utils/simstring.h"

#define L_DIALOG_WIDTH (260)

#define LEFT_ARROW (110)
#define RIGHT_ARROW (160)

#define RIGHT_COLUMN (L_DIALOG_WIDTH-D_MARGIN_RIGHT-preview_size)


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


enlarge_map_frame_t::enlarge_map_frame_t() :
	gui_frame_t( translator::translate("enlarge map") ),
	sets(new settings_t(welt->get_settings())), // Make a copy.
	memory(memory_str)
{
	sets->set_groesse_x(welt->get_size().x);
	sets->set_groesse_y(welt->get_size().y);

	changed_number_of_towns = false;
	scr_coord cursor = scr_coord(D_MARGIN_LEFT, D_MARGIN_TOP);

	memory.set_pos( scr_coord(cursor) );
	add_komponente( &memory );

	inp_x_size.set_pos(scr_coord(LEFT_ARROW, cursor.y) );
	inp_x_size.set_size(scr_size(RIGHT_ARROW-LEFT_ARROW+10, D_EDIT_HEIGHT));
	inp_x_size.add_listener(this);
	inp_x_size.set_value( sets->get_groesse_x() );
	inp_x_size.set_limits( welt->get_size().x, 32766 );
	inp_x_size.set_increment_mode( sets->get_groesse_x()>=512 ? 128 : 64 );
	inp_x_size.wrap_mode( false );
	add_komponente( &inp_x_size );
	cursor.y += max(D_EDIT_HEIGHT, LINESPACE) + D_V_SPACE;

	inp_y_size.set_pos(scr_coord(LEFT_ARROW, cursor.y) );
	inp_y_size.set_size(scr_size(RIGHT_ARROW-LEFT_ARROW+10, D_EDIT_HEIGHT));
	inp_y_size.add_listener(this);
	inp_y_size.set_limits( welt->get_size().y, 32766 );
	inp_y_size.set_value( sets->get_groesse_y() );
	inp_y_size.set_increment_mode( sets->get_groesse_y()>=512 ? 128 : 64 );
	inp_y_size.wrap_mode( false );
	add_komponente( &inp_y_size );

	// city stuff
	cursor.y = max(2*max(D_EDIT_HEIGHT, LINESPACE)+D_V_SPACE, preview_size+2)+D_MARGIN_TOP+D_V_SPACE;
	inp_number_of_towns.set_pos(scr_coord(RIGHT_COLUMN, cursor.y) );
	inp_number_of_towns.set_size(scr_size(preview_size, D_EDIT_HEIGHT));
	inp_number_of_towns.add_listener(this);
	inp_number_of_towns.set_limits(0,999);
	inp_number_of_towns.set_value(abs(sets->get_anzahl_staedte()) );
	add_komponente( &inp_number_of_towns );

	// Number of towns label
	cities_label.init("5WORLD_CHOOSE",cursor);
	cities_label.set_width( RIGHT_COLUMN );
	cities_label.align_to(&inp_number_of_towns, ALIGN_CENTER_V);
	add_komponente( &cities_label );
	cursor.y += max(D_EDIT_HEIGHT, LINESPACE) + D_V_SPACE;

	inp_town_size.set_pos(scr_coord(RIGHT_COLUMN, cursor.y) );
	inp_town_size.set_size(scr_size(preview_size, D_EDIT_HEIGHT));
	inp_town_size.add_listener(this);
	inp_town_size.set_limits(0,999999);
	inp_town_size.set_increment_mode(50);
	inp_town_size.set_value( sets->get_mittlere_einwohnerzahl() );
	add_komponente( &inp_town_size );

	// Town size label
	median_label.init("Median Citizen per town",cursor);
	median_label.set_width( RIGHT_COLUMN );
	median_label.align_to(&inp_town_size, ALIGN_CENTER_V);
	add_komponente( &median_label );
	cursor.y += max(D_EDIT_HEIGHT, LINESPACE);

	// Divider
	divider_1.init(cursor, L_DIALOG_WIDTH-D_MARGINS_X);
	add_komponente( &divider_1 );
	cursor.y += D_DIVIDER_HEIGHT;

	// start game
	start_button.init( button_t::roundbox, "enlarge map", scr_coord(D_MARGIN_LEFT, cursor.y), scr_size(L_DIALOG_WIDTH-D_MARGINS_X, D_BUTTON_HEIGHT) );
	start_button.add_listener( this );
	add_komponente( &start_button );
	cursor.y += D_BUTTON_HEIGHT;

	set_windowsize( scr_size(L_DIALOG_WIDTH, cursor.y+D_TITLEBAR_HEIGHT+D_MARGIN_BOTTOM) );

	update_preview();
}


enlarge_map_frame_t::~enlarge_map_frame_t()
{
	delete sets;
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool enlarge_map_frame_t::action_triggered( gui_action_creator_t *komp,value_t v)
{
	if(komp==&inp_x_size) {
		sets->set_groesse_x( v.i );
		inp_x_size.set_increment_mode( v.i>=64 ? (v.i>=512 ? 128 : 64) : 8 );
		inp_y_size.set_limits( welt->get_size().y, min(32766,16777216/sets->get_groesse_x()) );
		update_preview();
	}
	else if(komp==&inp_y_size) {
		sets->set_groesse_y( v.i );
		inp_y_size.set_increment_mode( v.i>=64 ? (v.i>=512 ? 128 : 64) : 8 );
		inp_x_size.set_limits( welt->get_size().x, min(32766,16777216/sets->get_groesse_y()) );
		update_preview();
	}
	else if(komp==&inp_number_of_towns) {
		sets->set_anzahl_staedte( v.i );
	}
	else if(komp==&inp_town_size) {
		sets->set_mittlere_einwohnerzahl( v.i );
	}
	else if(komp==&start_button) {
		// since soon those are invalid
		intr_refresh_display( true );
		welt->enlarge_map(sets, NULL);
		destroy_all_win( true );
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
		sets->set_groesse( sets->get_groesse_y(), sets->get_groesse_x() );
		update_preview();
	}

	gui_frame_t::draw(pos, size);

	int x = pos.x+RIGHT_COLUMN;
	int y = pos.y+D_TITLEBAR_HEIGHT+D_MARGIN_TOP;

	display_ddd_box_clip(x-2, y, preview_size+2, preview_size+2, MN_GREY0, MN_GREY4);
	display_array_wh(x-1, y+1, preview_size, preview_size, karte);
}


/**
 * Calculate the new Map-Preview. Initialize the new RNG!
 * @author Hj. Malthaner
 */
void enlarge_map_frame_t::update_preview()
{
	// reset noise seed
	setsimrand(0xFFFFFFFF, welt->get_settings().get_karte_nummer());

	// "welt" still knows the old size. The new size is saved in "sets".
	sint16 old_x = welt->get_size().x;
	sint16 old_y = welt->get_size().y;
	sint16 pre_x = min(sets->get_groesse_x(), preview_size);
	sint16 pre_y = min(sets->get_groesse_y(), preview_size);

	const int mx = sets->get_groesse_x()/pre_x;
	const int my = sets->get_groesse_y()/pre_y;


	for(  int j=0;  j<pre_y;  j++  ) {
		for(  int i=0;  i<pre_x;  i++  ) {
			COLOR_VAL color;
			koord pos(i*mx,j*my);

			if(  pos.x<=old_x  &&  pos.y<=old_y  ){
				if(  i==(old_x/mx)  ||  j==(old_y/my)  ){
					// border
					color = COL_WHITE;
				}
				else {
					const sint16 height = welt->lookup_hgt( pos );
					color = reliefkarte_t::calc_hoehe_farbe(height, sets->get_grundwasser());
				}
			}
			else {
				// new part
				const sint16 height = karte_t::perlin_hoehe(sets, pos, koord(old_x,old_y) );
				color = reliefkarte_t::calc_hoehe_farbe(height, sets->get_grundwasser());
			}
			karte[j*preview_size+i] = color;
		}
	}
	for(  int j=0;  j<preview_size;  j++  ) {
		for(  int i=(j<pre_y ? pre_x : 0);  i<preview_size;   i++  ) {
			karte[j*preview_size+i] = COL_GREY1;
		}
	}
	sets->heightfield = "";

	if(!changed_number_of_towns){// Interpolate number of towns.
		sint32 new_area = sets->get_groesse_x() * sets->get_groesse_y();
		sint32 old_area = old_x * old_y;
		sint32 const towns = welt->get_settings().get_anzahl_staedte();
		sets->set_anzahl_staedte( towns * new_area / old_area - towns );
		inp_number_of_towns.set_value(abs(sets->get_anzahl_staedte()) );
	}

	// guess the new memory needed
	const uint sx = sets->get_groesse_x();
	const uint sy = sets->get_groesse_y();
	const long memory = (
		sizeof(karte_t) +
		sizeof(spieler_t) * 8 +
		sizeof(convoi_t) * 1000 +
		(sizeof(schiene_t) + sizeof(vehikel_t)) * 10 * (sx + sy) +
		sizeof(stadt_t) * sets->get_anzahl_staedte() +
		(
			sizeof(grund_t) +
			sizeof(planquadrat_t) +
			sizeof(baum_t) * 2 +
			sizeof(void*) * 4
		) * sx * sy
	) / (1024 * 1024);
	sprintf(memory_str, translator::translate("Size (%d MB):"), memory);

}
