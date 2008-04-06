/*
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * Line management
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <algorithm>
#include <stdio.h>

#include "../simcolor.h"
#include "../simtools.h"
#include "../simworld.h"
#include "../simevent.h"
#include "../simgraph.h"
#include "../simplay.h"
#include "../simskin.h"
#include "../simwerkz.h"
#include "../simwin.h"

#include "components/list_button.h"

#include "../bauer/hausbauer.h"

#include "../besch/grund_besch.h"
#include "../besch/intro_dates.h"

#include "../dataobj/translator.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include "citybuilding_edit.h"


#define LINE_NAME_COLUMN_WIDTH (int)((BUTTON_WIDTH*2.25)+11)
#define SCL_HEIGHT (170)
#define N_BUTTON_WIDTH  (int)(BUTTON_WIDTH*1.5)


// new tool definition
wkz_build_haus_t citybuilding_edit_frame_t::haus_tool=wkz_build_haus_t();
char citybuilding_edit_frame_t::param_str[256];


static bool compare_haus_besch(const haus_besch_t* a, const haus_besch_t* b)
{
	int diff = a->gib_level()-b->gib_level();
	if(  diff==0  ) {
		diff = a->gib_typ()-b->gib_typ();
	}
	if(  diff==0  ) {
		diff = strcmp(a->gib_name(), b->gib_name());
	}
	return diff < 0;
}



citybuilding_edit_frame_t::citybuilding_edit_frame_t(spieler_t* sp_,karte_t* welt) :
	extend_edit_gui_t(sp_,welt),
	hauslist(16),
	lb_rotation( rot_str, COL_WHITE, gui_label_t::right ),
	lb_rotation_info( translator::translate("Rotation"), COL_BLACK, gui_label_t::left )
{
	besch = NULL;
	haus_tool.default_param = NULL;
	haus_tool.cursor = werkzeug_t::general_tool[WKZ_BUILD_HAUS]->cursor;

	bt_res.init( button_t::square_state, "residential house", koord(NAME_COLUMN_WIDTH+11, offset_of_comp-4 ) );
	bt_res.add_listener(this);
	bt_res.pressed = true;
	add_komponente(&bt_res);
	offset_of_comp += BUTTON_HEIGHT;

	bt_com.init( button_t::square_state, "shops and stores", koord(NAME_COLUMN_WIDTH+11, offset_of_comp-4 ) );
	bt_com.add_listener(this);
	bt_com.pressed = true;
	add_komponente(&bt_com);
	offset_of_comp += BUTTON_HEIGHT;

	bt_ind.init( button_t::square_state, "industrial building", koord(NAME_COLUMN_WIDTH+11, offset_of_comp-4 ) );
	bt_ind.add_listener(this);
	add_komponente(&bt_ind);
	bt_com.pressed = true;
	offset_of_comp += BUTTON_HEIGHT;

	lb_rotation_info.setze_pos( koord( NAME_COLUMN_WIDTH+11, offset_of_comp-4 ) );
	add_komponente(&lb_rotation_info);

	bt_left_rotate.init( button_t::repeatarrowleft, NULL, koord(NAME_COLUMN_WIDTH+11+NAME_COLUMN_WIDTH/2-16,	offset_of_comp-4 ) );
	bt_left_rotate.add_listener(this);
	add_komponente(&bt_left_rotate);

	bt_right_rotate.init( button_t::repeatarrowright, NULL, koord(NAME_COLUMN_WIDTH+11+NAME_COLUMN_WIDTH/2+50, offset_of_comp-4 ) );
	bt_right_rotate.add_listener(this);
	add_komponente(&bt_right_rotate);

	lb_rotation.setze_pos( koord( NAME_COLUMN_WIDTH+11+NAME_COLUMN_WIDTH/2+44, offset_of_comp-4 ) );
	add_komponente(&lb_rotation);
	offset_of_comp += BUTTON_HEIGHT;

	fill_list( is_show_trans_name );

	resize(koord(0,0));
}



// fill the current fablist
void citybuilding_edit_frame_t::fill_list( bool translate )
{
	const bool allow_obsolete = bt_obsolete.pressed;
	const bool use_timeline = bt_timeline.pressed;
	const sint32 month_now = bt_timeline.pressed ? welt->get_current_month() : 0;

	hauslist.clear();

	if(bt_res.pressed) {
		const vector_tpl<const haus_besch_t *> *s = hausbauer_t::get_citybuilding_list( gebaeude_t::wohnung );
		for (vector_tpl<const haus_besch_t *>::const_iterator i = s->begin(), end = s->end(); i != end; ++i) {

			const haus_besch_t *besch = (*i);
			if(!use_timeline  ||  (!besch->is_future(month_now)  &&  (!besch->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this
				hauslist.append(besch,16);
			}
		}
	}

	if(bt_com.pressed) {
		const vector_tpl<const haus_besch_t *> *s = hausbauer_t::get_citybuilding_list( gebaeude_t::gewerbe );
		for (vector_tpl<const haus_besch_t *>::const_iterator i = s->begin(), end = s->end(); i != end; ++i) {

			const haus_besch_t *besch = (*i);
			if(!use_timeline  ||  (!besch->is_future(month_now)  &&  (!besch->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this
				hauslist.append(besch,16);
			}
		}
	}

	if(bt_ind.pressed) {
		const vector_tpl<const haus_besch_t *> *s = hausbauer_t::get_citybuilding_list( gebaeude_t::industrie );
		for (vector_tpl<const haus_besch_t *>::const_iterator i = s->begin(), end = s->end(); i != end; ++i) {

			const haus_besch_t *besch = (*i);
			if(!use_timeline  ||  (!besch->is_future(month_now)  &&  (!besch->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this
				hauslist.append(besch,16);
			}
		}
	}

	std::sort(hauslist.begin(), hauslist.end(), compare_haus_besch);

	// now buil scrolled list
	scl.clear_elements();
	scl.setze_selection(-1);
	for (vector_tpl<const haus_besch_t *>::const_iterator i = hauslist.begin(), end = hauslist.end(); i != end; ++i) {
		// color code for objects: BLACK: normal, YELLOW: consumer only, GREEN: source only
		uint8 color=COL_BLACK;
		if(  (*i)->gib_typ()==gebaeude_t::wohnung  ) {
			color = COL_BLUE;
		}
		else if(  (*i)->gib_typ()==gebaeude_t::gewerbe  ) {
			color = COL_DARK_GREEN;
		}
		// translate or not?
		if(  translate  ) {
			scl.append_element( translator::translate( (*i)->gib_name() ), color );
		}
		else {
			scl.append_element( (*i)->gib_name(), color );
		}
		if(  (*i) == besch  ) {
			scl.setze_selection(scl.get_count()-1);
		}
	}
	// always update current selection (since the tool may depend on it)
	change_item_info( scl.gib_selection() );
}



bool citybuilding_edit_frame_t::action_triggered(gui_komponente_t *komp,value_t e)
{
	// only one chain can be shown
	if(  komp==&bt_res  ) {
		bt_res.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	else if(  komp==&bt_com  ) {
		bt_com.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	else if(  komp==&bt_ind  ) {
		bt_ind.pressed ^= 1;
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
		else if(  komp==&bt_right_rotate  &&  rotation!=besch->gib_all_layouts()-1) {
			rotation ++;
		}
		// update info ...
		change_item_info( scl.gib_selection() );
	}
	return extend_edit_gui_t::action_triggered(komp,e);
}



void citybuilding_edit_frame_t::change_item_info(sint32 entry)
{
	if(entry>=0  &&  entry<(sint32)hauslist.get_count()) {

		const haus_besch_t *new_besch = hauslist[entry];
		if(new_besch!=besch) {

			buf.clear();
			besch = new_besch;
			if(besch->gib_typ()==gebaeude_t::wohnung) {
				buf.append( translator::translate( "residential house" ) );
			}
			else if(besch->gib_typ()==gebaeude_t::gewerbe) {
				buf.append( translator::translate( "shops and stores" ) );
			}
			else if(besch->gib_typ()==gebaeude_t::industrie) {
				buf.append( translator::translate( "industrial building" ) );
			}
			buf.append("\n\n");
			buf.append( translator::translate( besch->gib_name() ) );

			buf.printf("\n%s: %i\n",translator::translate("Passagierrate"),besch->gib_level());
			buf.printf("%s: %i\n",translator::translate("Postrate"),besch->gib_post_level());

			buf.append(translator::translate("\nBauzeit von"));
			buf.append(besch->get_intro_year_month()/12);
			if(besch->get_retire_year_month()!=DEFAULT_RETIRE_DATE*12) {
				buf.append(translator::translate("\nBauzeit bis"));
				buf.append(besch->get_retire_year_month()/12);
			}
			buf.append("\n");

			const char *maker=besch->gib_copyright();
			if(maker!=NULL  && maker[0]!=0) {
				buf.append("\n");
				buf.printf(translator::translate("Constructed by %s"), maker);
				buf.append("\n");
			}

			info_text.recalc_size();
			cont.setze_groesse( info_text.gib_groesse() );

			// orientation (255=random)
			if(besch->gib_all_layouts()>1) {
				rotation = 255; // no definition yet
			}
			else {
				rotation = 0;
			}
		}

		// change lable numbers
		if(rotation == 255) {
			tstrncpy( rot_str, translator::translate("random"), 16 );
		}
		else {
			sprintf( rot_str, "%i", rotation );
		}

		// now the images (maximum is 2x2 size)
		// since these may be affected by rotation, we do this every time ...
		for(int i=0;  i<3;  i++  ) {
			img[i].set_image( IMG_LEER );
		}

		uint8 rot = (rotation==255) ? 0 : rotation;
		img[3].set_image( besch->gib_tile(rot,0,0)->gib_hintergrund(0,0,0) );

		// the tools will be always updated, even though the data up there might be still current
		sprintf( param_str, "%i%c%s", bt_climates.pressed, rotation==255 ? '#' : '0'+rotation, besch->gib_name() );
		haus_tool.default_param = param_str;
		welt->set_werkzeug( &haus_tool );
	}
	else if(welt->get_werkzeug()==&haus_tool) {
		besch = NULL;
		welt->set_werkzeug( werkzeug_t::general_tool[WKZ_ABFRAGE] );
	}
}
