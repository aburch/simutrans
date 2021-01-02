/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <algorithm>
#include <string.h>

#include "halt_list_frame.h"
#include "halt_list_filter_frame.h"

#include "../simhalt.h"
#include "../simware.h"
#include "../simfab.h"
#include "../unicode.h"
#include "simwin.h"
#include "../descriptor/skin_desc.h"

#include "../bauer/goods_manager.h"

#include "../dataobj/translator.h"

#include "../utils/cbuffer_t.h"


static bool passes_filter(haltestelle_t & s); // see below
static uint8 default_sortmode = 0;

/**
 * Scrolled list of halt_list_stats_ts.
 * Filters (by setting visibility) and sorts.
 */
class gui_scrolled_halt_list_t : public gui_scrolled_list_t
{
public:
	gui_scrolled_halt_list_t() :  gui_scrolled_list_t(gui_scrolled_list_t::windowskin, compare) {}

	void sort()
	{
		// set visibility according to filter
		for(  vector_tpl<gui_component_t*>::iterator iter = item_list.begin();  iter != item_list.end();  ++iter) {
			halt_list_stats_t *a = dynamic_cast<halt_list_stats_t*>(*iter);

			a->set_visible( passes_filter(*a->get_halt()) );
		}

		gui_scrolled_list_t::sort(0);
	}

	static bool compare(const gui_component_t *aa, const gui_component_t *bb)
	{
		const halt_list_stats_t *a = dynamic_cast<const halt_list_stats_t*>(aa);
		const halt_list_stats_t *b = dynamic_cast<const halt_list_stats_t*>(bb);

		return halt_list_frame_t::compare_halts(a->get_halt(), b->get_halt());
	}
};


/**
 * All filter and sort settings are static, so the old settings are
 * used when the window is reopened.
 */

/**
 * This variable defines by which column the table is sorted
 */
halt_list_frame_t::sort_mode_t halt_list_frame_t::sortby = nach_name;

/**
 * This variable defines the sort order (ascending or descending)
 * Values: 1 = ascending, 2 = descending)
 */
bool halt_list_frame_t::sortreverse = false;

/**
 * Default filter: no Oil rigs!
 */
int halt_list_frame_t::filter_flags = 0;

char halt_list_frame_t::name_filter_value[64] = "";

slist_tpl<const goods_desc_t *> halt_list_frame_t::waren_filter_ab;
slist_tpl<const goods_desc_t *> halt_list_frame_t::waren_filter_an;

const char *halt_list_frame_t::sort_text[SORT_MODES] = {
	"hl_btn_sort_name",
	"hl_btn_sort_waiting",
	"hl_btn_sort_type"
};


/**
* This function compares two stations
*
* @return halt1 < halt2
*/
bool halt_list_frame_t::compare_halts(halthandle_t const halt1, halthandle_t const halt2)
{
	int order;

	/***********************************
	* Compare station 1 and station 2
	***********************************/
	switch (sortby) {
		default:
		case nach_name: // sort by station name
			order = 0;
			break;
		case nach_wartend: // sort by waiting goods
			order = (int)(halt1->get_finance_history( 0, HALT_WAITING ) - halt2->get_finance_history( 0, HALT_WAITING ));
			break;
		case nach_typ: // sort by station type
			order = halt1->get_station_type() - halt2->get_station_type();
			break;
	}
	/**
	 * use name as an additional sort, to make sort more stable.
	 */
	if(order == 0) {
		order = strcmp(halt1->get_name(), halt2->get_name());
	}
	/***********************************
	 * Consider sorting order
	 ***********************************/
	return sortreverse ? order > 0 : order < 0;
}


static bool passes_filter_name(haltestelle_t const& s)
{
	if (!halt_list_frame_t::get_filter(halt_list_frame_t::name_filter)) return true;

	return utf8caseutf8(s.get_name(), halt_list_frame_t::access_name_filter());
}


