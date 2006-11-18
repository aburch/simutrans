#include "../../simtypes.h"

#include "../../dataobj/tabfile.h"

#include "../../boden/wege/weg.h"

/**
 * Convert waytype string to enum wegtyp
 * @author Hj. Malthaner
 */
uint8 get_waytype(const char * waytype)
{
	uint8 uv8 = road_wt;

	if(!STRICMP(waytype, "road")) {
		uv8 = road_wt;
	} else if(!STRICMP(waytype, "track")) {
		uv8 = track_wt;
	} else if(!STRICMP(waytype, "electrified_track")) {
		uv8 = overheadlines_wt;
	} else if(!STRICMP(waytype, "monorail_track")) {
		uv8 = monorail_wt;
	} else if(!STRICMP(waytype, "maglev_track")) {
		uv8 = maglev_wt;
	} else if(!STRICMP(waytype, "water")) {
		uv8 = water_wt;
	} else if(!STRICMP(waytype, "air")) {
		uv8 = air_wt;
	} else if(!STRICMP(waytype, "schiene_tram")) {
		uv8 = tram_wt;
	} else if(!STRICMP(waytype, "tram_track")) {
		uv8 = tram_wt;
	} else if(!STRICMP(waytype, "power")) {
		uv8 = powerline_wt;
	} else {
		printf("\nFATAL\ninvalid waytype %s\n", waytype);
		exit(1);
	}

	return uv8;
}
