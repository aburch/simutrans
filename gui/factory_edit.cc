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

#include "../bauer/fabrikbauer.h"

#include "../besch/grund_besch.h"
#include "../besch/intro_dates.h"

#include "../dataobj/translator.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "factory_edit.h"


#define LINE_NAME_COLUMN_WIDTH (int)((BUTTON_WIDTH*2.25)+11)
#define SCL_HEIGHT (170)
#define N_BUTTON_WIDTH  (int)(BUTTON_WIDTH*1.5)


static bool compare_fabrik_besch(const fabrik_besch_t* a, const fabrik_besch_t* b)
{
	int diff = strcmp( translator::translate(a->gib_name()), translator::translate(b->gib_name()) );
	return diff < 0;
}



factory_edit_frame_t::factory_edit_frame_t(spieler_t* sp_,karte_t* welt) :
	extend_edit_gui_t(sp_,welt),
	fablist(16),
	lb_rotation( rot_str, COL_WHITE, gui_label_t::right ),
	lb_rotation_info( translator::translate("Rotation"), COL_BLACK, gui_label_t::left ),
	lb_production( prod_str, COL_WHITE, gui_label_t::right ),
	lb_production_info( translator::translate("Produktion"), COL_BLACK, gui_label_t::left )
{
	bt_city_chain.init( button_t::square_state, "Only city chains", koord(NAME_COLUMN_WIDTH+11, offset_of_comp-4 ) );
	bt_city_chain.add_listener(this);
	add_komponente(&bt_city_chain);
	offset_of_comp += BUTTON_HEIGHT;

	bt_land_chain.init( button_t::square_state, "Only land chains", koord(NAME_COLUMN_WIDTH+11, offset_of_comp-4 ) );
	bt_land_chain.add_listener(this);
	add_komponente(&bt_land_chain);
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

	lb_production_info.setze_pos( koord( NAME_COLUMN_WIDTH+11, offset_of_comp-4 ) );
	add_komponente(&lb_production_info);

	bt_down_production.init( button_t::repeatarrowleft, NULL, koord(NAME_COLUMN_WIDTH+11+NAME_COLUMN_WIDTH/2-16,	offset_of_comp-4 ) );
	bt_down_production.add_listener(this);
	add_komponente(&bt_down_production);

	bt_up_production.init( button_t::repeatarrowright, NULL, koord(NAME_COLUMN_WIDTH+11+NAME_COLUMN_WIDTH/2+50, offset_of_comp-4 ) );
	bt_up_production.add_listener(this);
	add_komponente(&bt_up_production);

	lb_production.setze_pos( koord( NAME_COLUMN_WIDTH+11+NAME_COLUMN_WIDTH/2+44, offset_of_comp-4 ) );
	add_komponente(&lb_production);
	offset_of_comp += BUTTON_HEIGHT;

	fill_list( is_show_trans_name );

	resize(koord(0,0));
}



