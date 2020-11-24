/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "depotlist_frame.h"
#include "gui_theme.h"
#include "../simdepot.h"
#include "../simskin.h"
#include "../dataobj/translator.h"
#include "../descriptor/skin_desc.h"
#include "../simcity.h"

#include "../boden/wege/maglev.h"
#include "../boden/wege/monorail.h"
#include "../boden/wege/narrowgauge.h"
#include "../boden/wege/runway.h"
#include "../boden/wege/schiene.h"
#include "../boden/wege/strasse.h"
#include "../bauer/vehikelbauer.h"

#define IMG_WIDTH 20 // same as gui_tab_panel

enum sort_mode_t { by_waytype, by_convoys, by_vehicle, by_coord, by_region, SORT_MODES };

static waytype_t depot_types[MAX_DEPOT_TYPES] = { road_wt, track_wt, water_wt, air_wt, monorail_wt, maglev_wt, narrowgauge_wt, tram_wt };
static char const* const depot_type_texts[] = { "Truck", "Train", "Ship", "Air", "Monorail", "Maglev", "Narrowgauge", "Tram" };

uint8 depotlist_stats_t::sort_mode = by_waytype;
bool depotlist_stats_t::reverse = false;
uint16 depotlist_stats_t::name_width = 100;
uint8 depotlist_frame_t::depot_type_filter_bits = 255;

static karte_ptr_t welt;


bool depotlist_frame_t::is_available_wt(waytype_t wt) const
{
	switch (wt) {
		case maglev_wt:
			return maglev_t::default_maglev ? true : false;
		case monorail_wt:
			return monorail_t::default_monorail ? true : false;
		case track_wt:
			return schiene_t::default_schiene ? true : false;
		case tram_wt:
			return vehicle_builder_t::get_info(tram_wt).empty() ? false : true;
		case narrowgauge_wt:
			return narrowgauge_t::default_narrowgauge ? true : false;
		case road_wt:
			return strasse_t::default_strasse ? true : false;
		case water_wt:
			return vehicle_builder_t::get_info(water_wt).empty() ? false : true;
		case air_wt:
			return runway_t::default_runway ? true : false;
		default:
			return false;
	}
	return false;
}


const image_id depotlist_stats_t::get_depot_symbol(waytype_t wt)
{
	switch (wt) {
		case maglev_wt:
			return skinverwaltung_t::maglevhaltsymbol ? skinverwaltung_t::maglevhaltsymbol->get_image_id(0) : IMG_EMPTY;
		case monorail_wt:
			return skinverwaltung_t::monorailhaltsymbol ? skinverwaltung_t::monorailhaltsymbol->get_image_id(0) : IMG_EMPTY;
		case track_wt:
			return skinverwaltung_t::zughaltsymbol ? skinverwaltung_t::zughaltsymbol->get_image_id(0) : IMG_EMPTY;
		case tram_wt:
			return skinverwaltung_t::tramhaltsymbol ? skinverwaltung_t::tramhaltsymbol->get_image_id(0) : IMG_EMPTY;
		case narrowgauge_wt:
			return skinverwaltung_t::narrowgaugehaltsymbol ? skinverwaltung_t::narrowgaugehaltsymbol->get_image_id(0) : IMG_EMPTY;
		case road_wt:
			return skinverwaltung_t::autohaltsymbol ? skinverwaltung_t::autohaltsymbol->get_image_id(0) : IMG_EMPTY;
		case water_wt:
			return skinverwaltung_t::schiffshaltsymbol ? skinverwaltung_t::schiffshaltsymbol->get_image_id(0) : IMG_EMPTY;
		case air_wt:
			return skinverwaltung_t::airhaltsymbol ? skinverwaltung_t::airhaltsymbol->get_image_id(0) : IMG_EMPTY;
		default:
			return IMG_EMPTY;
	}
	return IMG_EMPTY;
}


