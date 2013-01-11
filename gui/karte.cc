#include "../simevent.h"
#include "../simcolor.h"
#include "../simconvoi.h"
#include "../simworld.h"
#include "../simdepot.h"
#include "../simhalt.h"
#include "../simskin.h"
#include "../boden/grund.h"
#include "../simfab.h"
#include "../simcity.h"
#include "karte.h"
#include "fahrplan_gui.h"

#include "../dataobj/translator.h"
#include "../dataobj/einstellungen.h"
#include "../dataobj/fahrplan.h"
#include "../dataobj/powernet.h"
#include "../dataobj/ribi.h"
#include "../dataobj/loadsave.h"

#include "../boden/wege/schiene.h"
#include "../dings/leitung2.h"
#include "../utils/cbuffer_t.h"
#include "../simgraph.h"
#include "../simtools.h"
#include "../player/simplay.h"

#include "../tpl/inthashtable_tpl.h"
#include "../tpl/slist_tpl.h"

#include <math.h>

sint32 reliefkarte_t::max_cargo=0;
sint32 reliefkarte_t::max_passed=0;

static sint32 max_waiting_change = 1;

static sint32 max_tourist_ziele = 1;
static sint32 max_waiting = 1;
static sint32 max_origin = 1;
static sint32 max_transfer = 1;
static sint32 max_service = 1;

static sint32 max_building_level = 0;

reliefkarte_t * reliefkarte_t::single_instance = NULL;
karte_t * reliefkarte_t::welt = NULL;
reliefkarte_t::MAP_MODES reliefkarte_t::mode = MAP_TOWN;
reliefkarte_t::MAP_MODES reliefkarte_t::last_mode = MAP_TOWN;
bool reliefkarte_t::is_visible = false;

#define MAX_MAP_TYPE_LAND 31
#define MAX_MAP_TYPE_WATER 5

// color for the land
static const uint8 map_type_color[MAX_MAP_TYPE_WATER+MAX_MAP_TYPE_LAND] =
{
	97, 99, 19, 21, 23,
	160, 161, 162, 163, 164, 165, 166, 167,
	205, 206, 207, 172, 174,
	159, 158, 157, 156, /* 155 monorail */ 154,
	115, 114, 113, 112,
	216, 217, 218, 219, 220, 221, 222, 223, 224
};

const uint8 reliefkarte_t::severity_color[MAX_SEVERITY_COLORS] =
{
	106, 2, 85, 86, 29, 30, 171, 71, 39, 132
};

// Kenfarben fuer die Karte
#define STRASSE_KENN      (208)
#define SCHIENE_KENN      (185)
#define CHANNEL_KENN      (147)
#define MONORAIL_KENN      (155)
#define RUNWAY_KENN      (28)
#define POWERLINE_KENN      (55)
#define HALT_KENN         COL_RED
#define BUILDING_KENN      COL_GREY3


// helper function for line segment_t
bool reliefkarte_t::line_segment_t::operator == (const line_segment_t & k) const
{
	return start == k.start  &&  end == k.end  &&  sp == k.sp  &&  fpl->similar( sp->get_welt(), k.fpl, sp );
}

// Ordering based on first start then end coordinate
bool reliefkarte_t::LineSegmentOrdering::operator()(const reliefkarte_t::line_segment_t& a, const reliefkarte_t::line_segment_t& b) const
{
	if(  a.start.x == b.start.x  ) {
		// same start ...
		return a.end.x < b.end.x;
	}
	return a.start.x < b.start.x;
}


static COLOR_VAL colore = 0;
static inthashtable_tpl< int, slist_tpl<schedule_t *> > waypoint_hash;



// add the schedule to the map (if there is a valid one)
void reliefkarte_t::add_to_schedule_cache( convoihandle_t cnv, bool with_waypoints )
{
	// make sure this is valid!
	if(  !cnv.is_bound()  ) {
		return;
	}
	schedule_t *fpl = cnv->get_schedule();

	colore += 8;
	if(  colore >= 208  ) {
		colore = (colore % 8) + 1;
		if(  colore == 7  ) {
			colore = 0;
		}
	}

	// ok, add this schedule to map
	// from here on we have a valid convoi
	int stops = 0;
	uint8 old_offset = 0, first_offset = 0, temp_offset = 0;
	koord old_stop, first_stop, temp_stop;
	bool last_diagonal = false;
	const bool add_schedule = fpl->get_waytype() != air_wt;

	FOR(  minivec_tpl<linieneintrag_t>, cur, fpl->eintrag  ) {

		//cycle on stops
		//try to read station's coordinates if there's a station at this schedule stop
		halthandle_t station = haltestelle_t::get_halt( welt, cur.pos, cnv->get_besitzer() );
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

		const int key = temp_stop.x + temp_stop.y*welt->get_groesse_x();
		waypoint_hash.put( key );
		// now get the offset
		slist_tpl<schedule_t *>*pt_list = waypoint_hash.access(key);
		if(  add_schedule  ) {
			// init key
			if(  !pt_list->is_contained( fpl )  ) {
				// not known => append
				temp_offset = pt_list->get_count();
			}
			else {
				// how many times we reached here?
				temp_offset = pt_list->index_of( fpl );
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
			if(  !schedule_cache.insert_unique_ordered( line_segment_t( temp_stop, temp_offset, old_stop, old_offset, fpl, cnv->get_besitzer(), colore, last_diagonal ), LineSegmentOrdering() )  &&  add_schedule  ) {
				// append if added and not yet there
				if(  !pt_list->is_contained( fpl )  ) {
					pt_list->append( fpl );
				}
				if(  stops == 2  ) {
					// append first stop too, when this is called for the first time
					const int key = first_stop.x + first_stop.y*welt->get_groesse_x();
					waypoint_hash.put( key );
					slist_tpl<schedule_t *>*pt_list = waypoint_hash.access(key);
					if(  !pt_list->is_contained( fpl )  ) {
						pt_list->append( fpl );
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
		schedule_cache.insert_unique_ordered( line_segment_t( first_stop, first_offset, old_stop, old_offset, fpl, cnv->get_besitzer(), colore, last_diagonal ), LineSegmentOrdering() );
	}
}



// some rountes for the relief map with schedules
static uint32 number_to_radius( uint32 n )
{
	return log2( n>>5 );
}


static void display_airport( const KOORD_VAL xx, const KOORD_VAL yy, const PLAYER_COLOR_VAL color )
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
				display_vline_wh_clip( x + i, y + j, 1,  color, true );
			}
		}
	}
}

static void display_harbor( const KOORD_VAL xx, const KOORD_VAL yy, const PLAYER_COLOR_VAL color )
{
	int x = xx + 5;
	int y = yy - 11 + 13;	//to not overwrite airline symbol

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
				display_vline_wh_clip( x + i, y + j, 1,  color, true );
			}
		}
	}
}
// those will be replaced by pak images later ...!


