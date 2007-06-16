/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 * filtering added by Volker Meyer
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <string.h>

#include "gui_container.h"
#include "components/gui_scrollpane.h"
#include "gui_convoiinfo.h"
#include "convoi_frame.h"
#include "convoi_filter_frame.h"
#include "../simvehikel.h"
#include "../simconvoi.h"
#include "../simplay.h"
#include "../simwin.h"
#include "../simworld.h"
#include "../simdepot.h"
#include "../tpl/slist_tpl.h"

#include "../besch/ware_besch.h"
#include "../bauer/warenbauer.h"
#include "../dataobj/translator.h"

#include "components/list_button.h"

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


/**
* This function is callable by quicksort
* This function compares two convois with current sort settings.
*
* @return  1 if cnv1 > cnv2
*         -1 if cnv1 < cnv2
*          0 if cnv1 == cnv2
* @author Volker Meyer
*/
int convoi_frame_t::compare_convois(const void *p1, const void *p2)
{
	long result;

	convoihandle_t cnv1 = *(const convoihandle_t *)p1;
	convoihandle_t cnv2 = *(const convoihandle_t *)p2;

	switch (sortby) {
		default:
		case nach_name:
			result = 0;
			break;
		case nach_gewinn:
			result = sgn(cnv1->gib_jahresgewinn() - cnv2->gib_jahresgewinn());
			break;
		case nach_typ:
			{
				vehikel_t *fahr1 = cnv1->gib_vehikel(0);
				vehikel_t *fahr2 = cnv2->gib_vehikel(0);

				result = fahr1->gib_typ() - fahr2->gib_typ();
				if(result == 0) {
					result = fahr1->gib_fracht_typ()->gib_catg_index() - fahr2->gib_fracht_typ()->gib_catg_index();
					if(result == 0) {
						result = fahr1->gib_basis_bild() - fahr2->gib_basis_bild();
					}
				}
			}
			break;
		case nach_id:
			result = cnv1.get_id()-cnv2.get_id();
			break;
	}
	/**
	 * use name as an additional sort, to make sort more stable.
	 */
	if(result == 0) {
		result = strcmp(cnv1->gib_internal_name(), cnv2->gib_internal_name());
	}
	return sortreverse ? -result : result;
}



bool convoi_frame_t::passes_filter(convoihandle_t cnv)
{
	if(!gib_filter(any_filter)) {
		return true;
	}

	if(gib_filter(name_filter) &&  !strstr(cnv->gib_name(), name_filter_value)) {
		return false;
	}

	vehikel_t *fahr = cnv->gib_vehikel(0);
	if(gib_filter(typ_filter)) {
		switch(fahr->gib_typ()) {
			case ding_t::automobil:
				if(!gib_filter(lkws_filter)) {
					return false;
				}
				break;
			case ding_t::waggon:
				if(!gib_filter(zuege_filter)) {
					return false;
				}
				break;
			case ding_t::schiff:
				if(!gib_filter(schiffe_filter)) {
					return false;
				}
				break;
			case ding_t::aircraft:
				if(!gib_filter(aircraft_filter)) {
					return false;
				}
			default:
				break;
		}
	}

	if(gib_filter(spezial_filter)) {
		if(!(gib_filter(noroute_filter) && cnv->hat_keine_route()) &&
			!(gib_filter(indepot_filter) && cnv->in_depot()) &&
			!(gib_filter(noline_filter) && !cnv->get_line().is_bound()) &&
			!(gib_filter(nofpl_filter) && cnv->gib_fahrplan() == 0) &&
			!(gib_filter(noincome_filter) && cnv->gib_jahresgewinn()/100 <= 0))
		{
			return false;
		}
	}

	if(gib_filter(ware_filter)) {
		unsigned i;
		for( i = 0; i < cnv->gib_vehikel_anzahl(); i++) {
			if(gib_ware_filter(cnv->gib_vehikel(i)->gib_fracht_typ())) {
				break;
			}
		}
		if(i == cnv->gib_vehikel_anzahl()) {
			return false;
		}
	}
	return true;
}


