#include "../../simtypes.h"

#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"

#include "../../boden/wege/weg.h"

/**
 * Convert waytype string to enum wegtyp
 * @author Hj. Malthaner
 */
uint8 get_waytype(const char * waytype)
{
	uint8 uv8 = weg_t::strasse;

	if(!STRICMP(waytype, "road")) {
		uv8 = weg_t::strasse;
	} else if(!STRICMP(waytype, "track")) {
		uv8 = weg_t::schiene;
	} else if(!STRICMP(waytype, "electrified_track")) {
		uv8 = weg_t::overheadlines;
	} else if(!STRICMP(waytype, "monorail_track")) {
		uv8 = weg_t::schiene_monorail;
	} else if(!STRICMP(waytype, "maglev_track")) {
		uv8 = weg_t::schiene_maglev;
	} else if(!STRICMP(waytype, "water")) {
		uv8 = weg_t::wasser;
	} else if(!STRICMP(waytype, "air")) {
		uv8 = weg_t::luft;
	} else if(!STRICMP(waytype, "schiene_tram")) {
		uv8 = weg_t::schiene_strab;
	} else if(!STRICMP(waytype, "tram_track")) {
		uv8 = weg_t::schiene_strab;
	} else if(!STRICMP(waytype, "power")) {
		uv8 = weg_t::powerline;
	} else {
//		cstring_t reason;
		printf("\nFATAL\ninvalid waytype %s\n", waytype);
		exit(1);
	}

	return uv8;
}