depotlist_stats_t::depotlist_stats_t(depot_t *d)
{
	this->depot = d;
	// pos button
	set_table_layout(8,1);
	gotopos.set_typ(button_t::posbutton_automatic);
	gotopos.set_targetpos3d(depot->get_pos());
	add_component(&gotopos);
	const waytype_t wt = d->get_wegtyp();
	waytype_symbol.set_image(get_depot_symbol(wt), true);
	add_component(&waytype_symbol);

	const weg_t *w = welt->lookup(depot->get_pos())->get_weg(wt != tram_wt ? wt : track_wt);
	const bool way_electrified = w ? w->is_electrified() : false;
	if (way_electrified) {
		new_component<gui_image_t>()->set_image(skinverwaltung_t::electricity->get_image_id(0), true);
	}
	else {
		new_component<gui_margin_t>(skinverwaltung_t::electricity->get_image(0)->w);
	}

	lb_name.buf().append( translator::translate(depot->get_name()) );
	const scr_coord_val temp_w = proportional_string_width( translator::translate(depot->get_name()) );
	if (temp_w > name_width) {
		name_width = temp_w;
	}
	lb_name.set_fixed_width(name_width);
	add_component(&lb_name);

	lb_cnv_count.init(SYSCOL_TEXT, gui_label_t::right);
	lb_cnv_count.set_fixed_width(proportional_string_width(translator::translate("%d convois")));
	add_component(&lb_cnv_count);
	lb_vh_count.init(SYSCOL_TEXT, gui_label_t::right);
	lb_vh_count.set_fixed_width(proportional_string_width(translator::translate("%d vehicles")));
	add_component(&lb_vh_count);

	lb_region.buf().printf( " %s ", depot->get_pos().get_2d().get_fullstr() );
	stadt_t* c = welt->get_city(depot->get_pos().get_2d());
	if (c) {
		lb_region.buf().append("- ");
		lb_region.buf().append( c->get_name() );
	}
	if (!welt->get_settings().regions.empty()) {
		if (!c) {
			lb_region.buf().append("-");
		}
		lb_region.buf().printf(" (%s)", translator::translate(welt->get_region_name(depot->get_pos().get_2d()).c_str()));
	}
	lb_region.update();
	add_component(&lb_region);
	update_label();

	new_component<gui_fill_t>();
}


void depotlist_stats_t::update_label()
{
	lb_cnv_count.buf().clear();
	int cnvs = depot->convoi_count();
	if( cnvs == 0 ) {
//		buf.append( translator::translate( "no convois" ) );
	}
	else if( cnvs == 1 ) {
		lb_cnv_count.buf().append( translator::translate( "1 convoi" ) );
	}
	else {
		lb_cnv_count.buf().printf( translator::translate( "%d convois" ), cnvs );
	}
	lb_cnv_count.update();

	int vhls = depot->get_vehicle_list().get_count();
	lb_vh_count.buf().clear();
	if( vhls == 0 ) {
		//buf.append( translator::translate( "Keine Einzelfahrzeuge im Depot" ) );
	}
	else if( vhls == 1 ) {
		lb_vh_count.buf().append( translator::translate( "1 vehicle" ) );
	}
	else {
		lb_vh_count.buf().printf( translator::translate( "%d vehicles" ), vhls );
	}
	lb_vh_count.update();
}


void depotlist_stats_t::set_size(scr_size size)
{
	gui_aligned_container_t::set_size(size);
}


bool depotlist_stats_t::is_valid() const
{
	return depot_t::get_depot_list().is_contained(depot);
}


bool depotlist_stats_t::infowin_event(const event_t * ev)
{
	bool swallowed = gui_aligned_container_t::infowin_event(ev);

	if(  !swallowed  &&  IS_LEFTRELEASE(ev)  ) {
		depot->show_info();
		swallowed = true;
	}
	return swallowed;
}


void depotlist_stats_t::draw(scr_coord pos)
{
	update_label();

	gui_aligned_container_t::draw(pos);
}


