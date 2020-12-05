/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string.h>
#include <algorithm>

#include "components/gui_convoiinfo.h"

#include "convoi_frame.h"
#include "convoi_filter_frame.h"

#include "simwin.h"
#include "../simconvoi.h"
#include "../simworld.h"
#include "../unicode.h"
#include "../descriptor/goods_desc.h"
#include "../bauer/goods_manager.h"
#include "../dataobj/translator.h"
#include "../player/simplay.h"
#include "../utils/simstring.h"
#include "../vehicle/simvehicle.h"
#include "../simline.h"

 /**
 * All filter and sort settings are static, so the old settings are
 * used when the window is reopened.
 */
convoi_frame_t::sort_mode_t convoi_frame_t::sortby = convoi_frame_t::by_name;
static uint8 default_sortmode = 0;
bool convoi_frame_t::sortreverse = false;
static uint8 cl_display_mode = gui_convoy_formation_t::appearance;

const char *convoi_frame_t::sort_text[SORT_MODES] = {
	"cl_btn_sort_name",
	"Line",
	"cl_btn_sort_income",
	"cl_btn_sort_type",
	"cl_btn_sort_id",
	"cl_btn_sort_max_speed",
	"cl_btn_sort_power",
	"cl_btn_sort_value",
	"cl_btn_sort_age"
};

/**
 * Scrolled list of gui_convoiinfo_ts.
 * Filters (by setting visibility) and sorts.
 */
class gui_scrolled_convoy_list_t : public gui_scrolled_list_t
{
	convoi_frame_t *main;

	static convoi_frame_t *main_static;
public:
	gui_scrolled_convoy_list_t(convoi_frame_t *m) :  gui_scrolled_list_t(gui_scrolled_list_t::windowskin)
	{
		main = m;
		set_cmp(compare);
	}

	void sort()
	{
		// set visibility according to filter
		for(  vector_tpl<gui_component_t*>::iterator iter = item_list.begin();  iter != item_list.end();  ++iter) {
			gui_convoiinfo_t *a = dynamic_cast<gui_convoiinfo_t*>(*iter);

			a->set_visible( main->passes_filter(a->get_cnv()) );
			a->set_mode(cl_display_mode);
		}
		main_static = main;
		gui_scrolled_list_t::sort(0);
	}

	static bool compare(const gui_component_t *aa, const gui_component_t *bb)
	{
		const gui_convoiinfo_t *a = dynamic_cast<const gui_convoiinfo_t*>(aa);
		const gui_convoiinfo_t *b = dynamic_cast<const gui_convoiinfo_t*>(bb);

		return main_static->compare_convois(a->get_cnv(), b->get_cnv());
	}
};
convoi_frame_t* gui_scrolled_convoy_list_t::main_static;


