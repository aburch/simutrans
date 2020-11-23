// ****************** List of all vehicles ************************


#include "gui_theme.h"
#include "vehiclelist_frame.h"

#include "../bauer/goods_manager.h"
#include "../bauer/vehikelbauer.h"

#include "../simskin.h"
#include "../simworld.h"

#include "../display/simgraph.h"

#include "../dataobj/translator.h"

#include "../descriptor/goods_desc.h"
#include "../descriptor/intro_dates.h"
#include "../descriptor/skin_desc.h"
#include "../descriptor/vehicle_desc.h"

#include "../utils/simstring.h"


enum sort_mode_t { best, by_intro, by_retire, by_power, by_capacity, by_name, SORT_MODES };

int vehiclelist_stats_t::sort_mode = by_intro;
bool vehiclelist_stats_t::reverse = false;

// for having uniform spaced columns
int vehiclelist_stats_t::img_width = 100;


vehiclelist_stats_t::vehiclelist_stats_t(const vehicle_desc_t *v)
{
	veh = v;

	// width of image
	scr_coord_val x, y, w, h;
	const image_id image = veh->get_image_id( ribi_t::dir_south, veh->get_freight_type() );
	display_get_base_image_offset(image, &x, &y, &w, &h );
	if( w > img_width ) {
		img_width = w + D_H_SPACE;
	}
	height = h;

	// name is the widest entry in column 1
	name_width = proportional_string_width( translator::translate( veh->get_name(), world()->get_settings().get_name_language_id() ) );
	if( veh->get_power() > 0 ) {
		char str[ 256 ];
		const uint8 et = (uint8)veh->get_engine_type() + 1;
		sprintf( str, " (%s)", translator::translate( vehicle_builder_t::engine_type_names[et] ) );
		name_width += proportional_string_width( str );
	}

	// column 1
	part1.clear();
	part1.append(" ");
	if( sint64 fix_cost = world()->scale_with_month_length( veh->get_maintenance() ) ) {
		char tmp[ 128 ];
		money_to_string( tmp, veh->get_value() / 100.0, false );
		part1.printf( translator::translate( "Cost: %8s (%.2f$/km %.2f$/m)\n" ), tmp, veh->get_running_cost() / 100.0, fix_cost / 100.0 );
	}
	else {
		char tmp[ 128 ];
		money_to_string( tmp, veh->get_value() / 100.0, false );
		part1.printf( translator::translate( "Cost: %8s (%.2f$/km)\n" ), tmp, veh->get_running_cost() / 100.0 );
	}
	if( veh->get_capacity() > 0 ) { // must translate as "Capacity: %3d%s %s\n"
		part1.append(" ");
		part1.printf( translator::translate( "Capacity: %d%s %s\n" ),
			veh->get_capacity(),
			translator::translate( veh->get_freight_type()->get_mass() ),
			veh->get_freight_type()->get_catg() == 0 ? translator::translate( veh->get_freight_type()->get_name() ) : translator::translate( veh->get_freight_type()->get_catg_name() )
		);
	}
	part1.printf( " %s %3d km/h\n", translator::translate( "Max. speed:" ), veh->get_topspeed() );
	if( veh->get_power() > 0 ) {
		part1.append(" ");
		if( veh->get_gear() != 64 ) {
			part1.printf( "%s %4d kW (x%0.2f)\n", translator::translate( "Power:" ), veh->get_power(), veh->get_gear() / 64.0 );
		}
		else {
			part1.printf( translator::translate( "Power: %4d kW\n" ), veh->get_power() );
		}
	}
	int text1w, text1h;
	display_calc_proportional_multiline_string_len_width( text1w, text1h, part1, part1.len() );
	col1_width = text1w + D_H_SPACE;

	// column 2
	part2.clear();
	part2.printf( "%s %4.1ft\n", translator::translate( "Weight:" ), veh->get_weight() / 1000.0 );
	part2.printf( "%s %s - ", translator::translate( "Available:" ), translator::get_short_date( veh->get_intro_year_month() / 12, veh->get_intro_year_month() % 12 ) );
	if (veh->get_retire_year_month() != DEFAULT_RETIRE_DATE * 12 &&
		(((!world()->get_settings().get_show_future_vehicle_info() && veh->will_end_prodection_soon(world()->get_timeline_year_month()))
			|| world()->get_settings().get_show_future_vehicle_info()
			|| veh->is_retired(world()->get_timeline_year_month()))))
	{
		part2.printf( "%s", translator::get_short_date( veh->get_retire_year_month() / 12, veh->get_retire_year_month() % 12 ) );
	}
	if( char const* const copyright = veh->get_copyright() ) {
		part2.append("\n");
		part2.printf( translator::translate( "Constructed by %s" ), copyright );
	}
	int text2w, text2h;
	display_calc_proportional_multiline_string_len_width( text2w, text2h, part2, part2.len() );
	col2_width = text2w;

	height = max( height, max( text1h, text2h ) + LINESPACE );
}


