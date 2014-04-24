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

#include "../dataobj/fahrplan.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"

#include "../vehicle/simvehikel.h"

#include "../utils/simstring.h"

#include "../besch/skin_besch.h"





static const char *sort_text[4] = {
	"Zielort",
	"via",
	"via Menge",
	"Menge"
};

const char cost_type[MAX_HALT_COST][64] =
{
	"Happy",
	"Unhappy",
	"No Route",
	"hl_btn_sort_waiting",
	"Arrived",
	"Departed",
	"Convoys",
	"Walked"
};

const uint8 index_of_haltinfo[MAX_HALT_COST] = {
	HALT_HAPPY,
	HALT_UNHAPPY,
	HALT_NOROUTE,
	HALT_WAITING,
	HALT_ARRIVED,
	HALT_DEPARTED,
	HALT_CONVOIS_ARRIVED,
	HALT_WALKED
};

#define COL_HAPPY COL_WHITE
#define COL_UNHAPPY COL_RED
#define COL_NO_ROUTE COL_BLUE
#define COL_WAITING COL_YELLOW
#define COL_ARRIVED COL_DARK_ORANGE
#define COL_DEPARTED COL_DARK_YELLOW

const int cost_type_color[MAX_HALT_COST] =
{
	COL_HAPPY,
	COL_UNHAPPY,
	COL_NO_ROUTE,
	COL_WAITING,
	COL_ARRIVED,
	COL_DEPARTED,
	COL_COUNVOI_COUNT,
	COL_LILAC
};

