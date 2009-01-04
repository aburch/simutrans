#ifndef gui_karte_h
#define gui_karte_h

#include "../ifc/gui_komponente.h"
#include "../tpl/array2d_tpl.h"

#include "../dataobj/fahrplan.h"

class karte_t;
class fabrik_t;
class grund_t;


#define MAX_MAP_TYPE_LAND 14
#define MAX_MAP_TYPE_WATER 5
#define MAX_SEVERITY_COLORS 10
#define MAX_MAP_ZOOM 4
// set to zero to use the small font
#define ALWAYS_LARGE 1

/**
 * Diese Klasse dient zur Darstellung der Reliefkarte
 * (18.06.00 von simwin getrennt)
 * Als Singleton implementiert.
 *
 * @author Hj. Malthaner
 */
class reliefkarte_t : public gui_komponente_t
{
public:
	typedef enum { PLAIN=-1, MAP_TOWN=0, MAP_PASSENGER, MAP_MAIL, MAP_FREIGHT, MAP_STATUS, MAP_SERVICE, MAP_TRAFFIC, MAP_ORIGIN, MAP_DESTINATION, MAP_WAITING, MAP_TRACKS, MAX_SPEEDLIMIT, MAP_POWERLINES, MAP_TOURIST, MAP_FACTORIES, MAP_DEPOT, MAP_FOREST, MAP_CITYLIMIT } MAP_MODES;

private:
	static karte_t *welt;
	array2d_tpl<uint8> *relief;

	reliefkarte_t();

	static reliefkarte_t *single_instance;

	/**
	* map mode: -1) normal; everything else: special map
	* @author hsiegeln
	*/
	static MAP_MODES mode;

	static const uint8 severity_color[MAX_SEVERITY_COLORS];

	static const uint8 map_type_color[MAX_MAP_TYPE_LAND+MAX_MAP_TYPE_WATER];

	inline void screen_to_karte(koord &) const;

	// to be prepared for more than one map => nonstatic
	void setze_relief_farbe_area(koord k, int areasize, uint8 color);

	const schedule_t *fpl;
	uint8 fpl_player_nr;

	koord last_world_pos;

	const fabrik_t* draw_fab_connections(uint8 colour, koord pos) const;

	void draw_schedule(const koord pos) const;

	static sint32 max_capacity;
	static sint32 max_departed;
	static sint32 max_arrived;
	static sint32 max_cargo;
	static sint32 max_convoi_arrived;
	static sint32 max_passed;
	static sint32 max_tourist_ziele;

public:
	void karte_to_screen(koord &) const;

	static bool is_visible;

	// the zoom factors
	sint16 zoom_out, zoom_in;

	// 45 rotated map
	bool rotate45;

	// show/hide schedule of convoi
	bool is_show_schedule;

	// show/hide factory connections
	bool is_show_fab;

	/**
	* returns a color based on an amount (high amount/scale -> color shifts from green to red)
	* @author hsiegeln
	*/
	static uint8 calc_severity_color(sint32 amount, sint32 scale);

	/**
	* returns a color based on the current high
	* @author hsiegeln
	*/
	static uint8 calc_hoehe_farbe(const sint16 hoehe, const sint16 grundwasser);

	// needed for town pasenger map
	static uint8 calc_relief_farbe(const grund_t *gr);

	// public, since the convoi updates need this
	// nonstatic, if we have someday many maps ...
	void setze_relief_farbe(koord k, int color);

	// we are single instance ...
	static reliefkarte_t *gib_karte();

	// update color with render mode (but few are ignored ... )
	void calc_map_pixel(const koord k);

	void calc_map();

	~reliefkarte_t();

	karte_t * get_welt() const {return welt;}

	void setze_welt(karte_t *welt);

	void set_mode(MAP_MODES new_mode);

	MAP_MODES get_mode() { return mode; }

	// updates the map (if needed)
	void neuer_monat();

	// these are the gui_container needed functions ...
	void infowin_event(const event_t *ev);

	void zeichnen(koord pos);

	void set_current_fpl(const schedule_t *current_fpl, uint8 player_nr) {fpl = current_fpl; fpl_player_nr = player_nr;};
};

#endif
