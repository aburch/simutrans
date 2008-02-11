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

#include "../simcolor.h"
#include "../simtools.h"
#include "../simworld.h"
#include "../simevent.h"
#include "../simgraph.h"
#include "../simplay.h"
#include "../simskin.h"
#include "../simwerkz.h"
#include "../simwin.h"

#include "../dataobj/translator.h"
#include "components/list_button.h"

#include "../besch/bild_besch.h"
#include "../besch/grund_besch.h"
#include "../besch/intro_dates.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include "baum_edit.h"


#define LINE_NAME_COLUMN_WIDTH (int)((BUTTON_WIDTH*2.25)+11)
#define SCL_HEIGHT (170)
#define N_BUTTON_WIDTH  (int)(BUTTON_WIDTH*1.5)



static bool compare_baum_besch(const baum_besch_t* a, const baum_besch_t* b)
{
	int diff = strcmp( translator::translate(a->gib_name()), translator::translate(b->gib_name()) );
	if(diff ==0) {
		diff = strcmp( a->gib_name(), b->gib_name() );
	}
	return diff < 0;
}



baum_edit_frame_t::baum_edit_frame_t(spieler_t* sp_,karte_t* welt) :
	extend_edit_gui_t(sp_,welt),
	baumlist(16)
{
	bt_timeline.setze_text( "Random age" );

	remove_komponente( &bt_obsolete );
	offset_of_comp -= BUTTON_HEIGHT;

	bbs.besch = NULL;
	fill_list( is_show_trans_name );

	resize( koord(0,0) );
}



// fill the current fablist
void baum_edit_frame_t::fill_list( bool translate )
{
	baumlist.clear();
	const vector_tpl<const baum_besch_t *> *s = baum_t::gib_all_besch();
	for (vector_tpl<const baum_besch_t *>::const_iterator i = s->begin(), end = s->end(); i != end; ++i) {
		if(*i) {
			baumlist.append( (*i), 16 );
		}
	}

	std::sort(baumlist.begin(), baumlist.end(), compare_baum_besch);

	// now buil scrolled list
	scl.clear_elements();
	scl.setze_selection(-1);
	for (vector_tpl<const baum_besch_t *>::const_iterator i = baumlist.begin(), end = baumlist.end(); i != end; ++i) {
		// color code for objects: BLACK: normal, YELLOW: consumer only, GREEN: source only
		uint8 color=COL_BLACK;
		// translate or not?
		if(  translate  ) {
			scl.append_element( translator::translate( (*i)->gib_name() ), color );
		}
		else {
			scl.append_element( (*i)->gib_name(), color );
		}
		if(  (*i) == bbs.besch  ) {
			scl.setze_selection(scl.get_count()-1);
		}
	}
	// always update current selection (since the tool may depend on it)
	change_item_info( scl.gib_selection() );
}



void baum_edit_frame_t::change_item_info(sint32 entry)
{
	for(int i=0;  i<4;  i++  ) {
		img[i].set_image( IMG_LEER );
	}
	buf.clear();
	if(entry>=0  &&  entry<(sint32)baumlist.get_count()) {

		bbs.besch = baumlist[entry];

		buf.append(translator::translate(bbs.besch->gib_name()));
		buf.append("\n");

		// climates
		buf.append( translator::translate("allowed climates:\n") );
		uint16 cl = bbs.besch->get_allowed_climate_bits();
		if(cl==0) {
			buf.append( translator::translate("None") );
			buf.append("\n");
		}
		else {
			for(uint16 i=0;  i<=arctic_climate;  i++  ) {
				if(cl &  (1<<i)) {
					buf.append( translator::translate( grund_besch_t::get_climate_name_from_bit( (enum climate)i ) ) );
					buf.append("\n");
				}
			}
		}

		buf.printf( "\n%s %i\n", translator::translate("Seasons"), bbs.besch->gib_seasons() );

		const char *maker=bbs.besch->gib_copyright();
		if(maker!=NULL  && maker[0]!=0) {
			buf.append("\n");
			buf.printf(translator::translate("Constructed by %s"), maker);
			buf.append("\n");
		}

		info_text.recalc_size();
		cont.setze_groesse( info_text.gib_groesse() );

		img[3].set_image( bbs.besch->gib_bild_nr( 0, 3 ) );

		bbs.ignore_climates = bt_climates.pressed;
		bbs.random_age = bt_timeline.pressed;

		// the tools will be always updated, even though the data up there might be still current
		welt->setze_maus_funktion( wkz_pflanze_baum, skinverwaltung_t::undoc_zeiger->gib_bild_nr(0), welt->Z_PLAN, (value_t)&bbs,  SFX_JACKHAMMER, SFX_FAILURE );
	}
	else if(bbs.besch!=NULL){
		bbs.besch = NULL;
		welt->setze_maus_funktion( wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
	}
}