// fill the current fablist
void factory_edit_frame_t::fill_list( bool translate )
{
	const bool allow_obsolete = bt_obsolete.pressed;
	const bool use_timeline = bt_timeline.pressed;
	const bool city_chain = bt_city_chain.pressed;
	const bool land_chain = bt_land_chain.pressed;
	const sint32 month_now = bt_timeline.pressed ? welt->get_current_month() : 0;

	fablist.clear();

	// timeline will be obeyed; however, we may show obsolete ones ...
	stringhashtable_iterator_tpl<const fabrik_besch_t *> iter(fabrikbauer_t::gib_fabesch());
	while(iter.next()) {

		const fabrik_besch_t *besch = iter.get_current_value();
		if(besch->gib_gewichtung()>0) {
			// DistributionWeight=0 is obsoluted item, only for backward compatibility

			if(!use_timeline  ||  (!besch->gib_haus()->is_future(month_now)  &&  (!besch->gib_haus()->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this

				if(city_chain) {
					if(besch->gib_platzierung()==fabrik_besch_t::Stadt  &&  besch->gib_produkt(0)==NULL) {
						fablist.append(besch,16);
					}
				}
				if(land_chain) {
					if(besch->gib_platzierung()==fabrik_besch_t::Land  &&  besch->gib_produkt(0)==NULL) {
						fablist.append(besch,16);
					}
				}
				if(!city_chain  &&  !land_chain) {
					fablist.append(besch,16);
				}
			}
		}
	}

	std::sort(fablist.begin(), fablist.end(), compare_fabrik_besch);

	// now buil scrolled list
	scl.clear_elements();
	scl.setze_selection(-1);
	for(  uint i=0;  i<fablist.get_count();  i++  ) {
		// color code for objects: BLACK: normal, YELLOW: consumer only, GREEN: source only
		uint8 color=COL_BLACK;
		if(fablist[i]->gib_produkt(0)==NULL) {
			color = COL_BLUE;
		}
		else if(fablist[i]->gib_lieferant(0)==NULL) {
			color = COL_DARK_GREEN;
		}
		// translate or not?
		if(  translate  ) {
			scl.append_element( translator::translate(fablist[i]->gib_name()), color );
		}
		else {
			scl.append_element( fablist[i]->gib_name(), color );
		}
		if(fablist[i]==bfs.besch) {
			scl.setze_selection(scl.get_count()-1);
		}
	}
	// always update current selection (since the tool may depend on it)
	change_item_info( scl.gib_selection() );
}



bool factory_edit_frame_t::action_triggered(gui_komponente_t *komp,value_t e)
{
	// only one chain can be shown
	if(  komp==&bt_city_chain  ) {
		bt_city_chain.pressed ^= 1;
		if(bt_city_chain.pressed) {
			bt_land_chain.pressed = 0;
		}
		fill_list( is_show_trans_name );
	}
	else if(  komp==&bt_land_chain  ) {
		bt_land_chain.pressed ^= 1;
		if(bt_land_chain.pressed) {
			bt_city_chain.pressed = 0;
		}
		fill_list( is_show_trans_name );
	}
	else if(bfs.besch) {
		if(  komp==&bt_up_production  ) {
			bfs.production += (bfs.production>=200) ? 10 : 1;
		}
		else if(  komp==&bt_down_production  &&  bfs.production>0) {
			bfs.production -= (bfs.production>200) ? 10 : 1;
		}
		else if(  komp==&bt_left_rotate  &&  bfs.rotation!=255) {
			if(bfs.rotation==0) {
				bfs.rotation = 255;
			}
			else {
				bfs.rotation --;
			}
		}
		else if(  komp==&bt_right_rotate  &&  bfs.rotation!=bfs.besch->gib_haus()->gib_all_layouts()-1) {
			bfs.rotation ++;
		}
		// update info ...
		change_item_info( scl.gib_selection() );
	}
	return extend_edit_gui_t::action_triggered(komp,e);
}



void factory_edit_frame_t::change_item_info(sint32 entry)
{
	if(entry>=0  &&  entry<(sint32)fablist.get_count()) {

		const fabrik_besch_t *fab_besch = fablist[entry];
		if(fab_besch!=bfs.besch) {

			bfs.production = (fab_besch->gib_produktivitaet()+simrand(fab_besch->gib_bereich()) )<<(welt->ticks_bits_per_tag-18);

			// show produced goods
			buf.clear();
			if(fab_besch->gib_produkte()>0) {
				buf.append( translator::translate("Produktion") );
				buf.append("\n");
				for(  int i=0;  i<fab_besch->gib_produkte();  i++  ) {
					buf.append( translator::translate(fab_besch->gib_produkt(i)->gib_ware()->gib_name()) );
					buf.append( " (" );
					buf.append( translator::translate(fab_besch->gib_produkt(i)->gib_ware()->gib_catg_name()) );
					buf.append( ")\n" );
				}
				buf.append("\n");
			}

			// show consumed goods
			if(fab_besch->gib_lieferanten()>0) {
				buf.append( translator::translate("Verbrauch") );
				buf.append("\n");
				for(  int i=0;  i<fab_besch->gib_lieferanten();  i++  ) {
					buf.append( translator::translate(fab_besch->gib_lieferant(i)->gib_ware()->gib_name()) );
					buf.append( " (" );
					buf.append( translator::translate(fab_besch->gib_lieferant(i)->gib_ware()->gib_catg_name()) );
					buf.append( ")\n" );
				}
				buf.append("\n");
			}

			// now the house stuff
			const haus_besch_t *besch = fab_besch->gib_haus();

			// climates
			buf.append( translator::translate("allowed climates:\n") );
			uint16 cl = besch->get_allowed_climate_bits();
			if(cl==0) {
				buf.append( translator::translate("none") );
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
			buf.append("\n");

			buf.append(translator::translate("Passagierrate"));
			buf.append(": ");
			buf.append(fablist[entry]->gib_pax_level());
			buf.append("\n");

			buf.append(translator::translate("Postrate"));
			buf.append(": ");
			buf.append(besch->gib_post_level());
			buf.append("\n");

			buf.append(translator::translate("\nBauzeit von"));
			buf.append(besch->get_intro_year_month()/12);
			if(besch->get_retire_year_month()!=DEFAULT_RETIRE_DATE*12) {
				buf.append(translator::translate("\nBauzeit bis"));
				buf.append(besch->get_retire_year_month()/12);
			}

			const char *maker=besch->gib_copyright();
			if(maker!=NULL  && maker[0]!=0) {
				buf.append("\n");
				buf.printf(translator::translate("Constructed by %s"), maker);
			}
			buf.append("\n");
			info_text.recalc_size();
			cont.setze_groesse( info_text.gib_groesse() );

			// orientation (255=random)
			if(besch->gib_all_layouts()>1) {
				bfs.rotation = 255; // no definition yet
			}
			else {
				bfs.rotation = 0;
			}

			// now for the tool
			bfs.besch = fablist[entry];
			bfs.ignore_climates = bt_climates.pressed;
		}

		// change lable numbers
		sprintf( prod_str, "%i", bfs.production );
		if(bfs.rotation == 255) {
			tstrncpy( rot_str, translator::translate("random"), 16 );
		}
		else {
			sprintf( rot_str, "%i", bfs.rotation );
		}

		// now the images (maximum is 2x2 size)
		// since these may be affected by rotation, we do this every time ...
		for(int i=0;  i<4;  i++  ) {
			img[i].set_image( IMG_LEER );
		}

		const haus_besch_t *besch = fab_besch->gib_haus();
		uint8 rot = (bfs.rotation==255) ? 0 : bfs.rotation;
		if(besch->gib_b(rot)==1) {
			if(besch->gib_h(rot)==1) {
				img[3].set_image( besch->gib_tile(rot,0,0)->gib_hintergrund(0,0,0) );
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
		if(bt_land_chain.pressed) {
			welt->setze_maus_funktion( wkz_build_industries_land, skinverwaltung_t::undoc_zeiger->gib_bild_nr(0), welt->Z_PLAN, (value_t)&bfs,  SFX_JACKHAMMER, SFX_FAILURE );
		}
		else if(bt_city_chain.pressed) {
			welt->setze_maus_funktion( wkz_build_industries_city, skinverwaltung_t::undoc_zeiger->gib_bild_nr(0), welt->Z_PLAN, (value_t)&bfs,  SFX_JACKHAMMER, SFX_FAILURE );
		}
		else {
			welt->setze_maus_funktion( wkz_build_fab, skinverwaltung_t::undoc_zeiger->gib_bild_nr(0), welt->Z_PLAN, (value_t)&bfs,  SFX_JACKHAMMER, SFX_FAILURE );
		}
	}
	else if(bfs.besch!=NULL) {
		for(int i=0;  i<4;  i++  ) {
			img[i].set_image( IMG_LEER );
		}
		buf.clear();
		prod_str[0] = 0;
		tstrncpy( rot_str, translator::translate("random"), 16 );

		bfs.besch = NULL;
		welt->setze_maus_funktion( wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
	}
}
