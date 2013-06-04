/*
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Curiosity (attractions) builder dialog
 */

#include <stdio.h>

#include "../simtools.h"
#include "../simworld.h"
#include "../simwerkz.h"

#include "../bauer/hausbauer.h"

#include "../besch/grund_besch.h"
#include "../besch/intro_dates.h"

#include "../dataobj/translator.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"


#include "curiosity_edit.h"


// new tool definition
wkz_build_haus_t curiosity_edit_frame_t::haus_tool=wkz_build_haus_t();
char curiosity_edit_frame_t::param_str[256];



static bool compare_haus_besch(const haus_besch_t* a, const haus_besch_t* b)
{
	int diff = strcmp(a->get_name(), b->get_name());
	return diff < 0;
}



curiosity_edit_frame_t::curiosity_edit_frame_t(spieler_t* sp_, karte_t* welt) :
	extend_edit_gui_t(translator::translate("curiosity builder"), sp_, welt),
	hauslist(16),
	lb_rotation( rot_str, COL_WHITE, gui_label_t::right ),
	lb_rotation_info( translator::translate("Rotation"), COL_BLACK, gui_label_t::left )
{
	rot_str[0] = 0;
	rotation = 255;
	besch = NULL;
	haus_tool.set_default_param(NULL);
	haus_tool.cursor = werkzeug_t::general_tool[WKZ_BUILD_HAUS]->cursor;

	bt_city_attraction.init( button_t::square_state, "City attraction", koord(get_tab_panel_width()+2*MARGIN, offset_of_comp-4 ) );
	bt_city_attraction.add_listener(this);
	bt_city_attraction.pressed = true;
	add_komponente(&bt_city_attraction);
	offset_of_comp += D_BUTTON_HEIGHT;

	bt_land_attraction.init( button_t::square_state, "Land attraction", koord(get_tab_panel_width()+2*MARGIN, offset_of_comp-4 ) );
	bt_land_attraction.add_listener(this);
	bt_land_attraction.pressed = true;
	add_komponente(&bt_land_attraction);
	offset_of_comp += D_BUTTON_HEIGHT;

	bt_monuments.init( button_t::square_state, "Monument", koord(get_tab_panel_width()+2*MARGIN, offset_of_comp-4 ) );
	bt_monuments.add_listener(this);
	add_komponente(&bt_monuments);
	offset_of_comp += D_BUTTON_HEIGHT;

	lb_rotation_info.set_pos( koord( get_tab_panel_width()+2*MARGIN, offset_of_comp-4 ) );
	add_komponente(&lb_rotation_info);

	bt_left_rotate.init( button_t::repeatarrowleft, NULL, koord(get_tab_panel_width()+2*MARGIN+COLUMN_WIDTH/2-16, offset_of_comp-4 ) );
	bt_left_rotate.add_listener(this);
	add_komponente(&bt_left_rotate);

	bt_right_rotate.init( button_t::repeatarrowright, NULL, koord(get_tab_panel_width()+2*MARGIN+COLUMN_WIDTH/2+50, offset_of_comp-4 ) );
	bt_right_rotate.add_listener(this);
	add_komponente(&bt_right_rotate);

	lb_rotation.set_pos( koord( get_tab_panel_width()+2*MARGIN+COLUMN_WIDTH/2+44, offset_of_comp-4 ) );
	add_komponente(&lb_rotation);
	offset_of_comp += D_BUTTON_HEIGHT;

	fill_list( is_show_trans_name );

	resize(koord(0,0));
}



