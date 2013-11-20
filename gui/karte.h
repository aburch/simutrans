#ifndef gui_karte_h
#define gui_karte_h

#include "components/gui_komponente.h"
#include "../halthandle_t.h"
#include "../convoihandle_t.h"
#include "../dataobj/fahrplan.h"
#include "../tpl/array2d_tpl.h"
#include "../tpl/vector_tpl.h"


class karte_ptr_t;
class fabrik_t;
class grund_t;
class stadt_t;
class spieler_t;
class schedule_t;
class loadsave_t;


#define MAX_SEVERITY_COLORS 10
#define MAX_MAP_ZOOM 4
// set to zero to use the small font
#define ALWAYS_LARGE 1

/**
 * This class is used to render the relief map.
 * Implemented as singleton.
 *
 * @author Hj. Malthaner
 */
class reliefkarte_t : public gui_komponente_t
{
public:
	enum MAP_MODES {
		PLAIN    = 0,
		MAP_TOWN = 1,
		MAP_PASSENGER = 1<<1,
		MAP_MAIL = 1<<2,
		MAP_FREIGHT = 1<<3,
		MAP_STATUS = 1<<4,
		MAP_SERVICE = 1<<5,
		MAP_TRAFFIC = 1<<6,
		MAP_ORIGIN = 1<<7,
		MAP_TRANSFER = 1<<8,
		MAP_WAITING = 1<<9,
		MAP_TRACKS = 1<<10,
		MAX_SPEEDLIMIT = 1<<11,
		MAP_POWERLINES = 1<<12,
		MAP_TOURIST = 1<<13,
		MAP_FACTORIES = 1<<14,
		MAP_DEPOT = 1<<15,
		MAP_FOREST = 1<<16,
		MAP_CITYLIMIT = 1<<17,
		MAP_PAX_DEST = 1<<18,
		MAP_OWNER = 1<<19,
		MAP_LINES = 1<<20,
		MAP_LEVEL = 1<<21,
		MAP_WAITCHANGE = 1<<22,
		MAP_MODE_HALT_FLAGS = (MAP_STATUS|MAP_SERVICE|MAP_ORIGIN|MAP_TRANSFER|MAP_WAITING|MAP_WAITCHANGE),
		MAP_MODE_FLAGS = (MAP_TOWN|MAP_CITYLIMIT|MAP_STATUS|MAP_SERVICE|MAP_WAITING|MAP_WAITCHANGE|MAP_TRANSFER|MAP_LINES|MAP_FACTORIES|MAP_ORIGIN|MAP_DEPOT|MAP_TOURIST|MAP_PAX_DEST)
	};

private:
	static karte_ptr_t welt;

	reliefkarte_t();

	static reliefkarte_t *single_instance;

	// the terrain map
	array2d_tpl<uint8> *relief;

	void set_relief_color_clip( sint16 x, sint16 y, uint8 color );

	void set_relief_farbe_area(koord k, int areasize, uint8 color);

	// all stuff connected with schedule display
	class line_segment_t
	{
	public:
		koord start, end;
		schedule_t *fpl;
		spieler_t *sp;
		waytype_t waytype;
		uint8 colorcount;
		uint8 start_offset;
		uint8 end_offset;
		bool start_diagonal;
		line_segment_t() {}
		line_segment_t( koord s, uint8 so, koord e, uint8 eo, schedule_t *f, spieler_t *p, uint8 cc, bool diagonal ) {
			fpl = f;
			waytype = f->get_waytype();
			sp = p;
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
		bool operator == (const line_segment_t & k) const;
	};
	// Ordering based on first start then end coordinate
	class LineSegmentOrdering
	{
	public:
		bool operator()(const reliefkarte_t::line_segment_t& a, const reliefkarte_t::line_segment_t& b) const;
	};
	vector_tpl<line_segment_t> schedule_cache;
	convoihandle_t current_cnv;
	uint8 fpl_player_nr;
	uint8 last_schedule_counter;
	vector_tpl<halthandle_t> stop_cache;

	// adds a schedule to cache
	void add_to_schedule_cache( convoihandle_t cnv, bool with_waypoints );

	/**
	 * map mode: -1) normal; everything else: special map
	 * @author hsiegeln
	 */
	static MAP_MODES mode;
	static MAP_MODES last_mode;
	static const uint8 severity_color[MAX_SEVERITY_COLORS];

	koord screen_to_karte(const scr_coord&) const;

	// for passenger destination display
	const stadt_t *city;
	unsigned long pax_destinations_last_change;

	koord last_world_pos;

	// current and new offset and size (to avoid drawing invisible parts)
	scr_coord cur_off, new_off;
	scr_size cur_size, new_size;

	// true, if full redraw is needed
	bool needs_redraw;

	const fabrik_t* get_fab(koord pos, bool large_area) const;

	const fabrik_t* draw_fab_connections(uint8 colour, scr_coord pos) const;

	static sint32 max_cargo;
	static sint32 max_passed;

	// the zoom factors
	sint16 zoom_out, zoom_in;

public:
	scr_coord karte_to_screen(const koord &k) const;

	static bool is_visible;

	// 45 rotated map
	bool isometric;

	/**
	 * returns a color based on an amount (high amount/scale -> color shifts from green to red)
	 * @author hsiegeln
	 */
	static uint8 calc_severity_color(sint32 amount, sint32 scale);

	/**
	 * returns a color based on an amount (high amount/scale -> color shifts from green to red)
	 * but using log scale
	 * @author prissi
	 */
	static uint8 calc_severity_color_log(sint32 amount, sint32 scale);

	/**
	* returns a color based on the current high
	* @author hsiegeln
	*/
	static uint8 calc_hoehe_farbe(const sint16 hoehe, const sint16 grundwasser);

	// needed for town passenger map
	static uint8 calc_relief_farbe(const grund_t *gr);

	// public, since the convoi updates need this
	// nonstatic, if we have someday many maps ...
	void set_relief_farbe(koord k, int color);

	// we are single instance ...
	static reliefkarte_t *get_karte();

	// HACK! since we cannot get cleanly the current offset/size, we use this helper function
	void set_xy_offset_size( scr_coord off, scr_size size ) {
		new_off = off;
		new_size = size;
	}

	scr_coord get_offset() const { return cur_off; };

	// update color with render mode (but few are ignored ... )
	void calc_map_pixel(const koord k);

	void calc_map();

	// calculates the current size of the map (but do not change anything else)
	void calc_map_size();

	~reliefkarte_t();

	void init();

	void set_mode(MAP_MODES new_mode);

	MAP_MODES get_mode() { return mode; }

	// updates the map (if needed)
	void neuer_monat();

	bool infowin_event(event_t const*) OVERRIDE;

	void draw(scr_coord pos);

	void set_current_cnv( convoihandle_t c );

	void set_city( const stadt_t* _city );

	const stadt_t* get_city() const { return city; };

	/**
	 * @returns true if zoom factors changed
	 */
	bool change_zoom_factor(bool magnify);

	void get_zoom_factors(sint16 &zoom_out_, sint16 &zoom_in_) const {
		zoom_in_ = zoom_in;
		zoom_out_ = zoom_out;
	}

	void rdwr(loadsave_t *file);
};

#endif
