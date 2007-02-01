/* einsetllungen.cc
 *
 * Spieleinstellungen
 *
 * Hj. Malthaner
 *
 * April 2000
 */

#include "einstellungen.h"
#include "umgebung.h"
#include "../simconst.h"
#include "../simtypes.h"
#include "../simdebug.h"
#include "loadsave.h"

einstellungen_t::einstellungen_t() :
	filename(""),
	heightfield("")
{
	groesse_x = 256;
	groesse_y = 256;

	nummer = 0;

	/* new setting since version 0.85.01
	 * @author prissi
	 */
	land_industry_chains = 6;
	city_industry_chains = 1;
	tourist_attractions = 2;

	anzahl_staedte = 12;
	mittlere_einwohnerzahl = 1600;

	scroll_multi = 1;

	verkehr_level = 5;

	show_pax = true;

	grundwasser = -2*Z_TILE_STEP;            //25-Nov-01        Markus Weber    Added
	for(  int i=0;  i<8;  i++ ) {
		climate_borders[i] = umgebung_t::climate_borders[i];
	}
	winter_snowline = umgebung_t::winter_snowline;

	max_mountain_height = 160;                  //can be 0-160.0  01-Dec-01        Markus Weber    Added
	map_roughness = 0.6;                        //can be 0-1      01-Dec-01        Markus Weber    Added

	station_coverage_size = umgebung_t::station_coverage_size;	//added May 05, valid 1...x

	// some settigns more
	allow_player_change = true;
	use_timeline = true;
	starting_year = umgebung_t::starting_year;
	bits_per_month = umgebung_t::bits_per_month;

	beginner_mode = false;
}


einstellungen_t::einstellungen_t(const einstellungen_t *other)
{
	groesse_x = other->groesse_x;
	groesse_y = other->groesse_y;

	nummer = other->nummer;
	land_industry_chains = other->land_industry_chains;
	city_industry_chains = other->city_industry_chains;
	tourist_attractions = other->tourist_attractions;

	anzahl_staedte = other->anzahl_staedte;
	mittlere_einwohnerzahl = other->mittlere_einwohnerzahl;
	scroll_multi = other->scroll_multi;
	verkehr_level = other->verkehr_level;
	show_pax = other->show_pax;

	grundwasser = other->grundwasser;
	for(  int i=0;  i<8;  i++ ) {
		climate_borders[i] = other->climate_borders[i];
	}
	winter_snowline = other->winter_snowline;
	max_mountain_height = other->max_mountain_height;
	map_roughness = other->map_roughness;
	heightfield = other->heightfield;

	station_coverage_size = other->station_coverage_size;

	allow_player_change = other->allow_player_change;
	beginner_mode = other->beginner_mode;
	just_in_time = other->just_in_time;

	use_timeline = other->use_timeline;
	starting_year = other->starting_year;
	bits_per_month = other->bits_per_month;

	filename = other->filename;
}

void
einstellungen_t::rdwr(loadsave_t *file)
{
	if(file->get_version() < 86000) {
		uint32 dummy;

		file->rdwr_long(groesse_x, " ");
		groesse_y = groesse_x;

		file->rdwr_long(nummer, "\n");

		// to be compatible with previous savegames
		dummy = 0;
		file->rdwr_long(dummy, " ");	//dummy!
		land_industry_chains = 6;
		city_industry_chains = 0;
		tourist_attractions = 12;

		// now towns
		mittlere_einwohnerzahl = 1600;
		dummy =  anzahl_staedte;
		file->rdwr_long(dummy, "\n");
		dummy &= 127;
		if(dummy>63) {
			dbg->warning("einstellungen_t::rdwr()","This game was saved with too many cities! (%i of maximum 63). Simutrans may crash!",dummy);
		}
		anzahl_staedte = dummy;

		// rest
		file->rdwr_long(scroll_multi, " ");
		file->rdwr_long(verkehr_level, "\n");
		file->rdwr_long(show_pax, "\n");
		dummy = grundwasser;
		file->rdwr_long(dummy, "\n");
		grundwasser = (sint16)(dummy/16)*Z_TILE_STEP;
		file->rdwr_double(max_mountain_height, "\n");
		file->rdwr_double(map_roughness, "\n");

		station_coverage_size = 3;
		beginner_mode = false;
	}
	else {
		// newer versions

		file->rdwr_long(groesse_x, " ");
		file->rdwr_long(nummer, "\n");

		// industries
		file->rdwr_long(land_industry_chains, " ");
		file->rdwr_long(city_industry_chains, " ");
		file->rdwr_long(tourist_attractions, "\n");

		// now towns
		file->rdwr_long(mittlere_einwohnerzahl, " ");
		file->rdwr_long(anzahl_staedte, " ");

		// rest
		file->rdwr_long(scroll_multi, " ");
		file->rdwr_long(verkehr_level, "\n");
		file->rdwr_long(show_pax, "\n");
		sint32 dummy = grundwasser/Z_TILE_STEP;
		file->rdwr_long(dummy, "\n");
		if(file->get_version() < 99005) {
			grundwasser = (sint16)(dummy/16)*Z_TILE_STEP;
		}
		else {
			grundwasser = (sint16)dummy*Z_TILE_STEP;
		}
		file->rdwr_double(max_mountain_height, "\n");
		file->rdwr_double(map_roughness, "\n");

		if(file->get_version() >= 86003) {
			dummy = station_coverage_size;
			file->rdwr_long(dummy, " ");
			station_coverage_size = (uint16)dummy;
		}

		if(file->get_version() >= 86006) {
			// handle also size on y direction
			file->rdwr_long(groesse_y, " ");
		}
		else {
			groesse_y = groesse_x;
		}

		if(file->get_version() >= 86011) {
			// some more settings
			file->rdwr_byte(allow_player_change, " ");
			file->rdwr_byte(use_timeline, " " );
			file->rdwr_short(starting_year, "\n");
		}
		else {
		    allow_player_change = 1;
		    use_timeline = 1;
		    starting_year = 1930;
		}

		if(file->get_version()>=88005) {
			file->rdwr_short(bits_per_month,"b");
		}
		else {
			bits_per_month = 18;
		}

		if(file->get_version()>=89003) {
			file->rdwr_bool(beginner_mode,"\n");
		}
		else {
			beginner_mode = false;
		}
		if(file->get_version()>=89004) {
			file->rdwr_bool(just_in_time,"\n");
		}
		else {
			just_in_time = umgebung_t::just_in_time;
		}

		// clear the name when loading ...
		if(file->is_loading()) {
			filename = "";
		}

		// climate corders
		if(file->get_version()>=91000) {
			for(  int i=0;  i<8;  i++ ) {
				file->rdwr_short(climate_borders[i], "c");
			}
			file->rdwr_short(winter_snowline, "c");
		}
		else {
			for(  int i=0;  i<8;  i++ ) {
				climate_borders[i] = umgebung_t::climate_borders[i];
			}
			winter_snowline = umgebung_t::winter_snowline;
		}
	}
}
