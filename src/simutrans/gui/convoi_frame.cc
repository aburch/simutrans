/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string.h>
#include <algorithm>

#include "components/gui_convoiinfo.h"

#include "convoi_frame.h"
#include "convoi_filter_frame.h"

#include "../builder/goods_manager.h"
#include "../builder/vehikelbauer.h"

#include "../obj/way/kanal.h"
#include "../obj/way/maglev.h"
#include "../obj/way/monorail.h"
#include "../obj/way/narrowgauge.h"
#include "../obj/way/runway.h"
#include "../obj/way/schiene.h"
#include "../obj/way/strasse.h"

#include "simwin.h"
#include "../simconvoi.h"
#include "../world/simworld.h"
#include "../utils/unicode.h"
#include "../descriptor/goods_desc.h"
#include "../builder/goods_manager.h"
#include "../dataobj/translator.h"
#include "../player/simplay.h"
#include "../utils/simstring.h"
#include "../vehicle/vehicle.h"

 /**
 * All filter and sort settings are static, so the old settings are
 * used when the window is reopened.
 */
convoi_frame_t::sort_mode_t convoi_frame_t::sortby = convoi_frame_t::nach_name;
bool convoi_frame_t::sortreverse = false;

const char *convoi_frame_t::sort_text[SORT_MODES] = {
	"cl_btn_sort_name",
	"cl_btn_sort_income",
	"cl_btn_sort_type",
	"cl_btn_sort_id"
};