bool depotlist_stats_t::compare(const gui_component_t *aa, const gui_component_t *bb)
{
	const depotlist_stats_t* fa = dynamic_cast<const depotlist_stats_t*>(aa);
	const depotlist_stats_t* fb = dynamic_cast<const depotlist_stats_t*>(bb);
	// good luck with mixed lists
	assert(fa != NULL  &&  fb != NULL);
	depot_t *a=fa->depot, *b=fb->depot;

	int cmp;
	switch( sort_mode ) {
	default:
	case by_coord:
		cmp = 0;
		break;

	case by_region:
		cmp = welt->get_region(a->get_pos().get_2d()) - welt->get_region(b->get_pos().get_2d());
		break;

	case by_waytype:
		cmp = a->get_waytype() - b->get_waytype();
		break;

	case by_convoys:
		cmp = a->convoi_count() - b->convoi_count();
		if( cmp == 0 ) {
			cmp = a->get_vehicle_list().get_count() - b->get_vehicle_list().get_count();
		}
		break;

	case by_vehicle:
		cmp = a->get_vehicle_list().get_count() - b->get_vehicle_list().get_count();
		if( cmp == 0 ) {
			cmp = a->convoi_count() - b->convoi_count();
		}
		break;

	}
	if (cmp == 0) {
		cmp = koord_distance( a->get_pos(), koord( 0, 0 ) ) - koord_distance( b->get_pos(), koord( 0, 0 ) );
		if( cmp == 0 ) {
			cmp = a->get_pos().x - b->get_pos().x;
		}
	}
	return reverse ? cmp > 0 : cmp < 0;
}




static const char *sort_text[SORT_MODES] = {
	"waytype",
	"convoys stored",
	"vehicles stored",
	"koord",
	"by_region"
};

depotlist_frame_t::depotlist_frame_t(player_t *player) :
	gui_frame_t( translator::translate("dp_title"), player ),
	scrolly(gui_scrolled_list_t::windowskin, depotlist_stats_t::compare)
{
	this->player = player;
	last_depot_count = 0;

	set_table_layout(1,0);

	add_table(2,1);
	{
		add_table(1, 2);
		{
			new_component<gui_label_t>("hl_txt_sort");
			add_table(4, 1);
			{
				sortedby.clear_elements();
				for (int i = 0; i < SORT_MODES; i++) {
					sortedby.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(sort_text[i]), SYSCOL_TEXT);
				}
				sortedby.set_selection(depotlist_stats_t::sort_mode);
				sortedby.add_listener(this);
				add_component(&sortedby);

				sort_asc.init(button_t::arrowup_state, "");
				sort_asc.set_tooltip(translator::translate("hl_btn_sort_asc"));
				sort_asc.add_listener(this);
				sort_asc.pressed = depotlist_stats_t::reverse;
				add_component(&sort_asc);

				sort_desc.init(button_t::arrowdown_state, "");
				sort_desc.set_tooltip(translator::translate("hl_btn_sort_desc"));
				sort_desc.add_listener(this);
				sort_desc.pressed = !depotlist_stats_t::reverse;
				add_component(&sort_desc);

				new_component<gui_margin_t>(D_H_SPACE*2);
			}
			end_table();
		}
		end_table();

		// left, filter cell
		add_table(1, 2);
		{
			// left top
			new_component<gui_label_t>("Filter:");

			// left bottom
			add_table(MAX_DEPOT_TYPES+1, 1)->set_spacing(scr_size(0,0));
			// All waytype button
			all_depot_types.init(button_t::roundbox_state, translator::translate("All"), scr_coord(0, 0), scr_size(proportional_string_width(translator::translate("All")) + gui_theme_t::gui_button_text_offset.w + gui_theme_t::gui_button_text_offset_right.x, D_BUTTON_HEIGHT));
			all_depot_types.pressed = (depot_type_filter_bits==255);
			all_depot_types.add_listener(this);
			add_component(&all_depot_types);

			// waytype buttons
			for (uint8 i = 0; i < MAX_DEPOT_TYPES; i++) {
				if (!is_available_wt(depot_types[i])) {
					new_component<gui_empty_t>()->set_visible(false);
					continue;
				}
				if (depot_types[i] != IMG_EMPTY) {
					filter_buttons[i].init(button_t::roundbox_state, NULL, scr_coord(0, 0), scr_size(10, D_BUTTON_HEIGHT));
					filter_buttons[i].set_image(depotlist_stats_t::get_depot_symbol(depot_types[i]));
					filter_buttons[i].set_tooltip(depot_type_texts[i]);
					filter_buttons[i].add_listener(this);
					filter_buttons[i].pressed = depot_type_filter_bits & (1<<i);
					add_component(filter_buttons + i);
				}
				else {
					new_component<gui_label_t>(depot_type_texts[i]);
				}
			}
			end_table();
		}
		end_table();
	}
	end_table();

	add_component(&scrolly);
	fill_list();

	set_resizemode(diagonal_resize);
	scrolly.set_maximize(true);
	reset_min_windowsize();
}


