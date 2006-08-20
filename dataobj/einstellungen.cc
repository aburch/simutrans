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

    industrie_dichte = 25*30;

    anzahl_staedte = 12;

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
    industrie_dichte =
      other->industrie_dichte;
    anzahl_staedte =
      other->anzahl_staedte;
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
    file->rdwr_int(industrie_dichte, " ");
    file->rdwr_int(anzahl_staedte, "\n");
    file->rdwr_int(scroll_multi, " ");
    file->rdwr_int(verkehr_level, "\n");
    file->rdwr_int(show_pax, "\n");
    file->rdwr_int(grundwasser, "\n");
    file->rdwr_double(max_mountain_height, "\n");
    file->rdwr_double(map_roughness, "\n");
}