void vehiclelist_stats_t::draw( scr_coord offset )
{
	uint32 month = world()->get_current_month();
	offset += pos;

	offset.x += D_MARGIN_LEFT;
	scr_coord_val x, y, w, h;
	const image_id image = veh->get_image_id( ribi_t::dir_south, veh->get_freight_type() );
	display_get_base_image_offset(image, &x, &y, &w, &h );
	display_base_img(image, offset.x - x, offset.y - y, world()->get_active_player_nr(), false, true);

	// first name
	offset.x += img_width;
	PIXVAL name_colval = SYSCOL_TEXT;
	if (veh->is_future(month)) {
		name_colval = SYSCOL_EMPTY;
	}
	else if (veh->is_available_only_as_upgrade()) {
		name_colval = COL_UPGRADEABLE;
	}
	else if (veh->is_obsolete(month, world())) {
		name_colval = COL_OBSOLETE;
	}
	else if (veh->is_retired(month)) {
		name_colval = COL_OUT_OF_PRODUCTION;
	}
	int dx = display_proportional_rgb(
		offset.x, offset.y,
		translator::translate( veh->get_name(), world()->get_settings().get_name_language_id() ),
		ALIGN_LEFT|DT_CLIP,
		name_colval,
		false
	);
	if( veh->get_power() > 0 ) {
		char str[ 256 ];
		const uint8 et = (uint8)veh->get_engine_type() + 1;
		sprintf( str, " (%s)", translator::translate( vehicle_builder_t::engine_type_names[et] ) );
		display_proportional_rgb( offset.x+dx, offset.y, str, ALIGN_LEFT|DT_CLIP, SYSCOL_TEXT, false );
	}

	int yyy = offset.y + LINESPACE;
	display_multiline_text_rgb( offset.x, yyy, part1, SYSCOL_TEXT );

	display_multiline_text_rgb( offset.x + col1_width, yyy, part2, SYSCOL_TEXT );
}

const char *vehiclelist_stats_t::get_text() const
{
	return translator::translate( veh->get_name() );
}

bool vehiclelist_stats_t::compare(const gui_component_t *aa, const gui_component_t *bb)
{
	const vehiclelist_stats_t* fa = dynamic_cast<const vehiclelist_stats_t*>(aa);
	const vehiclelist_stats_t* fb = dynamic_cast<const vehiclelist_stats_t*>(bb);
	// good luck with mixed lists
	assert(fa != NULL  &&  fb != NULL);
	const vehicle_desc_t*a=fa->veh, *b=fb->veh;

	int cmp = 0;
	switch( sort_mode ) {
	default:
	case best:
		return 0;

	case by_intro:
		cmp = a->get_intro_year_month() - b->get_intro_year_month();
		break;

	case by_retire:
		if (world()->get_settings().get_show_future_vehicle_info()) {
			cmp = a->get_retire_year_month() - b->get_retire_year_month();
		}
		else {
			uint32 month = world()->get_current_month();
			int a_date = a->is_future(month) ? 65535 : a->get_retire_year_month();
			int b_date = b->is_future(month) ? 65535 : b->get_retire_year_month();
			cmp = a_date - b_date;
		}
		break;

	case by_power:
		cmp = a->get_power() - b->get_power();
		break;

	case by_capacity:
		cmp = a->get_capacity() - b->get_capacity();
		break;

	case by_name:
		break;

	}
	if(  cmp == 0  ) {
		cmp = strcmp( translator::translate(a->get_name()), translator::translate(b->get_name()) );
	}
	return reverse ? cmp > 0 : cmp < 0;
}