halt_info_t::halt_info_t(halthandle_t halt) :
		gui_frame_t( halt->get_name(), halt->get_besitzer() ),
		scrolly(&text),
		text(&freight_info),
		sort_label(translator::translate("Hier warten/lagern:")),
		view(halt->get_basis_pos3d(), scr_size(max(64, get_base_tile_raster_width()), max(56, get_base_tile_raster_width() * 7 / 8)))
{
	this->halt = halt;
	halt->set_sortby( env_t::default_sortmode );

	const sint16 offset_below_viewport = D_MARGIN_TOP + D_BUTTON_HEIGHT + D_V_SPACE + view.get_size().h;
	const sint16 total_width = D_MARGIN_LEFT + 3*(D_BUTTON_WIDTH + D_H_SPACE) + max( D_BUTTON_WIDTH, view.get_size().w ) + D_MARGIN_RIGHT;

	input.set_pos(scr_coord(10,4));
	tstrncpy(edit_name, halt->get_name(), lengthof(edit_name));
	input.set_text(edit_name, lengthof(edit_name));
	input.add_listener(this);
	add_komponente(&input);

	add_komponente(&view);

	// chart
	chart.set_pos(scr_coord(66,offset_below_viewport+2));
	chart.set_size(scr_size(total_width-66-10, 100));
	chart.set_dimension(12, 10000);
	chart.set_visible(false);
	chart.set_background(MN_GREY1);
	const sint16 offset_below_chart = chart.get_pos().y + 100 +
	                                  +6+LINESPACE+D_V_SPACE; // chart x-axis labels plus space
	for (int cost = 0; cost<MAX_HALT_COST; cost++) {
		chart.add_curve(cost_type_color[cost], halt->get_finance_history(), MAX_HALT_COST, index_of_haltinfo[cost], MAX_MONTHS, 0, false, true, 0);
		filterButtons[cost].init(button_t::box_state, cost_type[cost],
			scr_coord(BUTTON1_X+(D_BUTTON_WIDTH+D_H_SPACE)*(cost%4), offset_below_chart+(D_BUTTON_HEIGHT+2)*(cost/4) ),
			scr_size(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
		filterButtons[cost].add_listener(this);
		filterButtons[cost].background_color = cost_type_color[cost];
		filterButtons[cost].set_visible(false);
		filterButtons[cost].pressed = false;
		add_komponente(filterButtons + cost);
	}
	add_komponente(&chart);
	chart_total_size = filterButtons[MAX_HALT_COST-1].get_pos().y + D_BUTTON_HEIGHT + D_V_SPACE - (chart.get_pos().y - 13);

	add_komponente(&sort_label);

	const sint16 yoff = offset_below_viewport + D_V_SPACE;

	// hsiegeln: added sort_button
	sort_button.init(button_t::roundbox, sort_text[env_t::default_sortmode],scr_coord(BUTTON1_X, yoff), scr_size(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	sort_button.set_tooltip("Sort waiting list by");
	sort_button.add_listener(this);
	add_komponente(&sort_button);

	toggler_departures.init( button_t::roundbox_state, "Departure board", scr_coord( BUTTON2_X, yoff ), scr_size( D_BUTTON_WIDTH, D_BUTTON_HEIGHT ) );
	toggler_departures.set_tooltip("Show/hide estimated arrival times");
	toggler_departures.add_listener( this );
	add_komponente( &toggler_departures );

	toggler.init(button_t::roundbox_state, "Chart", scr_coord(BUTTON3_X, yoff), scr_size(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	toggler.set_tooltip("Show/hide statistics");
	toggler.add_listener(this);
	add_komponente(&toggler);

	button.init(button_t::roundbox, "Details", scr_coord(BUTTON4_X, yoff), scr_size(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	button.set_tooltip("Open station/stop details");
	button.add_listener(this);
	add_komponente(&button);

	scrolly.set_pos(scr_coord(D_MARGIN_LEFT, yoff + D_BUTTON_HEIGHT + D_V_SPACE ));
	scrolly.set_show_scroll_x(true);
	add_komponente(&scrolly);

	set_windowsize(scr_size(total_width, view.get_size().h+208+D_SCROLLBAR_HEIGHT));
	set_min_windowsize(scr_size(total_width, view.get_size().h+131+D_SCROLLBAR_HEIGHT));

	set_resizemode(diagonal_resize);     // 31-May-02	markus weber	added
	resize(scr_coord(0,0));
}


halt_info_t::~halt_info_t()
{
	if(  halt.is_bound()  &&  strcmp(halt->get_name(),edit_name)  &&  edit_name[0]  ) {
		// text changed => call tool
		cbuffer_t buf;
		buf.printf( "h%u,%s", halt.get_id(), edit_name );
		werkzeug_t *w = create_tool( WKZ_RENAME_TOOL | SIMPLE_TOOL );
		w->set_default_param( buf );
		welt->set_werkzeug( w, halt->get_besitzer() );
		// since init always returns false, it is safe to delete immediately
		delete w;
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
		if(  toggler_departures.pressed  &&  next_refresh--<0  ) {
			old_len = -1;
		}
		if(  old_len != freight_info.len()  ) {
			if(  toggler_departures.pressed  ) {
				update_departures();
				joined_buf.append( freight_info );
			}
			text.recalc_size();
		}

		gui_frame_t::draw(pos, size);
		set_dirty();

		sint16 top = pos.y+36;
		COLOR_VAL indikatorfarbe = halt->get_status_farbe();
		display_fillbox_wh_clip(pos.x+10, top+2, D_INDICATOR_WIDTH, D_INDICATOR_HEIGHT, indikatorfarbe, true);

		// now what do we accept here?
		int left = 10+D_INDICATOR_WIDTH+2;
		if (halt->get_pax_enabled()) {
			display_color_img(skinverwaltung_t::passagiere->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 10;
		}
		if (halt->get_post_enabled()) {
			display_color_img(skinverwaltung_t::post->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 10;
		}
		if (halt->get_ware_enabled()) {
			display_color_img(skinverwaltung_t::waren->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 10;
		}

		// what kind of station?
		left -= 20;
		top -= 44;
		haltestelle_t::stationtyp const halttype = halt->get_station_type();
		if (halttype & haltestelle_t::railstation) {
			display_color_img(skinverwaltung_t::zughaltsymbol->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::loadingbay) {
			display_color_img(skinverwaltung_t::autohaltsymbol->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::busstop) {
			display_color_img(skinverwaltung_t::bushaltsymbol->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::dock) {
			display_color_img(skinverwaltung_t::schiffshaltsymbol->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::airstop) {
			display_color_img(skinverwaltung_t::airhaltsymbol->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::monorailstop) {
			display_color_img(skinverwaltung_t::monorailhaltsymbol->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::tramstop) {
			display_color_img(skinverwaltung_t::tramhaltsymbol->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::maglevstop) {
			display_color_img(skinverwaltung_t::maglevhaltsymbol->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::narrowgaugestop) {
			display_color_img(skinverwaltung_t::narrowgaugehaltsymbol->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 23;
		}

		top = pos.y+50;
		info_buf.clear();
		info_buf.printf("%s: %u", translator::translate("Storage capacity"), halt->get_capacity(0));
		left = pos.x+10;
		// passengers
		left += display_proportional(left, top, info_buf, ALIGN_LEFT, COL_BLACK, true);
		if (welt->get_settings().is_separate_halt_capacities()) {
			// here only for separate capacities
			display_color_img(skinverwaltung_t::passagiere->get_bild_nr(0), left, top, 0, false, false);
			left += 10;
			// post
			info_buf.clear();
			info_buf.printf(",  %u", halt->get_capacity(1));
			left += display_proportional(left, top, info_buf, ALIGN_LEFT, COL_BLACK, true);
			display_color_img(skinverwaltung_t::post->get_bild_nr(0), left, top, 0, false, false);
			left += 10;
			// goods
			info_buf.clear();
			info_buf.printf(",  %u", halt->get_capacity(2));
			left += display_proportional(left, top, info_buf, ALIGN_LEFT, COL_BLACK, true);
			display_color_img(skinverwaltung_t::waren->get_bild_nr(0), left, top, 0, false, false);
			left = 53+LINESPACE;
		}

		// Hajo: Reuse of info_buf buffer to get and display
		// information about the passengers happiness
		info_buf.clear();
		halt->info(info_buf);
		display_multiline_text(pos.x+10, pos.y+53+LINESPACE, info_buf, COL_BLACK);
	}
}


// activate the statistic
void halt_info_t::show_hide_statistics( bool show )
{
	toggler.pressed = show;
	const scr_coord offset = show ? scr_coord(0, chart_total_size) : scr_coord(0, -chart_total_size);
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


// a sophisticated guess of a convois arrival time, taking into account the braking too and the current convoi state
uint32 halt_info_t::calc_ticks_until_arrival( convoihandle_t cnv )
{
	/* calculate the time needed:
	 *   tiles << (8+12) / (kmh_to_speed(max_kmh) = ticks
	 */
	uint32 delta_t = 0;
	sint32 delta_tiles = cnv->get_route()->get_count() - cnv->front()->get_route_index();
	uint32 kmh_average = (cnv->get_average_kmh()*900 ) / 1024u;

	// last braking tile
	if(  delta_tiles > 1  &&  kmh_average > 25  ) {
		delta_tiles --;
		delta_t += 3276; // ( (1 << (8+12)) / kmh_to_speed(25) );
	}
	// second last braking tile
	if(  delta_tiles > 1  &&  kmh_average > 50  ) {
		delta_tiles --;
		delta_t += 1638; // ( (1 << (8+12)) / kmh_to_speed(50) );
	}
	// third last braking tile
	if(  delta_tiles > 1  &&  kmh_average > 100  ) {
		delta_tiles --;
		delta_t += 819; // ( (1 << (8+12)) / kmh_to_speed(100) );
	}
	// waiting at signal?
	if(  cnv->get_state() != convoi_t::DRIVING  ) {
		// extra time for acceleration
		delta_t += kmh_average * 25;
	}
	delta_t += ( ((sint64)delta_tiles << (8+12) ) / kmh_to_speed( kmh_average ) );
	return delta_t;
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

	const uint32 cur_ticks = welt->get_zeit_ms() % welt->ticks_per_world_month;
	static uint32 last_ticks = 0;

	if(  last_ticks > cur_ticks  ) {
		// new month has started => invalidate average buffer
		old_origins.clear();
	}
	last_ticks = cur_ticks;

	// iterate over all convoys stopping here
	FOR(  slist_tpl<convoihandle_t>, cnv, halt->get_loading_convois() ) {
		halthandle_t next_halt = cnv->get_schedule()->get_next_halt(cnv->get_besitzer(),halt);
		if(  next_halt.is_bound()  ) {
			halt_info_t::dest_info_t next( next_halt, 0, cnv );
			destinations.append_unique( next );
			if(  grund_t *gr = welt->lookup( cnv->get_vehikel(0)->last_stop_pos )  ) {
				if(  gr->get_halt().is_bound()  &&  gr->get_halt() != halt  ) {
					halt_info_t::dest_info_t prev( gr->get_halt(), 0, cnv );
					origins.append_unique( prev );
				}
			}
		}
	}

	// now exactly the same for convoys en route; the only change is that we estimate their arrival time too
	FOR(  vector_tpl<linehandle_t>, line, halt->registered_lines ) {
		for(  uint j = 0;  j < line->count_convoys();  j++  ) {
			convoihandle_t cnv = line->get_convoy(j);
			if(  cnv.is_bound()  &&  ( cnv->get_state() == convoi_t::DRIVING  ||  cnv->is_waiting() )  &&  haltestelle_t::get_halt( cnv->get_schedule()->get_current_eintrag().pos, cnv->get_besitzer() ) == halt  ) {
				halthandle_t prev_halt = haltestelle_t::get_halt( cnv->front()->last_stop_pos, cnv->get_besitzer() );
				sint32 delta_t = cur_ticks + calc_ticks_until_arrival( cnv );
				if(  prev_halt.is_bound()  ) {
					halt_info_t::dest_info_t prev( prev_halt, delta_t, cnv );
					// smooth times a little
					FOR( vector_tpl<dest_info_t>, &elem, old_origins ) {
						if(  elem.cnv == cnv ) {
							delta_t = ( delta_t + 3*elem.delta_ticks ) / 4;
							prev.delta_ticks = delta_t;
							break;
						}
					}
					origins.insert_ordered( prev, compare_hi );
				}
				halthandle_t next_halt = cnv->get_schedule()->get_next_halt(cnv->get_besitzer(),halt);
				if(  next_halt.is_bound()  ) {
					halt_info_t::dest_info_t next( next_halt, delta_t+2000, cnv );
					destinations.insert_ordered( next, compare_hi );
				}
			}
		}
	}

	FOR( vector_tpl<convoihandle_t>, cnv, halt->registered_convoys ) {
		if(  cnv.is_bound()  &&  ( cnv->get_state() == convoi_t::DRIVING  ||  cnv->is_waiting() )  &&  haltestelle_t::get_halt( cnv->get_schedule()->get_current_eintrag().pos, cnv->get_besitzer() ) == halt  ) {
			halthandle_t prev_halt = haltestelle_t::get_halt( cnv->front()->last_stop_pos, cnv->get_besitzer() );
			sint32 delta_t = cur_ticks + calc_ticks_until_arrival( cnv );
			if(  prev_halt.is_bound()  ) {
				halt_info_t::dest_info_t prev( prev_halt, delta_t, cnv );
				// smooth times a little
				FOR( vector_tpl<dest_info_t>, &elem, old_origins ) {
					if(  elem.cnv == cnv ) {
						delta_t = ( delta_t + 3*elem.delta_ticks ) / 4;
						prev.delta_ticks = delta_t;
						break;
					}
				}
				origins.insert_ordered( prev, compare_hi );
			}
			halthandle_t next_halt = cnv->get_schedule()->get_next_halt(cnv->get_besitzer(),halt);
			if(  next_halt.is_bound()  ) {
				halt_info_t::dest_info_t next( next_halt, delta_t+2000, cnv );
				destinations.insert_ordered( next, compare_hi );
			}
		}
	}

	// now we build the string ...
	joined_buf.clear();
	slist_tpl<halthandle_t> exclude;
	if(  destinations.get_count()>0  ) {
		joined_buf.append( " " );
		joined_buf.append( translator::translate( "Departures to\n" ) );
		FOR( vector_tpl<halt_info_t::dest_info_t>, hi, destinations ) {
			if(  freight_list_sorter_t::by_via_sum != env_t::default_sortmode  ||  !exclude.is_contained( hi.halt )  ) {
				joined_buf.printf( "  %s %s\n", tick_to_string( hi.delta_ticks, false ), hi.halt->get_name() );
				exclude.append( hi.halt );
			}
		}
		joined_buf.append( "\n " );
	}

	exclude.clear();
	if(  origins.get_count()>0  ) {
		joined_buf.append( translator::translate( "Arrivals from\n" ) );
		FOR( vector_tpl<halt_info_t::dest_info_t>, hi, origins ) {
			if(  freight_list_sorter_t::by_via_sum != env_t::default_sortmode  ||  !exclude.is_contained( hi.halt )  ) {
				joined_buf.printf( "  %s %s\n", tick_to_string( hi.delta_ticks, false ), hi.halt->get_name() );
				exclude.append( hi.halt );
			}
		}
		joined_buf.append( "\n" );
	}

	next_refresh = 5;
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool halt_info_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if (comp == &button) { 			// details button pressed
		create_win( new halt_detail_t(halt), w_info, magic_halt_detail + halt.get_id() );
	}
	else if (comp == &sort_button) { 	// @author hsiegeln sort button pressed
		env_t::default_sortmode = ((int)(halt->get_sortby())+1)%4;
		halt->set_sortby((freight_list_sorter_t::sort_mode_t) env_t::default_sortmode);
		sort_button.set_text(sort_text[env_t::default_sortmode]);
	}
	else  if (comp == &toggler) {
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
			werkzeug_t *w = create_tool( WKZ_RENAME_TOOL | SIMPLE_TOOL );
			w->set_default_param( buf );
			welt->set_werkzeug( w, halt->get_besitzer() );
			// since init always returns false, it is safe to delete immediately
			delete w;
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

	input.set_size(scr_size(get_windowsize().w-20, 13));

	view.set_pos(scr_coord(get_windowsize().w - view.get_size().w - 10 , 21));

	scrolly.set_size(get_client_windowsize()-scrolly.get_pos());

	const sint16 yoff = scrolly.get_pos().y-D_BUTTON_HEIGHT-D_V_SPACE;
	sort_button.set_pos(scr_coord(BUTTON1_X,yoff));
	toggler_departures.set_pos(scr_coord(BUTTON2_X,yoff));
	toggler.set_pos(scr_coord(BUTTON3_X,yoff));
	button.set_pos(scr_coord(BUTTON4_X,yoff));
	sort_label.set_pos(scr_coord(10,yoff-LINESPACE-D_V_SPACE));
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
