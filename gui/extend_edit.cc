/*
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * Line management
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "messagebox.h"

#include "extend_edit.h"

#include "../simcolor.h"

#include "../simworld.h"
#include "../simevent.h"
#include "../simgraph.h"
#include "../simplay.h"
#include "../simwerkz.h"

#include "../simwin.h"

#include "../utils/simstring.h"

#include "../dataobj/translator.h"
#include "components/list_button.h"

#include "../besch/grund_besch.h"
#include "../bauer/hausbauer.h"
#include "../bauer/fabrikbauer.h"
#include "../utils/cbuffer_t.h"



extend_edit_gui_t::extend_edit_gui_t(spieler_t* sp_,karte_t* welt) :
	gui_frame_t("extend edit tool", sp_),
	sp(sp_),
	scrolly(&cont),
	info_text("\n"),
	buf(2048),
	scl(gui_scrolled_list_t::select)
{
	this->welt = welt;

	is_show_trans_name = true;

	bt_climates.init( button_t::square_state, "ignore climates", koord(NAME_COLUMN_WIDTH+11, 10 ) );
	bt_climates.add_listener(this);
	add_komponente(&bt_climates);

	bt_timeline.init( button_t::square_state, "Use timeline start year", koord(NAME_COLUMN_WIDTH+11, 10+BUTTON_HEIGHT ) );
	bt_timeline.pressed = welt->gib_einstellungen()->gib_use_timeline();
	bt_timeline.add_listener(this);
	add_komponente(&bt_timeline);

	bt_obsolete.init( button_t::square_state, "Show obsolete", koord(NAME_COLUMN_WIDTH+11, 10+2*BUTTON_HEIGHT ) );
	bt_obsolete.add_listener(this);
	add_komponente(&bt_obsolete);

	offset_of_comp = 10+3*BUTTON_HEIGHT+4;

	// init scrolled list
	scl.setze_groesse(koord(NAME_COLUMN_WIDTH, SCL_HEIGHT-14));
	scl.setze_pos(koord(0,1));
	scl.setze_highlight_color(sp->get_player_color1()+1);
	scl.request_groesse(scl.gib_groesse());
	scl.setze_selection(-1);
	scl.add_listener(this);

	// tab panel
	tabs.setze_pos(koord(10,10));
	tabs.setze_groesse(koord(NAME_COLUMN_WIDTH, SCL_HEIGHT));
	tabs.add_tab(&scl, translator::translate("Translation"));//land
	tabs.add_tab(&scl, translator::translate("Object"));//city
	tabs.add_listener(this);
	add_komponente(&tabs);

	// item list
	info_text.setze_text(buf);
	info_text.setze_pos(koord(0,0));
	cont.add_komponente(&info_text);

	scrolly.set_visible(true);
	add_komponente(&scrolly);

	// image placeholder
	for(  sint16 i=3;  i>=0;  i--  ) {
		img[i].set_image(IMG_LEER);
		add_komponente( &img[i] );
	}

	// resize button
	set_min_windowsize(koord((short int)(BUTTON_WIDTH*4.5), 300));
	set_resizemode(diagonal_resize);
	resize(koord(0,0));
}



/**
 * Mausklicks werden hiermit an die GUI-Komponenten
 * gemeldet
 */
void extend_edit_gui_t::infowin_event(const event_t *ev)
{
	if(ev->ev_class == INFOWIN) {
		if(ev->ev_code == WIN_CLOSE) {
			change_item_info(-1);
			welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
		}
	}
	gui_frame_t::infowin_event(ev);
}



