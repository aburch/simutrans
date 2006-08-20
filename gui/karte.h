#ifndef gui_karte_h
#define gui_karte_h

#ifndef ifc_gui_komponente_h
#include "../ifc/gui_komponente.h"
#endif

#ifndef tpl_array2d_tpl_h
#include "../tpl/array2d_tpl.h"
#endif

class karte_t;
class fabrik_t;
class grund_t;

struct event_t;

#define MAX_MAP_TYPE 15
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
private:
    karte_t *welt;
    array2d_tpl<unsigned char> *relief;

    reliefkarte_t();

    static reliefkarte_t * single_instance;

    int zoom;

	/**
	 * map mode: -1) normal; everything else: special map
	 * @author hsiegeln
	 */
    static int mode;

	static const int map_type_color[MAX_MAP_TYPE];

	void calc_map(int mode);

	void setze_relief_farbe_area(koord k, int areasize, int color);

	/**
	 * false: updates are possible
	 * true: no updates are possible
	 * @author hsiegeln
	 */
	bool is_map_locked;

	/**
	 * returns a color based on an amount (high amount/scale -> color shifts from green to red)
	 * @author hsiegeln
	 */
	int calc_severity_color(int amount, int scale);

	fabrik_t * fab;
	grund_t * gr;

	void draw_fab_connections(const fabrik_t * fab, int colour, koord pos) const;

	bool is_shift_pressed;

public:
	typedef enum { MAP_TOWN=0, MAP_PASSENGER, MAP_MAIL, MAP_FREIGHT, MAP_STATUS, MAP_SERVICE, MAP_TRAFFIC, MAP_ORIGIN, MAP_DESTINATION, MAP_WAITING, MAP_TRACKS, MAX_SPEEDLIMIT, MAP_POWERLINES, MAP_TOURIST, MAP_FACTORIES };

	static const int severity_color[12];

    /**
     * Berechnet Farbe für Höhenstufe hdiff (hoehe - grundwasser).
     * @author Hj. Malthaner
     */
    static int calc_hoehe_farbe(const int hoehe, const int grundwasser);


    void setze_relief_farbe(koord k, int color);

    /**
     * Berechnet farbe für Welt and Position k. Ist statisch, da das
     * auch für andere Klassen (z.B. Welt-GUI) interessant ist.
     * @author Hj. Malthaner
     */
    static int calc_relief_farbe(const karte_t *welt, koord k);

    static reliefkarte_t *gib_karte();

    /**
     * Updated Kartenfarbe an Position k
     * @author Hj. Malthaner
     */
    void recalc_relief_farbe(koord k);

    ~reliefkarte_t();

    karte_t * gib_welt() const {return welt;};

    void setze_welt(karte_t *welt);
    void init();

    void infowin_event(const event_t *ev);

    void zeichnen(koord pos) const;

    void set_mode(int new_mode);

    int get_mode() { return mode; };

	// updates the map (if needed)
    void neuer_monat();
};

#endif
