/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <algorithm>
#include <string.h>

#include "halt_list_frame.h"
#include "halt_list_filter_frame.h"

#include "../player/simplay.h"
#include "../simhalt.h"
#include "../simware.h"
#include "../simfab.h"
#include "../unicode.h"
#include "simwin.h"
#include "../descriptor/skin_desc.h"

#include "../bauer/goods_manager.h"

#include "../dataobj/translator.h"

#include "../utils/cbuffer_t.h"


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
uint8 halt_list_frame_t::filter_flags = 0;

char halt_list_frame_t::name_filter[256] = "";

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


halt_list_frame_t::halt_list_frame_t() :
	gui_frame_t( translator::translate("hl_title"), welt->get_active_player())
{
	m_player = welt->get_active_player();
	filter_frame = NULL;

	set_table_layout(1, 0);

	add_table(4, 2);
	{
		new_component<gui_label_t>("Filter:", 2);
		name_filter_input.set_text(name_filter, lengthof(name_filter));
		add_component(&name_filter_input);
		filter_details.init(button_t::roundbox, "cl_btn_filter_settings");
		filter_details.add_listener(this);
		add_component(&filter_details);
		new_component<gui_fill_t>();

		new_component_span<gui_label_t>("hl_txt_sort", 1);
		sortedby.set_unsorted(); // do not sort
		for (size_t i = 0; i < lengthof(sort_text); i++) {
			sortedby.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(sort_text[i]), SYSCOL_TEXT);
		}
		sortedby.set_selection(get_sortierung());
		sortedby.add_listener(this);
		add_component(&sortedby);

		sorteddir.init(button_t::sortarrow_state, NULL);
		sorteddir.add_listener(this);
		sorteddir.pressed = get_reverse();
		add_component(&sorteddir);

		new_component<gui_fill_t>();
	}
	end_table();

	scrolly = new gui_scrolled_halt_list_t();
	scrolly->set_maximize(true);

	tabs.init_tabs(scrolly);
	tabs.add_listener(this);
	add_component(&tabs);

	sort_list();

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


static bool passes_filter_special(haltestelle_t const& s)
{
	if (!halt_list_frame_t::get_filter(halt_list_frame_t::spezial_filter)) return true;

	if (halt_list_frame_t::get_filter(halt_list_frame_t::ueberfuellt_filter)) {
		PIXVAL const color = s.get_status_farbe();
		if (color == color_idx_to_rgb(COL_RED) || color == color_idx_to_rgb(COL_ORANGE)) {
			return true; // overcrowded
		}
	}

	if (halt_list_frame_t::get_filter(halt_list_frame_t::ohneverb_filter)) {
		for (uint8 i = 0; i < goods_manager_t::get_max_catg_index(); ++i) {
			if (!s.get_connections(i).empty()) return false; // only display stations with NO connection
		}
		return true;
	}

	return false;
}