static bool passes_filter_type(haltestelle_t const& s)
{
	if (!halt_list_frame_t::get_filter(halt_list_frame_t::typ_filter)) return true;

	haltestelle_t::stationtyp const t = s.get_station_type();
	if (halt_list_frame_t::get_filter(halt_list_frame_t::frachthof_filter)       && t & haltestelle_t::loadingbay)      return true;
	if (halt_list_frame_t::get_filter(halt_list_frame_t::bahnhof_filter)         && t & haltestelle_t::railstation)     return true;
	if (halt_list_frame_t::get_filter(halt_list_frame_t::bushalt_filter)         && t & haltestelle_t::busstop)         return true;
	if (halt_list_frame_t::get_filter(halt_list_frame_t::airport_filter)         && t & haltestelle_t::airstop)         return true;
	if (halt_list_frame_t::get_filter(halt_list_frame_t::dock_filter)            && t & haltestelle_t::dock)            return true;
	if (halt_list_frame_t::get_filter(halt_list_frame_t::monorailstop_filter)    && t & haltestelle_t::monorailstop)    return true;
	if (halt_list_frame_t::get_filter(halt_list_frame_t::maglevstop_filter)      && t & haltestelle_t::maglevstop)      return true;
	if (halt_list_frame_t::get_filter(halt_list_frame_t::narrowgaugestop_filter) && t & haltestelle_t::narrowgaugestop) return true;
	if (halt_list_frame_t::get_filter(halt_list_frame_t::tramstop_filter)        && t & haltestelle_t::tramstop)        return true;
	return false;
}

static bool passes_filter_special(haltestelle_t & s)
{
	if (!halt_list_frame_t::get_filter(halt_list_frame_t::spezial_filter)) return true;

	if (halt_list_frame_t::get_filter(halt_list_frame_t::ueberfuellt_filter)) {
		PIXVAL const farbe = s.get_status_farbe();
		if (farbe == color_idx_to_rgb(COL_RED) || farbe == color_idx_to_rgb(COL_ORANGE)) {
			return true; // overcrowded
		}
	}

	if(halt_list_frame_t::get_filter(halt_list_frame_t::ohneverb_filter))
	{
		bool walking_connexion_only = true; // Walking connexion or no connexion at all.
		for(uint8 i = 0; i < goods_manager_t::get_max_catg_index(); ++i)
		{
			// TODO: Add UI to show different connexions for multiple classes
			uint8 g_class = 0;
			if(i == goods_manager_t::INDEX_PAS)
			{
				g_class = goods_manager_t::passengers->get_number_of_classes() - 1;
			}
			else if(i == goods_manager_t::INDEX_MAIL)
			{
				g_class = goods_manager_t::mail->get_number_of_classes() - 1;
			}

			if(!s.get_connexions(i, g_class)->empty())
			{
				// There might be a walking connexion here - do not count a walking connexion.
				for(auto &c : *s.get_connexions(i, g_class) )
				{
					if(c.value->best_line.is_bound() || c.value->best_convoy.is_bound())
					{
						walking_connexion_only = false;
					}
				}
			}
		}
		return walking_connexion_only; //only display stations with NO connexion (other than a walking connexion)
	}

	return false;
}


static bool passes_filter_out(haltestelle_t const& s)
{
	if (!halt_list_frame_t::get_filter(halt_list_frame_t::ware_ab_filter)) return true;

	/*
	 * Die Unterkriterien werden gebildet aus:
	 * - die Ware wird produziert (pax/post_enabled bzw. factory vorhanden)
	 * - es existiert eine Zugverbindung mit dieser Ware (!ziele[...].empty())
	 */

	for (uint32 i = 0; i != goods_manager_t::get_count(); ++i) {
		goods_desc_t const* const ware = goods_manager_t::get_info(i);
		if (!halt_list_frame_t::get_ware_filter_ab(ware)) continue;

		if (ware == goods_manager_t::passengers) {
			if (s.get_pax_enabled()) return true;
		} else if (ware == goods_manager_t::mail) {
			if (s.get_mail_enabled()) return true;
		} else if (ware != goods_manager_t::none) {
			// Sigh - a doubly nested loop per halt
			// Fortunately the number of factories and their number of outputs
			// is limited (usually 1-2 factories and 0-1 outputs per factory)
			FOR(slist_tpl<fabrik_t*>, const f, s.get_fab_list()) {
				FOR(array_tpl<ware_production_t>, const& j, f->get_output()) {
					if (j.get_typ() == ware) return true;
				}
			}
		}
	}

	return false;
}