static void display_thick_line( KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL x2, KOORD_VAL y2, COLOR_VAL col, bool dotting, short dot_full, short dot_empty, short thickness )
{
	double delta_x = abs( x1 - x2 );
	double delta_y = abs( y1 - y2 );

	if(  delta_x == 0.0  ||  delta_y/delta_x > 2.0  ) {
		// mostly vertical
		x1 -= thickness/2;
		x2 -= thickness/2;
		for(  int i = 0;  i < thickness;  i++  ) {
			if ( !dotting ) {
				display_direct_line( x1 + i, y1, x2 + i, y2, col );
			}
			else {
				display_direct_line_dotted( x1 + i, y1, x2 + i, y2, dot_full, dot_empty, col );
			}
		}
	}
	else if(  delta_y == 0.0  ||  delta_x/delta_y > 2.0  ) {
		// mostly horizontal
		y1 -= thickness/2;
		y2 -= thickness/2;
		for(  int i = 0;  i < thickness;  i++  ) {
			if ( !dotting ) {
				display_direct_line( x1, y1 + i, x2, y2 + i, col );
			}
			else {
				display_direct_line_dotted( x1, y1 + i, x2, y2 + i, dot_full, dot_empty, col );
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
				display_direct_line( x1+i, y1+i*y_multiplier, x2+i, y2+i*y_multiplier, col );
				display_direct_line( x1+i+1, y1+i*y_multiplier, x2+i+1, y2+i*y_multiplier, col );
			}
			else {
				display_direct_line_dotted ( x1 + i, y1 + i*y_multiplier, x2 + i, y2 + i*y_multiplier, dot_full, dot_empty, col );
				display_direct_line_dotted ( x1 + i + 1, y1 + i*y_multiplier, x2 + i + 1, y2 + i*y_multiplier, dot_full, dot_empty, col );
			}
		}
	}
}


static void line_segment_draw( waytype_t type, koord start, uint8 start_offset, koord end, uint8 end_offset, bool diagonal, COLOR_VAL colore )
{
	// airplanes are different, so we must check for them first
	if(  type ==  air_wt  ) {
		// ignore offset for airplanes
		draw_bezier( start.x, start.y, end.x, end.y, 50, 50, 50, 50, colore, 5, 5 );
		draw_bezier( start.x + 1, start.y + 1, end.x + 1, end.y + 1, 50, 50, 50, 50, colore, 5, 5 );
	}
	else {
		// add offsets
		start.x += start_offset*3;
		end.x += end_offset*3;
		start.y += start_offset*3;
		end.y += end_offset*3;
		// due to isometric drawing, order may be swapped
		if(  start.x > end.x  ) {
			// but we need start.x <= end.x!
			koord temp = start;
			start = end;
			end = temp;
			uint8 temp_offset = start_offset;
			start_offset = end_offset;
			end_offset = temp_offset;
			diagonal ^= 1;
		}
		// now determine line style
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
		// start.x is always <= end.x ...
		const int delta_y = end.y-start.y;
		if(  (start.x-end.x)*delta_y == 0  ) {
			// horizontal/vertical line
			display_thick_line( start.x, start.y, end.x, end.y, colore, dotted, 5, 3, thickness );
		}
		else {
			// two segment
			koord mid;
			int signum_y = delta_y/abs(delta_y);
			// diagonal line to right bottom
			if(  delta_y > 0  ) {
				if(  end_offset  &&  !diagonal  ) {
					// start with diagonal to avoid parallel lines
					diagonal = true;
				}
				if(  start_offset  &&  diagonal  ) {
					// end with diagonal to avoid parallel lines
					diagonal = false;
				}
			}
			// now draw a two segment line
			if(  diagonal  ) {
				// start with diagonal
				if(  abs(delta_y) > end.x-start.x  ) {
					mid.x = end.x;
					mid.y = start.y + (end.x-start.x)*signum_y;
				}
				else {
					mid.x = start.x + abs(delta_y);
					mid.y = end.y;
				}
				display_thick_line( start.x, start.y, mid.x, mid.y, colore, dotted, 5, 3, thickness );
				display_thick_line( mid.x, mid.y, end.x, end.y, colore, dotted, 5, 3, thickness );
			}
			else {
				// end with diagonal
				const int delta_y = end.y-start.y;
				if(  abs(delta_y) > end.x-start.x  ) {
					mid.x = start.x;
					mid.y = end.y - (end.x-start.x)*signum_y;
				}
				else {
					mid.x = end.x - abs(delta_y);
					mid.y = start.y;
				}
				display_thick_line( start.x, start.y, mid.x, mid.y, colore, dotted, 5, 3, thickness );
				display_thick_line( mid.x, mid.y, end.x, end.y, colore, dotted, 5, 3, thickness );
			}
		}
	}
}


// converts karte koordinates to screen corrdinates
void reliefkarte_t::karte_to_screen( koord &k ) const
{
	// must be down before/after, of one would loose bits ...
	if(  zoom_out==1  ) {
		k.x = k.x*zoom_in;
		k.y = k.y*zoom_in;
	}
	if(isometric) {
		// 45 rotate view
		sint32 x = welt->get_groesse_y()*zoom_in + (sint32)(k.x-k.y) - 1;
		k.y = k.x/2+k.y/2;
		k.x = x;
	}
	if(  zoom_in==1  ) {
		k.x = k.x/zoom_out;
		k.y = k.y/zoom_out;
	}
}


// and retransform
inline void reliefkarte_t::screen_to_karte( koord &k ) const
{
	k = koord( (k.x*zoom_out)/zoom_in, (k.y*zoom_out)/zoom_in );
	if(isometric) {
		k.y *= 2;
		k.x = (sint16)(((sint32)k.x+(sint32)k.y-(sint32)welt->get_groesse_y())/2);
		k.y = k.y - k.x;
	}
}


bool reliefkarte_t::change_zoom_factor(bool magnify)
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
			int max_zoom_in = min( 32767 / (2*get_welt()->get_groesse_max()), 16);
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
		calc_map_groesse();
	}
	return zoomed;
}


uint8 reliefkarte_t::calc_severity_color(sint32 amount, sint32 max_value)
{
	if(max_value!=0) {
		// color array goes from light blue to red
		sint32 severity = amount * MAX_SEVERITY_COLORS / (max_value+1);
		return reliefkarte_t::severity_color[ clamp( severity, 0, MAX_SEVERITY_COLORS-1 ) ];
	}
	return reliefkarte_t::severity_color[0];
}


uint8 reliefkarte_t::calc_severity_color_log(sint32 amount, sint32 max_value)
{
	if(  max_value>1  ) {
		sint32 severity;
		if(  amount <= 0x003FFFFF  ) {
			severity = log2( (uint32)( (amount << MAX_SEVERITY_COLORS) / (max_value+1) ) );
		}
		else {
			severity = (uint32)( log( (double)amount*(double)(1<<MAX_SEVERITY_COLORS)/(double)max_value) + 0.5 );
		}
		return reliefkarte_t::severity_color[ clamp( severity, 0, MAX_SEVERITY_COLORS-1 ) ];
	}
	return reliefkarte_t::severity_color[0];
}


