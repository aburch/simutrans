/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simevent.h"
#include "../simcolor.h"
#include "../simconvoi.h"
#include "../vehicle/vehicle.h"
#include "../world/simworld.h"
#include "../obj/depot.h"
#include "../simhalt.h"
#include "../simskin.h"
#include "../ground/grund.h"
#include "../tool/simtool.h"
#include "../simfab.h"
#include "../world/simcity.h"
#include "fabrik_info.h"
#include "simwin.h"
#include "minimap.h"

#include "../dataobj/translator.h"
#include "../dataobj/settings.h"
#include "../dataobj/schedule.h"
#include "../dataobj/powernet.h"
#include "../dataobj/ribi.h"
#include "../dataobj/loadsave.h"

#include "../obj/way/schiene.h"
#include "../obj/leitung2.h"
#include "../utils/cbuffer.h"
#include "../display/scr_coord.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../utils/simrandom.h"
#include "../player/simplay.h"

#include "../tpl/inthashtable_tpl.h"
#include "../tpl/slist_tpl.h"

#include <math.h>

sint32 minimap_t::max_cargo=0;
sint32 minimap_t::max_passed=0;

static sint32 max_waiting_change = 1;

static sint32 max_tourist_ziele = 1;
static sint32 max_waiting = 1;
static sint32 max_origin = 1;
static sint32 max_transfer = 1;
static sint32 max_service = 1;

static sint32 max_building_level = 0;

minimap_t * minimap_t::single_instance = NULL;
karte_ptr_t minimap_t::world;
minimap_t::MAP_DISPLAY_MODE minimap_t::mode = MAP_TOWN;
minimap_t::MAP_DISPLAY_MODE minimap_t::last_mode = MAP_TOWN;
bool minimap_t::is_visible = false;

#define MAX_MAP_TYPE_LAND 31
#define MAX_MAP_TYPE_WATER 5

// color for the land
static const uint8 map_type_color[MAX_MAP_TYPE_WATER+MAX_MAP_TYPE_LAND] =
{
	// water level
	97,
	99,
	19,
	21,
	23,
	// terrain level
	160,
	161,
	162,
	163,
	164,
	165,
	166,
	167,
	205,
	206,
	207,
	172,
	174,
	159,
	COL_LIGHT_ORANGE,
	COL_TOLL,
	156,
	154,
	115,
	114,
	113,
	112,
	216,
	217,
	218,
	219,
	220,
	COL_LILAC,
	222,
	223,
	224
};

const uint8 minimap_t::severity_color[MAX_SEVERITY_COLORS] =
{
	106,
	2,
	85,
	86,
	29,
	30,
	COL_YELLOW,
	71,
	39,
	COL_OPERATION
};


minimap_t::line_segment_t::line_segment_t(koord s, uint8 so, koord e, uint8 eo, schedule_t* sched, player_t* p, uint8 cc, bool diagonal)
{
	schedule = sched;
	waytype = sched->get_waytype();
	player = p;
	colorcount = cc;
	start_diagonal = diagonal;
	if(  s.x<e.x  ||  (s.x==e.x  &&  s.y<e.y)  ) {
		start = s;
		end = e;
		start_offset = so;
		end_offset = eo;
	}
	else {
		start = e;
		end = s;
		start_offset = eo;
		end_offset = so;
	}
}


// helper function for line segment_t
bool minimap_t::line_segment_t::operator==(const line_segment_t & other) const
{
	return
		start == other.start  &&
		end == other.end  &&
		player == other.player  &&
		schedule->similar( other.schedule, player );
}


// Ordering based on first start then end coordinate
bool minimap_t::LineSegmentOrdering::operator()(const minimap_t::line_segment_t& a, const minimap_t::line_segment_t& b) const
{
	if(  a.start.x == b.start.x  ) {
		// same start ...
		return a.end.x < b.end.x;
	}
	return a.start.x < b.start.x;
}


static uint8 colore_idx = 0;
static inthashtable_tpl< int, slist_tpl<schedule_t *> > waypoint_hash;


// add the schedule to the map (if there is a valid one)
void minimap_t::add_to_schedule_cache( convoihandle_t cnv, bool with_waypoints )
{
	// make sure this is valid!
	if(  !cnv.is_bound()  ) {
		return;
	}
	schedule_t *schedule = cnv->get_schedule();
	if(  !show_network_load_factor  ) {
		colore_idx += 8;
		if(  colore_idx >= 208  ) {
			colore_idx = (colore_idx % 8) + 1;
			if(  colore_idx == 7  ) {
				colore_idx = 0;
			}
		}
	}
	else {
		//TODO: extract common part from with schedule_list_gui_t::display()
		int capacity = 0, load = 0; // total capacity and load of line (=sum of all conv's cap/load)

		if(cnv->get_line().is_bound()) {
			for(  uint i = 0;  i < cnv->get_line()->count_convoys();  i++  ) {
				convoihandle_t const cnv_in_line = cnv->get_line()->get_convoy(i);
				// we do not want to count the capacity of depot convois
				if(  !cnv_in_line->in_depot()  ) {
					for(  unsigned j = 0;  j < cnv_in_line->get_vehicle_count();  j++  ) {
						capacity += cnv_in_line->get_vehicle(j)->get_cargo_max();
						load += cnv_in_line->get_vehicle(j)->get_total_cargo();
					}
				}
			}
		}
		else {
			// we do not want to count the capacity of depot convois
			if(!cnv->in_depot()) {
				for(unsigned j = 0; j < cnv->get_vehicle_count(); j++) {
					capacity += cnv->get_vehicle(j)->get_cargo_max();
					load += cnv->get_vehicle(j)->get_total_cargo();
				}
			}
		}

		// we check if cap is zero, since theoretically a
		// conv can consist of only 1 vehicle, which has no cap (eg. locomotive)
		// and we do not like to divide by zero, do we?
		if(capacity > 0) {
			const uint32 load_idx = clamp(load * MAX_SEVERITY_COLORS / capacity, 0, MAX_SEVERITY_COLORS-1);
			colore_idx = severity_color[MAX_SEVERITY_COLORS-1 - load_idx];
		}
		else {
			colore_idx = severity_color[MAX_SEVERITY_COLORS-1];
		}
	}

	// ok, add this schedule to map
	// from here on we have a valid convoi
	int stops = 0;
	uint8 old_offset = 0, first_offset = 0, temp_offset = 0;
	koord old_stop, first_stop, temp_stop;
	bool last_diagonal = false;
	const bool add_schedule = schedule->get_waytype() != air_wt;

	for(schedule_entry_t cur : schedule->entries  ) {

		//cycle on stops
		//try to read station's coordinates if there's a station at this schedule stop
		halthandle_t station = haltestelle_t::get_halt( cur.pos, cnv->get_owner() );
		if(  station.is_bound()  ) {
			stop_cache.append_unique( station );
			temp_stop = station->get_basis_pos();
			stops ++;
		}
		else if(  with_waypoints  ) {
			temp_stop = cur.pos.get_2d();
			stops ++;
		}
		else {
			continue;
		}

		const int key = temp_stop.x + temp_stop.y*world->get_size().x;
		waypoint_hash.put( key );
		// now get the offset
		slist_tpl<schedule_t *>*pt_list = waypoint_hash.access(key);
		if(  add_schedule  ) {
			// init key
			if(  !pt_list->is_contained( schedule )  ) {
				// not known => append
				temp_offset = pt_list->get_count();
			}
			else {
				// how many times we reached here?
				temp_offset = pt_list->index_of( schedule );
			}
		}
		else {
			temp_offset = 0;
		}

		if(  stops>1  ) {
			last_diagonal ^= true;
			if(  (temp_stop.x-old_stop.x)*(temp_stop.y-old_stop.y) == 0  ) {
				last_diagonal = false;
			}
			if(  !schedule_cache.insert_unique_ordered( line_segment_t( temp_stop, temp_offset, old_stop, old_offset, schedule, cnv->get_owner(), colore_idx, last_diagonal ), LineSegmentOrdering() )  &&  add_schedule  ) {
				// append if added and not yet there
				if(  !pt_list->is_contained( schedule )  ) {
					pt_list->append( schedule );
				}
				if(  stops == 2  ) {
					// append first stop too, when this is called for the first time
					const int key = first_stop.x + first_stop.y*world->get_size().x;
					waypoint_hash.put( key );
					slist_tpl<schedule_t *>*pt_list = waypoint_hash.access(key);
					if(  !pt_list->is_contained( schedule )  ) {
						pt_list->append( schedule );
					}
				}
			}
			old_stop = temp_stop;
			old_offset = temp_offset;
		}
		else {
			first_stop = temp_stop;
			first_offset = temp_offset;
			old_stop = temp_stop;
			old_offset = temp_offset;
		}
	}

	if(  stops > 2  ) {
		// connect to start
		last_diagonal ^= true;
		schedule_cache.insert_unique_ordered( line_segment_t( first_stop, first_offset, old_stop, old_offset, schedule, cnv->get_owner(), colore_idx, last_diagonal ), LineSegmentOrdering() );
	}
}



