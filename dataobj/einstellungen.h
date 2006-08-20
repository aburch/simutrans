#ifndef dataobj_einstellungen_h
#define dataobj_einstellungen_h

#include "../utils/cstring_t.h"
#include "../simtypes.h"

/**
 * einstellungen.h
 *
 * Spieleinstellungen
 *
 * Hj. Malthaner
 *
 * April 2000
 */

class loadsave_t;
class cstring_t;

class einstellungen_t
{
private:
    int groesse_x, groesse_y;
    int nummer;

    /* new setting since version 0.85.01
     * @author prissi
     * not used any more:    int industrie_dichte;
     */
    int land_industry_chains;
    int city_industry_chains;
    int tourist_attractions;

    int anzahl_staedte;
    int mittlere_einwohnerzahl;

    int scroll_multi;

    unsigned short station_coverage_size;

    /**
     * ab welchem level erzeugen gebaeude verkehr ?
     */
    int verkehr_level;

    /**
     * sollen Fussgaenger angezeigt werden ?
     */
    int show_pax;

     /**
     * Grundwasserspiegel ?
     */

    int grundwasser;

    double max_mountain_height;                  //01-Dec-01        Markus Weber    Added
    double map_roughness;                        //01-Dec-01        Markus Weber    Added

    unsigned char allow_player_change;
    unsigned char use_timeline;
    sint16 starting_year;
    sint16 bits_per_month;

	cstring_t filename;

	bool beginner_mode;

public:

    /**
     * If map is read from a heightfield, this is the name of the heightfield.
     * Set to empty string in order to avoid loading.
     * @author Hj. Malthaner
     */
    cstring_t heightfield;

    einstellungen_t();

    /**
     * Copy constructor, needed becuase of cstring
     * @author Hj. Malthaner
     */
    einstellungen_t(const einstellungen_t *);

    void setze_groesse_x(int g) {groesse_x=g;}
    void setze_groesse_y(int g) {groesse_y=g;}
    void setze_groesse(int w,int h) {groesse_x=w;groesse_y=h;}
    int gib_groesse_x() const {return groesse_x;}
    int gib_groesse_y() const {return groesse_y;}

    void setze_karte_nummer(int n) {nummer=n;}
    int gib_karte_nummer() const {return nummer;}

    void setze_land_industry_chains(int d) {land_industry_chains=d;}
    int gib_land_industry_chains() const {return land_industry_chains;}

    void setze_city_industry_chains(int d) {city_industry_chains=d;}
    int gib_city_industry_chains() const {return city_industry_chains;}

    void setze_tourist_attractions(int d) {tourist_attractions=d;}
    int gib_tourist_attractions() const {return tourist_attractions;}

    void setze_anzahl_staedte(int n) {anzahl_staedte=n;}
    int gib_anzahl_staedte() const {return anzahl_staedte;}

    void setze_mittlere_einwohnerzahl(int n) {mittlere_einwohnerzahl=n;}
    int gib_mittlere_einwohnerzahl() const {return mittlere_einwohnerzahl;}

    void setze_scroll_multi(int n) {scroll_multi=n;}
    int gib_scroll_multi() const {return scroll_multi;}

    void setze_verkehr_level(int l) {verkehr_level=l;}
    int gib_verkehr_level() const {return verkehr_level;}

    void setze_show_pax(bool yesno) {show_pax=yesno;}
    bool gib_show_pax() const {return show_pax != 0;}

    void rdwr(loadsave_t *file);

    void setze_grundwasser(int n) {grundwasser=n;}
    int gib_grundwasser() const {return grundwasser;}

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
    const char *gib_filename() const {return filename.chars();}

	void setze_beginner_mode(bool yesno) {beginner_mode=yesno;}	// prissi, Aug-06
	const bool gib_beginner_mode() const {return beginner_mode;}
};

#endif // dataobj_einstellungen_h
