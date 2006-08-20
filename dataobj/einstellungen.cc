/* einsetllungen.cc
 *
 * Spieleinstellungen
 *
 * Hj. Malthaner
 *
 * April 2000
 */

#include "einstellungen.h"
#include "../simtypes.h"
#include "loadsave.h"

einstellungen_t::einstellungen_t() : heightfield("")
{
    groesse = 256;
    nummer = 0;

    /* new setting since version 0.85.01
     * @author prissi
     */
    land_industry_chains = 1;
    city_industry_chains = 0;
    tourist_attractions = 2;

    anzahl_staedte = 12;
    mittlere_einwohnerzahl = 1600;

    scroll_multi = 1;

    verkehr_level = 5;

    show_pax = true;

    grundwasser = -32;                          //25-Nov-01        Markus Weber    Added

    max_mountain_height = 160;                  //can be 0-160.0  01-Dec-01        Markus Weber    Added
    map_roughness = 0.6;                        //can be 0-1      01-Dec-01        Markus Weber    Added
}

/**
 * Copy constructor, needed becuase of cstring
 * @author Hj. Malthaner
 */
einstellungen_t::einstellungen_t(const einstellungen_t *other)
{
    groesse =
      other->groesse;
    nummer =
      other->nummer;
    land_industry_chains =
      other->land_industry_chains;
    city_industry_chains =
      other->city_industry_chains;
    tourist_attractions =
      other->tourist_attractions;

    anzahl_staedte =
      other->anzahl_staedte;
    mittlere_einwohnerzahl =
      other->mittlere_einwohnerzahl;
    scroll_multi =
      other->scroll_multi;
    verkehr_level =
      other->verkehr_level;
    show_pax =
      other->show_pax;
    grundwasser =
      other->grundwasser;
    max_mountain_height =
      other->max_mountain_height;
    map_roughness =
      other->map_roughness;

    heightfield =
      other->heightfield;
}

void
einstellungen_t::rdwr(loadsave_t *file)
{
    file->rdwr_int(groesse, " ");
    file->rdwr_int(nummer, "\n");
    // to be compatible with previous savegames
    int dummy = ((land_industry_chains&1023)<<20) | ((city_industry_chains&1023)<<10) | (tourist_attractions&1023);
    file->rdwr_int(dummy, " ");	//dummy!
    land_industry_chains = dummy>>20;
    city_industry_chains = (dummy>>10)&1023;
    tourist_attractions = dummy&1023;
 	// now towns
    dummy =  (mittlere_einwohnerzahl<<16) + anzahl_staedte;
    file->rdwr_int(dummy, "\n");
    mittlere_einwohnerzahl = dummy>>16;
    anzahl_staedte = dummy & 63;
    //
    file->rdwr_int(scroll_multi, " ");
    file->rdwr_int(verkehr_level, "\n");
    file->rdwr_int(show_pax, "\n");
    file->rdwr_int(grundwasser, "\n");
    file->rdwr_double(max_mountain_height, "\n");
    file->rdwr_double(map_roughness, "\n");
}