convoi_frame_t::convoi_frame_t(spieler_t* sp) :
	gui_frame_t("cl_title", sp),
	owner(sp),
	scrolly(&cont),
	sort_label("cl_txt_sort"),
	filter_label("cl_txt_filter")
{
	filter_frame = NULL;

	sort_label.setze_pos(koord(BUTTON1_X, 4));
	add_komponente(&sort_label);
	sortedby.init(button_t::roundbox, "", koord(BUTTON1_X, 14), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	sortedby.add_listener(this);
	add_komponente(&sortedby);

	sorteddir.init(button_t::roundbox, "", koord(BUTTON2_X, 14), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	sorteddir.add_listener(this);
	add_komponente(&sorteddir);

	filter_label.setze_pos(koord(BUTTON3_X, 4));
	add_komponente(&filter_label);

	filter_on.init(button_t::roundbox, gib_filter(any_filter) ? "cl_btn_filter_enable" : "cl_btn_filter_disable", koord(BUTTON3_X, 14), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	filter_on.add_listener(this);
	add_komponente(&filter_on);

	filter_details.init(button_t::roundbox, "cl_btn_filter_settings", koord(BUTTON4_X, 14), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	filter_details.add_listener(this);
	add_komponente(&filter_details);

	scrolly.setze_pos(koord(1, 30));
	setze_opaque(true);
	setze_fenstergroesse(koord(BUTTON4_X+BUTTON_WIDTH+2, 191+16+16));
	set_min_windowsize(koord(BUTTON4_X+BUTTON_WIDTH+2, 191+16+16));
	set_resizemode(diagonal_resize);

	display_list();
	add_komponente(&scrolly);

	resize (koord(0,0));
}



convoi_frame_t::~convoi_frame_t()
{
	if(filter_frame) {
		destroy_win(filter_frame);
	}
}



void convoi_frame_t::display_list(void)
{
	const karte_t* welt = owner->gib_welt();
	const unsigned count = welt->get_convoi_count();
	ALLOCA(convoihandle_t, a, count);
	int n = 0;
	int ypos = 0;

	for (vector_tpl<convoihandle_t>::const_iterator i = welt->convois_begin(), end = welt->convois_end(); i != end; ++i) {
		convoihandle_t cnv = *i;
		if(cnv->gib_besitzer() == owner && passes_filter(cnv)) {
			a[n++] = cnv;
		}
	}
	qsort((void *)a, n, sizeof (convoihandle_t), compare_convois);

	sortedby.setze_text(sort_text[gib_sortierung()]);
	sorteddir.setze_text(gib_reverse() ? "cl_btn_sort_desc" : "cl_btn_sort_asc");

	cont.remove_all();

	for(int i = 0; i < n; i++) {
		gui_convoiinfo_t *cinfo = new gui_convoiinfo_t(a[i], i + 1);

		cinfo->setze_pos(koord(0, ypos));
		cinfo->setze_groesse(koord(500, 32));
		cont.add_komponente(cinfo);
		ypos += 40; // @author hsiegeln: changed from +=32 to +=40 to have more space for "serves line" info!
	}
	cont.setze_groesse(koord(500, ypos));
	//scrolly.setze_groesse(koord(318, gib_fenstergroesse().y - 1 - 16 - 16 - 20));
}



void convoi_frame_t::infowin_event(const event_t *ev)
{
	if(ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE) {
		if(filter_frame) {
			filter_frame->infowin_event(ev);
		}
	}
	gui_frame_t::infowin_event(ev);
}



/**
 * This method is called if an action is triggered
 * @author Markus Weber
 */
bool convoi_frame_t::action_triggered(gui_komponente_t *komp,value_t /* */)           // 28-Dec-01    Markus Weber    Added
{
	if(komp == &filter_on) {
		DBG_MESSAGE("convoi_frame_t::action_triggered()","toggle %i",gib_filter(any_filter));
		setze_filter(any_filter, !gib_filter(any_filter));
		filter_on.setze_text(gib_filter(any_filter) ? "cl_btn_filter_enable" : "cl_btn_filter_disable");
		display_list();
	}
	else if(komp == &sortedby) {
		setze_sortierung((sort_mode_t)((gib_sortierung() + 1) % SORT_MODES));
		display_list();
	}
	else if(komp == &sorteddir) {
		setze_reverse(!gib_reverse());
		display_list();
	}
	else if(komp == &filter_details) {
		if(filter_frame) {
			destroy_win(filter_frame);
		}
		else {
			filter_frame = new convoi_filter_frame_t(owner, this);
			create_win(filter_frame, w_autodelete, -1);
		}
	}
	return true;
}


void convoi_frame_t::resize(const koord size_change)                          // 28-Dec-01    Markus Weber    Added
{
	gui_frame_t::resize(size_change);
	koord groesse = gib_fenstergroesse()-koord(0,47);
	scrolly.setze_groesse(groesse);
}



void convoi_frame_t::zeichnen(koord pos, koord gr)
{
	filter_details.pressed = filter_frame != NULL;
	gui_frame_t::zeichnen(pos, gr);
}

void convoi_frame_t::setze_ware_filter(const ware_besch_t *ware, int mode)
{
	if(ware != warenbauer_t::nichts) {
		if(gib_ware_filter(ware)) {
			if(mode != 1) {
				waren_filter.remove(ware);
			}
		} else {
			if(mode != 0) {
				waren_filter.append(ware);
			}
		}
	}
}


void convoi_frame_t::setze_alle_ware_filter(int mode)
{
	if(mode == 0) {
		waren_filter.clear();
	}
	else {
		for(unsigned int i = 0; i<warenbauer_t::gib_waren_anzahl(); i++) {
			setze_ware_filter(warenbauer_t::gib_info(i), mode);
		}
	}
}
