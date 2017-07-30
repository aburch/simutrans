/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Window with destination information for a stop
 * @author Hj. Malthaner
 */

#include "halt_detail.h"
#include "halt_info.h"
#include "../simworld.h"
#include "../simware.h"
#include "../simcolor.h"
#include "../simconvoi.h"
#include "../simintr.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../simmenu.h"
#include "../simskin.h"
#include "../simline.h"

#include "../freight_list_sorter.h"

#include "../dataobj/schedule.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"

#include "../vehicle/simvehicle.h"

#include "../utils/simstring.h"

#include "../descriptor/skin_desc.h"





static const char *sort_text[halt_info_t::SORT_MODES] = {
	"Zielort",
	"via",
	"via Menge",
	"Menge",
	"origin (detail)",
	"origin (amount)",
	"destination (detail)"/*,
	"transferring time"*/
};

static const char cost_type[MAX_HALT_COST][64] =
{
	"Happy",
	"Unhappy",
	"No Route",
	"Too slow",
	"hl_btn_sort_waiting",
	"Arrived",
	"Departed",
	"Convoys"
};

static const char cost_tooltip[MAX_HALT_COST][128] =
{
	"The number of passengers who have travelled successfully from this stop",
	"The number of passengers who have left because of overcrowding or excess waiting",
	"The number of passengers who could not find a route to their destination",
	"The number of passengers who decline to travel because the journey would take too long",
	"The number of passengers/units of mail/goods waiting at this stop",
	"The number of passengers/units of mail/goods that have arrived at this stop",
	"The number of passengers/units of mail/goods that have departed from this stop",
	"The number of convoys that have serviced this stop"
};

const uint8 index_of_haltinfo[MAX_HALT_COST] = {
	HALT_HAPPY,
	HALT_UNHAPPY,
	HALT_NOROUTE,
	HALT_TOO_SLOW,
	HALT_WAITING,
	HALT_ARRIVED,
	HALT_DEPARTED,
	HALT_CONVOIS_ARRIVED
};

#define COL_HAPPY COL_WHITE
#define COL_UNHAPPY COL_RED
#define COL_WAITING COL_YELLOW
#define COL_ARRIVED COL_DARK_ORANGE
#define COL_DEPARTED COL_DARK_YELLOW

const int cost_type_color[MAX_HALT_COST] =
{
	COL_HAPPY,
	COL_UNHAPPY,
	COL_NO_ROUTE,
	COL_PURPLE,
	COL_WAITING,
	COL_ARRIVED,
	COL_DEPARTED,
	COL_COUNVOI_COUNT
};

#define L_BUTTON_WIDTH button_size.w
#define L_CHART_INDENT (66)