// some routines for the minimap with schedules
static uint32 number_to_radius( uint32 n )
{
	return log2( n>>5 );
}


static void display_airport( const scr_coord_val xx, const scr_coord_val yy, const FLAGGED_PIXVAL color )
{
	int x = xx + 5;
	int y = yy - 11;

	if ( y < 0 ) {
		y = 0;
	}

	const char symbol[] = {
		'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X',
		'X', '.', '.', '.', '.', 'X', '.', '.', '.', '.', 'X',
		'X', '.', '.', '.', '.', 'X', '.', '.', '.', '.', 'X',
		'X', '.', '.', '.', '.', 'X', '.', '.', '.', '.', 'X',
		'X', '.', '.', '.', 'X', 'X', 'X', '.', '.', '.', 'X',
		'X', '.', 'X', 'X', 'X', 'X', 'X', 'X', 'X', '.', 'X',
		'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X',
		'X', 'X', '.', '.', '.', 'X', '.', '.', '.', 'X', 'X',
		'X', '.', '.', '.', '.', 'X', '.', '.', '.', '.', 'X',
		'X', '.', '.', '.', 'X', 'X', 'X', '.', '.', '.', 'X',
		'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'
	};

	for ( int i = 0; i < 11; i++ ) {
		for ( int j = 0; j < 11; j++ ) {
			if ( symbol[i + j * 11] == 'X' ) {
				display_vline_wh_clip_rgb( x + i, y + j, 1,  color, true );
			}
		}
	}
}

static void display_harbor( const scr_coord_val xx, const scr_coord_val yy, const FLAGGED_PIXVAL color )
{
	int x = xx + 5;
	int y = yy - 11 + 13; //to not overwrite airline symbol

	if ( y < 0 ) {
		y = 0;
	}

	const char symbol[] = {
		'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X',
		'X', '.', '.', '.', 'X', 'X', 'X', '.', '.', '.', 'X',
		'X', '.', '.', '.', '.', 'X', '.', '.', '.', '.', 'X',
		'X', '.', 'X', 'X', 'X', 'X', 'X', 'X', 'X', '.', 'X',
		'X', '.', '.', '.', 'X', 'X', 'X', '.', '.', '.', 'X',
		'X', '.', '.', '.', '.', 'X', '.', '.', '.', '.', 'X',
		'X', 'X', 'X', '.', '.', 'X', '.', '.', 'X', 'X', 'X',
		'X', 'X', 'X', 'X', '.', 'X', '.', 'X', 'X', 'X', 'X',
		'X', '.', 'X', 'X', 'X', 'X', 'X', 'X', 'X', '.', 'X',
		'X', '.', '.', 'X', 'X', 'X', 'X', 'X', '.', '.', 'X',
		'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'
	};

	for ( int i = 0; i < 11; i++ ) {
		for ( int j = 0; j < 11; j++ ) {
			if ( symbol[i + j * 11] == 'X' ) {
				display_vline_wh_clip_rgb( x + i, y + j, 1,  color, true );
			}
		}
	}
}
// those will be replaced by pak images later ...!


static void display_thick_line( scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2, PIXVAL col, bool dotting, short dot_full, short dot_empty, short thickness )
{
	double delta_x = abs( x1 - x2 );
	double delta_y = abs( y1 - y2 );

	if(  delta_x == 0.0  ||  delta_y/delta_x > 2.0  ) {
		// mostly vertical
		x1 -= thickness/2;
		x2 -= thickness/2;
		for(  int i = 0;  i < thickness;  i++  ) {
			if ( !dotting ) {
				display_direct_line_rgb( x1 + i, y1, x2 + i, y2, col );
			}
			else {
				display_direct_line_dotted_rgb( x1 + i, y1, x2 + i, y2, dot_full, dot_empty, col );
			}
		}
	}
	else if(  delta_y == 0.0  ||  delta_x/delta_y > 2.0  ) {
		// mostly horizontal
		y1 -= thickness/2;
		y2 -= thickness/2;
		for(  int i = 0;  i < thickness;  i++  ) {
			if ( !dotting ) {
				display_direct_line_rgb( x1, y1 + i, x2, y2 + i, col );
			}
			else {
				display_direct_line_dotted_rgb( x1, y1 + i, x2, y2 + i, dot_full, dot_empty, col );
			}
		}
	}
	else {
		// diagonal
		int y_multiplier = (x1-x2)/(y1-y2) < 0 ? +1 : -1;
		thickness = (thickness*7)/8;
		x1 -= thickness/2;
		x2 -= thickness/2;
		y1 -= thickness*y_multiplier/2;
		y2 -= thickness*y_multiplier/2;
		for(  int i = 0;  i < thickness;  i++  ) {
			if ( !dotting ) {
				display_direct_line_rgb( x1+i, y1+i*y_multiplier, x2+i, y2+i*y_multiplier, col );
				display_direct_line_rgb( x1+i+1, y1+i*y_multiplier, x2+i+1, y2+i*y_multiplier, col );
			}
			else {
				display_direct_line_dotted_rgb( x1 + i, y1 + i*y_multiplier, x2 + i, y2 + i*y_multiplier, dot_full, dot_empty, col );
				display_direct_line_dotted_rgb( x1 + i + 1, y1 + i*y_multiplier, x2 + i + 1, y2 + i*y_multiplier, dot_full, dot_empty, col );
			}
		}
	}
}


static void line_segment_draw( waytype_t type, scr_coord start, const uint8 start_offset, scr_coord end, const uint8 end_offset, bool start_diagonal, const PIXVAL colore )
{
	// airplanes are different, so we must check for them first
	if(  type ==  air_wt  ) {
		// ignore offset for airplanes
		draw_bezier_rgb( start.x, start.y, end.x, end.y, 50, 50, 50, 50, colore, 5, 5 );
		draw_bezier_rgb( start.x + 1, start.y + 1, end.x + 1, end.y + 1, 50, 50, 50, 50, colore, 5, 5 );
	}
	else {
		// determine line style
		uint8 thickness = 3;
		bool dotted = false;
		switch(  type  ) {
			case monorail_wt:
			case maglev_wt:
				thickness = 5;
				break;
			case track_wt:
				thickness = 4;
				break;
			case road_wt:
				thickness = 2;
				break;
			case tram_wt:
			case narrowgauge_wt:
				thickness = 3;
				break;
			default:
				thickness = 3;
				dotted = true;
		}

		// move to the correct locations
		start.x += 3*start_offset;
		start.y += 3*start_offset;
		end.x	+= 3*end_offset;
		end.y	+= 3*end_offset;

		scr_coord delta = end-start;

		// Reduce diagonal line overlap: Genereally respect the requested start_diagonal.
		// In case of NW to SE lines, ignore the requested diagonal, if either start or end of the line has index 0
		if(sgn(delta.x) * sgn(delta.y) == 1){
			if(end_offset){
				start_diagonal = true;
			}
			if(start_offset){
				start_diagonal = false;
			}
		}


		// We always start straight, then diagonal. If the other way round is desired, simply swap start and end.
		if(start_diagonal ){
			delta = delta*-1;
			const scr_coord tmp = start;
			start=end;
			end=tmp;
		}

		const int d = min(abs(delta.x),abs(delta.y));//pick absolute of smallest component
		const scr_coord diag = scr_coord(d*sgn(delta.x),d*sgn(delta.y));

		const scr_coord mid=end-diag;

		if(start!=mid) {
			display_thick_line(start.x, start.y, mid.x, mid.y, colore, dotted, 5, 3, thickness);
		}
		if(mid!=end) {
			display_thick_line(mid.x, mid.y, end.x, end.y, colore, dotted, 5, 3, thickness);
		}
	}
}


// converts map (karte) koordinates to screen koordinates
scr_coord minimap_t::map_to_screen_coord(const koord &k) const
{
	assert(zoom_in ==1 || zoom_out ==1 );
	scr_coord_val x = (scr_coord_val)k.x * zoom_in;
    scr_coord_val y = (scr_coord_val)k.y * zoom_in;
	if(isometric) {
		// 45 rotate view
		scr_coord_val xrot = (scr_coord_val)world->get_size().y * zoom_in + x - y - 1;
		y = ( x + y )/2;
		x = xrot;
	}
	return scr_coord(x/zoom_out, y/zoom_out);
}


