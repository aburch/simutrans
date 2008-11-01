#ifndef dataobj_einstellungen_h
#define dataobj_einstellungen_h

#include "../utils/cstring_t.h"
#include "../simtypes.h"

/**
 * Spieleinstellungen
 *
 * Hj. Malthaner
 *
 * April 2000
 */

class loadsave_t;

class einstellungen_t
{
private:
	sint32 groesse_x, groesse_y;
	sint32 nummer;

	/* new setting since version 0.85.01
	 * @author prissi
	 * not used any more:    sint32 industrie_dichte;
	 */
	sint32 land_industry_chains;
	sint32 electric_promille;
	sint32 tourist_attractions;

	sint32 anzahl_staedte;
	sint32 mittlere_einwohnerzahl;

	sint32 scroll_multi;

	uint16 station_coverage_size;

	/**
	 * ab welchem level erzeugen gebaeude verkehr ?
	 */
	sint32 verkehr_level;

	/**
	 * sollen Fussgaenger angezeigt werden ?
	 */
	sint32 show_pax;

	 /**
	 * waterlevel, climate borders, lowest snow in winter
	 */

	sint16 grundwasser;
	sint16 climate_borders[MAX_CLIMATES];
	sint16 winter_snowline;

	double max_mountain_height;                  //01-Dec-01        Markus Weber    Added
	double map_roughness;                        //01-Dec-01        Markus Weber    Added

	unsigned char allow_player_change;
	unsigned char use_timeline;
	sint16 starting_year;
	sint16 bits_per_month;

	cstring_t filename;

	bool beginner_mode;
	bool just_in_time;

	// default 0, will be incremented after each 90 degree rotation until 4
	uint8 rotation;

	sint16 origin_x, origin_y;

public:

	/**
	 * If map is read from a heightfield, this is the name of the heightfield.
	 * Set to empty string in order to avoid loading.
	 * @author Hj. Malthaner
	 */
	cstring_t heightfield;

	einstellungen_t();

	void setze_groesse_x(sint32 g) {groesse_x=g;}
	void setze_groesse_y(sint32 g) {groesse_y=g;}
	void setze_groesse(sint32 w,sint32 h) {groesse_x=w;groesse_y=h;}
	sint32 gib_groesse_x() const {return groesse_x;}
	sint32 gib_groesse_y() const {return groesse_y;}

	void setze_karte_nummer(sint32 n) {nummer=n;}
	sint32 gib_karte_nummer() const {return nummer;}

	void setze_land_industry_chains(sint32 d) {land_industry_chains=d;}
	sint32 gib_land_industry_chains() const {return land_industry_chains;}

	void setze_electric_promille(sint32 d) {electric_promille=d;}
	sint32 gib_electric_promille() const {return electric_promille;}

	void setze_tourist_attractions(sint32 d) {tourist_attractions=d;}
	sint32 gib_tourist_attractions() const {return tourist_attractions;}

	void setze_anzahl_staedte(sint32 n) {anzahl_staedte=n;}
	sint32 gib_anzahl_staedte() const {return anzahl_staedte;}

	void setze_mittlere_einwohnerzahl(sint32 n) {mittlere_einwohnerzahl=n;}
	sint32 gib_mittlere_einwohnerzahl() const {return mittlere_einwohnerzahl;}

	void setze_scroll_multi(sint32 n) {scroll_multi=n;}
	sint32 gib_scroll_multi() const {return scroll_multi;}

	void setze_verkehr_level(sint32 l) {verkehr_level=l;}
	sint32 gib_verkehr_level() const {return verkehr_level;}

	void setze_show_pax(bool yesno) {show_pax=yesno;}
	bool gib_show_pax() const {return show_pax != 0;}

	void rdwr(loadsave_t *file);

	void setze_grundwasser(sint32 n) {grundwasser=n;}
	sint32 gib_grundwasser() const {return grundwasser;}

	void setze_max_mountain_height(double n) {max_mountain_height=n;}          //01-Dec-01        Markus Weber    Added
	double gib_max_mountain_height() const {return max_mountain_height;}

	void setze_map_roughness(double n) {map_roughness=n;}                      //01-Dec-01        Markus Weber    Added
	double gib_map_roughness() const {return map_roughness;}

	void setze_station_coverage(unsigned short n) {station_coverage_size=n;}	// prissi, May-2005
	unsigned short gib_station_coverage() const {return station_coverage_size;}

	void setze_allow_player_change(char n) {allow_player_change=n;}	// prissi, Oct-2005
	unsigned char gib_allow_player_change() const {return allow_player_change;}

	void setze_use_timeline(char n) {use_timeline=n;}	// prissi, Oct-2005
	unsigned char gib_use_timeline() const {return use_timeline;}

	void setze_starting_year(short n) {starting_year=n;}	// prissi, Oct-2005
	short gib_starting_year() const {return starting_year;}

	void setze_bits_per_month(short n) {bits_per_month=n;}	// prissi, Oct-2005
	short gib_bits_per_month() const {return bits_per_month;}

	void setze_filename(const char *n) {filename=n;}	// prissi, Jun-06
	const char* gib_filename() const { return filename; }

	void setze_beginner_mode(bool yesno) {beginner_mode=yesno;}	// prissi, Aug-06
	bool gib_beginner_mode() const {return beginner_mode;}

	void setze_just_in_time(bool yesno) {just_in_time=yesno;}	// prissi, Aug-06
	bool gib_just_in_time() const {return just_in_time;}

	void setze_climate_border(sint16 *);	// prissi, Aug-06
	const sint16 *gib_climate_borders() const {return climate_borders;}

	void setze_winter_snowline(sint16 sl) { winter_snowline = sl; }
	sint16 gib_winter_snowline() const {return winter_snowline;}

	void rotate90() { rotation = (rotation+1)&3; }
	uint8 get_rotation() const { return rotation; }

	void setze_origin_x(sint16 x) { origin_x = x; }
	void setze_origin_y(sint16 y) { origin_y = y; }
	sint16 get_origin_x() const { return origin_x; }
	sint16 get_origin_y() const { return origin_y; }
};

#endif
