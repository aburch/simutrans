/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 * filtering added by Volker Meyer
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>
#include <algorithm>

#include "gui_container.h"
#include "gui_convoiinfo.h"
#include "components/list_button.h"

#include "convoi_frame.h"
#include "convoi_filter_frame.h"

#include "../simconvoi.h"
#include "../simwin.h"
#include "../simworld.h"
#include "../simdepot.h"

#include "../besch/ware_besch.h"
#include "../bauer/warenbauer.h"

#include "../utils/simstring.h"

#include "../vehicle/simvehikel.h"

 /**
 * All filter and sort settings are static, so the old settings are
 * used when the window is reopened.
 */
convoi_frame_t::sort_mode_t convoi_frame_t::sortby = convoi_frame_t::nach_name;
bool convoi_frame_t::sortreverse = false;

uint32 convoi_frame_t::filter_flags = 0;

char convoi_frame_t::name_filter_value[64] = "";

slist_tpl<const ware_besch_t *> convoi_frame_t::waren_filter;

const char *convoi_frame_t::sort_text[SORT_MODES] = {
    "cl_btn_sort_name",
    "cl_btn_sort_income",
    "cl_btn_sort_type",
    "cl_btn_sort_id"
};


bool convoi_frame_t::passes_filter(convoihandle_t cnv)
{
	if(!get_filter(any_filter)) {
		return true;
	}

	if(get_filter(name_filter) &&  !strstr(cnv->get_name(), name_filter_value)) {
		return false;
	}

	vehikel_t const* const fahr = cnv->front();
	if(get_filter(typ_filter)) {
		switch(fahr->get_typ()) {
			case ding_t::automobil:
				if(!get_filter(lkws_filter)) {
					return false;
				}
				break;
			case ding_t::waggon:
				// filter trams: a convoi is considered tram if the first vehicle is a tram vehicle
				if(fahr->get_besch()->get_waytype()==tram_wt) {
					if (!get_filter(tram_filter)) {
						return false;
					}
				}
				else if (!get_filter(zuege_filter)) {
					return false;
				}
				break;
			case ding_t::schiff:
				if(!get_filter(schiffe_filter)) {
					return false;
				}
				break;
			case ding_t::aircraft:
				if(!get_filter(aircraft_filter)) {
					return false;
				}
				break;
			case ding_t::monorailwaggon:
				if(!get_filter(monorail_filter)) {
					return false;
				}
				break;
			case ding_t::maglevwaggon:
				if(!get_filter(maglev_filter)) {
					return false;
				}
				break;
			case ding_t::narrowgaugewaggon:
				if(!get_filter(narrowgauge_filter)) {
					return false;
				}
				break;
			default:
				break;
		}
	}

	if(get_filter(spezial_filter)) {
		if(!(get_filter(noroute_filter) && cnv->hat_keine_route()) &&
			!(get_filter(stucked_filter) && (cnv->get_state()==convoi_t::WAITING_FOR_CLEARANCE_TWO_MONTHS  ||  cnv->get_state()==convoi_t::CAN_START_TWO_MONTHS)) &&
			!(get_filter(indepot_filter) && cnv->in_depot()) &&
			!(get_filter(noline_filter) && !cnv->get_line().is_bound()) &&
			!(get_filter(nofpl_filter) && cnv->get_schedule() == 0) &&
			!(get_filter(noincome_filter) && cnv->get_jahresgewinn()/100 <= 0))
		{
			return false;
		}
	}

	if(get_filter(ware_filter)) {
		unsigned i;
		for( i = 0; i < cnv->get_vehikel_anzahl(); i++) {
			if(get_ware_filter(cnv->get_vehikel(i)->get_fracht_typ())) {
				break;
			}
		}
		if(i == cnv->get_vehikel_anzahl()) {
			return false;
		}
	}
	return true;
}