// fill the current hauslist
void curiosity_edit_frame_t::fill_list( bool translate )
{
	const bool allow_obsolete = bt_obsolete.pressed;
	const bool use_timeline = bt_timeline.pressed;
	const sint32 month_now = bt_timeline.pressed ? welt->get_current_month() : 0;

	hauslist.clear();

	if(bt_city_attraction.pressed) {
		FOR(vector_tpl<haus_besch_t const*>, const besch, *hausbauer_t::get_list(haus_besch_t::attraction_city)) {
			if(!use_timeline  ||  (!besch->is_future(month_now)  &&  (!besch->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this
				hauslist.insert_ordered(besch,compare_haus_besch);
			}
		}
	}

	if(bt_land_attraction.pressed) {
		FOR(vector_tpl<haus_besch_t const*>, const besch, *hausbauer_t::get_list(haus_besch_t::attraction_land)) {
			if(!use_timeline  ||  (!besch->is_future(month_now)  &&  (!besch->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this
				hauslist.insert_ordered(besch,compare_haus_besch);
			}
		}
	}

	if(bt_monuments.pressed) {
		FOR(vector_tpl<haus_besch_t const*>, const besch, *hausbauer_t::get_list(haus_besch_t::denkmal)) {
			if(!use_timeline  ||  (!besch->is_future(month_now)  &&  (!besch->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this
				hauslist.insert_ordered(besch,compare_haus_besch);
			}
		}
	}

	// now build scrolled list
	scl.clear_elements();
	scl.set_selection(-1);
	FOR(vector_tpl<haus_besch_t const*>, const i, hauslist) {
		// color code for objects: BLACK: normal, YELLOW: consumer only, GREEN: source only
		COLOR_VAL color;
		switch (i->get_utyp()) {
			case haus_besch_t::attraction_city: color = COL_BLUE;       break;
			case haus_besch_t::attraction_land: color = COL_DARK_GREEN; break;
			default:                            color = COL_BLACK;      break;
		}
		char const* const name = translate ? translator::translate(i->get_name()) : i->get_name();
		scl.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(name, color));
		if (i == besch) {
			scl.set_selection(scl.get_count()-1);
		}
	}
	// always update current selection (since the tool may depend on it)
	change_item_info( scl.get_selection() );
}



bool curiosity_edit_frame_t::action_triggered( gui_action_creator_t *komp,value_t e)
{
	// only one chain can be shown
	if(  komp==&bt_city_attraction  ) {
		bt_city_attraction.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	else if(  komp==&bt_land_attraction  ) {
		bt_land_attraction.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	else if(  komp==&bt_monuments  ) {
		bt_monuments.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	else if(besch) {
		if(  komp==&bt_left_rotate  &&  rotation!=255) {
			if(rotation==0) {
				rotation = 255;
			}
			else {
				rotation --;
			}
		}
		else if(  komp==&bt_right_rotate  &&  rotation!=besch->get_all_layouts()-1) {
			rotation ++;
		}
		// update info ...
		change_item_info( scl.get_selection() );
	}
	return extend_edit_gui_t::action_triggered(komp,e);
}



void curiosity_edit_frame_t::change_item_info(sint32 entry)
{
	if(entry>=0  &&  entry<(sint32)hauslist.get_count()) {

		const haus_besch_t *new_besch = hauslist[entry];

		if(new_besch!=besch) {

			buf.clear();
			besch = new_besch;
			if(besch->get_utyp()==haus_besch_t::attraction_city) {
				buf.printf("%s (%s: %i)",translator::translate( "City attraction" ), translator::translate("Bauzeit"),besch->get_extra());
			}
			else if(besch->get_utyp()==haus_besch_t::attraction_land) {
				buf.append( translator::translate( "Land attraction" ) );
			}
			else if(besch->get_utyp()==haus_besch_t::denkmal) {
				buf.append( translator::translate( "Monument" ) );
			}
			buf.append("\n\n");
			buf.append( translator::translate( besch->get_name() ) );

			buf.printf("\n\n%s: %i\n",translator::translate("Passagierrate"),besch->get_level());
			if(besch->get_utyp()==haus_besch_t::attraction_land) {
				// same with passengers
				buf.printf("%s: %i\n",translator::translate("Postrate"),besch->get_level());
			}
			else {
				buf.printf("%s: %i\n",translator::translate("Postrate"),besch->get_post_level());
			}

			buf.printf("%s%u", translator::translate("\nBauzeit von"), besch->get_intro_year_month() / 12);
			if(besch->get_retire_year_month()!=DEFAULT_RETIRE_DATE*12) {
				buf.printf("%s%u", translator::translate("\nBauzeit bis"), besch->get_retire_year_month() / 12);
			}
			buf.append("\n");

			if (char const* const maker = besch->get_copyright()) {
				buf.append("\n");
				buf.printf(translator::translate("Constructed by %s"), maker);
				buf.append("\n");
			}

			info_text.recalc_size();
			cont.set_groesse( info_text.get_groesse() + koord(0, 20) );

			// orientation (255=random)
			if(besch->get_all_layouts()>1) {
				rotation = 255; // no definition yet
			}
			else {
				rotation = 0;
			}
		}

		// change label numbers
		if(rotation == 255) {
			tstrncpy(rot_str, translator::translate("random"), lengthof(rot_str));
		}
		else {
			sprintf( rot_str, "%i", rotation );
		}

		// now the images (maximum is 2x2 size)
		// since these may be affected by rotation, we do this every time ...
		for(int i=0;  i<4;  i++  ) {
			img[i].set_image( IMG_LEER );
		}

		uint8 rot = (rotation==255) ? 0 : rotation;
		if(besch->get_b(rot)==1) {
			if(besch->get_h(rot)==1) {
				img[3].set_image( besch->get_tile(rot,0,0)->get_hintergrund(0,0,0) );
			}
			else {
				img[2].set_image( besch->get_tile(rot,0,0)->get_hintergrund(0,0,0) );
				img[3].set_image( besch->get_tile(rot,0,1)->get_hintergrund(0,0,0) );
			}
		}
		else {
			if(besch->get_h(rot)==1) {
				img[1].set_image( besch->get_tile(rot,0,0)->get_hintergrund(0,0,0) );
				img[3].set_image( besch->get_tile(rot,1,0)->get_hintergrund(0,0,0) );
			}
			else {
				// maximum 2x2 image
				for(int i=0;  i<4;  i++  ) {
					img[i].set_image( besch->get_tile(rot,i/2,i&1)->get_hintergrund(0,0,0) );
				}
			}
		}

		// the tools will be always updated, even though the data up there might be still current
		sprintf( param_str, "%i%c%s", bt_climates.pressed, rotation==255 ? '#' : '0'+rotation, besch->get_name() );
		haus_tool.set_default_param(param_str);
		welt->set_werkzeug( &haus_tool, sp );
	}
	else if(welt->get_werkzeug(sp->get_player_nr())==&haus_tool) {
		for(int i=0;  i<4;  i++  ) {
			img[i].set_image( IMG_LEER );
		}
		tstrncpy(rot_str, translator::translate("random"), lengthof(rot_str));
		uint8 rot = (rotation==255) ? 0 : rotation;
		if (besch) {
			img[3].set_image( besch->get_tile(rot,0,0)->get_hintergrund(0,0,0) );
		}

		besch = NULL;
		welt->set_werkzeug( werkzeug_t::general_tool[WKZ_ABFRAGE], sp );
	}
}


void curiosity_edit_frame_t::zeichnen(koord pos, koord gr)
{
	// remove constructed monuments from list
	if(besch  &&  besch->get_utyp()==haus_besch_t::denkmal  &&  !hausbauer_t::is_valid_denkmal(besch)  ) {
		change_item_info(0x7FFFFFFF);
		scl.set_selection(-1);
		img[3].set_image( IMG_LEER );
		fill_list( is_show_trans_name );
	}

	extend_edit_gui_t::zeichnen(pos,gr);
}
