#include <stdlib.h>
#include "../../simdebug.h"
#include "../../utils/simstring.h"	// for STRICMP
#include "../../dataobj/tabfile.h"

/**
 * Convert waytype string to enum waytype_t
 * @author Hj. Malthaner
 */
waytype_t get_waytype(const char* waytype)
{
	waytype_t uv8 = road_wt;

	if (!STRICMP(waytype, "none")) {
		uv8 = ignore_wt;
	} else if (!STRICMP(waytype, "road")) {
		uv8 = road_wt;
	} else if (!STRICMP(waytype, "track")) {
		uv8 = track_wt;
	} else if (!STRICMP(waytype, "electrified_track")) {
		uv8 = overheadlines_wt;
	} else if (!STRICMP(waytype, "maglev_track")) {
		uv8 = maglev_wt;
	} else if (!STRICMP(waytype, "monorail_track")) {
		uv8 = monorail_wt;
	} else if (!STRICMP(waytype, "narrowgauge_track")) {
		uv8 = narrowgauge_wt;
	} else if (!STRICMP(waytype, "water")) {
		uv8 = water_wt;
	} else if (!STRICMP(waytype, "air")) {
		uv8 = air_wt;
	} else if (!STRICMP(waytype, "schiene_tram")) {
		uv8 = tram_wt;
	} else if (!STRICMP(waytype, "tram_track")) {
		uv8 = tram_wt;
	} else if (!STRICMP(waytype, "power")) {
		uv8 = powerline_wt;
	} else if (!STRICMP(waytype, "decoration")) {
		uv8 = any_wt;
	} else if (!STRICMP(waytype, "overtaking_mode")) {
		uv8 = overtake_wt;
	} else {
		dbg->fatal("get_waytype()","invalid waytype \"%s\"\n", waytype);
		exit(1);
	}

	return uv8;
}

/**
 * Convert overtaking_mode string to enum overtaking_mode_t
 * @author teamhimeH
 */
overtaking_mode_t get_overtaking_mode(const char* o_mode)
{
	overtaking_mode_t uv8 = invalid_mode;

	if (!STRICMP(o_mode, "oneway")) {
		uv8 = oneway_mode;
	} else if (!STRICMP(o_mode, "twoway")) {
		uv8 = twoway_mode;
	} else if (!STRICMP(o_mode, "loading_convoy_only")) {
		uv8 = loading_only_mode;
	} else if (!STRICMP(o_mode, "inverted")) {
		uv8 = inverted_mode;
	} else if (!STRICMP(o_mode, "prohibited")) {
		uv8 = prohibited_mode;
	} else if (!STRICMP(o_mode, "none")  ||  !STRICMP(o_mode, "")) {
		uv8 = invalid_mode;
	} else {
		dbg->fatal("get_overtaking_mode()","invalid overtaking_mode \"%s\"\n", o_mode);
		exit(1);
	}

	return uv8;
}