bool convoi_frame_t::compare_convois(convoihandle_t const cnv1, convoihandle_t const cnv2)
{
	long result=0;

	switch (sortby) {
		default:
		case nach_name:
			result = strcmp(cnv1->get_internal_name(), cnv2->get_internal_name());
			break;
		case nach_gewinn:
			result = sgn(cnv1->get_jahresgewinn() - cnv2->get_jahresgewinn());
			break;
		case nach_typ:
			if(cnv1->get_vehikel_anzahl()*cnv2->get_vehikel_anzahl()>0) {
				vehikel_t const* const fahr1 = cnv1->front();
				vehikel_t const* const fahr2 = cnv2->front();

				result = fahr1->get_typ() - fahr2->get_typ();
				if(result == 0) {
					result = fahr1->get_fracht_typ()->get_catg_index() - fahr2->get_fracht_typ()->get_catg_index();
					if(result == 0) {
						result = fahr1->get_basis_bild() - fahr2->get_basis_bild();
					}
				}
			}
			break;
		case nach_id:
			result = cnv1.get_id()-cnv2.get_id();
			break;
	}
	return sortreverse ? result > 0 : result < 0;
}



void convoi_frame_t::sort_list()
{
	const karte_t* welt = owner->get_welt();

	convois.clear();
	convois.resize( welt->get_convoi_count() );

	for (vector_tpl<convoihandle_t>::const_iterator i = welt->convois_begin(), end = welt->convois_end(); i != end; ++i) {
		convoihandle_t cnv = *i;
		if(cnv->get_besitzer()==owner  &&   passes_filter(cnv)) {
			convois.append(cnv);
		}
	}
	std::sort(convois.begin(), convois.end(), compare_convois);

	sortedby.set_text(sort_text[get_sortierung()]);
	sorteddir.set_text( get_reverse() ? "cl_btn_sort_desc" : "cl_btn_sort_asc");

	// only now we know how many convois we have
	resize(koord(0,0));
}