// and re-transform
koord minimap_t::screen_to_map_coord(const scr_coord &c) const
{
	sint32 x = ((sint32)c.x*zoom_out)/zoom_in;
	sint32 y = ((sint32)c.y*zoom_out)/zoom_in;
	if(isometric) {
		y *= 2;
		x  = (x + y - world->get_size().y)/2;
		y  = y - x;
	}
	return koord(x,y);
}


bool minimap_t::change_zoom_factor(bool magnify)
{
	bool zoomed = false;
	if(  magnify  ) {
		// zoom in
		if(  zoom_out > 1  ) {
			zoom_out--;
			zoomed = true;
		}
		else {
			// check here for maximum zoom-out, otherwise there will be integer overflows
			// with large maps as we calculate with sint16 coordinates ...
			const int max_zoom_in = min( INT_MAX / (2*world->get_size_max()), 16);
			if(  zoom_in < max_zoom_in  ) {
				zoom_in++;
				zoomed = true;
			}
		}
	}
	else {
		// zoom out
		if(  zoom_in > 1  ) {
			zoom_in--;
			zoomed = true;
		}
		else if(  zoom_out < 16  ) {
			zoom_out++;
			zoomed = true;
		}
	}

	if(  zoomed  ){
		// recalc map size
		calc_map_size();
	}
	return zoomed;
}


PIXVAL minimap_t::calc_severity_color(sint32 amount, sint32 max_value)
{
	if(max_value!=0) {
		// color array goes from light blue to red
		const sint32 severity = ((sint64)amount * MAX_SEVERITY_COLORS) / ((sint64)max_value + 1);
		return color_idx_to_rgb( minimap_t::severity_color[ clamp( severity, 0, MAX_SEVERITY_COLORS-1 ) ]);
	}
	return color_idx_to_rgb( minimap_t::severity_color[0]);
}


PIXVAL minimap_t::calc_severity_color_log(sint32 amount, sint32 max_value)
{
	if(  max_value>1  ) {
		sint32 severity;
		if(  amount <= 0x003FFFFF  ) {
			severity = log2( (uint32)( (amount << MAX_SEVERITY_COLORS) / (max_value+1) ) );
		}
		else {
			severity = (uint32)( log( (double)amount*(double)(1<<MAX_SEVERITY_COLORS)/(double)max_value) + 0.5 );
		}
		return color_idx_to_rgb( minimap_t::severity_color[ clamp( severity, 0, MAX_SEVERITY_COLORS-1 ) ]);
	}
	return color_idx_to_rgb( minimap_t::severity_color[0]);
}


void minimap_t::set_map_color_clip( sint16 x, sint16 y, PIXVAL color )
{
	if(  0<=x  &&  (uint16)x < map_data->get_width()  &&  0<=y  &&  (uint16)y < map_data->get_height()  ) {
		map_data->at( x, y ) = color;
	}
}


void minimap_t::set_map_color(koord k_, const PIXVAL color)
{
	// if map is in normal mode, set new color for map
	// otherwise do nothing
	// result: convois will not "paint over" special maps
	if ( map_data==NULL  ||  !world->is_within_limits(k_)) {
		return;
	}

	scr_coord c = map_to_screen_coord(k_);
	c -= cur_off;

	if(  isometric  ) {
		// since isometric is distorted
		const sint32 xw = zoom_out>=2 ? 1 : 2*zoom_in;
		// increase size at zoom_in 2, 5, 9, 11
		const scr_coord_val mid_y = ((xw+1) / 5) + (xw / 18);
		// center line
		for(  int x=0;  x<xw;  x++  ) {
			set_map_color_clip( c.x+x, c.y+mid_y, color );
		}
		// lines above and below
		if(  mid_y > 0  ) {
			scr_coord_val left = 2, right = xw-2 + ((xw>>1)&1);
			for(  scr_coord_val y_offset = 1;  y_offset <= mid_y;  y_offset++  ) {
				for(  int x=left;  x<right;  x++  ) {
					set_map_color_clip( c.x+x, c.y+mid_y+y_offset, color );
					set_map_color_clip( c.x+x, c.y+mid_y-y_offset, color );
				}
				left += 2;
				right -= 2;
			}
		}
	}
	else {
		for(  sint32 x = max(0,c.x);  x < zoom_in+c.x  &&  (uint32)x < map_data->get_width();  x++  ) {
			for(  sint32 y = max(0,c.y);  y < zoom_in+c.y  &&  (uint32)y < map_data->get_height();  y++  ) {
				map_data->at(x, y) = color;
			}
		}
	}
}


/**
 * calculates ground color for position relative to water height
 * @param hoehe height of the tile
 * @param groundwater water height
 */
PIXVAL minimap_t::calc_height_color(const sint16 hoehe, const sint16 groundwater)
{
	sint16 relative_index;
	if(  hoehe>groundwater  ) {
		// adjust index for world_maximum_height
		relative_index = (hoehe-groundwater)*MAX_MAP_TYPE_LAND/world->get_settings().get_maximumheight();
		if(  (hoehe-groundwater)*MAX_MAP_TYPE_LAND%world->get_settings().get_maximumheight()!=0  ) {
			// to avoid relative_index==0
			relative_index += 1;
		}
	}
	else {
		relative_index = hoehe-groundwater;
	}
	return color_idx_to_rgb(map_type_color[clamp( relative_index+MAX_MAP_TYPE_WATER-1, 0, MAX_MAP_TYPE_WATER+MAX_MAP_TYPE_LAND-1 )]);
}


/**
 * Calculates the minimap color of a ground tile
 */
PIXVAL minimap_t::calc_ground_color(const grund_t *gr)
{
	PIXVAL color = color_idx_to_rgb(COL_BLACK);

#ifdef DEBUG_ROUTES
	/* for debug purposes only ...*/
	if(gr->get_flag(grund_t::marked)) {
		color = color_idx_to_rgb(COL_PURPLE);
	}
	else
#endif
	if(gr->get_halt().is_bound()) {
		color = COL_HALT;
	}
	else {
		switch(gr->get_typ()) {
			case grund_t::brueckenboden:
				color = color_idx_to_rgb(MN_GREY3);
				break;
			case grund_t::tunnelboden:
				color = color_idx_to_rgb(COL_BROWN);
				break;
			case grund_t::monorailboden:
				color = COL_MONORAIL;
				break;
			case grund_t::fundament:
				{
					// object at zero is either factory or house (or attraction ... )
					gebaeude_t *gb = gr->find<gebaeude_t>();
					fabrik_t *fab = gb ? gb->get_fabrik() : NULL;
					if(fab==NULL) {
						color = color_idx_to_rgb(COL_GREY3);
					}
					else {
						color = fab->get_color();
					}
				}
				break;
			case grund_t::wasser:
				{
					// object at zero is either factory or boat
					gebaeude_t *gb = gr->find<gebaeude_t>();
					fabrik_t *fab = gb ? gb->get_fabrik() : NULL;
					if(fab==NULL) {
						sint16 height = corner_sw(gr->get_grund_hang());
						if( mode&MAP_HIDE_CONTOUR ) {
							color = color_idx_to_rgb(map_type_color[1]); // second deep water color
						}
						else {
							color = calc_height_color( world->lookup_hgt( gr->get_pos().get_2d() ) + height, world->get_water_hgt( gr->get_pos().get_2d() ) );
						}
						//color = color_idx_to_rgb(COL_BLUE); // water with boat?
					}
					else {
						color = fab->get_color();
					}
				}
				break;
			// normal ground ...
			default:
				if(gr->hat_wege()) {
					switch(gr->get_weg_nr(0)->get_waytype()) {
						case road_wt: color = COL_ROAD; break;
						case tram_wt:
						case track_wt: color = COL_RAIL; break;
						case water_wt: color = COL_CANAL; break;
						case air_wt: color = COL_RUNWAY; break;
						case monorail_wt:
						default: // all other ways light red ...
							color = color_idx_to_rgb(135); break;
					}
				}
				else {
					const leitung_t* lt = gr->find<leitung_t>();
					if(lt!=NULL) {
						color = COL_POWERLINE;
					}
					else {
						if( mode&MAP_HIDE_CONTOUR ) {
							color = color_idx_to_rgb(map_type_color[MAX_MAP_TYPE_WATER+2]); // lowest land color
						}
						else if( mode&MAP_CLIMATES ) {
							static uint8 climate_color[ 8 ] = { 0, COL_YELLOW, COL_LIGHT_GREEN, COL_GREEN, COL_DARK_GREEN, COL_DARK_YELLOW, COL_BROWN, COL_GREY4 };
							color = color_idx_to_rgb( climate_color[ world->get_climate(gr->get_pos().get_2d()) ] );
						}
						else {
							sint16 height = corner_sw(gr->get_grund_hang());
							if(  gr->get_hoehe() > world->get_groundwater()  ) {
								color = calc_height_color( gr->get_hoehe() + height, world->get_groundwater() );
							}
							else {
								color = calc_height_color( gr->get_hoehe() + height, gr->get_hoehe() + height - 1);
							}
						}
					}
				}
				break;
		}
	}
	return color;
}