bool convoi_frame_t::passes_filter(convoihandle_t cnv)
{
	if(  !filter_is_on  ) {
		// no filtering enabled
		return true;
	}

	if(  name_filter!=NULL  &&  !utf8caseutf8(cnv->get_name(), name_filter)  ) {
		// not the right name
		return false;
	}

	vehicle_t const* const tdriver = cnv->front();
	if(  get_filter(convoi_filter_frame_t::typ_filter)  ) {
		switch(tdriver->get_typ()) {
			case obj_t::road_vehicle:
				if(!get_filter(convoi_filter_frame_t::lkws_filter)) {
					return false;
				}
				break;
			case obj_t::rail_vehicle:
				// filter trams: a convoi is considered tram if the first vehicle is a tram vehicle
				if(tdriver->get_desc()->get_waytype()==tram_wt) {
					if (!get_filter(convoi_filter_frame_t::tram_filter)) {
						return false;
					}
				}
				else if (!get_filter(convoi_filter_frame_t::zuege_filter)) {
					return false;
				}
				break;
			case obj_t::water_vehicle:
				if(!get_filter(convoi_filter_frame_t::schiffe_filter)) {
					return false;
				}
				break;
			case obj_t::air_vehicle:
				if(!get_filter(convoi_filter_frame_t::aircraft_filter)) {
					return false;
				}
				break;
			case obj_t::monorail_vehicle:
				if(!get_filter(convoi_filter_frame_t::monorail_filter)) {
					return false;
				}
				break;
			case obj_t::maglev_vehicle:
				if(!get_filter(convoi_filter_frame_t::maglev_filter)) {
					return false;
				}
				break;
			case obj_t::narrowgauge_vehicle:
				if(!get_filter(convoi_filter_frame_t::narrowgauge_filter)) {
					return false;
				}
				break;
			default:
				break;
		}
	}

	if(  get_filter(convoi_filter_frame_t::spezial_filter)  ) {
		if ((!get_filter(convoi_filter_frame_t::noroute_filter)  || cnv->get_state() != convoi_t::NO_ROUTE) &&
				(!get_filter(convoi_filter_frame_t::stucked_filter)  || (cnv->get_state() != convoi_t::WAITING_FOR_CLEARANCE_TWO_MONTHS && cnv->get_state() != convoi_t::CAN_START_TWO_MONTHS)) &&
				(!get_filter(convoi_filter_frame_t::indepot_filter)  || !cnv->in_depot()) &&
				(!get_filter(convoi_filter_frame_t::noline_filter)   ||  cnv->get_line().is_bound()) &&
				(!get_filter(convoi_filter_frame_t::nofpl_filter)    ||  cnv->get_schedule()) &&
				(!get_filter(convoi_filter_frame_t::noincome_filter) ||  cnv->get_jahresgewinn() >= 100) &&
				(!get_filter(convoi_filter_frame_t::obsolete_filter) || !cnv->has_obsolete_vehicles()))
		{
			return false;
		}
	}

	if(  get_filter(convoi_filter_frame_t::ware_filter)  ) {
		unsigned i;
		for(  i = 0; i < cnv->get_vehicle_count(); i++) {
			const goods_desc_t *wb = cnv->get_vehicle(i)->get_cargo_type();
			if(  wb->get_catg()!=0  ) {
				wb = goods_manager_t::get_info_catg(wb->get_catg());
			}
			if(  waren_filter->is_contained(wb)  ) {
				return true;
			}
		}
		if(  i == cnv->get_vehicle_count()  ) {
			return false;
		}
	}
	return true;
}


bool convoi_frame_t::compare_convois(convoihandle_t const cnv1, convoihandle_t const cnv2)
{
	sint32 result = 0;

	switch (sortby) {
		default:
		case by_name:
			result = strcmp(cnv1->get_internal_name(), cnv2->get_internal_name());
			break;
		case by_line:
			result = cnv1->get_line().get_id() - cnv2->get_line().get_id();
			break;
		case by_profit:
			result = sgn(cnv1->get_jahresgewinn() - cnv2->get_jahresgewinn());
			break;
		case by_type:
			if(cnv1->get_vehicle_count()*cnv2->get_vehicle_count()>0) {
				vehicle_t const* const tdriver1 = cnv1->front();
				vehicle_t const* const tdriver2 = cnv2->front();

				result = tdriver1->get_typ() - tdriver2->get_typ();
				if(result == 0) {
					result = tdriver1->get_cargo_type()->get_catg_index() - tdriver2->get_cargo_type()->get_catg_index();
					if(result == 0) {
						result = tdriver1->get_base_image() - tdriver2->get_base_image();
					}
				}
			}
			break;
		case by_id:
			result = cnv1.get_id()-cnv2.get_id();
			break;
		case by_max_speed:
			result = cnv1->get_min_top_speed() - cnv2->get_min_top_speed();
			break;
		case by_power:
			result = cnv1->get_sum_power() - cnv2->get_sum_power();
			break;
		case by_value:
			result = cnv1->get_purchase_cost() - cnv2->get_purchase_cost();
			break;
		case by_age:
			result = cnv1->get_average_age() - cnv2->get_average_age();
			break;
	}
	return sortreverse ? result > 0 : result < 0;
}


void convoi_frame_t::fill_list()
{
	last_world_convois = welt->convoys().get_count();

	scrolly->clear_elements();
	FOR(vector_tpl<convoihandle_t>, const cnv, welt->convoys()) {
		if(cnv->get_owner()==owner) {
			scrolly->new_component<gui_convoiinfo_t>(cnv);
		}
	}
	sort_list();
}


void convoi_frame_t::sort_list()
{
	scrolly->sort();
}


void convoi_frame_t::sort_list( char *name, uint32 filter, const slist_tpl<const goods_desc_t *> *wares )
{
	name_filter = name;
	waren_filter = wares;
	filter_flags = filter;
	if(  filter_is_on  ) {
		sort_list();
	}
}