convoi_frame_t::convoi_frame_t(spieler_t* sp) :
	gui_frame_t("cl_title", sp),
	owner(sp),
	vscroll( scrollbar_t::vertical ),
	sort_label("cl_txt_sort"),
	filter_label("cl_txt_filter")
{
	filter_frame = NULL;

	sort_label.set_pos(koord(BUTTON1_X, 4));
	add_komponente(&sort_label);
	sortedby.init(button_t::roundbox, "", koord(BUTTON1_X, 14), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	sortedby.add_listener(this);
	add_komponente(&sortedby);

	sorteddir.init(button_t::roundbox, "", koord(BUTTON2_X, 14), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	sorteddir.add_listener(this);
	add_komponente(&sorteddir);

	filter_label.set_pos(koord(BUTTON3_X, 4));
	add_komponente(&filter_label);

	filter_on.init(button_t::roundbox, get_filter(any_filter) ? "cl_btn_filter_enable" : "cl_btn_filter_disable", koord(BUTTON3_X, 14), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	filter_on.add_listener(this);
	add_komponente(&filter_on);

	filter_details.init(button_t::roundbox, "cl_btn_filter_settings", koord(BUTTON4_X, 14), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	filter_details.add_listener(this);
	add_komponente(&filter_details);

	set_fenstergroesse(koord(BUTTON4_X+BUTTON_WIDTH+2, 191+16+16));
	set_min_windowsize(koord(BUTTON4_X+BUTTON_WIDTH+2, 191+16+16));

	sort_list();

	set_resizemode(diagonal_resize);
	resize(koord(0,0));
}



convoi_frame_t::~convoi_frame_t()
{
	if(filter_frame) {
		destroy_win(filter_frame);
	}
}



bool convoi_frame_t::infowin_event(const event_t *ev)
{
	if(ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE) {
		if(filter_frame) {
			filter_frame->infowin_event(ev);
		}
	}
	else if(IS_WHEELUP(ev)  ||  IS_WHEELDOWN(ev)) {
		// otherwise these events are only registered where directly over the scroll region
		// (and sometime even not then ... )
		vscroll.infowin_event(ev);
		return true;
	}
	else if((IS_LEFTRELEASE(ev)  ||  IS_RIGHTRELEASE(ev))  &&  ev->my>47  &&  ev->mx+11<get_fenstergroesse().x) {
		int y = (ev->my-47)/40 + vscroll.get_knob_offset();
		if(y<(sint32)convois.get_count()) {
			// let gui_convoiinfo_t() handle this, since then it will be automatically consistent
			gui_convoiinfo_t ci(convois[y], 0);
			ci.infowin_event( ev );
			return true;
		}
	}
	return gui_frame_t::infowin_event(ev);
}



/**
 * This method is called if an action is triggered
 * @author Markus Weber
 */
bool convoi_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)           // 28-Dec-01    Markus Weber    Added
{
	if(komp == &filter_on) {
		DBG_MESSAGE("convoi_frame_t::action_triggered()","toggle %i",get_filter(any_filter));
		set_filter(any_filter, !get_filter(any_filter));
		filter_on.set_text(get_filter(any_filter) ? "cl_btn_filter_enable" : "cl_btn_filter_disable");
		sort_list();
	}
	else if(komp == &sortedby) {
		set_sortierung((sort_mode_t)((get_sortierung() + 1) % SORT_MODES));
		sort_list();
	}
	else if(komp == &sorteddir) {
		set_reverse(!get_reverse());
		sort_list();
	}
	else if(komp == &filter_details) {
		if(filter_frame) {
			destroy_win(filter_frame);
		}
		else {
			filter_frame = new convoi_filter_frame_t(owner, this);
			create_win(filter_frame, w_info, (long)this);
		}
	}
	return true;
}



void convoi_frame_t::resize(const koord size_change)                          // 28-Dec-01    Markus Weber    Added
{
	gui_frame_t::resize(size_change);
	koord groesse = get_fenstergroesse()-koord(0,47);
	remove_komponente(&vscroll);
	if((sint32)convois.get_count()<=groesse.y/40) {
		vscroll.set_knob_offset(0);
	}
	else {
		add_komponente(&vscroll);
		vscroll.set_pos(koord(groesse.x-11, 47-16));
		vscroll.set_groesse(groesse-koord(0,11));
		vscroll.set_knob( groesse.y/40, convois.get_count() );
		vscroll.set_scroll_amount( 1 );
	}
}



void convoi_frame_t::zeichnen(koord pos, koord gr)
{
	filter_details.pressed = filter_frame != NULL;

	gui_frame_t::zeichnen(pos, gr);

	PUSH_CLIP(pos.x, pos.y+47, gr.x-11, gr.y-48 );

	uint32 start = vscroll.get_knob_offset();
	sint16 yoffset = 47;
	for(  unsigned i=start;  i<convois.get_count()  &&  yoffset<gr.y+47;  i++  ) {
		convoihandle_t cnv = convois[i];

		if(cnv.is_bound()) {
			gui_convoiinfo_t ci(cnv, 0);
			ci.zeichnen( pos+koord(0,yoffset) );
		}
		// full height of a convoi is 40 for all info
		yoffset += 40;
	}

	POP_CLIP();
}



void convoi_frame_t::set_ware_filter(const ware_besch_t *ware, int mode)
{
	if(ware!=warenbauer_t::nichts) {
		if(get_ware_filter(ware)) {
			if(mode != 1) {
				waren_filter.remove(ware);
			}
		}
		else {
			if(mode != 0) {
				waren_filter.append(ware);
			}
		}
	}
}



void convoi_frame_t::set_alle_ware_filter(int mode)
{
	if(mode == 0) {
		waren_filter.clear();
	}
	else {
		for(unsigned int i = 0; i<warenbauer_t::get_waren_anzahl(); i++) {
			set_ware_filter(warenbauer_t::get_info(i), mode);
		}
	}
}