void minimap_t::calc_map_pixel(const koord k)
{
	// no pixels visible, so noting to calculate
	if(!is_visible) {
		return;
	}

	// always use to uppermost ground
	const planquadrat_t *plan=world->access(k);
	if(plan==NULL  ||  plan->get_boden_count()==0) {
		return;
	}
	const grund_t *gr=plan->get_boden_bei(plan->get_boden_count()-1);

	if(  mode!=MAP_PAX_DEST  &&  gr->get_convoi_vehicle()  ) {
		set_map_color( k, COL_VEHICLE );
		return;
	}

	// first use ground color
	set_map_color( k, calc_ground_color (gr) );

	switch(mode&~MAP_MODE_FLAGS) {
		// show passenger coverage
		// display coverage
		case MAP_PASSENGER:
			for( int i = 0; i < plan->get_haltlist_count(); i++  ) {
				halthandle_t halt = plan->get_haltlist()[i];
				if (halt->get_pax_enabled() && !halt->get_pax_connections().empty()) {
					set_map_color( k, color_idx_to_rgb(halt->get_owner()->get_player_color1() + 3) );
					break;
				}
			}
			break;

		// show mail coverage
		// display coverage
		case MAP_MAIL:
			for( int i = 0; i < plan->get_haltlist_count(); i++  ) {
				halthandle_t halt = plan->get_haltlist()[i];
				if (halt->get_mail_enabled() && !halt->get_mail_connections().empty()) {
					set_map_color( k, color_idx_to_rgb(halt->get_owner()->get_player_color1() + 3) );
					break;
				}
			}
			break;

		// show usage
		case MAP_FREIGHT:
			// need to init the maximum?
			if(max_cargo==0) {
				max_cargo = 1;
				calc_map();
			}
			else if(  gr->hat_wege()  ) {
				// now calc again ...
				sint32 cargo=0;

				// maximum two ways for one ground
				const weg_t *w=gr->get_weg_nr(0);
				if(w) {
					cargo = w->get_statistics(WAY_STAT_GOODS);
					const weg_t *w=gr->get_weg_nr(1);
					if(w) {
						cargo += w->get_statistics(WAY_STAT_GOODS);
					}
					if(  cargo > max_cargo  ) {
						max_cargo = cargo;
					}
					set_map_color(k, calc_severity_color_log(cargo, max_cargo));
				}
			}
			break;

		// show traffic (=convois/month)
		case MAP_TRAFFIC:
			// need to init the maximum?
			if(  max_passed==0  ) {
				max_passed = 1;
				calc_map();
			}
			else if(gr->hat_wege()) {
				// now calc again ...
				sint32 passed=0;

				// maximum two ways for one ground
				const weg_t *w=gr->get_weg_nr(0);
				if(w) {
					passed = w->get_statistics(WAY_STAT_CONVOIS);
					if(  weg_t *w=gr->get_weg_nr(1)  ) {
						passed += w->get_statistics(WAY_STAT_CONVOIS);
					}
					if(  passed > max_passed  ) {
						max_passed = passed;
					}
					set_map_color(k, calc_severity_color_log( passed, max_passed ) );
				}
			}
			break;

		// show tracks: white: no electricity, red: electricity, yellow: signal
		case MAP_TRACKS:
			// show track
			if (gr->hat_weg(track_wt)) {
				const schiene_t * sch = (const schiene_t *) (gr->get_weg(track_wt));
				if(sch->is_electrified()) {
					set_map_color(k, color_idx_to_rgb(COL_RED));
				}
				else {
					set_map_color(k, color_idx_to_rgb(COL_WHITE));
				}
				// show signals
				if(sch->has_sign()  ||  sch->has_signal()) {
					set_map_color(k, color_idx_to_rgb(COL_YELLOW));
				}
			}
			break;

		// show max speed (if there)
		case MAX_SPEEDLIMIT:
			{
				sint32 speed=gr->get_max_speed();
				if(speed) {
					set_map_color(k, calc_severity_color(gr->get_max_speed(), 450));
				}
			}
			break;

		// find power lines
		case MAP_POWERLINES:
			{
				const leitung_t* lt = gr->find<leitung_t>();
				if(lt!=NULL) {
					const sint32 saturated_demand = std::min<uint64>(lt->get_net()->get_demand(), INT32_MAX);
					const sint32 saturated_supply = std::min<uint64>(lt->get_net()->get_supply(), INT32_MAX);
					set_map_color(k, calc_severity_color(saturated_demand, saturated_supply) );
				}
			}
			break;

		case MAP_FOREST:
			if(  gr->get_top()>1  &&  gr->obj_bei(gr->get_top()-1)->get_typ()==obj_t::baum  ) {
				set_map_color(k, color_idx_to_rgb(COL_GREEN) );
			}
			break;

		case MAP_OWNER:
			// show ownership
			{
				if(  gr->is_halt()  ) {
					set_map_color(k, color_idx_to_rgb(gr->get_halt()->get_owner()->get_player_color1()+3));
				}
				else if(  weg_t *weg = gr->get_weg_nr(0)  ) {
					set_map_color(k, weg->get_owner()==NULL ? color_idx_to_rgb(COL_ORANGE) : color_idx_to_rgb(weg->get_owner()->get_player_color1()+3) );
				}
				if(  gebaeude_t *gb = gr->find<gebaeude_t>()  ) {
					if(  gb->get_owner()!=NULL  ) {
						set_map_color(k, color_idx_to_rgb(gb->get_owner()->get_player_color1()+3) );
					}
				}
				break;
			}

		case MAP_LEVEL:
			if(  max_building_level == 0  ) {
				// init maximum
				max_building_level = 1;
				calc_map();
			}
			else if(  gr->get_typ() == grund_t::fundament  ) {
				if(  gebaeude_t *gb = gr->find<gebaeude_t>()  ) {
					if(  gb->is_city_building()  ) {
						sint32 level = gb->get_tile()->get_desc()->get_level();
						if(  level > max_building_level  ) {
							max_building_level = level;
						}
						set_map_color(k, calc_severity_color( level, max_building_level ) );
					}
				}
			}
			break;

		default:
			break;
	}
}


scr_size minimap_t::get_min_size() const
{
	return get_max_size(); //scr_size(0,0);
}


scr_size minimap_t::get_max_size() const
{
	scr_coord size = map_to_screen_coord( koord( world->get_size().x, 0 ) );
	scr_coord down = map_to_screen_coord( koord( world->get_size().x, world->get_size().y ) );
	size.y = down.y;
	if(  isometric  ) {
		size.x += zoom_in*2;
	}
	return scr_size(size.x, size.y);
}


void minimap_t::calc_map_size()
{
	set_size( get_max_size() ); // of the gui_component to adjust scroll bars
	needs_redraw = true;
}


void minimap_t::calc_map()
{
	// only use bitmap size like screen size
	scr_size minimap_size ( min( get_size().w, new_size.w ), min( get_size().h, new_size.h ) );
	// actually the following line should reduce new/deletes, but does not work properly
	if(  map_data==NULL  ||  (sint16) map_data->get_width()!=minimap_size.w  ||  (sint16) map_data->get_height()!=minimap_size.h  ) {
		delete map_data;
		map_data = new array2d_tpl<PIXVAL> ( minimap_size.w,minimap_size.h);
	}
	cur_off = new_off;
	cur_size = new_size;
	needs_redraw = false;
	is_visible = true;

	// redraw the map
	if(  !isometric  ) {
		koord k;
		koord start_off = koord( (cur_off.x*zoom_out)/zoom_in, (cur_off.y*zoom_out)/zoom_in );
		koord end_off = start_off+koord( ( map_data->get_width()*zoom_out)/zoom_in+1, ( map_data->get_height()*zoom_out)/zoom_in+1 );
		for(  k.y=start_off.y;  k.y<end_off.y;  k.y+=zoom_out  ) {
			for(  k.x=start_off.x;  k.x<end_off.x;  k.x+=zoom_out  ) {
				calc_map_pixel(k);
			}
		}
	}
	else {
		// always the whole map ...
		if(isometric) {
			map_data->init( color_idx_to_rgb(COL_BLACK) );
		}
		koord k;
		for(  k.y=0;  k.y < world->get_size().y;  k.y++  ) {
			for(  k.x=0;  k.x < world->get_size().x;  k.x++  ) {
				calc_map_pixel(k);
			}
		}
	}
}