static bool passes_filter_in(haltestelle_t const& s)
{
	if (!halt_list_frame_t::get_filter(halt_list_frame_t::ware_an_filter)) return true;
	/*
	 * Die Unterkriterien werden gebildet aus:
	 * - die Ware wird verbraucht (pax/post_enabled bzw. factory vorhanden)
	 * - es existiert eine Zugverbindung mit dieser Ware (!ziele[...].empty())
	 */

	for (uint32 i = 0; i != goods_manager_t::get_count(); ++i) {
		goods_desc_t const* const ware = goods_manager_t::get_info(i);
		if (!halt_list_frame_t::get_ware_filter_an(ware)) continue;

		if (ware == goods_manager_t::passengers) {
			if (s.get_pax_enabled()) return true;
		} else if (ware == goods_manager_t::mail) {
			if (s.get_mail_enabled()) return true;
		}
		else if (ware != goods_manager_t::none) {
			// Sigh - a doubly nested loop per halt
			// Fortunately the number of factories and their number of outputs
			// is limited (usually 1-2 factories and 0-1 outputs per factory)
			FOR(slist_tpl<fabrik_t*>, const f, s.get_fab_list()) {
				FOR(array_tpl<ware_production_t>, const& j, f->get_input()) {
					if (j.get_typ() == ware) return true;
				}
			}
		}
	}

	return false;
}


/**
 * Check all filters for one halt.
 * returns true, if it is not filtered away.
 */
static bool passes_filter(haltestelle_t & s)
{
	if (halt_list_frame_t::get_filter(halt_list_frame_t::any_filter)) {
		if (!passes_filter_name(s))    return false;
		if (!passes_filter_type(s))    return false;
		if (!passes_filter_special(s)) return false;
		if (!passes_filter_out(s))     return false;
		if (!passes_filter_in(s))      return false;
	}
	return true;
}


halt_list_frame_t::halt_list_frame_t(player_t *player) :
	gui_frame_t( translator::translate("hl_title"), player)
{
	m_player = player;
	filter_frame = NULL;

	set_table_layout(1,0);

	add_table(2,2);
	{
		new_component<gui_label_t>("hl_txt_sort");

		filter_on.init(button_t::square_state, "cl_txt_filter");
		filter_on.set_tooltip(translator::translate("cl_btn_filter_tooltip"));
		filter_on.pressed = filter_is_on;
		filter_on.add_listener(this);
		add_component(&filter_on);

		// sort ascend/descend button
		add_table(4, 1);
		{
			for (int i = 0; i < SORT_MODES; i++) {
				sortedby.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(sort_text[i]), SYSCOL_TEXT);
			}
			sortedby.set_selection(default_sortmode);
			sortedby.set_width_fixed(true);
			sortedby.set_size(scr_size(D_BUTTON_WIDTH*1.5, D_EDIT_HEIGHT));
			sortedby.add_listener(this);
			add_component(&sortedby);

			sort_asc.init(button_t::arrowup_state, "");
			sort_asc.set_tooltip(translator::translate("hl_btn_sort_asc"));
			sort_asc.add_listener(this);
			sort_asc.pressed = sortreverse;
			add_component(&sort_asc);

			sort_desc.init(button_t::arrowdown_state, "");
			sort_desc.set_tooltip(translator::translate("hl_btn_sort_desc"));
			sort_desc.add_listener(this);
			sort_desc.pressed = !sortreverse;
			add_component(&sort_desc);
			new_component<gui_margin_t>(10);
		}
		end_table();

		filter_details.init(button_t::roundbox, "hl_btn_filter_settings");
		filter_details.set_size(D_BUTTON_SIZE);
		filter_details.add_listener(this);
		add_component(&filter_details);
	}
	end_table();

	scrolly = new_component<gui_scrolled_halt_list_t>();
	scrolly->set_maximize( true );

	fill_list();

	set_resizemode(diagonal_resize);
	set_resizemode(diagonal_resize);
	reset_min_windowsize();
}


