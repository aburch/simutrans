#include <stdio.h>

#include "../../simdebug.h"

#include "../../dataobj/ribi.h"

#include "../tunnel_besch.h"
#include "../obj_besch.h"
#include "../obj_node_info.h"
#include "tunnel_reader.h"

#include "../../bauer/tunnelbauer.h"


void tunnel_reader_t::register_obj(obj_besch_t *&data)
{
	tunnel_besch_t *besch = static_cast<tunnel_besch_t *>(data);
	printf("tunnel_reader_t::register_obj(): Tunnel %s geladen\n", besch->get_name());
	tunnelbauer_t::register_besch(besch);
}


bool
tunnel_reader_t::successfully_loaded() const
{
	return tunnelbauer_t::laden_erfolgreich();
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

		if(version == 2) {
	    	// versioned node, version 2 - snow image support
			besch->topspeed = decode_uint32(p);
			besch->preis = decode_uint32(p);
			besch->maintenance = decode_uint32(p);
			besch->wegtyp = decode_uint8(p);
			besch->intro_date = decode_uint16(p);
			besch->obsolete_date = decode_uint16(p);
			besch->number_seasons = decode_uint8(p);
			if(node.size == 26)
			{
				// Node size == 26 - extra features to be read. Version 2a.
				// Backwards compatible with version 2 with this code.
				besch->max_weight =  decode_uint32(p);
				besch->way_constraints_permissive = decode_uint8(p);
				besch->way_constraints_prohibitive = decode_uint8(p);
			}
			else
			{
				besch->max_weight = 999;
				besch->way_constraints_permissive = 0;
				besch->way_constraints_prohibitive = 0;
			}
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
			besch->max_weight = 999;
			besch->way_constraints_permissive = 0;
			besch->way_constraints_prohibitive = 0;
			


		} else {
			dbg->fatal("tunnel_reader_t::read_node()","illegal version %d",version);
		}

		DBG_DEBUG("bridge_reader_t::read_node()",
		     "version=%d waytype=%d price=%d topspeed=%d, intro_year=%d, max_weight%d",
			 version, besch->wegtyp, besch->preis, besch->topspeed, besch->intro_date/12, besch->max_weight);
	}

	return besch;
}