minimap_t::minimap_t()
{
	map_data = NULL;
	zoom_in = 1;
	zoom_out = 1;
	isometric = false;
	show_network_load_factor = false;
	mode = MAP_TOWN;
	selected_city = NULL;
	cur_off = new_off = scr_coord(0,0);
	cur_size = new_size = scr_size(0,0);
	needs_redraw = true;
	transport_type_showed_on_map = simline_t::line;
}


minimap_t::~minimap_t()
{
	delete map_data;
}


minimap_t *minimap_t::get_instance()
{
	if(single_instance == NULL) {
		single_instance = new minimap_t();
	}
	return single_instance;
}


void minimap_t::init()
{
	delete map_data;
	map_data = NULL;
	needs_redraw = true;
	is_visible = false;

	calc_map_size();
	max_building_level = max_cargo = max_passed = 0;
	max_tourist_ziele = max_waiting = max_origin = max_transfer = max_service = 1;
	last_schedule_counter = world->get_schedule_counter()-1;
	set_selected_cnv(convoihandle_t());
}


void minimap_t::set_display_mode(MAP_DISPLAY_MODE new_mode)
{
	mode = new_mode;
	needs_redraw = true;
}


void minimap_t::new_month()
{
	needs_redraw = true;
}


void minimap_t::invalidate_map_lines_cache()
{
	last_schedule_counter = world->get_schedule_counter() - 1;
	needs_redraw = true;
}


// these two are the only gui_container specific routines


// handle event
bool minimap_t::infowin_event(const event_t *ev)
{
	scr_coord c( ev->mx, ev->my );
	koord k = screen_to_map_coord(c);

	// get factory under mouse cursor
	last_world_pos = k;

	// recenter
	if(IS_LEFTCLICK(ev) || IS_LEFTDRAG(ev)) {
		world->get_viewport()->set_follow_convoi( convoihandle_t() );

		const sint8 min_hgt = world->is_within_grid_limits(k) ? world->min_hgt(k) : 0;

		world->get_viewport()->change_world_position(koord3d(k,min_hgt));
		return true;
	}

	return false;
}


// helper function for finding nearby factory
const fabrik_t* minimap_t::get_factory_near( const koord, bool enlarge ) const
{
	const fabrik_t *fab = fabrik_t::get_fab(last_world_pos);
	for(  int i=0;  i<4  && fab==NULL;  i++  ) {
		fab = fabrik_t::get_fab( last_world_pos+koord::nesw[i] );
	}
	if(  enlarge  ) {
		for(  int i=0;  i<4  && fab==NULL;  i++  ) {
			fab = fabrik_t::get_fab( last_world_pos+koord::nesw[i]*2 );
		}
	}
	return fab;
}


// helper function for redraw: factory connections
const fabrik_t* minimap_t::draw_factory_connections(const fabrik_t* const fab, bool supplier_link, const scr_coord pos) const
{
	if(fab) {
		PIXVAL color = supplier_link ? color_idx_to_rgb(COL_RED) : color_idx_to_rgb(COL_WHITE);
		scr_coord fabpos = map_to_screen_coord( fab->get_pos().get_2d() ) + pos;
		const vector_tpl<koord>& consumer = supplier_link ? fab->get_suppliers() : fab->get_consumer();
		for(koord lieferziel : consumer) {
			const fabrik_t * fab2 = fabrik_t::get_fab(lieferziel);
			if (fab2) {
				const scr_coord end = map_to_screen_coord( lieferziel ) + pos;
				display_direct_line_rgb(fabpos.x, fabpos.y, end.x, end.y, color);
				display_fillbox_wh_clip_rgb(end.x, end.y, 3, 3, ((world->get_ticks() >> 10) & 1) == 0 ? color_idx_to_rgb(COL_RED) : color_idx_to_rgb(COL_WHITE), true);

				scr_coord boxpos = end + scr_coord(10, 0);
				const char * name = translator::translate(fab2->get_name());
				int name_width = proportional_string_width(name)+(LINESPACE/2);
				boxpos.x = clamp( boxpos.x, pos.x, pos.x+get_size().w-name_width );
				display_ddd_proportional_clip(boxpos.x, boxpos.y, color_idx_to_rgb(5), color_idx_to_rgb(COL_WHITE), name, true);
			}
		}
	}
	return fab;
}


// show the schedule on the minimap
void minimap_t::set_selected_cnv( convoihandle_t c )
{
	current_cnv = c;
	schedule_cache.clear();
	stop_cache.clear();
	colore_idx = 0;
	add_to_schedule_cache( current_cnv, true );
	last_schedule_counter = world->get_schedule_counter()-1;
}