/**
 * This method is called if an action is triggered
 */
bool depotlist_frame_t::action_triggered( gui_action_creator_t *comp,value_t v)
{
	if(comp == &sortedby) {
		depotlist_stats_t::sort_mode = max(0, v.i);
		scrolly.sort(0);
	}
	else if (comp == &sort_asc || comp == &sort_desc) {
		depotlist_stats_t::reverse = !depotlist_stats_t::reverse;
		scrolly.sort(0);
		sort_asc.pressed = depotlist_stats_t::reverse;
		sort_desc.pressed = !depotlist_stats_t::reverse;
	}
	else if (comp == &all_depot_types) {
		all_depot_types.pressed ^= 1;
		if (all_depot_types.pressed) {
			for (int i = 0; i < MAX_DEPOT_TYPES; i++) {
				filter_buttons[i].pressed = true;
			}
			depot_type_filter_bits = 255;
		}
		else {
			for (int i = 0; i < MAX_DEPOT_TYPES; i++) {
				if (!is_available_wt(depot_types[i])) {
					continue;
				}
				depot_type_filter_bits &= ~(1<<i);
				filter_buttons[i].pressed = false;
			}
		}
		fill_list();
	}
	else {
		for (int i = 0; i < MAX_DEPOT_TYPES; i++) {
			if (comp == filter_buttons + i) {
				all_depot_types.pressed = false;
				filter_buttons[i].pressed ^= 1;
				if (filter_buttons[i].pressed) {
					depot_type_filter_bits |= (1<<i);
				}
				else {
					depot_type_filter_bits &= ~(1<<i);
				}

				if (depot_type_filter_bits == 255) {
					all_depot_types.pressed = true;
				}
				fill_list();
				return true;
			}
		}

	}
	return true;
}


void depotlist_frame_t::fill_list()
{
	scrolly.clear_elements();
	FOR(slist_tpl<depot_t*>, const depot, depot_t::get_depot_list()) {
		if( depot->get_owner() == player ) {
			for (int i = 0; i < MAX_DEPOT_TYPES; i++) {
				if (depot->get_waytype() == depot_types[i] && depot_type_filter_bits & (1<<i)) {
					scrolly.new_component<depotlist_stats_t>( depot );
				}
			}
		}
	}
	scrolly.sort(0);
	scrolly.set_size( scrolly.get_size());

	last_depot_count = depot_t::get_depot_list().get_count();
}


void depotlist_frame_t::draw(scr_coord pos, scr_size size)
{
	if(  depot_t::get_depot_list().get_count() != last_depot_count  ) {
		fill_list();
	}

	gui_frame_t::draw(pos,size);
}


void depotlist_frame_t::rdwr(loadsave_t *file)
{
	file->rdwr_byte(depot_type_filter_bits);
	file->rdwr_bool(sort_asc.pressed);
	uint8 s = depotlist_stats_t::sort_mode;
	file->rdwr_byte(s);

	if (file->is_loading()) {
		sortedby.set_selection(s);
		depotlist_stats_t::sort_mode = s;
		depotlist_stats_t::reverse = sort_asc.pressed;
		sort_desc.pressed = !sort_asc.pressed;
		for (int i = 0; i < MAX_DEPOT_TYPES; i++) {
			filter_buttons[i].pressed = depot_type_filter_bits & (1 << i);
		}
		fill_list();
		all_depot_types.pressed = (depot_type_filter_bits == 255);
	}
}
