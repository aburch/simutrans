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
#include "../simskin.h"
#include "../simwerkz.h"
#include "../simmenu.h"
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


// new tool definition
wkz_plant_tree_t baum_edit_frame_t::baum_tool=wkz_plant_tree_t();
char baum_edit_frame_t::param_str[256];



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

	besch = NULL;
	baum_tool.default_param = NULL;

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
			baumlist.push_back(*i);
		}
	}

	std::sort(baumlist.begin(), baumlist.end(), compare_baum_besch);

	// now buil scrolled list
	scl.clear_elements();
	scl.setze_selection(-1);
	for (vector_tpl<const baum_besch_t *>::const_iterator i = baumlist.begin(), end = baumlist.end(); i != end; ++i) {
		scl.append_element( new gui_scrolled_list_t::const_text_scrollitem_t(
			translate ? translator::translate( (*i)->gib_name() ):(*i)->gib_name(),
			COL_BLACK )
		);
		if(  (*i) == besch  ) {
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

		besch = baumlist[entry];

		buf.append(translator::translate(besch->gib_name()));
		buf.append("\n");

		// climates
		buf.append( translator::translate("allowed climates:\n") );
		uint16 cl = besch->get_allowed_climate_bits();
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

		buf.printf( "\n%s %i\n", translator::translate("Seasons"), besch->gib_seasons() );

		const char *maker=besch->gib_copyright();
		if(maker!=NULL  && maker[0]!=0) {
			buf.append("\n");
			buf.printf(translator::translate("Constructed by %s"), maker);
			buf.append("\n");
		}

		info_text.recalc_size();
		cont.setze_groesse( info_text.gib_groesse() );

		img[3].set_image( besch->gib_bild_nr( 0, 3 ) );

		sprintf( param_str, "%i%i,%s", bt_climates.pressed, bt_timeline.pressed, besch->gib_name() );
		baum_tool.default_param = param_str;
		baum_tool.cursor = werkzeug_t::general_tool[WKZ_PLANT_TREE]->cursor;
		welt->set_werkzeug( &baum_tool );
	}
	else if(welt->get_werkzeug()==&baum_tool) {
		besch = NULL;
		welt->set_werkzeug( werkzeug_t::general_tool[WKZ_ABFRAGE] );
	}
}