// draw the map (and the overlays!)
void minimap_t::draw(scr_coord pos)
{
	// sanity check, needed for overlarge maps
	if(  (new_off.x|new_off.y)<0  ) {
		new_off = cur_off;
	}
	if(  (new_size.w|new_size.h)<0  ) {
		new_size = cur_size;
	}

	if(  last_mode != mode  ) {
		// only needing update, if last mode was also not about halts or background rendering ...
		needs_redraw = (mode^last_mode) & (~MAP_MODE_FLAGS | MAP_CLIMATES | MAP_HIDE_CONTOUR);

		if(  (mode & MAP_LINES) == 0  ||  (mode^last_mode) & MAP_MODE_HALT_FLAGS  ) {
			// rebuilt stop_cache needed
			stop_cache.clear();
			if(  (mode & MAP_LINES)  &&  (last_mode & MAP_LINES)  &&  current_cnv.is_bound()  ) {
				schedule_cache.clear();
				add_to_schedule_cache(current_cnv, true);
				needs_redraw = true;
			}
		}

		if(  (mode & MAP_LINES)  &&  (last_mode & MAP_LINES) == 0  &&  current_cnv.is_bound()  ) {
			schedule_cache.clear();
			add_to_schedule_cache(current_cnv, true);
			needs_redraw = true;
		}

		if(  (mode^last_mode) & (MAP_PASSENGER|MAP_MAIL|MAP_FREIGHT|MAP_LINES)  ||  (mode&MAP_LINES  &&  stop_cache.empty())  ) {
			// rebuilt line display
			last_schedule_counter = world->get_schedule_counter()-1;
		}

		last_mode = mode;
	}

	if(  needs_redraw  ||  cur_off!=new_off  ||  cur_size!=new_size  ) {
		calc_map();
		needs_redraw = false;
	}

	if( map_data==NULL) {
		return;
	}

	if(  mode & MAP_PAX_DEST  &&  selected_city!=NULL  ) {
		const uint32 current_pax_destinations = selected_city->get_pax_destinations_new_change();
		if(  pax_destinations_last_change > current_pax_destinations  ) {
			// new month started.
			calc_map();
		}
		else if(  pax_destinations_last_change < current_pax_destinations  ) {
			// new pax_dest in city.
			const sparse_tpl<PIXVAL> *pax_dests = selected_city->get_pax_destinations_new();
			koord pos, min, max;
			PIXVAL color;
			for(  uint16 i = 0;  i < pax_dests->get_data_count();  i++  ) {
				pax_dests->get_nonzero( i, pos, color );
				min = koord((pos.x*world->get_size().x)/PAX_DESTINATIONS_SIZE,
				            (pos.y*world->get_size().y)/PAX_DESTINATIONS_SIZE);
				max = koord(((pos.x+1)*world->get_size().x)/PAX_DESTINATIONS_SIZE,
				            ((pos.y+1)*world->get_size().y)/PAX_DESTINATIONS_SIZE);
				pos = min;
				do {
					do {
						set_map_color(pos, color);
						pos.y++;
					} while(pos.y < max.y);
					pos.x++;
				} while (pos.x < max.x);
			}
		}
		pax_destinations_last_change = selected_city->get_pax_destinations_new_change();
	}

	if(  (uint16)cur_size.w > map_data->get_width()  ) {
		display_fillbox_wh_clip_rgb( pos.x+new_off.x+map_data->get_width(), new_off.y+pos.y, 32767, map_data->get_height(), color_idx_to_rgb(COL_BLACK), true);
	}
	if(  (uint16)cur_size.h > map_data->get_height()  ) {
		display_fillbox_wh_clip_rgb( pos.x+new_off.x, pos.y+new_off.y+map_data->get_height(), 32767, 32767, color_idx_to_rgb(COL_BLACK), true);
	}
	display_array_wh( cur_off.x+pos.x, new_off.y+pos.y, map_data->get_width(), map_data->get_height(), map_data->to_array());

	if(  !current_cnv.is_bound()  &&  mode & MAP_LINES    ) {
		vector_tpl<linehandle_t> linee;

		if(  last_schedule_counter != world->get_schedule_counter()  ) {
			// rebuild cache
			last_schedule_counter = world->get_schedule_counter();
			schedule_cache.clear();
			stop_cache.clear();
			waypoint_hash.clear();
			colore_idx = 0;

			for(  int np = 0;  np < MAX_PLAYER_COUNT;  np++  ) {
				if(  player_showed_on_map != -1  &&  player_showed_on_map != np  ) {
					continue;
				}
				//cycle on players
				if(  world->get_player( np )  &&  world->get_player( np )->simlinemgmt.get_line_count() > 0   ) {

					world->get_player( np )->simlinemgmt.get_lines( simline_t::line, &linee );
					for(  uint32 j = 0;  j < linee.get_count();  j++  ) {
						//cycle on lines

						if(  transport_type_showed_on_map != simline_t::line  &&  linee[j]->get_linetype() != transport_type_showed_on_map  ) {
							continue;
						}

						if(  !is_matching_freight_catg( linee[j]->get_goods_catg_index() )  ) {
							continue;
						}

						// ware matches; now find at least a running convoi on this line ...
						convoihandle_t cnv;
						for(  uint32 k = 0;  k < linee[j]->count_convoys();  k++  ) {
							convoihandle_t test_cnv = linee[j]->get_convoy(k);
							if(  test_cnv.is_bound()  ) {
								int state = test_cnv->get_state();
								if( state != convoi_t::INITIAL  &&  state != convoi_t::ENTERING_DEPOT  &&  state != convoi_t::SELF_DESTRUCT  ) {
									cnv = test_cnv;
									break;
								}
							}
						}
						if(  !cnv.is_bound()  ) {
							continue;
						}
						int state = cnv->get_state();
						if( state != convoi_t::INITIAL  &&  state != convoi_t::ENTERING_DEPOT  &&  state != convoi_t::SELF_DESTRUCT  ) {
							add_to_schedule_cache( cnv, false );
						}
					}
				}
			}

			// now add all unbound convois
			player_t * required_vehicle_owner = NULL;
			if (player_showed_on_map != -1) {
				required_vehicle_owner = world->get_player(player_showed_on_map);
			}
			for(convoihandle_t cnv : world->convoys() ) {
				if(  !cnv.is_bound()  ||  cnv->get_line().is_bound()  ) {
					// not there or already part of a line
					continue;
				}
				if(  required_vehicle_owner!= NULL  &&  required_vehicle_owner != cnv->get_owner()  ) {
					continue;
				}
				if(  transport_type_showed_on_map != simline_t::line  ) {
					if(  transport_type_showed_on_map != simline_t::waytype_to_linetype(cnv->front()->get_waytype())  ) {
						continue;
					}
				}
				int state = cnv->get_state();
				if(  state != convoi_t::INITIAL  &&  state != convoi_t::ENTERING_DEPOT  &&  state != convoi_t::SELF_DESTRUCT  ) {
					if(  !is_matching_freight_catg(cnv->get_goods_catg_index())  ) {
						continue;
					}
					add_to_schedule_cache( cnv, false );
				}
			}
		}
		/************ ATTENTION: The schedule pointers schedule in the line segments ******************
		 ************            are invalid after this point!                       ******************/
	}
	//end MAP_LINES

	bool showing_schedule = false;
	if(  mode & MAP_LINES  ) {
		showing_schedule = !schedule_cache.empty();
	}
	else {
		schedule_cache.clear();
		colore_idx = 0;
		last_schedule_counter = world->get_schedule_counter()-1;
	}

	// since the schedule whitens out the background, we have to draw it first
	int offset = 1;
	koord last_start(0,0), last_end(0,0);
	bool diagonal = false;
	if(  showing_schedule  ) {
		// lighten background
		if(  isometric  ) {
			// isometric => lighten in three parts

			scr_coord p1 = map_to_screen_coord( koord(0,0) );
			scr_coord p2 = map_to_screen_coord( koord( world->get_size().x, 0 ) );
			scr_coord p3 = map_to_screen_coord( koord( world->get_size().x, world->get_size().y ) );
			scr_coord p4 = map_to_screen_coord( koord( 0, world->get_size().y ) );

			// top and bottom part
			const int toplines = min( p4.y, p2.y );
			for( scr_coord_val y = 0;  y < toplines;  y++  ) {
				display_blend_wh_rgb( pos.x+p1.x-2*y, pos.y+y, 4*y+4, 1, color_idx_to_rgb(COL_WHITE), 75 );
				display_blend_wh_rgb( pos.x+p3.x-2*y, pos.y+p3.y-y-1, 4*y+4, 1, color_idx_to_rgb(COL_WHITE), 75 );
			}
			// center area
			if(  p1.x < p3.x  ) {
				for( scr_coord_val y = toplines;  y < p3.y-toplines;  y++  ) {
					display_blend_wh_rgb( pos.x+(y-toplines)*2, pos.y+y, 4*toplines+4, 1, color_idx_to_rgb(COL_WHITE), 75 );
				}
			}
			else {
				for( scr_coord_val y = toplines;  y < p3.y-toplines;  y++  ) {
					display_blend_wh_rgb( pos.x+(y-toplines)*2, pos.y+p3.y-y-1, 4*toplines+4, 1, color_idx_to_rgb(COL_WHITE), 75 );
				}
			}
		}
		else {
			// easier with rectangular maps ...
			display_blend_wh_rgb( cur_off.x+pos.x, cur_off.y+pos.y, map_data->get_width(), map_data->get_height(), color_idx_to_rgb(COL_WHITE), 75 );
		}

		scr_coord k1,k2;
		// DISPLAY STATIONS AND AIRPORTS: moved here so station spots are not overwritten by lines drawn
		for(line_segment_t seg : schedule_cache  ) {

			uint8 color = seg.colorcount;
			if(  (event_get_last_control_shift() ^ tool_t::control_invert)==2  ||  current_cnv.is_bound()  ) {
				// on control / single convoi use only player colors
				static uint8 last_color = color;
				color = seg.player->get_player_color1()+1;
				// all lines same thickness if same color
				if(  color == last_color  ) {
					offset = 0;
				}
				last_color = color;
			}
			if(  seg.start != last_start  ||  seg.end != last_end  ) {
				last_start = seg.start;
				k1 = map_to_screen_coord( seg.start );
				k1 += pos;
				last_end = seg.end;
				k2 = map_to_screen_coord( seg.end );
				k2 += pos;
				// use same diagonal for all parallel segments
				diagonal = seg.start_diagonal;
			}
			// and finally draw ...
			line_segment_draw( seg.waytype, k1, seg.start_offset*offset, k2, seg.end_offset*offset, diagonal, color_idx_to_rgb(color) );
		}
	}

	// display station information here (even without overlay)
	halthandle_t display_station;
	// only fill cache if needed
	if(  mode & MAP_MODE_HALT_FLAGS  &&  stop_cache.empty()  ) {
		if(  mode & MAP_ORIGIN  ) {
			for(halthandle_t halt : haltestelle_t::get_alle_haltestellen() ) {
				if(  halt->get_pax_enabled()  ||  halt->get_mail_enabled()  ) {
					stop_cache.append( halt );
				}
			}
		}
		else if(  mode & MAP_TRANSFER  ) {
			for(halthandle_t halt : haltestelle_t::get_alle_haltestellen() ) {
				for (int catg = goods_manager_t::INDEX_PAS; catg != goods_manager_t::get_max_catg_index(); ++catg) {
					if (catg != goods_manager_t::INDEX_NONE && halt->is_transfer(catg)) {
						stop_cache.append(halt);
						break;
					}
				}
			}
		}
		else if(  mode&MAP_STATUS  ||  mode&MAP_SERVICE  ||  mode&MAP_WAITING  ||  mode&MAP_WAITCHANGE  ) {
			for(halthandle_t halt : haltestelle_t::get_alle_haltestellen() ) {
				stop_cache.append( halt );
			}
		}
	}
	// now draw stop cache
	// if needed to get new values
	sint32 new_max_waiting_change = 1;
	for(halthandle_t station : stop_cache  ) {

		if(  !station.is_bound()  ) {
			// maybe deleted in the meanwhile
			continue;
		}

		int radius = 0;
		PIXVAL color;
		int diagonal_dist = 0;
		scr_coord temp_stop = map_to_screen_coord( station->get_basis_pos() );
		temp_stop = temp_stop + pos;

		if(  mode & MAP_STATUS  ) {
			color = station->get_status_farbe();
			radius = number_to_radius( station->get_capacity(0) );
		}
		else if( mode & MAP_SERVICE  ) {
			const sint32 service = (sint32)station->get_finance_history( 1, HALT_CONVOIS_ARRIVED );
			if(  service > max_service  ) {
				max_service = service;
			}
			color = calc_severity_color_log( service, max_service );
			radius = log2( (uint32)( (service << 7) / max_service ) );
		}
		else if( mode & MAP_WAITING  ) {
			const sint32 waiting = (sint32)station->get_finance_history( 0, HALT_WAITING );
			if(  waiting > max_waiting  ) {
				max_waiting = waiting;
			}
			color = calc_severity_color_log( waiting, max_waiting );
			radius = number_to_radius( waiting );
		}
		else if( mode & MAP_WAITCHANGE  ) {
			const sint32 waiting_diff = (sint32)(station->get_finance_history( 0, HALT_WAITING ) - station->get_finance_history( 1, HALT_WAITING ));
			if(  waiting_diff > new_max_waiting_change  ) {
				new_max_waiting_change = waiting_diff;
			}
			if(  waiting_diff < -new_max_waiting_change  ) {
				new_max_waiting_change = -waiting_diff;
			}
			const sint32 span = max(new_max_waiting_change,max_waiting_change);
			const sint32 diff = waiting_diff + span;
			color = calc_severity_color( diff, span*2 ) ;
			radius = number_to_radius( abs(waiting_diff) );
		}
		else if( mode & MAP_ORIGIN  ) {
			if(  !station->get_pax_enabled()  &&  !station->get_mail_enabled()  ) {
				continue;
			}
			const sint32 pax_origin = (sint32)(station->get_finance_history( 1, HALT_HAPPY ) + station->get_finance_history( 1, HALT_UNHAPPY ) + station->get_finance_history( 1, HALT_NOROUTE ));
			if(  pax_origin > max_origin  ) {
				max_origin = pax_origin;
			}
			color = calc_severity_color_log( pax_origin, max_origin );
			radius = number_to_radius( pax_origin );
		}
		else if( mode & MAP_TRANSFER  ) {
			const sint32 transfer = (sint32)(station->get_finance_history( 1, HALT_ARRIVED ) + station->get_finance_history( 1, HALT_DEPARTED ));
			if(  transfer > max_transfer  ) {
				max_transfer = transfer;
			}
			color = calc_severity_color_log( transfer, max_transfer );
			radius = number_to_radius( transfer );
		}
		else {
			const int stype = station->get_station_type();
			color = color_idx_to_rgb(station->get_owner()->get_player_color1()+3);

			// invalid=0, loadingbay=1, railstation = 2, dock = 4, busstop = 8, airstop = 16, monorailstop = 32, tramstop = 64, maglevstop=128, narrowgaugestop=256
			if(  stype > 0  ) {
				radius = 1;
				if(  stype & ~(haltestelle_t::loadingbay | haltestelle_t::busstop | haltestelle_t::tramstop)  ) {
					radius = 3;
				}
			}
			// with control, show only circles
			if((event_get_last_control_shift() ^ tool_t::control_invert)==2) {
				// else elongate them ...
				const int key = station->get_basis_pos().x + station->get_basis_pos().y * world->get_size().x;
				diagonal_dist = waypoint_hash.get( key ).get_count();
				if(  diagonal_dist  ) {
					diagonal_dist--;
				}
				diagonal_dist = (diagonal_dist*3)-1;
			}

			// show the mode of transport of the station
			if(  skinverwaltung_t::station_type  ) {
				int icon = 0;
				for(  int type=0;  type<9;  type++  ) {
					if(  (stype>>type)&1  ) {
						image_id img = skinverwaltung_t::station_type->get_image_id(type);
						if(  img!=IMG_EMPTY  ) {
							display_color_img( img, temp_stop.x+diagonal_dist+4+(icon/2)*12, temp_stop.y+diagonal_dist+4+(icon&1)*12, station->get_owner()->get_player_nr(), false, false );
							icon++;
						}
					}
				}
			}
			else {
				if(  stype & haltestelle_t::airstop  ) {
					display_airport( temp_stop.x+diagonal_dist, temp_stop.y+diagonal_dist, color );
				}

				if(  stype & haltestelle_t::dock  ) {
					display_harbor( temp_stop.x+diagonal_dist, temp_stop.y+diagonal_dist, color );
				}
			}
		}

		// avoid too small circles when zoomed out
		if(  zoom_in > 1  ) {
			radius ++;
		}

		int out_radius = (radius == 0) ? 1 : radius;
		display_filled_circle_rgb( temp_stop.x, temp_stop.y, radius, color );
		display_circle_rgb( temp_stop.x, temp_stop.y, out_radius, color_idx_to_rgb(COL_BLACK) );
		if(  diagonal_dist>0  ) {
			display_filled_circle_rgb( temp_stop.x+diagonal_dist, temp_stop.y+diagonal_dist, radius, color );
			display_circle_rgb( temp_stop.x+diagonal_dist, temp_stop.y+diagonal_dist, out_radius, color_idx_to_rgb(COL_BLACK) );
			for(  int i=1;  i < diagonal_dist;  i++  ) {
				display_filled_circle_rgb( temp_stop.x+i, temp_stop.y+i, radius, color );
			}
			out_radius = sqrt_i32( 2*out_radius+1 );
			display_direct_line_rgb( temp_stop.x+out_radius, temp_stop.y-out_radius, temp_stop.x+out_radius+diagonal_dist, temp_stop.y-out_radius+diagonal_dist, color_idx_to_rgb(COL_BLACK) );
			display_direct_line_rgb( temp_stop.x-out_radius, temp_stop.y+out_radius, temp_stop.x-out_radius+diagonal_dist, temp_stop.y+out_radius+diagonal_dist, color_idx_to_rgb(COL_BLACK) );
		}

		if(  koord_distance( last_world_pos, station->get_basis_pos() ) <= 2  ) {
			// draw stop name with an index if close to mouse
			display_station = station;
		}
	}
	if(  display_station.is_bound()  ) {
		scr_coord temp_stop = map_to_screen_coord( display_station->get_basis_pos() );
		temp_stop = temp_stop + pos;
		display_ddd_proportional_clip( temp_stop.x + 10, temp_stop.y + 7, color_idx_to_rgb(display_station->get_owner()->get_player_color1()+3), color_idx_to_rgb(COL_WHITE), display_station->get_name(), false );
	}
	max_waiting_change = new_max_waiting_change; // update waiting tendencies

	// if we do not do this here, vehicles would erase the town names
	// ADD: if CRTL key is pressed, temporary show the name
	if(  mode & MAP_TOWN  ) {
		const weighted_vector_tpl<stadt_t*>& staedte = world->get_cities();
		const PIXVAL col = color_idx_to_rgb(showing_schedule ? COL_BLACK : COL_WHITE);

		for(stadt_t* const stadt : staedte ) {
			const char * name = stadt->get_name();

			scr_coord p = map_to_screen_coord( stadt->get_pos() );
			p += pos;
			display_proportional_clip_rgb( p.x, p.y, name, ALIGN_LEFT, col, true );
		}
	}

	// draw city limit
	if(  mode & MAP_CITYLIMIT  ) {
		const PIXVAL col = color_idx_to_rgb(showing_schedule ? COL_DARK_BROWN : COL_ORANGE);

		// for all cities
		for(stadt_t* const stadt :  world->get_cities()  ) {
			koord k[4];
			k[0] = stadt->get_linksoben(); // top left
			k[2] = stadt->get_rechtsunten(); // bottom right
			k[1] =  koord(k[0].x, k[2].y); // bottom left
			k[3] =  koord(k[2].x, k[0].y); // top right

			k[0] += koord(0, -1); // top left
			k[2] += koord(1,  0); // bottom right
			k[3] += koord(1, -1); // top right

			// calculate and draw the rotated coordinates
			scr_coord c[4];
			for(uint i=0; i<lengthof(c); i++) {
				c[i] = map_to_screen_coord(k[i]) + pos;
			}

			display_direct_line_dotted_rgb( c[0].x, c[0].y, c[1].x, c[1].y, 3, 3, col );
			display_direct_line_dotted_rgb( c[1].x, c[1].y, c[2].x, c[2].y, 3, 3, col );
			display_direct_line_dotted_rgb( c[2].x, c[2].y, c[3].x, c[3].y, 3, 3, col );
			display_direct_line_dotted_rgb( c[3].x, c[3].y, c[0].x, c[0].y, 3, 3, col );
		}
	}

	// since we do iterate the tourist info list, this must be done here
	// find tourist spots
	if(  mode & MAP_TOURIST  ) {
		for(gebaeude_t* const gb : world->get_attractions()  ) {
			if(  gb->get_first_tile() == gb  ) {
				scr_coord gb_pos = map_to_screen_coord( gb->get_pos().get_2d() );
				gb_pos = gb_pos + pos;
				int const pax = gb->get_passagier_level();
				if(  max_tourist_ziele < pax  ) {
					max_tourist_ziele = pax;
				}
				PIXVAL color = calc_severity_color_log(gb->get_passagier_level(), max_tourist_ziele);
				int radius = max( (number_to_radius( pax*4 )*zoom_in)/zoom_out, 1 );
				display_filled_circle_rgb( gb_pos.x, gb_pos.y, radius, color );
				display_circle_rgb( gb_pos.x, gb_pos.y, radius, color_idx_to_rgb(COL_BLACK) );
			}
			// otherwise larger attraction will be shown more often ...
		}
	}

	if(  mode & MAP_FACTORIES  ) {
		for(fabrik_t* const f :  world->get_fab_list()  ) {
			// find top-left tile position
			koord3d fab_tl_pos = f->get_pos();
			if (grund_t *gr = world->lookup(f->get_pos())) {
				if (gebaeude_t* gb = gr->find<gebaeude_t>()) {
					fab_tl_pos = gb->get_pos() - gb->get_tile()->get_offset();
				}
			}
			scr_coord fab_pos = map_to_screen_coord( fab_tl_pos.get_2d() );
			fab_pos = fab_pos + pos;
			koord size = f->get_desc()->get_building()->get_size(f->get_rotate());
			sint16 x_size = max( 5, size.x*zoom_in );
			sint16 y_size = max( 5, size.y*zoom_in );
			display_fillbox_wh_clip_rgb( fab_pos.x-1, fab_pos.y-1, x_size+2, y_size+2, color_idx_to_rgb(COL_BLACK), false );
			display_fillbox_wh_clip_rgb( fab_pos.x, fab_pos.y, x_size, y_size, f->get_color(), false );
		}
	}

	if(  mode & MAP_DEPOT  ) {
		for(depot_t* const d :  depot_t::get_depot_list()  ) {
			if(  d->get_owner() == world->get_active_player()  ) {
				scr_coord depot_pos = map_to_screen_coord( d->get_pos().get_2d() );
				depot_pos = depot_pos + pos;
				// offset of one to avoid
				static uint8 depot_typ_to_color[19]={ COL_ORANGE, COL_YELLOW, COL_RED, 0, 0, 0, 0, 0, 0, COL_PURPLE, COL_DARK_RED, COL_DARK_ORANGE, 0, 0, 0, 0, 0, 0, COL_LIGHT_RED };
				display_filled_circle_rgb( depot_pos.x, depot_pos.y, 4, color_idx_to_rgb(depot_typ_to_color[d->get_typ() - obj_t::bahndepot]) );
				display_circle_rgb( depot_pos.x, depot_pos.y, 4, color_idx_to_rgb(COL_BLACK) );
			}
		}
	}

	// zoom/resize "selection box" in map
	// this must be rotated by 45 degree (sin45=cos45=0,5*sqrt(2)=0.707...)
	const sint16 raster=get_tile_raster_width();

	// calculate and draw the rotated coordinates
	koord ij = world->get_viewport()->get_world_position();
	const koord diff = koord( display_get_width()/(2*raster), display_get_height()/raster );

	koord view[4];
	scr_coord test[4];
	// default coordinates - may be off if screen shows high mountains
	view[0] = ij + koord( -diff.y+diff.x, -diff.y-diff.x );
	view[1] = ij + koord( -diff.y-diff.x, -diff.y+diff.x );
	view[2] = ij + koord( diff.y-diff.x, diff.y+diff.x );
	view[3] = ij + koord( diff.y+diff.x, diff.y-diff.x );

	// try to find tile under the four corners of the screen
	test[0] = scr_coord(display_get_width(),0);
	test[1] = scr_coord(0,0);
	test[2] = scr_coord(0,display_get_height());
	test[3] = scr_coord(display_get_width(),display_get_height());

	for(int i=0; i<4; i++) {
		sint32 dummy1, dummy2;
		if (grund_t *gr = world->get_viewport()->get_ground_on_screen_coordinate( test[i], dummy1, dummy2 ) ) {
			view[i] = gr->get_pos().get_2d();
		}
	}

	scr_coord c[4];
	// translate to coordinates in the minimap
	for(  int i=0;  i<4;  i++  ) {
		c[i] = map_to_screen_coord( view[i] ) + pos;
	}
	for(  int i=0;  i<4;  i++  ) {
		display_direct_line_rgb( c[i].x, c[i].y, c[(i+1)%4].x, c[(i+1)%4].y, color_idx_to_rgb(showing_schedule?COL_RED:COL_YELLOW));
	}

	if(  !showing_schedule  ) {
		// Add factory name tooltips and draw factory connections, if on a factory
		const fabrik_t* const fab = get_factory_near(last_world_pos, (mode & MAP_FACTORIES));
		if(fab) {
			if (mode & MAP_FACTORIES) {
				draw_factory_connections(fab,event_get_last_control_shift() & 1, pos);
			}
			scr_coord fabpos = map_to_screen_coord( fab->get_pos().get_2d() );
			scr_coord boxpos = fabpos + scr_coord(10, 0);
			const char * name = translator::translate(fab->get_name());
			int name_width = proportional_string_width(name)+(LINESPACE/2);
			boxpos.x = clamp( boxpos.x, 0, 0+get_size().w-name_width );
			boxpos += pos;
			display_ddd_proportional_clip(boxpos.x, boxpos.y, color_idx_to_rgb(10), color_idx_to_rgb(COL_WHITE), name, true);
		}

		for (uint32 i = 0; i < win_get_open_count(); i++) {
			gui_frame_t *g = win_get_index(i);
			if(g->get_rdwr_id()== magic_factory_info) {
				// is a factory info window
				const fabrik_t * const fab = dynamic_cast<fabrik_info_t *>(g)->get_factory();
				draw_factory_connections(fab, true, pos);
				draw_factory_connections(fab, false, pos);
			}
		}

	}

	if (current_cnv.is_bound()) {
		scr_coord_val offset = zoom_in / zoom_out /2;
		scr_coord_val wd = max( zoom_in/zoom_out, 1) * 2 + 1; // to have it always odd
		for(  int i = 0;  i < current_cnv->get_vehicle_count();  i++  ) {
			const scr_coord veh_pos = map_to_screen_coord(current_cnv->get_vehicle(i)->get_pos().get_2d()) + pos;
			display_fillbox_wh_clip_rgb(veh_pos.x-(wd/2)+offset, veh_pos.y-(wd/2)+offset, wd, wd, color_idx_to_rgb(COL_MAGENTA), true);
		}
	}
}


