#include <stdio.h>

#include "../../simdebug.h"

#include "../../dataobj/ribi.h"

#include "../intro_dates.h"
#include "../tunnel_besch.h"
#include "../obj_besch.h"
#include "../obj_node_info.h"
#include "tunnel_reader.h"

#include "../../bauer/tunnelbauer.h"
#include "../../dataobj/pakset_info.h"


void tunnel_reader_t::register_obj(obj_besch_t *&data)
{
	tunnel_besch_t *besch = static_cast<tunnel_besch_t *>(data);
	if(besch->get_topspeed()==0) {
		convert_old_tunnel(besch);
	}
	DBG_DEBUG("tunnel_reader_t::register_obj", "Loaded '%s'", besch->get_name());
	tunnelbauer_t::register_besch(besch);

	checksum_t *chk = new checksum_t();
	besch->calc_checksum(chk);
	pakset_info_t::append(besch->get_name(), chk);
}

/**
 * Sets default data for ancient tunnel paks
 */
void tunnel_reader_t::convert_old_tunnel(tunnel_besch_t *besch)
{
	// old style, need to convert
	if(strcmp(besch->get_name(),"RoadTunnel")==0) {
		besch->wegtyp = (uint8)road_wt;
		besch->topspeed = 120;
	}
	else {
		besch->wegtyp = (uint8)track_wt;
		besch->topspeed = 280;
	}
	besch->maintenance = 500;
	besch->preis = 200000;
	besch->intro_date = DEFAULT_INTRO_DATE*12;
	besch->obsolete_date = DEFAULT_RETIRE_DATE*12;
}


obj_besch_t * tunnel_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	tunnel_besch_t *besch = new tunnel_besch_t();
	besch->topspeed = 0;	// indicate, that we have to convert this to reasonable date, when read completely
	besch->node_info = new obj_besch_t*[node.children];

	if(node.size>0) {
		// newer versioned node
		ALLOCA(char, besch_buf, node.size);

		fread(besch_buf, node.size, 1, fp);

		char * p = besch_buf;

		const uint16 v = decode_uint16(p);
		const int version = v & 0x8000 ? v & 0x7FFF : 0;

		if( version == 4 ) {
			// versioned node, version 4 - broad portal support
			besch->topspeed = decode_uint32(p);
			besch->preis = decode_uint32(p);
			besch->maintenance = decode_uint32(p);
			besch->wegtyp = decode_uint8(p);
			besch->intro_date = decode_uint16(p);
			besch->obsolete_date = decode_uint16(p);
			besch->number_seasons = decode_uint8(p);
			besch->has_way = decode_uint8(p);
			besch->broad_portals = decode_uint8(p);
		}
		else if(version == 3) {
			// versioned node, version 3 - underground way image support
			besch->topspeed = decode_uint32(p);
			besch->preis = decode_uint32(p);
			besch->maintenance = decode_uint32(p);
			besch->wegtyp = decode_uint8(p);
			besch->intro_date = decode_uint16(p);
			besch->obsolete_date = decode_uint16(p);
			besch->number_seasons = decode_uint8(p);
			besch->has_way = decode_uint8(p);
			besch->broad_portals = 0;
		}
		else if(version == 2) {
			// versioned node, version 2 - snow image support
			besch->topspeed = decode_uint32(p);
			besch->preis = decode_uint32(p);
			besch->maintenance = decode_uint32(p);
			besch->wegtyp = decode_uint8(p);
			besch->intro_date = decode_uint16(p);
			besch->obsolete_date = decode_uint16(p);
			besch->number_seasons = decode_uint8(p);
			besch->has_way = 0;
			besch->broad_portals = 0;
		}
		else if(version == 1) {
			// first versioned node, version 1
			besch->topspeed = decode_uint32(p);
			besch->preis = decode_uint32(p);
			besch->maintenance = decode_uint32(p);
			besch->wegtyp = decode_uint8(p);
			besch->intro_date = decode_uint16(p);
			besch->obsolete_date = decode_uint16(p);
			besch->number_seasons = 0;
			besch->has_way = 0;
			besch->broad_portals = 0;
		} else {
			dbg->fatal("tunnel_reader_t::read_node()","illegal version %d",version);
		}

		DBG_DEBUG("tunnel_reader_t::read_node()",
		     "version=%d waytype=%d price=%d topspeed=%d, intro_year=%d",
		     version, besch->wegtyp, besch->preis, besch->topspeed, besch->intro_date/12);
	}

	return besch;
}
