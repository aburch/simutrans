// ****************** List of all vehicles ************************



#include "gui_theme.h"
#include "../descriptor/vehicle_desc.h"
#include "../bauer/vehikelbauer.h"
#include "../simskin.h"
#include "../simworld.h"
#include "../dataobj/translator.h"
#include "../descriptor/skin_desc.h"
#include "../descriptor/intro_dates.h"

#include "../display/simgraph.h"
#include "../utils/simstring.h"

#include "vehiclelist_frame.h"

enum sort_mode_t { best, by_intro, by_retire, by_power, by_capacity, by_name, SORT_MODES };

int vehiclelist_stats_t::sort_mode = by_intro;
bool vehiclelist_stats_t::reverse = false;

// for having uniform spaced columns
int vehiclelist_stats_t::img_width = 100;
int vehiclelist_stats_t::col1_width = 100;
int vehiclelist_stats_t::col2_width = 100;

static const char* engine_type_names[9] =
{
	"unknown",
	"steam",
	"diesel",
	"electric",
	"bio",
	"sail",
	"fuel_cell",
	"hydrogene",
	"battery"
};


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

	// name is the widest entry in column 1
	int dx = proportional_string_width( translator::translate( veh->get_name(), world()->get_settings().get_name_language_id() ) );
	if( veh->get_power() > 0 ) {
		char str[ 256 ];
		sprintf( str, " (%s)", translator::translate( engine_type_names[ veh->get_engine_type() + 1 ] ) );
		dx += proportional_string_width( str );
	}
	dx += D_H_SPACE;
	if( dx > col1_width ) {
		col1_width = dx;
	}

	// column 1
	part1.clear();
	if( sint64 fix_cost = world()->scale_with_month_length( veh->get_maintenance() ) ) {
		char tmp[ 128 ];
		money_to_string( tmp, veh->get_price() / 100.0, false );
		part1.printf( translator::translate( "Cost: %8s (%.2f$/km %.2f$/m)\n" ), tmp, veh->get_running_cost() / 100.0, fix_cost / 100.0 );
	}
	else {
		char tmp[ 128 ];
		money_to_string( tmp, veh->get_price() / 100.0, false );
		part1.printf( translator::translate( "Cost: %8s (%.2f$/km)\n" ), tmp, veh->get_running_cost() / 100.0 );
	}

	if( veh->get_capacity() > 0 ) { // must translate as "Capacity: %3d%s %s\n"
		part1.printf( translator::translate( "Capacity: %d%s %s\n" ),
			veh->get_capacity(),
			translator::translate( veh->get_freight_type()->get_mass() ),
			veh->get_freight_type()->get_catg() == 0 ? translator::translate( veh->get_freight_type()->get_name() ) : translator::translate( veh->get_freight_type()->get_catg_name() )
		);
	}
	else {
		part1.append( "\n" );
	}
	part1.printf( "%s %3d km/h\n", translator::translate( "Max. speed:" ), veh->get_topspeed() );
	if( veh->get_power() > 0 ) {
		if( veh->get_gear() != 64 ) {
			part1.printf( "%s %4d kW (x%0.2f)\n", translator::translate( "Power:" ), veh->get_power(), veh->get_gear() / 64.0 );
		}
		else {
			part1.printf( translator::translate( "Power: %4d kW\n" ), veh->get_power() );
		}
	}


	// column 2
	part2.clear();
	part2.printf( "%s %4.1ft\n", translator::translate( "Weight:" ), veh->get_weight() / 1000.0 );

	int len = part2.len();
	part2.printf( "%s %s - ", translator::translate( "Available:" ), translator::get_short_date( veh->get_intro_year_month() / 12, veh->get_intro_year_month() % 12 ) );
	if( veh->get_retire_year_month() != DEFAULT_RETIRE_DATE * 12 ) {
		part2.printf( "%s\n", translator::get_short_date( veh->get_retire_year_month() / 12, veh->get_retire_year_month() % 12 ) );
	}
	else {
		part2.append( "*\n" );
	}

	dx = proportional_string_width( part2.get_str() + len );
	if( dx > col2_width ) {
		col2_width = dx;
	}

	if( char const* const copyright = veh->get_copyright() ) {
		len = part2.len();
		part2.printf( translator::translate( "Constructed by %s" ), copyright );
		dx = proportional_string_width( part2.get_str()+len );
		if( dx > col2_width ) {
			col2_width = dx;
		}
	}
	part2.append( "\n" );
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
	int dx = display_proportional_rgb( 
		offset.x, offset.y, 
		translator::translate( veh->get_name(), world()->get_settings().get_name_language_id() ), 
		ALIGN_LEFT|DT_CLIP,
		veh->is_future(month) ? SYSCOL_TEXT_HIGHLIGHT : (veh->is_available(month) ? SYSCOL_TEXT : color_idx_to_rgb(COL_BLUE)), 
		false
	);
	if( veh->get_power() > 0 ) {
		char str[ 256 ];
		sprintf( str, " (%s)", translator::translate( engine_type_names[ veh->get_engine_type() + 1 ] ) );
		display_proportional_rgb( offset.x+dx, offset.y, str, ALIGN_LEFT|DT_CLIP, SYSCOL_TEXT, false );
	}

	int yyy = offset.y + LINESPACE;
	display_multiline_text_rgb( offset.x, yyy, part1, SYSCOL_TEXT );

	display_multiline_text_rgb( offset.x + col1_width, yyy + LINESPACE, part2, SYSCOL_TEXT );
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
		cmp = a->get_retire_year_month() - b->get_retire_year_month();
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
	current_wt = waytype_t::any_wt;

	set_table_layout(1,0);
	new_component<gui_label_t>("hl_txt_sort");

	add_table(3,0);
	{
		sortedby.init( button_t::roundbox, sort_text[ vehiclelist_stats_t::sort_mode ] );
		sortedby.add_listener( this );
		add_component( &sortedby );

		sorteddir.init( button_t::roundbox, vehiclelist_stats_t::reverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc" );
		sorteddir.add_listener( this );
		add_component( &sorteddir );

		bt_obsolete.init( button_t::square_state, "Show obsolete" );
		bt_obsolete.add_listener( this );
		add_component( &bt_obsolete );
	}
	end_table();

	max_idx = 0;
	tabs_to_wt[max_idx++] = waytype_t::any_wt;
	tabs.add_tab(&scrolly, translator::translate("All"));
	// now add all specific tabs
	if(  !vehicle_builder_t::get_info(maglev_wt).empty()  ) {
		tabs.add_tab(&scrolly, translator::translate("Maglev"), skinverwaltung_t::maglevhaltsymbol, translator::translate("Maglev"));
		tabs_to_wt[max_idx++] = waytype_t::maglev_wt;
	}
	if(  !vehicle_builder_t::get_info(monorail_wt).empty()  ) {
		tabs.add_tab(&scrolly, translator::translate("Monorail"), skinverwaltung_t::monorailhaltsymbol, translator::translate("Monorail"));
		tabs_to_wt[max_idx++] = waytype_t::monorail_wt;
	}
	if(  !vehicle_builder_t::get_info(track_wt).empty()  ) {
		tabs.add_tab(&scrolly, translator::translate("Train"), skinverwaltung_t::zughaltsymbol, translator::translate("Train"));
		tabs_to_wt[max_idx++] = waytype_t::track_wt;
	}
	if(  !vehicle_builder_t::get_info(narrowgauge_wt).empty()  ) {
		tabs.add_tab(&scrolly, translator::translate("Narrowgauge"), skinverwaltung_t::narrowgaugehaltsymbol, translator::translate("Narrowgauge"));
		tabs_to_wt[max_idx++] = waytype_t::narrowgauge_wt;
	}
	if(  !vehicle_builder_t::get_info(tram_wt).empty()  ) {
		tabs.add_tab(&scrolly, translator::translate("Tram"), skinverwaltung_t::tramhaltsymbol, translator::translate("Tram"));
		tabs_to_wt[max_idx++] = waytype_t::tram_wt;
	}
	if(  !vehicle_builder_t::get_info(road_wt).empty()  ) {
		tabs.add_tab(&scrolly, translator::translate("Truck"), skinverwaltung_t::autohaltsymbol, translator::translate("Truck"));
		tabs_to_wt[max_idx++] = waytype_t::road_wt;
	}
	if(  !vehicle_builder_t::get_info(water_wt).empty()  ) {
		tabs.add_tab(&scrolly, translator::translate("Ship"), skinverwaltung_t::schiffshaltsymbol, translator::translate("Ship"));
		tabs_to_wt[max_idx++] = waytype_t::water_wt;
	}
	if( !vehicle_builder_t::get_info(air_wt).empty()  ) {
		tabs.add_tab(&scrolly, translator::translate("Air"), skinverwaltung_t::airhaltsymbol, translator::translate("Air"));
		tabs_to_wt[max_idx++] = waytype_t::air_wt;
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
 * @author Markus Weber/Volker Meyer
 */
bool vehiclelist_frame_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if(comp == &sortedby) {
		vehiclelist_stats_t::sort_mode = (vehiclelist_stats_t::sort_mode + 1) % SORT_MODES;
		sortedby.set_text(sort_text[vehiclelist_stats_t::sort_mode]);
		if( vehiclelist_stats_t::sort_mode == 0 ) { 
			fill_list();	// using sorting from vehikelbauer
		}
		else {
			scrolly.sort( 0 );
		}
	}
	else if(comp == &sorteddir) {
		vehiclelist_stats_t::reverse = !vehiclelist_stats_t::reverse;
		sorteddir.set_text( vehiclelist_stats_t::reverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
		scrolly.sort(0);
	}
	else if(comp == &bt_obsolete) {
		bt_obsolete.pressed ^= 1;
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
	vehiclelist_stats_t::img_width = 32; // reset col1 width
	vehiclelist_stats_t::col1_width = 100; // reset col1 width
	vehiclelist_stats_t::col2_width = 100; // reset col2 width
	uint32 month = world()->get_current_month();
	if(  current_wt == any_wt  ) {
		// adding all vehiles, i.e. iterate over all available waytypes
		for(  int i=1;  i<max_idx;  i++  ) {
			FOR(slist_tpl<vehicle_desc_t const*>, const veh, vehicle_builder_t::get_info(tabs_to_wt[i])) {
				if(  bt_obsolete.pressed  ||  !veh->is_retired(month)  ) {
					scrolly.new_component<vehiclelist_stats_t>( veh );
				}
			}
		}
	}
	else {
		FOR(slist_tpl<vehicle_desc_t const*>, const veh, vehicle_builder_t::get_info(current_wt)) {
			if(  bt_obsolete.pressed  ||  !veh->is_retired(month)  ) {
				scrolly.new_component<vehiclelist_stats_t>( veh );
			}
		}
	}
	if( vehiclelist_stats_t::sort_mode != 0 ) {
		scrolly.sort(0);
	}
	else {
		scrolly.set_size( scrolly.get_size() );
	}
}