halt_info_t::halt_info_t(halthandle_t halt) :
		gui_frame_t( halt->get_name(), halt->get_owner() ),
		scrolly(&text),
		text(&freight_info),
		sort_label(translator::translate("Hier warten/lagern:")),
		view(halt->get_basis_pos3d(), scr_size(max(64, get_base_tile_raster_width()), max(56, get_base_tile_raster_width() * 7 / 8)))
{
	this->halt = halt;
	halt->set_sortby( env_t::default_sortmode );

	if(halt->get_station_type() & haltestelle_t::airstop && halt->has_no_control_tower())
	{
		sprintf(modified_name, "%s [%s]", halt->get_name(), translator::translate("NO CONTROL TOWER"));
		set_name(modified_name);
	}

	const scr_size button_size(max(D_BUTTON_WIDTH, 100), D_BUTTON_HEIGHT);
	scr_coord cursor(D_MARGIN_LEFT, D_MARGIN_TOP);

	const sint16 offset_below_viewport = 21 + view.get_size().h;
	const sint16 client_width = 3*(L_BUTTON_WIDTH + D_H_SPACE) + max( L_BUTTON_WIDTH, view.get_size().w );
	const sint16 total_width = D_MARGIN_LEFT + client_width + D_MARGIN_RIGHT;

	input.set_pos(cursor);
	input.set_size(scr_size(client_width, D_EDIT_HEIGHT));
	tstrncpy(edit_name, halt->get_name(), lengthof(edit_name));
	input.set_text(edit_name, lengthof(edit_name));
	input.add_listener(this);
	add_component(&input);
	cursor.y += D_EDIT_HEIGHT + D_V_SPACE;

	view.set_pos(cursor); // will be right aligned in set_windowsize()
	add_component(&view);
	cursor.y += view.get_size().h + D_V_SPACE;

	scr_coord old_cursor = cursor;

	// chart
	chart.set_pos(cursor + scr_coord(L_CHART_INDENT,0));
	chart.set_size(scr_size(client_width - L_CHART_INDENT, 100));
	chart.set_dimension(12, 10000);
	chart.set_visible(false);
	chart.set_background(SYSCOL_CHART_BACKGROUND);
	chart.set_ltr(env_t::left_to_right_graphs);
	add_component(&chart);
	cursor.y += 100 + 20 + D_V_SPACE;

	floating_cursor_t auto_cursor(cursor, D_MARGIN_LEFT, total_width - D_MARGIN_RIGHT);
	for (int cost = 0; cost<MAX_HALT_COST; cost++) {
		chart.add_curve(cost_type_color[cost], halt->get_finance_history(), MAX_HALT_COST, index_of_haltinfo[cost], MAX_MONTHS, 0, false, true, 0);
		filterButtons[cost].init(button_t::box_state, cost_type[cost], auto_cursor.next_pos(button_size), button_size);
		filterButtons[cost].add_listener(this);
		filterButtons[cost].background_color = cost_type_color[cost];
		filterButtons[cost].set_visible(false);
		filterButtons[cost].pressed = false;
		filterButtons[cost].set_tooltip(cost_tooltip[cost]);
		add_component(filterButtons + cost);
	}
	cursor = old_cursor;

	sort_label.set_pos(cursor);
	sort_label.set_width(client_width);
	add_component(&sort_label);
	cursor.y += D_LABEL_HEIGHT + D_V_SPACE;

	// hsiegeln: added sort_button
	sort_button.init(button_t::roundbox, sort_text[env_t::default_sortmode], cursor, scr_size(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	sort_button.set_tooltip("Sort waiting list by");
	sort_button.add_listener(this);
	add_component(&sort_button);
	cursor.x += button_size.w + D_H_SPACE;

	toggler_departures.init( button_t::roundbox_state, "Departure board", cursor, button_size);
	toggler_departures.set_tooltip("Show/hide estimated arrival times");
	//toggler_departures.set_visible(false); // (This is for disabling the departures board)
	toggler_departures.add_listener( this );
	add_component( &toggler_departures );
	cursor.x += button_size.w + D_H_SPACE;

	toggler.init(button_t::roundbox_state, "Chart", cursor, button_size);
	toggler.set_tooltip("Show/hide statistics");
	toggler.add_listener(this);
	add_component(&toggler);
	cursor.x += button_size.w + D_H_SPACE;

	button.init(button_t::roundbox, "Details", cursor, button_size);
	button.set_tooltip("Open station/stop details");
	button.add_listener(this);
	add_component(&button);
	cursor.x = D_MARGIN_RIGHT;
	cursor.y += button_size.h + D_V_SPACE;

	scrolly.set_pos(scr_coord(D_MARGIN_LEFT, cursor.y));
	scrolly.set_size(scr_size(client_width, 10 * D_LABEL_HEIGHT));
	scrolly.set_show_scroll_x(true);
	add_component(&scrolly);
	cursor.y += scrolly.get_size().h;

	set_windowsize(scr_size(total_width, cursor.y + D_MARGIN_BOTTOM));
	set_min_windowsize(scr_size(total_width, cursor.y - 5 * D_LABEL_HEIGHT + D_MARGIN_BOTTOM));

	set_resizemode(diagonal_resize);     // 31-May-02	markus weber	added
	resize(scr_coord(0,0));
}


halt_info_t::~halt_info_t()
{
	if(  halt.is_bound()  &&  strcmp(halt->get_name(),edit_name)  &&  edit_name[0]  ) {
		// text changed => call tool
		cbuffer_t buf;
		buf.printf( "h%u,%s", halt.get_id(), edit_name );
		tool_t *tool = create_tool( TOOL_RENAME | SIMPLE_TOOL );
		tool->set_default_param( buf );
		welt->set_tool( tool, halt->get_owner() );
		// since init always returns false, it is safe to delete immediately
		delete tool;
	}
}


koord3d halt_info_t::get_weltpos(bool)
{
	return halt->get_basis_pos3d();
}


bool halt_info_t::is_weltpos()
{
	return ( welt->get_viewport()->is_on_center(get_weltpos(false)));
}


/**
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 * @author Hj. Malthaner
 */
void halt_info_t::draw(scr_coord pos, scr_size size)
{
	if(halt.is_bound()) {

		// buffer update now only when needed by halt itself => dedicated buffer for this
		int old_len = freight_info.len();
		halt->get_freight_info(freight_info);

		if(  toggler_departures.pressed  ) {
			old_len = -1;
		}
		if(  old_len != freight_info.len()  ) {
			if(  toggler_departures.pressed  ) {
				update_departures();
				joined_buf.append( freight_info );
			}
		}

		gui_frame_t::draw(pos, size);
		set_dirty();

		sint16 top = pos.y + D_TITLEBAR_HEIGHT + input.get_pos().y + input.get_size().h + D_V_SPACE;
		//sint16 top = pos.y+36;
		COLOR_VAL indikatorfarbe = halt->get_status_farbe();
		display_fillbox_wh_clip(pos.x+10, top+2, D_INDICATOR_WIDTH, D_INDICATOR_HEIGHT, indikatorfarbe, true);

		// now what do we accept here?
		int left = 10+D_INDICATOR_WIDTH+2;
		if (halt->get_pax_enabled()) {
			display_color_img(skinverwaltung_t::passengers->get_image_id(0), pos.x+left, top, 0, false, false);
			left += 10;
		}
		if (halt->get_mail_enabled()) {
			display_color_img(skinverwaltung_t::mail->get_image_id(0), pos.x+left, top, 0, false, false);
			left += 10;
		}
		if (halt->get_ware_enabled()) {
			display_color_img(skinverwaltung_t::goods->get_image_id(0), pos.x+left, top, 0, false, false);
			left += 10;
		}

		// what kind of station?
		left -= 20;
		top -= 44;
		haltestelle_t::stationtyp const halttype = halt->get_station_type();
		if (halttype & haltestelle_t::railstation && skinverwaltung_t::zughaltsymbol) {
			display_color_img(skinverwaltung_t::zughaltsymbol->get_image_id(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::loadingbay && skinverwaltung_t::autohaltsymbol) {
			display_color_img(skinverwaltung_t::autohaltsymbol->get_image_id(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::busstop && skinverwaltung_t::bushaltsymbol) {
			display_color_img(skinverwaltung_t::bushaltsymbol->get_image_id(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::dock && skinverwaltung_t::schiffshaltsymbol) {
			display_color_img(skinverwaltung_t::schiffshaltsymbol->get_image_id(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::airstop && skinverwaltung_t::airhaltsymbol) {
			display_color_img(skinverwaltung_t::airhaltsymbol->get_image_id(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::monorailstop && skinverwaltung_t::monorailhaltsymbol) {
			display_color_img(skinverwaltung_t::monorailhaltsymbol->get_image_id(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::tramstop && skinverwaltung_t::tramhaltsymbol) {
			display_color_img(skinverwaltung_t::tramhaltsymbol->get_image_id(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::maglevstop && skinverwaltung_t::maglevhaltsymbol) {
			display_color_img(skinverwaltung_t::maglevhaltsymbol->get_image_id(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::narrowgaugestop && skinverwaltung_t::narrowgaugehaltsymbol) {
			display_color_img(skinverwaltung_t::narrowgaugehaltsymbol->get_image_id(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		top += 44;
		top += D_LABEL_HEIGHT;

		info_buf.clear();
		info_buf.printf("%s: %u", translator::translate("Storage capacity"), halt->get_capacity(0));
		
		left = pos.x + D_MARGIN_LEFT;
		// passengers
		left += display_proportional(left, top, info_buf, ALIGN_LEFT, SYSCOL_TEXT, true);
		if (welt->get_settings().is_separate_halt_capacities()) {
			// here only for separate capacities
			display_color_img(skinverwaltung_t::passengers->get_image_id(0), left, top, 0, false, false);
			left += 10;
			// mail
			info_buf.clear();
			info_buf.printf(",  %u", halt->get_capacity(1));
			left += display_proportional(left, top, info_buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			display_color_img(skinverwaltung_t::mail->get_image_id(0), left, top, 0, false, false);
			left += 10;
			// goods
			info_buf.clear();
			info_buf.printf(",  %u", halt->get_capacity(2));
			left += display_proportional(left, top, info_buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			display_color_img(skinverwaltung_t::goods->get_image_id(0), left, top, 0, false, false);
			left = 53+LINESPACE;
		}
		top += LINESPACE + D_V_SPACE;

		// Hajo: Reuse of info_buf buffer to get and display
		// information about the passengers happiness
		info_buf.clear();
		halt->info(info_buf);
		display_multiline_text(pos.x + D_MARGIN_LEFT, top, info_buf, SYSCOL_TEXT);
	}
}


// activate the statistic
void halt_info_t::show_hide_statistics( bool show )
{
	toggler.pressed = show;
	sint16 statistics_height = filterButtons[MAX_HALT_COST-1].get_size().h + filterButtons[MAX_HALT_COST-1].get_pos().y - chart.get_pos().y + D_V_SPACE;
	const scr_coord offset = show ? scr_coord(0, statistics_height) : scr_coord(0, -statistics_height);
	set_min_windowsize(get_min_windowsize() + offset);
	scrolly.set_pos(scrolly.get_pos() + offset);
	chart.set_visible(show);
	set_windowsize(get_windowsize() + offset);
	resize(scr_coord(0,0));
	for (int i=0;i<MAX_HALT_COST;i++) {
		filterButtons[i].set_visible(toggler.pressed);
	}
}


// activate the statistic
void halt_info_t::show_hide_departures( bool show )
{
	toggler_departures.pressed = show;
	if(  show  ) {
		update_departures();
		text.set_buf( &joined_buf );
	}
	else {
		joined_buf.clear();
		text.set_buf( &freight_info );
	}
}

// refreshes the departure string
void halt_info_t::update_departures()
{
	if (!halt.is_bound()) {
		return;
	}

	vector_tpl<halt_info_t::dest_info_t> old_origins(origins);

	destinations.clear();
	origins.clear();

	const sint64 cur_ticks = welt->get_ticks();

	typedef inthashtable_tpl<uint16, sint64> const arrival_times_map; // Not clear why this has to be redefined here.
	const arrival_times_map& arrival_times = halt->get_estimated_convoy_arrival_times();
	const arrival_times_map& departure_times = halt->get_estimated_convoy_departure_times();

	convoihandle_t cnv;
	sint32 delta_t;
	const uint32 max_listings = 12;
	uint32 listing_count = 0;

	FOR(arrival_times_map, const& iter, arrival_times)
	{
		if(listing_count++ > max_listings)
		{
			break;
		}
		cnv.set_id(iter.key);
		if(!cnv.is_bound())
		{
			continue;
		}

		if(!cnv->get_schedule())
		{
			//XXX vehicle in depot.
			continue;
		}

		halthandle_t prev_halt = haltestelle_t::get_halt(cnv->front()->last_stop_pos, cnv->get_owner());
		delta_t = iter.value - cur_ticks;

		halt_info_t::dest_info_t prev(prev_halt, max(delta_t, 0l), cnv);

		origins.insert_ordered(prev, compare_hi);
	}

	listing_count = 0;

	FOR(arrival_times_map, const& iter, departure_times)
	{
		if(listing_count++ > max_listings)
		{
			break;
		}
		cnv.set_id(iter.key);
		if(!cnv.is_bound())
		{
			continue;
		}

		if(!cnv->get_schedule())
		{
			//XXX vehicle in depot.
			continue;
		}

		halthandle_t next_halt = cnv->get_schedule()->get_next_halt(cnv->get_owner(), halt);
		delta_t = iter.value - cur_ticks;
		
		halt_info_t::dest_info_t next(next_halt, max(delta_t, 0l), cnv);

		destinations.insert_ordered( next, compare_hi );
	}

	// now we build the string ...
	joined_buf.clear();
	//slist_tpl<halthandle_t> exclude;
	if(  origins.get_count()>0  ) 
	{
		joined_buf.append( " " );
		joined_buf.append( translator::translate( "Arrivals from\n" ) );
		FOR( vector_tpl<halt_info_t::dest_info_t>, hi, origins ) 
		{
			//if(  freight_list_sorter_t::by_via_sum != env_t::default_sortmode  ||  !exclude.is_contained( hi.halt )  )
			//{
				char timebuf[32];
				welt->sprintf_ticks(timebuf, sizeof(timebuf), hi.delta_ticks );
				joined_buf.printf( "  %s %s < %s\n", timebuf, hi.cnv->get_name(), hi.halt.is_bound() ? hi.halt->get_name() : "Unknown");
				//exclude.append( hi.halt );
			//}
		}
		joined_buf.append( "\n" );
	}

	//exclude.clear();
	if(  destinations.get_count()>0  ) 
	{
		
		joined_buf.append( translator::translate( "Departures to\n" ) );
		FOR( vector_tpl<halt_info_t::dest_info_t>, hi, destinations ) 
		{
			//if(  freight_list_sorter_t::by_via_sum != env_t::default_sortmode  ||  !exclude.is_contained( hi.halt )  ) 
			//{
				char timebuf[32];
				welt->sprintf_ticks(timebuf, sizeof(timebuf), hi.delta_ticks );
				joined_buf.printf( "  %s %s > %s\n", timebuf, hi.cnv->get_name(), hi.halt.is_bound() ? hi.halt->get_name() : "Unknown");
				//exclude.append( hi.halt );
			//}
		}
		joined_buf.append( "\n " );
	}
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool halt_info_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if (comp == &button) { 			// details button pressed
		create_win( new halt_detail_t(halt), w_info, magic_halt_detail + halt.get_id() );
	} else if (comp == &sort_button) { 	// @author hsiegeln sort button pressed
		env_t::default_sortmode = ((int)(halt->get_sortby())+1)%SORT_MODES;
		halt->set_sortby((freight_list_sorter_t::sort_mode_t) env_t::default_sortmode);
		sort_button.set_text(sort_text[env_t::default_sortmode]);
	} else  if (comp == &toggler) {
		show_hide_statistics( toggler.pressed^1 );
	}
	else  if (comp == &toggler_departures) {
		show_hide_departures( toggler_departures.pressed^1 );
	}
	else if(  comp == &input  ) {
		if(  strcmp(halt->get_name(),edit_name)  ) {
			// text changed => call tool
			cbuffer_t buf;
			buf.printf( "h%u,%s", halt.get_id(), edit_name );
			tool_t *tool = create_tool( TOOL_RENAME | SIMPLE_TOOL );
			tool->set_default_param( buf );
			welt->set_tool( tool, halt->get_owner() );
			// since init always returns false, it is safe to delete immediately
			delete tool;
		}
	}
	else {
		for( int i = 0; i<MAX_HALT_COST; i++) {
			if (comp == &filterButtons[i]) {
				filterButtons[i].pressed = !filterButtons[i].pressed;
				if(filterButtons[i].pressed) {
					chart.show_curve(i);
				}
				else {
					chart.hide_curve(i);
				}
				break;
			}
		}
	}

	return true;
}


/**
 * Set window size and adjust component sizes and/or positions accordingly
 * @author Markus Weber
 */
void halt_info_t::set_windowsize(scr_size size)
{
	gui_frame_t::set_windowsize(size);
	const scr_coord_val client_width = get_windowsize().w - D_MARGIN_LEFT - D_MARGIN_RIGHT;
		
	input.set_width(client_width);

	view.set_pos(scr_coord(D_MARGIN_LEFT + client_width - view.get_size().w, view.get_pos().y));

	chart.set_width(client_width - chart.get_pos().x + D_MARGIN_LEFT);
	floating_cursor_t auto_cursor(filterButtons[0].get_pos(), D_MARGIN_LEFT, D_MARGIN_LEFT + client_width);
	for (int cost = 0; cost<MAX_HALT_COST; cost++) {
		filterButtons[cost].set_pos(auto_cursor.next_pos(filterButtons[cost].get_size()));
	}

	scrolly.set_size(get_client_windowsize()-scrolly.get_pos());

	// the buttons shall be placed above the "waiting" scroll area. Thus they will start at 
	// scrolly.get_pos().y - <button height> - D_V_SPACE.
	const scr_coord delta(0, scrolly.get_pos().y - sort_button.get_size().h - D_V_SPACE - sort_button.get_pos().y);

	sort_label.set_pos(sort_label.get_pos() + delta );
	sort_label.set_width(client_width);
	sort_button.set_pos(sort_button.get_pos() + delta);
	toggler_departures.set_pos(toggler_departures.get_pos() + delta);
	toggler.set_pos(toggler.get_pos() + delta);
	button.set_pos(button.get_pos() + delta);
}



void halt_info_t::map_rotate90( sint16 new_ysize )
{
	view.map_rotate90(new_ysize);
}


halt_info_t::halt_info_t():
	gui_frame_t("", NULL),
	scrolly(&text),
	text(&freight_info),
	sort_label(NULL),
	view(koord3d::invalid, scr_size(64, 64))
{
	// just a dummy
}


void halt_info_t::rdwr(loadsave_t *file)
{
	koord3d halt_pos;
	scr_size size = get_windowsize();
	uint32 flags = 0;
	bool stats = toggler.pressed;
	bool departures = toggler_departures.pressed;
	sint32 xoff = scrolly.get_scroll_x();
	sint32 yoff = scrolly.get_scroll_y();
	if(  file->is_saving()  ) {
		halt_pos = halt->get_basis_pos3d();
		for( int i = 0; i<MAX_HALT_COST; i++) {
			if(  filterButtons[i].pressed  ) {
				flags |= (1<<i);
			}
		}
	}
	halt_pos.rdwr( file );
	size.rdwr( file );
	file->rdwr_long( flags );
	file->rdwr_byte( env_t::default_sortmode );
	file->rdwr_bool( stats );
	file->rdwr_bool( departures );
	file->rdwr_long( xoff );
	file->rdwr_long( yoff );
	if(  file->is_loading()  ) {
		halt = welt->lookup( halt_pos )->get_halt();
		// now we can open the window ...
		scr_coord const& pos = win_get_pos(this);
		halt_info_t *w = new halt_info_t(halt);
		create_win(pos.x, pos.y, w, w_info, magic_halt_info + halt.get_id());
		if(  stats  ) {
			size.h -= 170;
		}
		w->set_windowsize( size );
		for( int i = 0; i<MAX_HALT_COST; i++) {
			w->filterButtons[i].pressed = (flags>>i)&1;
			if(w->filterButtons[i].pressed) {
				w->chart.show_curve(i);
			}
		}
		if(  stats  ) {
			w->show_hide_statistics( true );
		}
		if(  departures  ) {
			w->show_hide_departures( true );
		}
		halt->get_freight_info(w->freight_info);
		w->text.recalc_size();
		w->scrolly.set_scroll_position( xoff, yoff );
		// we must invalidate halthandle
		halt = halthandle_t();
		destroy_win( this );
	}
}