static const char *sort_text[SORT_MODES] = {
	"Unsorted",
	"Intro. date:",
	"Retire. date:",
	"Power:",
	"Capacity:",
	"Name"
};

vehiclelist_frame_t::vehiclelist_frame_t() :
	gui_frame_t( translator::translate("vh_title") ),
	scrolly(gui_scrolled_list_t::windowskin, vehiclelist_stats_t::compare)
{
	current_wt = any_wt;
	scrolly.set_cmp( vehiclelist_stats_t::compare );

	set_table_layout(1,0);

	add_table(2,0);
	{
		// left column
		add_table(1, 3);
		{
			add_table(1, 2);
			{
				new_component<gui_label_t>( "hl_txt_sort" );

				add_table(4, 1);
				{
					sort_by.clear_elements();
					for(int i = 0; i < SORT_MODES; i++) {
						sort_by.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(sort_text[i]), SYSCOL_TEXT);
					}
					sort_by.set_selection( vehiclelist_stats_t::sort_mode );
					sort_by.set_width_fixed( true );
					sort_by.set_size(scr_size(D_BUTTON_WIDTH*1.5, D_EDIT_HEIGHT));
					sort_by.add_listener( this );
					add_component( &sort_by );

					sort_asc.init(button_t::arrowup_state, "");
					sort_asc.set_tooltip(translator::translate("hl_btn_sort_asc"));
					sort_asc.add_listener(this);
					sort_asc.pressed = vehiclelist_stats_t::reverse;
					add_component(&sort_asc);

					sort_desc.init(button_t::arrowdown_state, "");
					sort_desc.set_tooltip(translator::translate("hl_btn_sort_desc"));
					sort_desc.add_listener(this);
					sort_desc.pressed = !vehiclelist_stats_t::reverse;
					add_component(&sort_desc);
					new_component<gui_margin_t>(LINESPACE);
				}
				end_table();

			}
			end_table();

			new_component<gui_margin_t>(0,LINESPACE);

			lb_count.set_size(scr_size(D_LABEL_WIDTH*1.5, D_LABEL_HEIGHT));
			add_component( &lb_count );
		}
		end_table();

		// right column
		add_table(1, 3);
		{
			add_table(2,1);
			{
				new_component<gui_label_t>("Filter:");
				add_table(1, 2);
				{
					engine_filter.clear_elements();
					engine_filter.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("All"), SYSCOL_TEXT);
					for (int i = 0; i < vehicle_desc_t::MAX_TRACTION_TYPE; i++) {
						engine_filter.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(vehicle_desc_t::get_engine_type((vehicle_desc_t::engine_t)i)), SYSCOL_TEXT);
					}
					engine_filter.set_selection( 0 );
					engine_filter.add_listener( this );
					add_component( &engine_filter );

					ware_filter.clear_elements();
					ware_filter.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("All"), SYSCOL_TEXT);
					idx_to_ware.append(NULL);
					for (int i = 0; i < goods_manager_t::get_count(); i++) {
						const goods_desc_t *ware = goods_manager_t::get_info(i);
						if (ware == goods_manager_t::none) {
							continue;
						}
						if (ware->get_catg() == 0) {
							ware_filter.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(ware->get_name()), SYSCOL_TEXT);
							idx_to_ware.append(ware);
						}
					}
					// now add other good categories
					for (int i = 1; i < goods_manager_t::get_max_catg_index(); i++) {
						const goods_desc_t *ware = goods_manager_t::get_info_catg(i);
						if (ware->get_catg() != 0) {
							ware_filter.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(ware->get_catg_name()), SYSCOL_TEXT);
							idx_to_ware.append(ware);
						}
					}
					ware_filter.set_selection(0);
					ware_filter.add_listener(this);
					add_component(&ware_filter);
				}
				end_table();
			}
			end_table();

			add_table(2, 2);
			{
				bt_obsolete.init(button_t::square_state, "Show obsolete");
				bt_obsolete.add_listener(this);
				add_component(&bt_obsolete);

				bt_outdated.init(button_t::square_state, "Show outdated");
				bt_outdated.add_listener(this);
				add_component(&bt_outdated);

				bt_future.init(button_t::square_state, "Show future");
				bt_future.add_listener(this);
				if (!welt->get_settings().get_show_future_vehicle_info()) {
					bt_future.set_visible(false);
				}
				bt_future.pressed = false;
				add_component(&bt_future);

				bt_only_upgrade.init(button_t::square_state, "Show upgraded");
				bt_only_upgrade.add_listener(this);
				add_component(&bt_only_upgrade);
			}
			end_table();
		}
		end_table();
	}
	end_table();

	max_idx = 0;
	tabs_to_wt[max_idx++] = any_wt;
	tabs.add_tab(&scrolly, translator::translate("All"));
	// now add all specific tabs
	if(  !vehicle_builder_t::get_info(maglev_wt).empty()  ) {
		tabs.add_tab(&scrolly, translator::translate("Maglev"), skinverwaltung_t::maglevhaltsymbol, translator::translate("Maglev"));
		tabs_to_wt[max_idx++] = maglev_wt;
	}
	if(  !vehicle_builder_t::get_info(monorail_wt).empty()  ) {
		tabs.add_tab(&scrolly, translator::translate("Monorail"), skinverwaltung_t::monorailhaltsymbol, translator::translate("Monorail"));
		tabs_to_wt[max_idx++] = monorail_wt;
	}
	if(  !vehicle_builder_t::get_info(track_wt).empty()  ) {
		tabs.add_tab(&scrolly, translator::translate("Train"), skinverwaltung_t::zughaltsymbol, translator::translate("Train"));
		tabs_to_wt[max_idx++] = track_wt;
	}
	if(  !vehicle_builder_t::get_info(narrowgauge_wt).empty()  ) {
		tabs.add_tab(&scrolly, translator::translate("Narrowgauge"), skinverwaltung_t::narrowgaugehaltsymbol, translator::translate("Narrowgauge"));
		tabs_to_wt[max_idx++] = narrowgauge_wt;
	}
	if(  !vehicle_builder_t::get_info(tram_wt).empty()  ) {
		tabs.add_tab(&scrolly, translator::translate("Tram"), skinverwaltung_t::tramhaltsymbol, translator::translate("Tram"));
		tabs_to_wt[max_idx++] = tram_wt;
	}
	if(  !vehicle_builder_t::get_info(road_wt).empty()  ) {
		tabs.add_tab(&scrolly, translator::translate("Truck"), skinverwaltung_t::autohaltsymbol, translator::translate("Truck"));
		tabs_to_wt[max_idx++] = road_wt;
	}
	if(  !vehicle_builder_t::get_info(water_wt).empty()  ) {
		tabs.add_tab(&scrolly, translator::translate("Ship"), skinverwaltung_t::schiffshaltsymbol, translator::translate("Ship"));
		tabs_to_wt[max_idx++] = water_wt;
	}
	if( !vehicle_builder_t::get_info(air_wt).empty()  ) {
		tabs.add_tab(&scrolly, translator::translate("Air"), skinverwaltung_t::airhaltsymbol, translator::translate("Air"));
		tabs_to_wt[max_idx++] = air_wt;
	}
	tabs.add_listener(this);
	add_component(&tabs);

	fill_list();

	set_resizemode(diagonal_resize);
	scrolly.set_maximize(true);
	reset_min_windowsize();
}


