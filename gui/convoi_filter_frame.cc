/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 * written by Volker Meyer
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Displays a filter settings dialog for the convoi list
 *
 * @author V. Meyer
 */

#include "convoi_filter_frame.h"
#include "convoi_frame.h"
#include "gui_convoiinfo.h"
#include "../simcolor.h"

#include "../besch/ware_besch.h"
#include "../bauer/warenbauer.h"
#include "../dataobj/translator.h"

#define D_HALF_BUTTON_WIDTH (D_BUTTON_WIDTH/2)

koord convoi_filter_frame_t::filter_buttons_pos[FILTER_BUTTONS] = {
	koord(D_MARGIN_LEFT, D_MARGIN_TOP),
	koord(125, 0),
	koord(D_MARGIN_LEFT, 2*D_BUTTON_HEIGHT+D_V_SPACE),
	koord(D_MARGIN_LEFT + D_H_SPACE, 3*D_BUTTON_HEIGHT + D_V_SPACE),
	koord(D_MARGIN_LEFT + D_H_SPACE, 4*D_BUTTON_HEIGHT + D_V_SPACE),
	koord(D_MARGIN_LEFT + D_H_SPACE, 5*D_BUTTON_HEIGHT + D_V_SPACE),
	koord(D_MARGIN_LEFT + D_H_SPACE, 6*D_BUTTON_HEIGHT + D_V_SPACE),
	koord(D_MARGIN_LEFT + D_H_SPACE, 7*D_BUTTON_HEIGHT + D_V_SPACE),
	koord(D_MARGIN_LEFT + D_H_SPACE, 8*D_BUTTON_HEIGHT + D_V_SPACE),
	koord(D_MARGIN_LEFT + D_H_SPACE, 9*D_BUTTON_HEIGHT + D_V_SPACE),
	koord(D_MARGIN_LEFT + D_H_SPACE, 10*D_BUTTON_HEIGHT + D_V_SPACE),
	koord(D_MARGIN_LEFT, 11*D_BUTTON_HEIGHT + 2*D_V_SPACE),
	koord(D_MARGIN_LEFT + D_H_SPACE, 12*D_BUTTON_HEIGHT + 2*D_V_SPACE),
	koord(D_MARGIN_LEFT + D_H_SPACE, 13*D_BUTTON_HEIGHT + 2*D_V_SPACE),
	koord(D_MARGIN_LEFT + D_H_SPACE, 14*D_BUTTON_HEIGHT + 2*D_V_SPACE),
	koord(D_MARGIN_LEFT + D_H_SPACE, 15*D_BUTTON_HEIGHT + 2*D_V_SPACE),
	koord(D_MARGIN_LEFT + D_H_SPACE, 16*D_BUTTON_HEIGHT + 2*D_V_SPACE),
	koord(D_MARGIN_LEFT + D_H_SPACE, 17*D_BUTTON_HEIGHT + 2*D_V_SPACE),
	koord(D_MARGIN_LEFT + D_H_SPACE, 18*D_BUTTON_HEIGHT + 2*D_V_SPACE)
};

const char *convoi_filter_frame_t::filter_buttons_text[FILTER_BUTTONS] = {
	"clf_chk_name_filter",
	"clf_chk_waren",
	"clf_chk_type_filter",
	"clf_chk_cars",
	"clf_chk_trains",
	"clf_chk_ships",
	"clf_chk_aircrafts",
	"clf_chk_monorail",
	"clf_chk_maglev",
	"clf_chk_narrowgauge",
	"clf_chk_trams",
	"clf_chk_spezial_filter",
	"clf_chk_noroute",
	"clf_chk_stucked",
	"clf_chk_noincome",
	"clf_chk_indepot",
	"clf_chk_noschedule",
	"clf_chk_noline",
	"clf_chk_obsolete"
};