void reliefkarte_t::set_relief_color_clip( sint16 x, sint16 y, uint8 color )
{
	if(  0<=x  &&  (uint16)x < relief->get_width()  &&  0<=y  &&  (uint16)y < relief->get_height()  ) {
		relief->at( x, y ) = color;
	}
}


void reliefkarte_t::set_relief_farbe(koord k, const int color)
{
	// if map is in normal mode, set new color for map
	// otherwise do nothing
	// result: convois will not "paint over" special maps
	if (relief==NULL  ||  !welt->ist_in_kartengrenzen(k)) {
		return;
	}

	karte_to_screen(k);
	k -= cur_off;

	if(  isometric  ) {
		// since isometric is distorted
		const sint32 xw = zoom_out>=2 ? 1 : 2*zoom_in;
		// increase size at zoom_in 2, 5, 9, 11
		const KOORD_VAL mid_y = ((xw+1) / 5) + (xw / 18);
		// center line
		for(  int x=0;  x<xw;  x++  ) {
			set_relief_color_clip( k.x+x, k.y+mid_y, color );
		}
		// lines above and below
		if(  mid_y > 0  ) {
			KOORD_VAL left = 2, right = xw-2 + ((xw>>1)&1);
			for(  KOORD_VAL y_offset = 1;  y_offset <= mid_y;  y_offset++  ) {
				for(  int x=left;  x<right;  x++  ) {
					set_relief_color_clip( k.x+x, k.y+mid_y+y_offset, color );
					set_relief_color_clip( k.x+x, k.y+mid_y-y_offset, color );
				}
				left += 2;
				right -= 2;
			}
		}
	}
	else {
		for(  sint32 x = max(0,k.x);  x < zoom_in+k.x  &&  (uint32)x < relief->get_width();  x++  ) {
			for(  sint32 y = max(0,k.y);  y < zoom_in+k.y  &&  (uint32)y < relief->get_height();  y++  ) {
				relief->at(x, y) = color;
			}
		}
	}
}


void reliefkarte_t::set_relief_farbe_area(koord k, int areasize, uint8 color)
{
	koord p;
	if(isometric) {
		k -= koord( areasize/2, areasize/2 );
		for(  p.x = 0;  p.x<areasize;  p.x++  ) {
			for(  p.y = 0;  p.y<areasize;  p.y++  ) {
				set_relief_farbe( k+p, color );
			}
		}
	}
	else {
		karte_to_screen(k);
		areasize *= zoom_in;
		k -= koord( areasize/2, areasize/2 );
		k.x = clamp( k.x, 0, get_groesse().x-areasize-1 );
		k.y = clamp( k.y, 0, get_groesse().y-areasize-1 );
		k -= cur_off;
		for(  p.x = max(0,k.x);  (uint16)p.x < areasize+k.x  &&  (uint16)p.x < relief->get_width();  p.x++  ) {
			for(  p.y = max(0,k.y);  (uint16)p.y < areasize+k.y  &&  (uint16)p.y < relief->get_height();  p.y++  ) {
				relief->at(p.x, p.y) = color;
			}
		}
	}
}


/**
 * calculates ground color for position (hoehe - grundwasser).
 * @author Hj. Malthaner
 */
uint8 reliefkarte_t::calc_hoehe_farbe(const sint16 hoehe, const sint16 grundwasser)
{
	return map_type_color[clamp( (hoehe-grundwasser)+MAX_MAP_TYPE_WATER-1, 0, MAX_MAP_TYPE_WATER+MAX_MAP_TYPE_LAND-1 )];
}


/**
 * Updated Kartenfarbe an Position k
 * @author Hj. Malthaner
 */
uint8 reliefkarte_t::calc_relief_farbe(const grund_t *gr)
{
	uint8 color = COL_BLACK;

#ifdef DEBUG_ROUTES
	/* for debug purposes only ...*/
	if(welt->ist_markiert(gr)) {
		color = COL_PURPLE;
	}else
#endif
	if(gr->get_halt().is_bound()) {
		color = HALT_KENN;
	}
	else {
		switch(gr->get_typ()) {
			case grund_t::brueckenboden:
				color = MN_GREY3;
				break;
			case grund_t::tunnelboden:
				color = COL_BROWN;
				break;
			case grund_t::monorailboden:
				color = MONORAIL_KENN;
				break;
			case grund_t::fundament:
				{
					// object at zero is either factory or house (or attraction ... )
					gebaeude_t *gb = gr->find<gebaeude_t>();
					fabrik_t *fab = gb ? gb->get_fabrik() : NULL;
					if(fab==NULL) {
						color = COL_GREY3;
					}
					else {
						color = fab->get_kennfarbe();
					}
				}
				break;
			case grund_t::wasser:
				{
					// object at zero is either factory or boat
					gebaeude_t *gb = gr->find<gebaeude_t>();
					fabrik_t *fab = gb ? gb->get_fabrik() : NULL;
					if(fab==NULL) {
#ifndef DOUBLE_GROUNDS
						sint16 height = (gr->get_grund_hang()&1);
#else
						sint16 height = (gr->get_grund_hang()%3);
#endif
						color = calc_hoehe_farbe(welt->lookup_hgt(gr->get_pos().get_2d())+height, welt->get_grundwasser());
						//color = COL_BLUE;	// water with boat?
					}
					else {
						color = fab->get_kennfarbe();
					}
				}
				break;
			// normal ground ...
			default:
				if(gr->hat_wege()) {
					switch(gr->get_weg_nr(0)->get_waytype()) {
						case road_wt: color = STRASSE_KENN; break;
						case tram_wt:
						case track_wt: color = SCHIENE_KENN; break;
						case water_wt: color = CHANNEL_KENN; break;
						case air_wt: color = RUNWAY_KENN; break;
						case monorail_wt:
						default:	// all other ways light red ...
							color = 135; break;
							break;
					}
				}
				else {
					const leitung_t* lt = gr->find<leitung_t>();
					if(lt!=NULL) {
						color = POWERLINE_KENN;
					}
					else {
#ifndef DOUBLE_GROUNDS
						sint16 height = (gr->get_grund_hang()&1);
#else
						sint16 height = (gr->get_grund_hang()%3);
#endif
						color = calc_hoehe_farbe(gr->get_hoehe()+height, welt->get_grundwasser());
					}
				}
				break;
		}
	}
	return color;
}