static bool passes_filter_out(haltestelle_t const& s)
{
	if (!halt_list_frame_t::get_filter(halt_list_frame_t::ware_ab_filter)) return true;

	/*
	 * Die Unterkriterien werden gebildet aus:
	 * - die Ware wird produziert (pax/post_enabled bzw. fabrik vorhanden)
	 * - es existiert eine Zugverbindung mit dieser Ware (!ziele[...].empty())
	 */

	for (uint32 i = 0; i != goods_manager_t::get_count(); ++i) {
		goods_desc_t const* const ware = goods_manager_t::get_info(i);
		if (!halt_list_frame_t::get_ware_filter_ab(ware)) continue;

		if (ware == goods_manager_t::passengers) {
			if (s.get_pax_enabled()) return true;
		}
		else if (ware == goods_manager_t::mail) {
			if (s.get_mail_enabled()) return true;
		}
		else if (ware != goods_manager_t::none) {
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
	 * - die Ware wird verbraucht (pax/post_enabled bzw. fabrik vorhanden)
	 * - es existiert eine Zugverbindung mit dieser Ware (!ziele[...].empty())
	 */

	for (uint32 i = 0; i != goods_manager_t::get_count(); ++i) {
		goods_desc_t const* const ware = goods_manager_t::get_info(i);
		if (!halt_list_frame_t::get_ware_filter_an(ware)) continue;

		if (ware == goods_manager_t::passengers) {
			if (s.get_pax_enabled()) return true;
		}
		else if (ware == goods_manager_t::mail) {
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
static bool passes_filter(haltestelle_t const& s)
{
	if (!passes_filter_special(s)) return false;
	if (!passes_filter_out(s))     return false;
	if (!passes_filter_in(s))      return false;
	return true;
}


/**
* This function refreshes the station-list
*/
void halt_list_frame_t::sort_list()
{
	last_world_stops = haltestelle_t::get_alle_haltestellen().get_count(); // count of stations

	haltestelle_t::stationtyp current_type = tabs.get_active_tab_stationtype();

	scrolly->clear_elements();
	FOR(vector_tpl<halthandle_t>, const halt, haltestelle_t::get_alle_haltestellen()) {
		if (halt->get_owner() != m_player) {
			continue;
		}
		if (name_filter[0] != 0  &&  !utf8caseutf8(halt->get_name(), name_filter)) {
			// not the right name
			continue;
		}
		if (current_type != 0  &&  (halt->get_station_type() & current_type) == 0) {
			continue;
		}
		if(  passes_filter(*halt.get_rep())  ) {
			scrolly->new_component<halt_list_stats_t>(halt);
		}
	}
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
	if (  comp == &sortedby  ) {
		set_sortierung((sort_mode_t)((get_sortierung() + 1) % SORT_MODES));
		sort_list();
	}
	else if (comp == &tabs) {
		sort_list();
	}
	else if (comp == &sorteddir) {
		set_reverse(!get_reverse());
		sorteddir.pressed = get_reverse();
		sort_list();
	}
	else if (  comp == &filter_details  ) {
		if (filter_frame) {
			destroy_win(filter_frame);
		}
		else {
			filter_frame = new halt_list_filter_frame_t(m_player, this);
			create_win(filter_frame, w_info, magic_haltlist_filter);
		}
	}
	return true;
}


void halt_list_frame_t::draw(scr_coord pos, scr_size size)
{
	filter_details.pressed = filter_frame != NULL;

	if(  last_world_stops != haltestelle_t::get_alle_haltestellen().get_count()  ||  strcmp(last_name_filter, name_filter)  ) {
		// some deleted/ added => resort
		strcpy(last_name_filter, name_filter);
		sort_list();
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


void halt_list_frame_t::rdwr(loadsave_t* file)
{
	scr_size size = get_windowsize();
	uint8 player_nr = m_player->get_player_nr();
	sint16 sort_mode = sortby;

	file->rdwr_byte(player_nr);
	size.rdwr(file);
	tabs.rdwr(file);
	scrolly->rdwr(file);
	file->rdwr_str(name_filter, lengthof(name_filter));
	file->rdwr_short(sort_mode);
	file->rdwr_bool(sortreverse);
	file->rdwr_byte(filter_flags);
	if (file->is_saving()) {
		uint8 good_nr = waren_filter_ab.get_count();
		file->rdwr_byte(good_nr);
		if (good_nr > 0) {
			FOR(slist_tpl<const goods_desc_t*>, const i, waren_filter_ab) {
				char* name = const_cast<char*>(i->get_name());
				file->rdwr_str(name, 256);
			}
		}
		good_nr = waren_filter_an.get_count();
		file->rdwr_byte(good_nr);
		if (good_nr > 0) {
			FOR(slist_tpl<const goods_desc_t*>, const i, waren_filter_an) {
				char* name = const_cast<char*>(i->get_name());
				file->rdwr_str(name, 256);
			}
		}
	}
	else {
		// restore warenfilter
		uint8 good_nr;
		file->rdwr_byte(good_nr);
		if (good_nr > 0) {
			waren_filter_ab.clear();
			for (sint16 i = 0; i < good_nr; i++) {
				char name[256];
				file->rdwr_str(name, lengthof(name));
				if (const goods_desc_t* gd = goods_manager_t::get_info(name)) {
					waren_filter_ab.append(gd);
				}
			}
		}
		file->rdwr_byte(good_nr);
		if (good_nr > 0) {
			waren_filter_an.clear();
			for (sint16 i = 0; i < good_nr; i++) {
				char name[256];
				file->rdwr_str(name, lengthof(name));
				if (const goods_desc_t* gd = goods_manager_t::get_info(name)) {
					waren_filter_an.append(gd);
				}
			}
		}

		sortby = (sort_mode_t)sort_mode;
		m_player = welt->get_player(player_nr);
		set_owner(m_player);
		win_set_magic(this, magic_halt_list + player_nr);
		sort_list();
		set_windowsize(size);
	}
}