const slist_tpl<const goods_desc_t*>* convoi_frame_t::waren_filter = NULL;
waytype_t convoi_frame_t::current_wt = waytype_t::ignore_wt;
uint32 convoi_frame_t::filter_flags = 0;
char convoi_frame_t::last_name_filter[256] = "";
char convoi_frame_t::name_filter[256] = "";


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
	if(current_wt &&  cnv->front()->get_desc()->get_waytype() != current_wt  ) {
		// not the right kind of vehivle
		return false;
	}

	if(  name_filter[0]!=0  &&  !utf8caseutf8(cnv->get_name(), name_filter)  ) {
		// not the right name
		return false;
	}

	if(  get_filter(convoi_filter_frame_t::special_filter)  ) {
		if ((!get_filter(convoi_filter_frame_t::noroute_filter)  || cnv->get_state() != convoi_t::NO_ROUTE) &&
				(!get_filter(convoi_filter_frame_t::stucked_filter)  || (cnv->get_state() != convoi_t::WAITING_FOR_CLEARANCE_TWO_MONTHS && cnv->get_state() != convoi_t::CAN_START_TWO_MONTHS)) &&
				(!get_filter(convoi_filter_frame_t::indepot_filter)  || !cnv->in_depot()) &&
				(!get_filter(convoi_filter_frame_t::noline_filter)   ||  cnv->get_line().is_bound()) &&
				(!get_filter(convoi_filter_frame_t::noschedule_filter)    ||  cnv->get_schedule()) &&
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
		case nach_name:
			result = strcmp(cnv1->get_internal_name(), cnv2->get_internal_name());
			break;
		case nach_gewinn:
			result = sgn(cnv1->get_jahresgewinn() - cnv2->get_jahresgewinn());
			break;
		case nach_typ:
			if(cnv1->get_vehicle_count()*cnv2->get_vehicle_count()>0) {
				vehicle_t const* const fahr1 = cnv1->front();
				vehicle_t const* const fahr2 = cnv2->front();

				result = fahr1->get_typ() - fahr2->get_typ();
				if(result == 0) {
					result = fahr1->get_cargo_type()->get_catg_index() - fahr2->get_cargo_type()->get_catg_index();
					if(result == 0) {
						result = fahr1->get_base_image() - fahr2->get_base_image();
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


void convoi_frame_t::fill_list()
{
	last_world_convois = welt->convoys().get_count();
	current_wt = tabs.get_active_tab_waytype();

	const bool all = owner->is_public_service();
	scrolly->clear_elements();
	FOR(vector_tpl<convoihandle_t>, const cnv, welt->convoys()) {
		if(  all  ||  cnv->get_owner()==owner  ) {
			if(  passes_filter( cnv )  ) {
				scrolly->new_component<gui_convoiinfo_t>( cnv );
			}
		}
	}
	sort_list();
}


void convoi_frame_t::sort_list()
{
	scrolly->sort();
	sortedby.set_selection(sortby);
}


void convoi_frame_t::sort_list( uint32 filter, const slist_tpl<const goods_desc_t *> *wares )
{
	waren_filter = wares;
	filter_flags = filter;
	fill_list();
}


convoi_frame_t::convoi_frame_t() :
	gui_frame_t( translator::translate("cl_title"), welt->get_active_player())
{
	owner = welt->get_active_player();

	set_table_layout(1,0);

	add_table(4,2);
	{
		new_component<gui_label_t>("Filter:");
		name_filter_input.set_text( name_filter, lengthof(name_filter) );
		add_component(&name_filter_input);
		filter_details.init(button_t::roundbox, "cl_btn_filter_settings");
		filter_details.add_listener(this);
		add_component(&filter_details);
		new_component<gui_fill_t>();

		new_component<gui_label_t>("cl_txt_sort");
		sortedby.set_unsorted(); // do not sort
		for(  size_t i=0;  i < lengthof(sort_text);  i++  ) {
			sortedby.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(sort_text[i]),SYSCOL_TEXT);
		}
		sortedby.set_selection(get_sortierung());
		sortedby.add_listener(this);
		add_component(&sortedby);

		sorteddir.init(button_t::sortarrow_automatic, NULL);
		sorteddir.add_listener(this);
		sorteddir.pressed = get_reverse();
		add_component(&sorteddir);

		new_component<gui_fill_t>();
	}
	end_table();

	scrolly = new gui_scrolled_convoy_list_t(this);
	scrolly->set_maximize( true );

	tabs.init_tabs(scrolly);
	for(  uint32 i = 0;  i < tabs.get_count();  i++  ) {
		if(current_wt == tabs.get_tab_waytype(i)) {
			tabs.set_active_tab_index(i);
			break;
		}
	}
	tabs.add_listener(this);
	add_component(&tabs);

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
	if(  comp == &tabs  ) {
		fill_list();
	}
	else if(  comp == &sortedby  ) {
		set_sortierung( (sort_mode_t)((get_sortierung() + 1) % SORT_MODES) );
		sort_list();
	}
	else if(  comp == &sorteddir  ) {
		set_reverse( !get_reverse() );
		sort_list();
	}
	else if(  comp == &filter_details  ) {
		if(  !destroy_win( magic_convoi_list_filter+owner->get_player_nr() )  ) {
			convoi_filter_frame_t *gui_cff = new convoi_filter_frame_t(owner, this);
			gui_cff->init(filter_flags, waren_filter);
			create_win( gui_cff, w_info, magic_convoi_list_filter+owner->get_player_nr() );

		}
	}
	return true;
}


void convoi_frame_t::draw(scr_coord pos, scr_size size)
{
	filter_details.pressed = win_get_magic( magic_convoi_list_filter+owner->get_player_nr() );
	sorteddir.pressed = get_reverse();

	if (last_world_convois != welt->convoys().get_count()  ||  strcmp(last_name_filter,name_filter)) {
		strcpy( last_name_filter, name_filter );
		// some deleted/ added => resort
		fill_list();
	}

	gui_frame_t::draw(pos, size);
}


void convoi_frame_t::rdwr( loadsave_t *file )
{
	scr_size size = get_windowsize();
	uint8 player_nr = owner->get_player_nr();
	sint16 sort_mode = sortby;

	file->rdwr_byte( player_nr );
	size.rdwr( file );
	tabs.rdwr( file );
	scrolly->rdwr( file );
	file->rdwr_str( name_filter, lengthof( name_filter ) );
	file->rdwr_short( sort_mode );
	file->rdwr_bool( sortreverse );
	file->rdwr_long( filter_flags );
	if( file->is_saving() ) {
		uint8 good_nr = get_filter(convoi_filter_frame_t::ware_filter) ? waren_filter->get_count() : 0;
		file->rdwr_byte( good_nr );
		if (good_nr > 0) {
			FOR( slist_tpl<const goods_desc_t *>, const i, *waren_filter ) {
				char *name = const_cast<char *>(i->get_name());
				file->rdwr_str(name,256);
			}
		}
	}
	else {
		uint8 good_nr;
		file->rdwr_byte( good_nr );
		if( good_nr > 0 ) {
			static slist_tpl<const goods_desc_t *>waren_filter_rd;
			for( sint16 i = 0; i < good_nr; i++ ) {
				char name[256];
				file->rdwr_str(name, lengthof(name));
				if (const goods_desc_t *gd = goods_manager_t::get_info(name)) {
					waren_filter_rd.append(gd);
				}
			}
			waren_filter = &waren_filter_rd;
		}

		sortby = (sort_mode_t)sort_mode;
		owner = welt->get_player( player_nr );
		win_set_magic(this, magic_convoi_list+player_nr );
		current_wt = tabs.get_active_tab_waytype();
		fill_list();
		set_windowsize( size );
	}
}