bool extend_edit_gui_t::action_triggered(gui_komponente_t *komp,value_t /* */)           // 28-Dec-01    Markus Weber    Added
{
	if (komp == &tabs) {
		// switch list translation or object name
		if(tabs.get_active_tab_index()==0 && is_show_trans_name==false) {
			// show translation list
			is_show_trans_name = true;
			fill_list( is_show_trans_name );
		}
		else if(tabs.get_active_tab_index()==1  &&   is_show_trans_name==true) {
			// show object list
			is_show_trans_name = false;
			fill_list( is_show_trans_name );
		}
	}
	else if (komp == &scl) {
		// select an item of scroll list ?
		change_item_info(scl.gib_selection());
	}
	else if(  komp==&bt_obsolete  ) {
		bt_obsolete.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	else if(  komp==&bt_climates  ) {
		bt_climates.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	else if(  komp==&bt_timeline  ) {
		bt_timeline.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	return true;
}



/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void extend_edit_gui_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);

	// test region
	koord groesse = gib_fenstergroesse()-koord(NAME_COLUMN_WIDTH+16,offset_of_comp+16);
	scrolly.setze_groesse(groesse);
	scrolly.setze_pos( koord( NAME_COLUMN_WIDTH+16, offset_of_comp ) );

	// image placeholders
	sint16 rw = get_tile_raster_width()/(4*get_zoom_factor());
	static const koord img_offsets[4]={ koord(2,-6), koord(0,-5), koord(4,-5), koord(2,-4) };
	for(  sint16 i=0;  i<4;  i++  ) {
		koord pos = koord(4,gib_fenstergroesse().y-4-16) + img_offsets[i]*rw;
		img[i].setze_pos( pos );
	}

}



#if 0
/*
*
*/
void extend_edit_gui_t::get_baum_list()
{
	scl.setze_selection(-1);
	scl.clear_elements();
	keys.clear();
	v_baum.clear();

//	baum_t::get_list(v_baum);

	for(vector_tpl<const baum_besch_t *>::const_iterator i=v_baum.begin(); i!=v_baum.end();++i){

		const char* str = translator::translate((*i)->gib_name());

		keys.append(str);
	}
	draw_list();
}



/*
*	build list of factory
*/
void extend_edit_gui_t::get_fab_list()
{
	scl.setze_selection(-1);
	scl.clear_elements();
	keys.clear();
	v_fab.clear();

	stringhashtable_iterator_tpl<const fabrik_besch_t *> iter(fabrikbauer_t::gib_fabesch());
	while(iter.next()) {

		if(iter.get_current_value()->gib_gewichtung()>0) {
			// DistributionWeight=0 is obsoluted item, only for backward compatibility
			v_fab.push_back(iter.get_current_value());
		}
	}

	for(vector_tpl<const fabrik_besch_t*>::const_iterator i=v_fab.begin(); i!=v_fab.end();++i){

		const char* str = translator::translate((*i)->gib_name());

		keys.append(str);
	}
	draw_list();
}



/*
*	build list of attraction or monument
*/
void extend_edit_gui_t::get_house_list()
{
	scl.setze_selection(-1);
	scl.clear_elements();

	keys.clear();
	v_house.clear();

	if(active_type>0 && active_type<4) {
		hausbauer_t::get_list(active_type,v_house);
	}

	for(vector_tpl<const haus_besch_t*>::const_iterator i=v_house.begin(); i!=v_house.end();++i){

		const char* str =translator::translate((*i)->gib_name());

		char *text = new char[strlen(str)+1];
		char *dest = text;
		const char *src = str;

		while(  *src!=0  ) {
			*dest = *src;

			if(src[0]=='\n') {
				// use only one line
				break;
			}
			src ++;
			dest ++;
		}
		*dest = 0;
		str = text;
		keys.append(str);

		delete [] text;

	}
	draw_list();


}



/*
*	make selected item information
*/
void extend_edit_gui_t::draw_item_info()
{

	const haus_besch_t* besch = NULL;

	buf.clear();
	buf.append("\n\n\n\n\n\n"); // you can avoid image

	if(scl.gib_selection() > -1) {

		if(active_type>0 && active_type<4) {
			// attraction, monument
			besch = v_house[scl.gib_selection()];
		}
		else if(active_type==0) {
			// factory
			besch = v_fab[scl.gib_selection()]->gib_haus();
		}

		if(active_type<4) {
			// all buildings are here

			int x,y,level,climate;
			x = besch->gib_groesse().x;
			y = besch->gib_groesse().y;

			image_id bild = besch->gib_tile(0,0,0)->gib_hintergrund(0,0,0);
			img1.set_image(bild);

			climate = besch->get_allowed_climate_bits();
			buf.append("x: ");
			buf.append(x);
			buf.append("\ny: ");
			buf.append(y);
			buf.append("\n\n");
			buf.append(translator::translate("Passagierrate"));
			buf.append(": ");
			if(active_type==0){
				// factory use pax_level
				level = v_fab[scl.gib_selection()]->gib_pax_level();
				buf.append(level);
			}
			else{
				// Passengers level
				level = besch->gib_level();
				buf.append(level); // each tile
				buf.append(" (");
				buf.append(x*y*level); // total
				buf.append(")");
			}
				buf.append("\n\n");

			if(active_type==2) {
				buf.append(translator::translate("City size"));
				buf.append(": ");
				buf.append(besch->gib_bauzeit());
				buf.append("\n\n");
			}

			if(active_type==0 && v_fab[scl.gib_selection()]->gib_platzierung()== fabrik_besch_t::Wasser) {
				// location=water factory doesn't use climate value
				buf.append("\n\n\n\n\n\n\n\n");
			}
			else{
				// climate
				if(climate & 1) buf.append(translator::translate(grund_besch_t::get_climate_name_from_bit(water_climate)));
				buf.append("\n");
				if(climate & 2) buf.append(translator::translate(grund_besch_t::get_climate_name_from_bit(desert_climate)));
				buf.append("\n");
				if(climate & 4) buf.append(translator::translate(grund_besch_t::get_climate_name_from_bit(tropic_climate)));
				buf.append("\n");
				if(climate & 8) buf.append(translator::translate(grund_besch_t::get_climate_name_from_bit(mediterran_climate)));
				buf.append("\n");
				if(climate & 16) buf.append(translator::translate(grund_besch_t::get_climate_name_from_bit(temperate_climate)));
				buf.append("\n");
				if(climate & 32) buf.append(translator::translate(grund_besch_t::get_climate_name_from_bit(tundra_climate)));
				buf.append("\n");
				if(climate & 64) buf.append(translator::translate(grund_besch_t::get_climate_name_from_bit(rocky_climate)));
				buf.append("\n");
				if(climate & 128) buf.append(translator::translate(grund_besch_t::get_climate_name_from_bit(arctic_climate)));
				buf.append("\n");
			}

			if(active_type==0) {
				// factory

				buf.append("\n");
				buf.append("\n");
				buf.append(translator::translate("Suppliers"));
				buf.append(":\n");
				for(int i=0; i<v_fab[scl.gib_selection()]->gib_lieferanten(); i++ ) {
					// list all consumption
					buf.append("\n  -");
					buf.append(translator::translate(v_fab[scl.gib_selection()]->gib_lieferant(i)->gib_ware()->gib_name()));
					buf.append("\n");

					for(vector_tpl<const fabrik_besch_t*>::const_iterator j=v_fab.begin(); j!=v_fab.end();++j){

						for(int k = 0; k<(*j)->gib_produkte(); k++) {
							const fabrik_produkt_besch_t *produkt = (*j)->gib_produkt(k);
							if(produkt->gib_ware()==v_fab[scl.gib_selection()]->gib_lieferant(i)->gib_ware()  &&  (*j)->gib_gewichtung()>0) {

								buf.append(translator::translate((*j)->gib_name()));
								buf.append("\n");
							}
						}
					}

				}
				buf.append("\n");
				buf.append(translator::translate("Abnehmer"));
				buf.append(":\n");
				for(int i=0; i<v_fab[scl.gib_selection()]->gib_produkte(); i++ ) {
					// list all production
					buf.append("\n  -");
					buf.append(translator::translate(v_fab[scl.gib_selection()]->gib_produkt(i)->gib_ware()->gib_name()));
					buf.append("\n");

					for(vector_tpl<const fabrik_besch_t*>::const_iterator j=v_fab.begin(); j!=v_fab.end();++j){

						for(int k = 0; k<(*j)->gib_lieferanten(); k++) {
							const fabrik_lieferant_besch_t *produkt = (*j)->gib_lieferant(k);
							if(produkt->gib_ware()==v_fab[scl.gib_selection()]->gib_produkt(i)->gib_ware()  &&  (*j)->gib_gewichtung()>0) {

								buf.append(translator::translate((*j)->gib_name()));
								buf.append("\n");
							}
						}
					}
				}
			}
		}
		else if(active_type==4) {
			// baum

			image_id bild = v_baum[scl.gib_selection()]->gib_bild( 0, 3 )->gib_nummer();
			img1.set_image(bild);
				if(v_baum[scl.gib_selection()]->is_allowed_climate(water_climate)) buf.append(translator::translate(grund_besch_t::get_climate_name_from_bit(water_climate)));
				buf.append("\n");
				if(v_baum[scl.gib_selection()]->is_allowed_climate(desert_climate)) buf.append(translator::translate(grund_besch_t::get_climate_name_from_bit(desert_climate)));
				buf.append("\n");
				if(v_baum[scl.gib_selection()]->is_allowed_climate(tropic_climate)) buf.append(translator::translate(grund_besch_t::get_climate_name_from_bit(tropic_climate)));
				buf.append("\n");
				if(v_baum[scl.gib_selection()]->is_allowed_climate(mediterran_climate)) buf.append(translator::translate(grund_besch_t::get_climate_name_from_bit(mediterran_climate)));
				buf.append("\n");
				if(v_baum[scl.gib_selection()]->is_allowed_climate(temperate_climate)) buf.append(translator::translate(grund_besch_t::get_climate_name_from_bit(temperate_climate)));
				buf.append("\n");
				if(v_baum[scl.gib_selection()]->is_allowed_climate(tundra_climate)) buf.append(translator::translate(grund_besch_t::get_climate_name_from_bit(tundra_climate)));
				buf.append("\n");
				if(v_baum[scl.gib_selection()]->is_allowed_climate(rocky_climate)) buf.append(translator::translate(grund_besch_t::get_climate_name_from_bit(rocky_climate)));
				buf.append("\n");
				if(v_baum[scl.gib_selection()]->is_allowed_climate(arctic_climate)) buf.append(translator::translate(grund_besch_t::get_climate_name_from_bit(arctic_climate)));
				buf.append("\n");

		}
		else {
			// illegal, do nothing
		}
		buf.append("\n\n\n\n\n\n\n\n\n\n");
		scrolly.setze_scroll_position(0,70); // avoid image if possible
	}
}



/*
* draw scroll list
*/
void extend_edit_gui_t::draw_list(){

	scl.clear_elements();
	COLOR_VAL color = COL_BLACK;
	int j=0;

	for(slist_tpl<cstring_t>::const_iterator i=keys.begin(); i!=keys.end();++i){
		if(active_type>0  &&  active_type<4) {
			// attraction, monument

			if(v_house[j]->gib_level() == 0) {
				// level = 0
				color = COL_WHITE;
			}
			else if(v_house[j]->get_retire_year_month() < welt->get_current_month()) {
				// obsolute
				color = COL_BLUE;
			}
			else if(v_house[j]->get_intro_year_month() > welt->get_current_month()) {
				// future
				color = COL_YELLOW;
			}
			else {
				color = COL_BLACK;
			}
		}
		else if(active_type == 0) {
			// factory

			if(v_fab[j]->gib_produkt(0)==NULL  &&  v_fab[j]->gib_platzierung() == fabrik_besch_t::Land) {
				// land consumer
				color = COL_DARK_GREEN;
			}
			else if(v_fab[j]->gib_produkt(0)==NULL  &&  v_fab[j]->gib_platzierung() == fabrik_besch_t::Stadt) {
				// city consumer
				color = COL_DARK_ORANGE;
			}
			else {
				color = COL_BLACK;
			}
		}

		if(is_show_trans_name) {
			// show translation name

			scl.append_element(*i, color);
		}
		else {
			// show object name

			if(active_type > 0  &&  active_type < 4) {
				// attraction, monument
				scl.append_element(v_house[j]->gib_name(), color);
			}
			else if(active_type == 0) {
				// factory
				scl.append_element(v_fab[j]->gib_name(), color);
			}
			else if(active_type == 4) {
				// tree
				scl.append_element(v_baum[j]->gib_name(), color);
			}
		}
		j++;

	}
	// show last selected item
	scl.show_selection(scl.gib_selection());
}
#endif