void reliefkarte_t::calc_map_pixel(const koord k)
{
	// we ignore requests, when nothing visible ...
	if(!is_visible) {
		return;
	}

	// always use to uppermost ground
	const planquadrat_t *plan=welt->lookup(k);
	if(plan==NULL  ||  plan->get_boden_count()==0) {
		return;
	}
	const grund_t *gr=plan->get_boden_bei(plan->get_boden_count()-1);

	if(  mode!=MAP_PAX_DEST  &&  gr->get_convoi_vehicle()  ) {
		set_relief_farbe( k, VEHIKEL_KENN );
		return;
	}

	// first use ground color
	set_relief_farbe( k, calc_relief_farbe(gr) );

	switch(mode&~MAP_MODE_FLAGS) {
		// show passenger coverage
		// display coverage
		case MAP_PASSENGER:
			if(  plan->get_haltlist_count()>0  ) {
				halthandle_t halt = plan->get_haltlist()[0];
				if (halt->get_pax_enabled() && !halt->get_pax_connections().empty()) {
					set_relief_farbe( k, halt->get_besitzer()->get_player_color1() + 3 );
				}
			}
			break;

		// show mail coverage
		// display coverage
		case MAP_MAIL:
			if(  plan->get_haltlist_count()>0  ) {
				halthandle_t halt = plan->get_haltlist()[0];
				if (halt->get_post_enabled() && !halt->get_mail_connections().empty()) {
					set_relief_farbe( k, halt->get_besitzer()->get_player_color1() + 3 );
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
					set_relief_farbe(k, calc_severity_color_log(cargo, max_cargo));
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
					set_relief_farbe_area(k, 1, calc_severity_color_log( passed, max_passed ) );
				}
			}
			break;

		// show tracks: white: no electricity, red: electricity, yellow: signal
		case MAP_TRACKS:
			// show track
			if (gr->hat_weg(track_wt)) {
				const schiene_t * sch = (const schiene_t *) (gr->get_weg(track_wt));
				if(sch->is_electrified()) {
					set_relief_farbe(k, COL_RED);
				}
				else {
					set_relief_farbe(k, COL_WHITE);
				}
				// show signals
				if(sch->has_sign()  ||  sch->has_signal()) {
					set_relief_farbe(k, COL_YELLOW);
				}
			}
			break;

		// show max speed (if there)
		case MAX_SPEEDLIMIT:
			{
				sint32 speed=gr->get_max_speed();
				if(speed) {
					set_relief_farbe(k, calc_severity_color(gr->get_max_speed(), 450));
				}
			}
			break;

		// find power lines
		case MAP_POWERLINES:
			{
				const leitung_t* lt = gr->find<leitung_t>();
				if(lt!=NULL) {
					set_relief_farbe(k, calc_severity_color(lt->get_net()->get_demand(),lt->get_net()->get_supply()) );
				}
			}
			break;

		case MAP_FOREST:
			if(  gr->get_top()>1  &&  gr->obj_bei(gr->get_top()-1)->get_typ()==ding_t::baum  ) {
				set_relief_farbe(k, COL_GREEN );
			}
			break;

		case MAP_OWNER:
			// show ownership
			{
				if(  gr->is_halt()  ) {
					set_relief_farbe(k, gr->get_halt()->get_besitzer()->get_player_color1()+3);
				}
				else if(  weg_t *weg = gr->get_weg_nr(0)  ) {
					set_relief_farbe(k, weg->get_besitzer()==NULL ? COL_ORANGE : weg->get_besitzer()->get_player_color1()+3 );
				}
				if(  gebaeude_t *gb = gr->find<gebaeude_t>()  ) {
					if(  gb->get_besitzer()!=NULL  ) {
						set_relief_farbe(k, gb->get_besitzer()->get_player_color1()+3 );
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
					if(  gb->get_haustyp() != gebaeude_t::unbekannt  ) {
						sint32 level = gb->get_tile()->get_besch()->get_level();
						if(  level > max_building_level  ) {
							max_building_level = level;
						}
						set_relief_farbe_area(k, 1, calc_severity_color( level, max_building_level ) );
					}
				}
			}
			break;

		default:
			break;
	}
}


void reliefkarte_t::calc_map_groesse()
{
	koord size( welt->get_groesse_x(), 0 );
	koord down( welt->get_groesse_x(), welt->get_groesse_y() );
	karte_to_screen( size );
	karte_to_screen( down );
	size.y = down.y;
	if(  isometric  ) {
		size.x += zoom_in*2;
	}
	set_groesse( size ); // of the gui_komponete to adjust scroll bars
	needs_redraw = true;
}


void reliefkarte_t::calc_map()
{
	// only use bitmap size like screen size
	koord relief_size( min( get_groesse().x, new_size.x ), min( get_groesse().y, new_size.y ) );
	// actually the following line should reduce new/deletes, but does not work properly
	if(  relief==NULL  ||  (sint16)relief->get_width()!=relief_size.x  ||  (sint16)relief->get_height()!=relief_size.y  ) {
		delete relief;
		relief = new array2d_tpl<unsigned char> (relief_size.x,relief_size.y);
	}
	cur_off = new_off;
	cur_size = new_size;
	needs_redraw = false;
	is_visible = true;

	// redraw the map
	if(  !isometric  ) {
		koord k;
		koord start_off = koord( (cur_off.x*zoom_out)/zoom_in, (cur_off.y*zoom_out)/zoom_in );
		koord end_off = start_off+koord( (relief->get_width()*zoom_out)/zoom_in+1, (relief->get_height()*zoom_out)/zoom_in+1 );
		for(  k.y=start_off.y;  k.y<end_off.y;  k.y+=zoom_out  ) {
			for(  k.x=start_off.x;  k.x<end_off.x;  k.x+=zoom_out  ) {
				calc_map_pixel(k);
			}
		}
	}
	else {
		// always the whole map ...
		if(isometric) {
			relief->init( COL_BLACK );
		}
		koord k;
		for(  k.y=0;  k.y < welt->get_groesse_y();  k.y++  ) {
			for(  k.x=0;  k.x < welt->get_groesse_x();  k.x++  ) {
				calc_map_pixel(k);
			}
		}
	}
}


reliefkarte_t::reliefkarte_t()
{
	relief = NULL;
	zoom_in = 1;
	zoom_out = 1;
	isometric = false;
	mode = MAP_TOWN;
	city = NULL;
	cur_off = new_off = cur_size = new_size = koord(0,0);
	needs_redraw = true;
}


reliefkarte_t::~reliefkarte_t()
{
	if(relief != NULL) {
		delete relief;
	}
}


reliefkarte_t *reliefkarte_t::get_karte()
{
	if(single_instance == NULL) {
		single_instance = new reliefkarte_t();
	}
	return single_instance;
}


void reliefkarte_t::set_welt(karte_t *welt)
{
	this->welt = welt;			// Welt fuer display_win() merken
	if(relief) {
		delete relief;
		relief = NULL;
	}
	needs_redraw = true;
	is_visible = false;

	if(welt) {
		calc_map_groesse();
		max_building_level = max_cargo = max_passed = 0;
		max_tourist_ziele = max_waiting = max_origin = max_transfer = max_service = 1;
		last_schedule_counter = welt->get_schedule_counter()-1;
		set_current_cnv(convoihandle_t());
	}
}


void reliefkarte_t::set_mode(MAP_MODES new_mode)
{
	mode = new_mode;
	needs_redraw = true;
}


void reliefkarte_t::neuer_monat()
{
	needs_redraw = true;
}


// these two are the only gui_container specific routines


// handle event
bool reliefkarte_t::infowin_event(const event_t *ev)
{
	koord k( ev->mx, ev->my );
	screen_to_karte( k );

	// get factory under mouse cursor
	last_world_pos = k;

	// recenter
	if(IS_LEFTCLICK(ev) || IS_LEFTDRAG(ev)) {
		welt->set_follow_convoi( convoihandle_t() );
		int z = 0;
		if(welt->ist_in_kartengrenzen(k)) {
			z = welt->min_hgt(k);
		}
		welt->change_world_position(koord3d(k,z));
		return true;
	}

	return false;
}


// helper function for finding nearby factory
const fabrik_t* reliefkarte_t::get_fab( const koord, bool enlarge ) const
{
	const fabrik_t *fab = fabrik_t::get_fab(welt, last_world_pos);
	for(  int i=0;  i<4  && fab==NULL;  i++  ) {
		fab = fabrik_t::get_fab( welt, last_world_pos+koord::nsow[i] );
	}
	if(  enlarge  ) {
		for(  int i=0;  i<4  && fab==NULL;  i++  ) {
			fab = fabrik_t::get_fab( welt, last_world_pos+koord::nsow[i]*2 );
		}
	}
	return fab;
}


// helper function for redraw: factory connections
const fabrik_t* reliefkarte_t::draw_fab_connections(const uint8 colour, const koord pos) const
{
	const fabrik_t* const fab = get_fab( last_world_pos, true );
	if(fab) {
		koord fabpos = fab->get_pos().get_2d();
		karte_to_screen( fabpos );
		fabpos += pos;
		const vector_tpl<koord>& lieferziele = event_get_last_control_shift() & 1 ? fab->get_suppliers() : fab->get_lieferziele();
		FOR(vector_tpl<koord>, lieferziel, lieferziele) {
			const fabrik_t * fab2 = fabrik_t::get_fab(welt, lieferziel);
			if (fab2) {
				karte_to_screen( lieferziel );
				const koord end = lieferziel+pos;
				display_direct_line(fabpos.x, fabpos.y, end.x, end.y, colour);
				display_fillbox_wh_clip(end.x, end.y, 3, 3, ((welt->get_zeit_ms() >> 10) & 1) == 0 ? COL_RED : COL_WHITE, true);

				koord boxpos = end + koord(10, 0);
				const char * name = translator::translate(fab2->get_name());
				int name_width = proportional_string_width(name)+8;
				boxpos.x = clamp( boxpos.x, pos.x, pos.x+get_groesse().x-name_width );
				display_ddd_proportional_clip(boxpos.x, boxpos.y, name_width, 0, 5, COL_WHITE, name, true);
			}
		}
	}
	return fab;
}


// show the schedule on the minimap
void reliefkarte_t::set_current_cnv( convoihandle_t c )
{
	current_cnv = c;
	schedule_cache.clear();
	stop_cache.clear();
	colore = 0;
	add_to_schedule_cache( current_cnv, true );
	last_schedule_counter = welt->get_schedule_counter()-1;
}


// draw the map (and the overlays!)
void reliefkarte_t::zeichnen(koord pos)
{
	// sanity check, needed for overlarge maps
	if(  (new_off.x|new_off.y)<0  ) {
		new_off = cur_off;
	}
	if(  (new_size.x|new_size.y)<0  ) {
		new_size = cur_size;
	}

	if(  last_mode != mode  ) {
		// only needing update, if last mode was also not about halts ...
		needs_redraw = (mode^last_mode) & ~MAP_MODE_FLAGS;

		if(  (mode & MAP_LINES) == 0  ||  (mode^last_mode) & MAP_MODE_HALT_FLAGS  ) {
			// rebuilt stop_cache needed
			stop_cache.clear();
		}

		if(  (mode^last_mode) & (MAP_PASSENGER|MAP_MAIL|MAP_FREIGHT|MAP_LINES)  ||  (mode&MAP_LINES  &&  stop_cache.empty())  ) {
			// rebuilt line display
			last_schedule_counter = welt->get_schedule_counter()-1;
		}

		last_mode = mode;
	}

	if(  needs_redraw  ||  cur_off!=new_off  ||  cur_size!=new_size  ) {
		calc_map();
		needs_redraw = false;
	}

	if(relief==NULL) {
		return;
	}

	if(  mode & MAP_PAX_DEST  &&  city!=NULL  ) {
		const unsigned long current_pax_destinations = city->get_pax_destinations_new_change();
		if(  pax_destinations_last_change > current_pax_destinations  ) {
			// new month started.
			calc_map();
		}
		else if(  pax_destinations_last_change < current_pax_destinations  ) {
			// new pax_dest in city.
			const sparse_tpl<uint8> *pax_dests = city->get_pax_destinations_new();
			koord pos, min, max;
			uint8 color;
			for(  uint16 i = 0;  i < pax_dests->get_data_count();  i++  ) {
				pax_dests->get_nonzero( i, pos, color );
				min = koord((pos.x*welt->get_groesse_x())/PAX_DESTINATIONS_SIZE,
				            (pos.y*welt->get_groesse_y())/PAX_DESTINATIONS_SIZE);
				max = koord(((pos.x+1)*welt->get_groesse_x())/PAX_DESTINATIONS_SIZE,
				            ((pos.y+1)*welt->get_groesse_y())/PAX_DESTINATIONS_SIZE);
				pos = min;
				do {
					do {
						set_relief_farbe(pos, color);
						pos.y++;
					} while(pos.y < max.y);
					pos.x++;
				} while (pos.x < max.x);
			}
		}
		pax_destinations_last_change = city->get_pax_destinations_new_change();
	}

	if(  (uint16)cur_size.x > relief->get_width()  ) {
		display_fillbox_wh_clip( pos.x+new_off.x+relief->get_width(), new_off.y+pos.y, 32767, relief->get_height(), COL_BLACK, true);
	}
	if(  (uint16)cur_size.y > relief->get_height()  ) {
		display_fillbox_wh_clip( pos.x+new_off.x, pos.y+new_off.y+relief->get_height(), 32767, 32767, COL_BLACK, true);
	}
	display_array_wh( cur_off.x+pos.x, new_off.y+pos.y, relief->get_width(), relief->get_height(), relief->to_array());

	if(  !current_cnv.is_bound()  &&  mode & MAP_LINES    ) {
		vector_tpl<linehandle_t> linee;

		if(  last_schedule_counter != welt->get_schedule_counter()  ) {
			// rebuilt cache
			last_schedule_counter = welt->get_schedule_counter();
			schedule_cache.clear();
			stop_cache.clear();
			waypoint_hash.clear();
			colore = 0;

			for(  int np = 0;  np < MAX_PLAYER_COUNT;  np++  ) {
				//cycle on players
				if(  welt->get_spieler( np )  &&  welt->get_spieler( np )->simlinemgmt.get_line_count() > 0   ) {

					welt->get_spieler( np )->simlinemgmt.get_lines( simline_t::line, &linee );
					for(  uint32 j = 0;  j < linee.get_count();  j++  ) {
						//cycle on lines

						// does this line has a matching freight
						if(  mode & MAP_PASSENGER  ) {
							if(  !linee[j]->get_goods_catg_index().is_contained( warenbauer_t::INDEX_PAS )  ) {
								// no pasengers
								continue;
							}
						}
						else if(  mode & MAP_MAIL  ) {
							if(  !linee[j]->get_goods_catg_index().is_contained( warenbauer_t::INDEX_MAIL )  ) {
								// no mail
								continue;
							}
						}
						else if(  mode & MAP_FREIGHT  ) {
							uint8 i=0;
							for(  ;  i < linee[j]->get_goods_catg_index().get_count();  i++  ) {
								if(  linee[j]->get_goods_catg_index()[i] > warenbauer_t::INDEX_NONE  ) {
									break;
								}
							}
							if(  i >= linee[j]->get_goods_catg_index().get_count()  ) {
								// not any freight here ...
								continue;
							}
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

			// now add all unboad convois
			FOR( vector_tpl<convoihandle_t>, cnv, welt->convoys() ) {
				if(  !cnv.is_bound()  ||  cnv->get_line().is_bound()  ) {
					// not there or already part of a line
					continue;
				}
				int state = cnv->get_state();
				if( state != convoi_t::INITIAL  &&  state != convoi_t::ENTERING_DEPOT  &&  state != convoi_t::SELF_DESTRUCT  ) {
					// does this line has a matching freight
					if(  mode & MAP_PASSENGER  ) {
						if(  !cnv->get_goods_catg_index().is_contained( warenbauer_t::INDEX_PAS )  ) {
							// no pasengers
							continue;
						}
					}
					else if(  mode & MAP_MAIL  ) {
						if(  !cnv->get_goods_catg_index().is_contained( warenbauer_t::INDEX_MAIL )  ) {
							// no mail
							continue;
						}
					}
					else if(  mode & MAP_FREIGHT  ) {
						uint8 i=0;
						for(  ;  i < cnv->get_goods_catg_index().get_count();  i++  ) {
							if(  cnv->get_goods_catg_index()[i] > warenbauer_t::INDEX_NONE  ) {
								break;
							}
						}
						if(  i >= cnv->get_goods_catg_index().get_count()  ) {
							// not any freight here ...
							continue;
						}
					}
					add_to_schedule_cache( cnv, false );
				}
			}
		}
		/************ ATTENTION: The schedule pointers fpl in the line segments ******************
		 ************            are invalid after this point!                  ******************/
	}
	//end MAP_LINES

	bool showing_schedule = false;
	if(  mode & MAP_LINES  ) {
		showing_schedule = !schedule_cache.empty();
	}
	else {
		schedule_cache.clear();
		colore = 0;
		last_schedule_counter = welt->get_schedule_counter()-1;
	}

	// since the schedule whitens out the background, we have to draw it first
	int offset = 1;
	koord last_start(0,0), last_end(0,0), k1, k2;
	bool diagonal = false;
	if(  showing_schedule  ) {
		// lighten background
		if(  isometric  ) {
			// isometric => lighten in three parts

			koord p1( 0, 0 );
			karte_to_screen( p1 );
			koord p2( welt->get_groesse_x(), 0 );
			karte_to_screen( p2 );
			koord p3( welt->get_groesse_x(), welt->get_groesse_y() );
			karte_to_screen( p3 );
			koord p4( 0, welt->get_groesse_y() );
			karte_to_screen( p4 );

			// top and bottom part
			const int toplines = min( p4.y, p2.y );
			for( KOORD_VAL y = 0;  y < toplines;  y++  ) {
				display_blend_wh( pos.x+p1.x-2*y, pos.y+y, 4*y+4, 1, COL_WHITE, 75 );
				display_blend_wh( pos.x+p3.x-2*y, pos.y+p3.y-y-1, 4*y+4, 1, COL_WHITE, 75 );
			}
			// center area
			if(  p1.x < p3.x  ) {
				for( KOORD_VAL y = toplines;  y < p3.y-toplines;  y++  ) {
					display_blend_wh( pos.x+(y-toplines)*2, pos.y+y, 4*toplines+4, 1, COL_WHITE, 75 );
				}
			}
			else {
				for( KOORD_VAL y = toplines;  y < p3.y-toplines;  y++  ) {
					display_blend_wh( pos.x+(y-toplines)*2, pos.y+p3.y-y-1, 4*toplines+4, 1, COL_WHITE, 75 );
				}
			}
		}
		else {
			// easier with rectagular maps ...
			display_blend_wh( cur_off.x+pos.x, cur_off.y+pos.y, relief->get_width(), relief->get_height(), COL_WHITE, 75 );
		}

		// DISPLAY STATIONS AND AIRPORTS: moved here so station spots are not overwritten by lines drawn
		FOR(  vector_tpl<line_segment_t>, seg, schedule_cache  ) {

			COLOR_VAL color = seg.colorcount;
			if(  event_get_last_control_shift()==2  ||  current_cnv.is_bound()  ) {
				// on control / single convoi use only player colors
				static COLOR_VAL last_color = color;
				color = seg.sp->get_player_color1()+1;
				// all lines same thickness if same color
				if(  color == last_color  ) {
					offset = 0;
				}
				last_color = color;
			}
			if(  seg.start != last_start  ||  seg.end != last_end  ) {
				last_start = k1 = seg.start;
				karte_to_screen( k1 );
				k1 += pos;
				last_end = k2 = seg.end;
				karte_to_screen( k2 );
				k2 += pos;
				// use same diagonal for all parallel segments
				diagonal = seg.start_diagonal;
			}
			// and finally draw ...
			line_segment_draw( seg.waytype, k1, seg.start_offset*offset, k2, seg.end_offset*offset, diagonal, color );
		}
	}

	// display station information here (even without overlay)
	halthandle_t display_station;
	// only fille cache if needed
	if(  mode & MAP_MODE_HALT_FLAGS  &&  stop_cache.empty()  ) {
		if(  mode&MAP_ORIGIN  ) {
			FOR( const slist_tpl<halthandle_t>, halt, haltestelle_t::get_alle_haltestellen() ) {
				if(  halt->get_pax_enabled()  ||  halt->get_post_enabled()  ) {
					stop_cache.append( halt );
				}
			}
		}
		else if(  mode&MAP_TRANSFER  ) {
			FOR( const slist_tpl<halthandle_t>, halt, haltestelle_t::get_alle_haltestellen() ) {
				if(  halt->is_transfer(warenbauer_t::INDEX_PAS)  ||  halt->is_transfer(warenbauer_t::INDEX_MAIL)  ) {
					stop_cache.append( halt );
				}
				else {
					// good transfer?
					bool transfer = false;
					for(  int i=warenbauer_t::INDEX_NONE+1  &&  !transfer;  i<=warenbauer_t::get_max_catg_index();  i ++  ) {
						transfer = halt->is_transfer( i );
					}
					if(  transfer  ) {
						stop_cache.append( halt );
					}
				}
			}
		}
		else if(  mode&MAP_STATUS  ||  mode&MAP_SERVICE  ||  mode&MAP_WAITING  ||  mode&MAP_WAITCHANGE  ) {
			FOR( const slist_tpl<halthandle_t>, halt, haltestelle_t::get_alle_haltestellen() ) {
				stop_cache.append( halt );
			}
		}
	}
	// now draw stop cache
	// if needed to get new values
	sint32 new_max_waiting_change = 1;
	FOR(  vector_tpl<halthandle_t>, station, stop_cache  ) {

		int radius = 0;
		COLOR_VAL color;
		koord temp_stop = station->get_basis_pos();
		int diagonal_dist = 0;
		karte_to_screen( temp_stop );
		temp_stop = temp_stop + pos;

		if(  mode & MAP_STATUS  ) {
			color = station->get_status_farbe();
			radius = number_to_radius( station->get_capacity(0) );
		}
		else if( mode & MAP_SERVICE  ) {
			const sint32 service = station->get_finance_history( 1, HALT_CONVOIS_ARRIVED );
			if(  service > max_service  ) {
				max_service = service;
			}
			color = calc_severity_color_log( service, max_service );
			radius = log2( (uint32)( (service << 7) / max_service ) );
		}
		else if( mode & MAP_WAITING  ) {
			const sint32 waiting = station->get_finance_history( 0, HALT_WAITING );
			if(  waiting > max_waiting  ) {
				max_waiting = waiting;
			}
			color = calc_severity_color_log( waiting, max_waiting );
			radius = number_to_radius( waiting );
		}
		else if( mode & MAP_WAITCHANGE  ) {
			const sint32 waiting_diff = station->get_finance_history( 0, HALT_WAITING ) - station->get_finance_history( 1, HALT_WAITING );
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
			if(  !station->get_pax_enabled()  &&  !station->get_post_enabled()  ) {
				continue;
			}
			const sint32 pax_origin = station->get_finance_history( 1, HALT_HAPPY ) + station->get_finance_history( 1, HALT_UNHAPPY ) + station->get_finance_history( 1, HALT_NOROUTE );
			if(  pax_origin > max_origin  ) {
				max_origin = pax_origin;
			}
			color = calc_severity_color_log( pax_origin, max_origin );
			radius = number_to_radius( pax_origin );
		}
		else if( mode & MAP_TRANSFER  ) {
			const sint32 transfer = station->get_finance_history( 1, HALT_ARRIVED ) + station->get_finance_history( 1, HALT_DEPARTED );
			if(  transfer > max_transfer  ) {
				max_transfer = transfer;
			}
			color = calc_severity_color_log( transfer, max_transfer );
			radius = number_to_radius( transfer );
		}
		else {
			const int stype = station->get_station_type();
			color = station->get_besitzer()->get_player_color1()+3;

			// invalid=0, loadingbay=1, railstation = 2, dock = 4, busstop = 8, airstop = 16, monorailstop = 32, tramstop = 64, maglevstop=128, narrowgaugestop=256
			if(  stype > 0  ) {
				radius = 1;
				if(  stype & ~(haltestelle_t::loadingbay | haltestelle_t::busstop | haltestelle_t::tramstop)  ) {
					radius = 3;
				}
			}
			// with control, show only circles
			if(  event_get_last_control_shift()!=2  ) {
				// else elongate them ...
				const int key = station->get_basis_pos().x + station->get_basis_pos().y * welt->get_groesse_x();
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
						image_id img = skinverwaltung_t::station_type->get_bild_nr(type);
						if(  img!=IMG_LEER  ) {
							display_color_img( img, temp_stop.x+diagonal_dist+4+(icon/2)*12, temp_stop.y+diagonal_dist+4+(icon&1)*12, station->get_besitzer()->get_player_nr(), false, false );
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
		display_filled_circle( temp_stop.x, temp_stop.y, radius, color );
		display_circle( temp_stop.x, temp_stop.y, out_radius, COL_BLACK );
		if(  diagonal_dist>0  ) {
			display_filled_circle( temp_stop.x+diagonal_dist, temp_stop.y+diagonal_dist, radius, color );
			display_circle( temp_stop.x+diagonal_dist, temp_stop.y+diagonal_dist, out_radius, COL_BLACK );
			for(  int i=1;  i < diagonal_dist;  i++  ) {
				display_filled_circle( temp_stop.x+i, temp_stop.y+i, radius, color );
			}
			out_radius = sqrt_i32( 2*out_radius+1 );
			display_direct_line( temp_stop.x+out_radius, temp_stop.y-out_radius, temp_stop.x+out_radius+diagonal_dist, temp_stop.y-out_radius+diagonal_dist, COL_BLACK );
			display_direct_line( temp_stop.x-out_radius, temp_stop.y+out_radius, temp_stop.x-out_radius+diagonal_dist, temp_stop.y+out_radius+diagonal_dist, COL_BLACK );
		}

		if(  koord_distance( last_world_pos, station->get_basis_pos() ) <= 2  ) {
			// draw stop name with an index if close to mouse
			display_station = station;
		}
	}
	if(  display_station.is_bound()  ) {
		koord temp_stop = display_station->get_basis_pos();
		karte_to_screen( temp_stop );
		temp_stop = temp_stop + pos;
		display_ddd_proportional_clip( temp_stop.x + 10, temp_stop.y + 7, proportional_string_width( display_station->get_name() ) + 8, 0, display_station->get_besitzer()->get_player_color1()+3, COL_WHITE, display_station->get_name(), false );
	}
	max_waiting_change = new_max_waiting_change;	// update waiting tendenies

	// if we do not do this here, vehicles would erase the town names
	// ADD: if CRTL key is pressed, temporary show the name
	if(  mode & MAP_TOWN  ) {
		const weighted_vector_tpl<stadt_t*>& staedte = welt->get_staedte();
		const COLOR_VAL col = showing_schedule ? COL_BLACK : COL_WHITE;

		FOR( weighted_vector_tpl<stadt_t*>, const stadt, staedte ) {
			koord p = stadt->get_pos();
			const char * name = stadt->get_name();

			int w = proportional_string_width(name);
			karte_to_screen( p );
			p.x = clamp( p.x, 0, get_groesse().x-w );
			p += pos;
			display_proportional_clip( p.x, p.y, name, ALIGN_LEFT, col, true );
		}
	}

	// draw city limit
	if(  mode & MAP_CITYLIMIT  ) {

		// for all cities
		FOR(  weighted_vector_tpl<stadt_t*>,  const stadt,  welt->get_staedte()  ) {
			koord k[4];
			k[0] = stadt->get_linksoben(); // top left
			k[2] = stadt->get_rechtsunten(); // bottom right

			// calculate and draw the rotated coordinates

			k[1] =  koord(k[0].x, k[2].y); // bottom left
			k[3] =  koord(k[2].x, k[0].y); // top right

			k[0] += koord(0, -1); // top left
			karte_to_screen(k[0]);
			k[0] = k[0] + pos;

			karte_to_screen(k[1]); // bottom left
			k[1] = k[1] + pos;

			k[2] += koord(1, 0); // bottom right
			karte_to_screen(k[2]);
			k[2] += pos;

			k[3] += koord(1, -1); // top right
			karte_to_screen(k[3]);
			k[3] += pos;

			display_direct_line_dotted( k[0].x, k[0].y, k[1].x, k[1].y, 3, 3, COL_ORANGE );
			display_direct_line_dotted( k[1].x, k[1].y, k[2].x, k[2].y, 3, 3, COL_ORANGE );
			display_direct_line_dotted( k[2].x, k[2].y, k[3].x, k[3].y, 3, 3, COL_ORANGE );
			display_direct_line_dotted( k[3].x, k[3].y, k[0].x, k[0].y, 3, 3, COL_ORANGE );
		}
	}

	// since we do iterate the tourist info list, this must be done here
	// find tourist spots
	if(  mode & MAP_TOURIST  ) {
		FOR(  weighted_vector_tpl<gebaeude_t*>, const gb, welt->get_ausflugsziele()  ) {
			if(  gb->get_first_tile() == gb  ) {
				koord gb_pos = gb->get_pos().get_2d();
				karte_to_screen( gb_pos );
				gb_pos = gb_pos + pos;
				int const pax = gb->get_passagier_level();
				if(  max_tourist_ziele < pax  ) {
					max_tourist_ziele = pax;
				}
				COLOR_VAL color = calc_severity_color_log(gb->get_passagier_level(), max_tourist_ziele);
				int radius = max( (number_to_radius( pax*4 )*zoom_in)/zoom_out, 1 );
				display_filled_circle( gb_pos.x, gb_pos.y, radius, color );
				display_circle( gb_pos.x, gb_pos.y, radius, COL_BLACK );
			}
			// otherwise larger attraction will be shown more often ...
		}
	}

	if(  mode & MAP_FACTORIES) {
		FOR(  slist_tpl<fabrik_t*>,  const f,  welt->get_fab_list()  ) {
			koord fab_pos = f->get_pos().get_2d();
			karte_to_screen( fab_pos );
			fab_pos = fab_pos + pos;
			koord size = f->get_besch()->get_haus()->get_groesse(f->get_rotate());
			sint16 x_size = max( 5, size.x*zoom_in );
			sint16 y_size = max( 5, size.y*zoom_in );
			display_fillbox_wh_clip( fab_pos.x-1, fab_pos.y-1, x_size+2, y_size+2, COL_BLACK, false );
			display_fillbox_wh_clip( fab_pos.x, fab_pos.y, x_size, y_size, f->get_kennfarbe(), false );
		}
	}

	if(  mode & MAP_DEPOT  ) {
		FOR(  slist_tpl<depot_t*>,  const d,  depot_t::get_depot_list()  ) {
			if(  d->get_besitzer() == welt->get_active_player()  ) {
				koord depot_pos = d->get_pos().get_2d();
				karte_to_screen( depot_pos );
				depot_pos = depot_pos + pos;
				// offset of one to avoid
				static COLOR_VAL depot_typ_to_color[19]={ COL_ORANGE, COL_YELLOW, COL_RED, 0, 0, 0, 0, 0, 0, COL_PURPLE, COL_DARK_RED, COL_DARK_ORANGE, 0, 0, 0, 0, 0, 0, COL_LIGHT_RED };
				display_filled_circle( depot_pos.x, depot_pos.y, 4, depot_typ_to_color[d->get_typ() - ding_t::bahndepot] );
				display_circle( depot_pos.x, depot_pos.y, 4, COL_BLACK );
			}
		}
	}

	// zoom/resize "selection box" in map
	// this must be rotated by 45 degree (sin45=cos45=0,5*sqrt(2)=0.707...)
	const sint16 raster=get_tile_raster_width();

	// calculate and draw the rotated coordinates
	koord ij = welt->get_world_position();
	const koord diff = koord( display_get_width()/(2*raster), display_get_height()/raster );

	koord view[4];
	view[0] = ij + koord( -diff.y+diff.x, -diff.y-diff.x );
	view[1] = ij + koord( -diff.y-diff.x, -diff.y+diff.x );
	view[2] = ij + koord( diff.y-diff.x, diff.y+diff.x );
	view[3] = ij + koord( diff.y+diff.x, diff.y-diff.x );
	for(  int i=0;  i<4;  i++  ) {
		karte_to_screen( view[i] );
		view[i] += pos;
	}
	for(  int i=0;  i<4;  i++  ) {
		display_direct_line( view[i].x, view[i].y, view[(i+1)%4].x, view[(i+1)%4].y, COL_YELLOW);
	}

	if(  !showing_schedule  ) {
		// Add factory name tooltips and draw factory connections, if on a factory
		const fabrik_t* const fab = (mode & MAP_FACTORIES) ?
			draw_fab_connections(event_get_last_control_shift() & 1 ? COL_RED : COL_WHITE, pos)
			:
			get_fab( last_world_pos, false );

		if(fab) {
			koord fabpos = fab->get_pos().get_2d();
			karte_to_screen( fabpos );
			koord boxpos = fabpos + koord(10, 0);
			const char * name = translator::translate(fab->get_name());
			int name_width = proportional_string_width(name)+8;
			boxpos.x = clamp( boxpos.x, 0, 0+get_groesse().x-name_width );
			boxpos += pos;
			display_ddd_proportional_clip(boxpos.x, boxpos.y, name_width, 0, 10, COL_WHITE, name, true);
		}
	}
}


void reliefkarte_t::set_city( const stadt_t* _city )
{
	if(  city != _city  ) {
		city = _city;
		if(  _city  ) {
			pax_destinations_last_change = _city->get_pax_destinations_new_change();
		}
		calc_map();
	}
}


void reliefkarte_t::rdwr(loadsave_t *file)
{
	file->rdwr_short(zoom_out);
	file->rdwr_short(zoom_in);
}