halt_list_frame_t::~halt_list_frame_t()
{
	if(filter_frame) {
		destroy_win(filter_frame);
	}
}


/**
* This function refreshes the station-list
*/
void halt_list_frame_t::fill_list()
{
	last_world_stops = haltestelle_t::get_alle_haltestellen().get_count();				// count of stations

	scrolly->clear_elements();
	FOR(vector_tpl<halthandle_t>, const halt, haltestelle_t::get_alle_haltestellen()) {
		if(  halt->get_owner() == m_player  ) {
			scrolly->new_component<halt_list_stats_t>(halt) ;
		}
	}
	sort_list();
}


void halt_list_frame_t::sort_list()
{
	scrolly->sort();
}


bool halt_list_frame_t::infowin_event(const event_t *ev)
{
	if(ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE) {
		if(filter_frame) {
			filter_frame->infowin_event(ev);
		}
	}
	return gui_frame_t::infowin_event(ev);
}


/**
 * This method is called if an action is triggered
 */
bool halt_list_frame_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if (comp == &filter_on) {
		set_filter(any_filter, !get_filter(any_filter));
		filter_is_on = !filter_is_on;
		filter_on.pressed = filter_is_on;
		sort_list();
	}
	else if (comp == &sortedby) {
		int tmp = sortedby.get_selection();
		if (tmp >= 0 && tmp < sortedby.count_elements())
		{
			sortedby.set_selection(tmp);
			sortby = (halt_list_frame_t::sort_mode_t)tmp;
		}
		else {
			sortedby.set_selection(0);
			sortby = halt_list_frame_t::nach_name;
		}
		default_sortmode = (uint8)tmp;
		sort_list();
	}
	else if (comp == &sort_asc || comp == &sort_desc) {
		set_reverse(!get_reverse());
		sort_list();
		sort_asc.pressed = sortreverse;
		sort_desc.pressed = !sortreverse;
	}
	else if (comp == &filter_details) {
		if (filter_frame) {
			destroy_win(filter_frame);
		}
		else {
			filter_frame = new halt_list_filter_frame_t(m_player, this);
			create_win(filter_frame, w_info, (ptrdiff_t)this);
		}
	}
	return true;
}


void halt_list_frame_t::draw(scr_coord pos, scr_size size)
{
	filter_details.pressed = filter_frame != NULL;

	if(  last_world_stops != haltestelle_t::get_alle_haltestellen().get_count()  ) {
		// some deleted/ added => resort
		fill_list();
	}

	gui_frame_t::draw(pos, size);
}


void halt_list_frame_t::set_ware_filter_ab(const goods_desc_t *ware, int mode)
{
	if(ware != goods_manager_t::none) {
		if(get_ware_filter_ab(ware)) {
			if(mode != 1) {
				waren_filter_ab.remove(ware);
			}
		}
		else {
			if(mode != 0) {
				waren_filter_ab.append(ware);
			}
		}
	}
}


void halt_list_frame_t::set_ware_filter_an(const goods_desc_t *ware, int mode)
{
	if(ware != goods_manager_t::none) {
		if(get_ware_filter_an(ware)) {
			if(mode != 1) {
				waren_filter_an.remove(ware);
			}
		}
		else {
			if(mode != 0) {
				waren_filter_an.append(ware);
			}
		}
	}
}


void halt_list_frame_t::set_alle_ware_filter_ab(int mode)
{
	if(mode == 0) {
		waren_filter_ab.clear();
	}
	else {
		for(unsigned int i = 0; i<goods_manager_t::get_count(); i++) {
			set_ware_filter_ab(goods_manager_t::get_info(i), mode);
		}
	}
}


void halt_list_frame_t::set_alle_ware_filter_an(int mode)
{
	if(mode == 0) {
		waren_filter_an.clear();
	}
	else {
		for(unsigned int i = 0; i<goods_manager_t::get_count(); i++) {
			set_ware_filter_an(goods_manager_t::get_info(i), mode);
		}
	}
}