convoi_filter_frame_t::filter_flag_t convoi_filter_frame_t::filter_buttons_types[FILTER_BUTTONS] = {
	name_filter,
	ware_filter,
	typ_filter,
	lkws_filter,
	zuege_filter,
	schiffe_filter,
	aircraft_filter,
	monorail_filter,
	maglev_filter,
	narrowgauge_filter,
	tram_filter,
	spezial_filter,
	noroute_filter,
	stucked_filter,
	noincome_filter,
	indepot_filter,
	nofpl_filter,
	noline_filter,
	obsolete_filter
};

slist_tpl<const ware_besch_t *>convoi_filter_frame_t::active_ware;
char convoi_filter_frame_t::name_filter_text[] = "";


convoi_filter_frame_t::convoi_filter_frame_t(spieler_t *sp, convoi_frame_t *m, uint32 f ) :
	gui_frame_t( translator::translate("clf_title"), sp),
	filter_flags(f),
	main_frame(m),
	ware_scrolly(&ware_cont)
{
	int yp = D_MARGIN_TOP;

	for(  int i=0; i < FILTER_BUTTONS; i++  ) {
		filter_buttons[i].init(button_t::square_state, filter_buttons_text[i], filter_buttons_pos[i]);
		filter_buttons[i].add_listener(this);
		add_komponente(filter_buttons + i);
		if(filter_buttons_types[i] < sub_filter) {
			filter_buttons[i].foreground = COL_WHITE;
		}
		filter_buttons[i].pressed = get_filter(filter_buttons_types[i]);
	}

	yp += D_BUTTON_HEIGHT;

	name_filter_input.set_pos(koord(D_MARGIN_LEFT, yp));
	name_filter_input.set_groesse(koord(100, D_BUTTON_HEIGHT));
	name_filter_input.set_text( name_filter_text, lengthof(name_filter_text) );
	name_filter_input.add_listener(this);
	add_komponente(&name_filter_input);

	ware_alle.init(button_t::roundbox, "clf_btn_alle", koord(125, yp), koord(D_HALF_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	ware_alle.add_listener(this);
	add_komponente(&ware_alle);
	ware_keine.init(button_t::roundbox, "clf_btn_keine", koord(125+D_HALF_BUTTON_WIDTH+D_H_SPACE, yp), koord(D_HALF_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	ware_keine.add_listener(this);
	add_komponente(&ware_keine);
	ware_invers.init(button_t::roundbox, "clf_btn_invers", koord(125+2*(D_HALF_BUTTON_WIDTH+D_H_SPACE), yp), koord(D_HALF_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	ware_invers.add_listener(this);
	add_komponente(&ware_invers);

	ware_scrolly.set_pos(koord(125, 2*D_BUTTON_HEIGHT+4));
	ware_scrolly.set_scroll_amount_y(D_BUTTON_HEIGHT);
	add_komponente(&ware_scrolly);

	all_ware.clear();
	int n=0;
	for(  int i=0;  i < warenbauer_t::get_waren_anzahl();  i++  ) {
		const ware_besch_t *ware = warenbauer_t::get_info(i);
		if(  ware == warenbauer_t::nichts  ) {
			continue;
		}
		if(  ware->get_catg() == 0  ) {
			// Sonderfracht: Each good is special
			ware_item_t *item = new ware_item_t(this, ware);
			item->init(button_t::square_state, translator::translate(ware->get_name()), koord(5, D_BUTTON_HEIGHT*n++));
			item->pressed = active_ware.is_contained(ware);
			ware_cont.add_komponente(item);
			all_ware.append(item);
		}
	}
	// now add other good categories
	for(  int i=1;  i < warenbauer_t::get_max_catg_index();  i++  ) {
		if(  warenbauer_t::get_info_catg(i)->get_catg() != 0  ) {
			ware_item_t *item = new ware_item_t(this, warenbauer_t::get_info_catg(i));
			item->init(button_t::square_state, translator::translate(warenbauer_t::get_info_catg(i)->get_catg_name()), koord(5, D_BUTTON_HEIGHT*n++));
			item->pressed = active_ware.is_contained(warenbauer_t::get_info_catg(i));
			ware_cont.add_komponente(item);
			all_ware.append(item);
		}
	}

	ware_cont.set_groesse(koord(100, n*D_BUTTON_HEIGHT));
	ware_scrolly.set_groesse(koord(125, 13*D_BUTTON_HEIGHT));

	set_fenstergroesse(koord(317, D_TITLEBAR_HEIGHT+((KOORD_VAL)FILTER_BUTTONS)*D_BUTTON_HEIGHT+8+10));
	set_min_windowsize(koord(255, D_TITLEBAR_HEIGHT+((KOORD_VAL)FILTER_BUTTONS)*D_BUTTON_HEIGHT+8-2));

	set_resizemode(diagonal_resize);
	resize(koord(0,0));
}


/**
 * This method is called if an action is triggered
 * @author V. Meyer
 */
bool convoi_filter_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	for( int i = 0; i < FILTER_BUTTONS; i++) {
		if(komp == filter_buttons + i) {
			filter_buttons[i].pressed ^= 1;
			set_filter( filter_buttons_types[i], !get_filter(filter_buttons_types[i]) );
			sort_list();
			return true;
		}
	}
	if(komp == &ware_alle) {
		FOR( slist_tpl<ware_item_t *>, wi, all_ware ) {
			wi->pressed = true;
		}
		sort_list();
		return true;
	}
	if(komp == &ware_keine) {
		FOR( slist_tpl<ware_item_t *>, wi, all_ware ) {
			wi->pressed = false;
		}
		sort_list();
		return true;
	}
	if(  komp == &ware_invers  ) {
		FOR( slist_tpl<ware_item_t *>, wi, all_ware ) {
			wi->pressed ^= true;
		}
		sort_list();
		return true;
	}
	if(  komp == &name_filter_input  &&  filter_flags&name_filter  ) {
		sort_list();
		return true;
	}
	return false;
}


void convoi_filter_frame_t::sort_list()
{
	active_ware.clear();
	FOR( slist_tpl<ware_item_t *>, wi, all_ware ) {
		if(  wi->pressed  ) {
/*			uint8 catg = wi->ware->get_catg();
			if(  catg  ) {
				// now all goods of this category
				for(  int i=1;  i<warenbauer_t::get_max_catg_index();  i++   ) {
					if(  warenbauer_t::get_info(i)->get_catg()==catg  ) {
						active_ware.append( warenbauer_t::get_info(i) );
					}
				}
			}
			else {
				active_ware.append( wi->ware );
			}
*/			active_ware.append( wi->ware );
		}
	}
	main_frame->sort_list( filter_flags&name_filter ? name_filter_text : NULL, filter_flags, &active_ware );
}


void convoi_filter_frame_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);

	const koord gr = get_fenstergroesse()-koord(0, D_TITLEBAR_HEIGHT);

	const KOORD_VAL w1 = gr.x/2-4;
	const KOORD_VAL w2 = (gr.x+1)/2;
	const KOORD_VAL pos2 = gr.x-w2;
	const KOORD_VAL h = (gr.y-2-2*D_BUTTON_HEIGHT-4);

	name_filter_input.set_groesse(koord(min(w1-14,142), D_BUTTON_HEIGHT));

	// column 2
	filter_buttons_pos[1] = koord(pos2, 2);
	filter_buttons[1].set_pos(filter_buttons_pos[1]);
	ware_alle.set_pos(koord(pos2, D_BUTTON_HEIGHT));
	ware_alle.set_groesse(koord(w2/3-D_H_SPACE, D_BUTTON_HEIGHT));
	ware_keine.set_pos(koord(pos2+(w2+0)/3, D_BUTTON_HEIGHT));
	ware_keine.set_groesse(koord((w2+1)/3-D_H_SPACE, D_BUTTON_HEIGHT));
	ware_invers.set_pos(koord(pos2+(w2+0)/3+(w2+1)/3, D_BUTTON_HEIGHT));
	ware_invers.set_groesse(koord((w2+2)/3-D_H_SPACE, D_BUTTON_HEIGHT));
	ware_scrolly.set_pos(koord(pos2, 2*D_BUTTON_HEIGHT+4));
	ware_scrolly.set_groesse(koord(w2, h));
}