/**
 * This method is called if an action is triggered
 */
bool vehiclelist_frame_t::action_triggered( gui_action_creator_t *comp,value_t v)
{
	if(comp == &sort_by) {
		vehiclelist_stats_t::sort_mode = max(0,v.i);
		fill_list();
	}
	else if(comp == &engine_filter) {
		fill_list();
	}
	else if(comp == &ware_filter) {
		fill_list();
	}
	else if (comp == &sort_asc || comp == &sort_desc) {
		vehiclelist_stats_t::reverse = !vehiclelist_stats_t::reverse;
		scrolly.sort(0);
		sort_asc.pressed = vehiclelist_stats_t::reverse;
		sort_desc.pressed = !vehiclelist_stats_t::reverse;
	}
	else if(comp == &bt_obsolete) {
		bt_obsolete.pressed ^= 1;
		fill_list();
	}
	else if (comp == &bt_outdated) {
		bt_outdated.pressed ^= 1;
		fill_list();
	}
	else if (comp == &bt_future) {
		bt_future.pressed ^= 1;
		fill_list();
	}
	else if (comp == &bt_only_upgrade) {
		bt_only_upgrade.pressed ^= 1;
		fill_list();
	}
	else if(comp == &tabs) {
		int const tab = tabs.get_active_tab_index();
		if(  current_wt != tabs_to_wt[ tab ]  ) {
			current_wt = tabs_to_wt[ tab ];
			fill_list();
		}
	}
	return true;
}


