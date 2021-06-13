/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdlib.h>
#include "../../simdebug.h"
#include "../../utils/simstring.h" // for STRICMP
#include "../../dataobj/tabfile.h"

/**
 * Convert waytype string to enum waytype_t
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
	} else {
		dbg->fatal("get_waytype()","invalid waytype \"%s\"\n", waytype);
	}

	return uv8;
}
