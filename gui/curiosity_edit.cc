/*
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * Line management
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <algorithm>
#include <stdio.h>

#include "messagebox.h"

#include "../simcolor.h"
#include "../simtools.h"

#include "../simworld.h"
#include "../simevent.h"
#include "../simgraph.h"
#include "../simplay.h"
#include "../simwerkz.h"

#include "../simwin.h"

#include "../utils/simstring.h"

#include "../dataobj/translator.h"
#include "components/list_button.h"

#include "../bauer/hausbauer.h"

#include "../besch/grund_besch.h"
#include "../besch/intro_dates.h"

#include "../utils/cbuffer_t.h"

#include "curiosity_edit.h"


#define LINE_NAME_COLUMN_WIDTH (int)((BUTTON_WIDTH*2.25)+11)
#define SCL_HEIGHT (170)
#define N_BUTTON_WIDTH  (int)(BUTTON_WIDTH*1.5)



static bool compare_haus_besch(const haus_besch_t* a, const haus_besch_t* b)
{
	int diff = strcmp(a->gib_name(), b->gib_name());
	return diff < 0;
}



curiosity_edit_frame_t::curiosity_edit_frame_t(spieler_t* sp_,karte_t* welt) :
	extend_edit_gui_t(sp_,welt),
	hauslist(16),
	lb_rotation( rot_str, COL_WHITE, gui_label_t::right ),
	lb_rotation_info( translator::translate("Rotation"), COL_BLACK, gui_label_t::left )
{
	bt_city_attraction.init( button_t::square_state, "City attraction", koord(NAME_COLUMN_WIDTH+11, offset_of_comp-4 ) );
	bt_city_attraction.add_listener(this);
	bt_city_attraction.pressed = true;
	add_komponente(&bt_city_attraction);
	offset_of_comp += BUTTON_HEIGHT;

	bt_land_attraction.init( button_t::square_state, "Land attraction", koord(NAME_COLUMN_WIDTH+11, offset_of_comp-4 ) );
	bt_land_attraction.add_listener(this);
	bt_land_attraction.pressed = true;
	add_komponente(&bt_land_attraction);
	offset_of_comp += BUTTON_HEIGHT;

	bt_monuments.init( button_t::square_state, "Monument", koord(NAME_COLUMN_WIDTH+11, offset_of_comp-4 ) );
	bt_monuments.add_listener(this);
	add_komponente(&bt_monuments);
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
void curiosity_edit_frame_t::fill_list( bool translate )
{
	const bool allow_obsolete = bt_obsolete.pressed;
	const bool use_timeline = bt_timeline.pressed;
	const sint32 month_now = bt_timeline.pressed ? welt->get_current_month() : 0;

	hauslist.clear();

	if(bt_city_attraction.pressed) {
		const slist_tpl<const haus_besch_t *> *s = hausbauer_t::get_list( haus_besch_t::attraction_city );
		for (slist_tpl<const haus_besch_t *>::const_iterator i = s->begin(), end = s->end(); i != end; ++i) {

			const haus_besch_t *besch = (*i);
			if(!use_timeline  ||  (!besch->is_future(month_now)  &&  (!besch->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this
				hauslist.append(besch,16);
			}
		}
	}

	if(bt_land_attraction.pressed) {
		const slist_tpl<const haus_besch_t *> *s = hausbauer_t::get_list( haus_besch_t::attraction_land );
		for (slist_tpl<const haus_besch_t *>::const_iterator i = s->begin(), end = s->end(); i != end; ++i) {

			const haus_besch_t *besch = (*i);
			if(!use_timeline  ||  (!besch->is_future(month_now)  &&  (!besch->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this
				hauslist.append(besch,16);
			}
		}
	}

	if(bt_monuments.pressed) {
		const slist_tpl<const haus_besch_t *> *s = hausbauer_t::get_list( haus_besch_t::denkmal );
		for (slist_tpl<const haus_besch_t *>::const_iterator i = s->begin(), end = s->end(); i != end; ++i) {

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
		if(  (*i)->gib_utyp()==haus_besch_t::attraction_city  ) {
			color = COL_BLUE;
		}
		else if(  (*i)->gib_utyp()==haus_besch_t::attraction_land  ) {
			color = COL_DARK_GREEN;
		}
		// translate or not?
		if(  translate  ) {
			scl.append_element( translator::translate( (*i)->gib_name() ), color );
		}
		else {
			scl.append_element( (*i)->gib_name(), color );
		}
		if(  (*i) == bhs.besch  ) {
			scl.setze_selection(scl.get_count()-1);
		}
	}
	// always update current selection (since the tool may depend on it)
	change_item_info( scl.gib_selection() );
}



bool curiosity_edit_frame_t::action_triggered(gui_komponente_t *komp,value_t e)
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
	else if(bhs.besch) {
		if(  komp==&bt_left_rotate  &&  bhs.rotation!=255) {
			if(bhs.rotation==0) {
				bhs.rotation = 255;
			}
			else {
				bhs.rotation --;
			}
		}
		else if(  komp==&bt_right_rotate  &&  bhs.rotation!=bhs.besch->gib_all_layouts()-1) {
			bhs.rotation ++;
		}
		// update info ...
		change_item_info( scl.gib_selection() );
	}
	return extend_edit_gui_t::action_triggered(komp,e);
}



void curiosity_edit_frame_t::change_item_info(sint32 entry)
{
	if(entry>=0  &&  entry<(sint32)hauslist.get_count()) {

		const haus_besch_t *besch = hauslist[entry];
		if(besch!=bhs.besch) {

			buf.clear();
			if(besch->gib_utyp()==haus_besch_t::attraction_city) {
				buf.printf("%s (%s: %i)",translator::translate( "City attraction" ), translator::translate("Bauzeit"),besch->gib_bauzeit());
			}
			else if(besch->gib_utyp()==haus_besch_t::attraction_land) {
				buf.append( translator::translate( "Land attraction" ) );
			}
			else if(besch->gib_utyp()==haus_besch_t::denkmal) {
				buf.append( translator::translate( "Monument" ) );
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
				bhs.rotation = 255; // no definition yet
			}
			else {
				bhs.rotation = 0;
			}

			// now for the tool
			bhs.besch = besch;
			bhs.ignore_climates = bt_climates.pressed;
		}

		// change lable numbers
		if(bhs.rotation == 255) {
			tstrncpy( rot_str, translator::translate("random"), 16 );
		}
		else {
			sprintf( rot_str, "%i", bhs.rotation );
		}

		// now the images (maximum is 2x2 size)
		// since these may be affected by rotation, we do this every time ...
		for(int i=0;  i<4;  i++  ) {
			img[i].set_image( IMG_LEER );
		}

		uint8 rot = (bhs.rotation==255) ? 0 : bhs.rotation;
		if(besch->gib_b(rot)==1) {
			if(besch->gib_h(rot)==1) {
				img[3].set_image( besch->gib_tile(0)->gib_hintergrund(0,0,0) );
			}
			else {
				img[2].set_image( besch->gib_tile(rot,0,0)->gib_hintergrund(0,0,0) );
				img[3].set_image( besch->gib_tile(rot,0,1)->gib_hintergrund(0,0,0) );
			}
		}
		else {
			if(besch->gib_h(rot)==1) {
				img[1].set_image( besch->gib_tile(rot,0,0)->gib_hintergrund(0,0,0) );
				img[3].set_image( besch->gib_tile(rot,1,0)->gib_hintergrund(0,0,0) );
			}
			else {
				// maximum 2x2 image
				for(int i=0;  i<4;  i++  ) {
					img[i].set_image( besch->gib_tile(rot,i/2,i&1)->gib_hintergrund(0,0,0) );
				}
			}
		}

		// the tools will be always updated, even though the data up there might be still current
		bhs.add_to_next_city = besch->gib_utyp()!=haus_besch_t::attraction_land;
		welt->setze_maus_funktion( wkz_add_haus, skinverwaltung_t::undoc_zeiger->gib_bild_nr(0), welt->Z_PLAN, (value_t)&bhs,  SFX_JACKHAMMER, SFX_FAILURE );
	}
	else if(bhs.besch!=NULL) {
		for(int i=0;  i<4;  i++  ) {
			img[i].set_image( IMG_LEER );
		}
		buf.clear();
		tstrncpy( rot_str, translator::translate("random"), 16 );

		bhs.besch = NULL;
		welt->setze_maus_funktion( wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
	}
}