convoi_frame_t::convoi_frame_t(player_t* player) :
	gui_frame_t( translator::translate("cl_title"), player),
	owner(player)
{
	name_filter = NULL;
	filter_flags = 0;
	filter_is_on = false;

	set_table_layout(1,0);

	add_table(4,2);
	{
		new_component_span<gui_label_t>("cl_txt_sort", 2);
		new_component<gui_label_t>("cl_txt_mode");
		filter_on.init(button_t::square, "cl_txt_filter");
		filter_on.set_tooltip(translator::translate("cl_btn_filter_tooltip"));
		filter_on.add_listener(this);
		add_component(&filter_on);

		for (int i = 0; i < SORT_MODES; i++) {
			sortedby.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(sort_text[i]), SYSCOL_TEXT);
		}
		sortedby.set_selection(default_sortmode);
		sortedby.set_width_fixed(true);
		sortedby.set_size(scr_size(D_BUTTON_WIDTH*1.5, D_EDIT_HEIGHT));
		sortedby.add_listener(this);
		add_component(&sortedby);

		// sort ascend/descend button
		add_table(3,1);
		{
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

		display_mode.init(button_t::roundbox, gui_convoy_formation_t::cnvlist_mode_button_texts[cl_display_mode]);
		display_mode.add_listener(this);
		add_component(&display_mode);

		filter_details.init(button_t::roundbox, "cl_btn_filter_settings");
		filter_details.set_size(D_BUTTON_SIZE);
		filter_details.add_listener(this);
		add_component(&filter_details);
	}
	end_table();

	scrolly = new_component<gui_scrolled_convoy_list_t>(this);
	scrolly->set_maximize( true );

	fill_list();

	set_resizemode(diagonal_resize);
	reset_min_windowsize();
}


convoi_frame_t::~convoi_frame_t()
{
	destroy_win( magic_convoi_list_filter+owner->get_player_nr() );
}


bool convoi_frame_t::infowin_event(const event_t *ev)
{
	if(ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE) {
		destroy_win( magic_convoi_list_filter+owner->get_player_nr() );
	}
	return gui_frame_t::infowin_event(ev);
}


/**
 * This method is called if an action is triggered
 */
bool convoi_frame_t::action_triggered( gui_action_creator_t *comp, value_t /* */ )
{
	if(  comp == &filter_on  ) {
		filter_is_on = !filter_is_on;
		filter_on.pressed = filter_is_on;
		sort_list();
	}
	else if(  comp == &sortedby  ) {
		int tmp = sortedby.get_selection();
		if (tmp >= 0 && tmp < sortedby.count_elements())
		{
			sortedby.set_selection(tmp);
			sortby = (convoi_frame_t::sort_mode_t)tmp;
		}
		else {
			sortedby.set_selection(0);
			sortby = convoi_frame_t::by_name;
		}
		default_sortmode = (uint8)tmp;
		sort_list();
	}
	else if (comp == &sort_asc || comp == &sort_desc) {
		set_reverse( !get_reverse() );
		sort_list();
		sort_asc.pressed = sortreverse;
		sort_desc.pressed = !sortreverse;
	}
	else if (comp == &display_mode) {
		cl_display_mode = (cl_display_mode + 1) % gui_convoy_formation_t::CONVOY_OVERVIEW_MODES;
		display_mode.set_text(gui_convoy_formation_t::cnvlist_mode_button_texts[cl_display_mode]);
		sort_list();
		resize(scr_size(0, 0));
	}
	else if(  comp == &filter_details  ) {
		if(  !destroy_win( magic_convoi_list_filter+owner->get_player_nr() )  ) {
			create_win( new convoi_filter_frame_t(owner, this), w_info, magic_convoi_list_filter+owner->get_player_nr() );
		}
	}
	return true;
}


void convoi_frame_t::draw(scr_coord pos, scr_size size)
{
	filter_details.pressed = win_get_magic( magic_convoi_list_filter+owner->get_player_nr() );
	filter_on.pressed = filter_is_on;

	if (last_world_convois != welt->convoys().get_count()) {
		// some deleted/ added => resort
		fill_list();
	}

	gui_frame_t::draw(pos, size);
}