void minimap_t::set_selected_city( const stadt_t* _city )
{
	if(  selected_city != _city  ) {
		selected_city = _city;
		if(  _city  ) {
			pax_destinations_last_change = _city->get_pax_destinations_new_change();
		}
		calc_map();
	}
}


void minimap_t::rdwr(loadsave_t *file)
{
	file->rdwr_short(zoom_out);
	file->rdwr_short(zoom_in);
	file->rdwr_bool(isometric);
}


bool minimap_t::is_matching_freight_catg(const minivec_tpl<uint8> &goods_catg_index)
{
	// does this line/convoi has a matching freight
	if(  freight_type_group_index_showed_on_map == goods_manager_t::passengers  ) {
		return goods_catg_index.is_contained(goods_manager_t::INDEX_PAS);
	}
	else if(  freight_type_group_index_showed_on_map == goods_manager_t::mail  ) {
		return goods_catg_index.is_contained(goods_manager_t::INDEX_MAIL);
	}
	else if(  freight_type_group_index_showed_on_map == goods_manager_t::none  ) {
		// all freights but not pax or mail
		for(  uint8 i = 0;  i < goods_catg_index.get_count();  i++  ) {
			if(  goods_catg_index[i] > goods_manager_t::INDEX_NONE  ) {
				return true;
			}
		}
		return false;
	}
	else if(  freight_type_group_index_showed_on_map != NULL  ) {
		for(  uint8 i = 0;  i < goods_catg_index.get_count();  i++  ) {
			if(  goods_catg_index[i] == freight_type_group_index_showed_on_map->get_catg_index()  ) {
				return true;
			}
		}
		return false;
	}
	// NULL show all but obey modes
	if(  mode & MAP_PASSENGER  ) {
		return goods_catg_index.is_contained(goods_manager_t::INDEX_PAS);
	}
	else if(  mode & MAP_MAIL  ) {
		return goods_catg_index.is_contained(goods_manager_t::INDEX_MAIL);
	}
	else if(  mode & MAP_FREIGHT  ) {
		// all freights but not pax or mail
		for(  uint8 i = 0;  i < goods_catg_index.get_count();  i++  ) {
			if(  goods_catg_index[i]>2  ) {
				return true;
			}
		}
		return false;
	}
	// all true
	return true;
}