void vehiclelist_frame_t::fill_list()
{
	scrolly.clear_elements();
	count = 0;
	vehiclelist_stats_t::img_width = 32; // reset col1 width
	uint32 month = world()->get_current_month();
	const goods_desc_t *ware = idx_to_ware[ max( 0, ware_filter.get_selection() ) ];
	if(  current_wt == any_wt  ) {
		// adding all vehiles, i.e. iterate over all available waytypes
		for(  int i=1;  i<max_idx;  i++  ) {
			FOR(slist_tpl<vehicle_desc_t *>, const veh, vehicle_builder_t::get_info(tabs_to_wt[i])) {
				// engine type filter
				if(engine_filter.get_selection() > 0 && ((uint8)veh->get_engine_type()+1) != engine_filter.get_selection()) {
					continue;
				}

				// timeline status filter
				if (!bt_only_upgrade.pressed && veh->is_available_only_as_upgrade()) {
					continue;
				}
				if (!bt_outdated.pressed && (veh->is_retired(month) && !veh->is_obsolete(month, world()))) {
					continue;
				}
				if (!bt_obsolete.pressed && veh->is_obsolete(month, world())) {
					continue;
				}
				if (!bt_future.pressed && veh->is_future(month)) {
					continue;
				}
				// goods category filter
				if( ware ) {
					const goods_desc_t *vware = veh->get_freight_type();
					if(  (ware->get_catg_index() > 0  &&  vware->get_catg_index() == ware->get_catg_index())  ||  vware->get_index() == ware->get_index()  ) {
						scrolly.new_component<vehiclelist_stats_t>( veh );
						count++;
					}
				}
				else {
					scrolly.new_component<vehiclelist_stats_t>( veh );
					count++;
				}
			}
		}
	}
	else {
		FOR(slist_tpl<vehicle_desc_t *>, const veh, vehicle_builder_t::get_info(current_wt)) {
			// engine type filter
			if (engine_filter.get_selection() > 0 && ((uint8)veh->get_engine_type() + 1) != engine_filter.get_selection()) {
				continue;
			}

			// timeline status filter
			if (!bt_only_upgrade.pressed && veh->is_available_only_as_upgrade()) {
				continue;
			}
			if (!bt_outdated.pressed && (veh->is_retired(month) && !veh->is_obsolete(month, world()))) {
				continue;
			}
			if (!bt_obsolete.pressed && veh->is_obsolete(month, world())) {
				continue;
			}
			if (!bt_future.pressed && veh->is_future(month)) {
				continue;
			}
			// goods category filter
			if( ware ) {
				const goods_desc_t *vware = veh->get_freight_type();
				if(  (ware->get_catg_index() > 0  &&  vware->get_catg_index() == ware->get_catg_index())  ||  vware->get_index() == ware->get_index()  ) {
					scrolly.new_component<vehiclelist_stats_t>( veh );
					count++;
				}
			}
			else {
				scrolly.new_component<vehiclelist_stats_t>( veh );
				count++;
			}
		}
	}
	if( vehiclelist_stats_t::sort_mode != 0 ) {
		scrolly.sort(0);
	}
	else {
		scrolly.set_size( scrolly.get_size() );
	}
	switch (count) {
		case 0:
			lb_count.buf().printf(translator::translate("Vehicle not found"), count);
			lb_count.set_color(SYSCOL_EMPTY);
			break;
		case 1:
			lb_count.buf().printf(translator::translate("One vehicle found"), count);
			lb_count.set_color(SYSCOL_TEXT);
			break;
		default:
			lb_count.buf().printf(translator::translate("%u vehicles found"), count);
			lb_count.set_color(SYSCOL_TEXT);
			break;
	}
	lb_count.update();
}
