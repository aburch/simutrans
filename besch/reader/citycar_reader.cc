#include <stdio.h>

#include "../../simvehikel.h"
#include "../../simverkehr.h"
#include "../stadtauto_besch.h"
#include "../intro_dates.h"

#include "citycar_reader.h"
#include "../obj_node_info.h"

#include "../../simdebug.h"


void citycar_reader_t::register_obj(obj_besch_t *&data)
{
    stadtauto_besch_t *besch = static_cast<stadtauto_besch_t *>(data);

	// init the lenght information
	for( int i=0;  i<8;  i++ ) {
		if(i<4) {
			besch->length[i] = 12+2;
		} else if(i<6) {
			besch->length[i] = 8+2;
		} else {
			besch->length[i] = 16+2;
		}
	}

    stadtauto_t::register_besch(besch);
//    printf("...Stadtauto %s geladen\n", besch->gib_name());
}


bool citycar_reader_t::successfully_loaded() const
{
	return stadtauto_t::laden_erfolgreich();
}


obj_besch_t * citycar_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

  stadtauto_besch_t *besch = new stadtauto_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

  // Hajo: Read data
  fread(besch_buf, node.size, 1, fp);

  char * p = besch_buf;

  // Hajo: old versions of PAK files have no version stamp.
  // But we know, the higher most bit was always cleared.

  const uint16 v = decode_uint16(p);
  const int version = v & 0x8000 ? v & 0x7FFF : 0;

  if(version == 2) {
    // Versioned node, version 1

    besch->gewichtung = decode_uint16(p);
    besch->geschw = kmh_to_speed(decode_uint16(p)/16);
    besch->intro_date = decode_uint16(p);
    besch->obsolete_date = decode_uint16(p);
  }
  else if(version == 1) {
    // Versioned node, version 1

    besch->gewichtung = decode_uint16(p);
    besch->geschw = kmh_to_speed(decode_uint16(p)/16);
    uint16 intro_date = decode_uint16(p);
    besch->intro_date = (intro_date/16)*12  + (intro_date%12);
    uint16 obsolete_date = decode_uint16(p);
    besch->obsolete_date= (obsolete_date/16)*12  + (obsolete_date%12);
  }
  else {
    besch->gewichtung = v;
    besch->geschw = kmh_to_speed(80);
    besch->intro_date = DEFAULT_INTRO_DATE*12;
    besch->obsolete_date = DEFAULT_RETIRE_DATE*12;
  }
  DBG_DEBUG("citycar_reader_t::read_node()","version=%i, weight=%i, intro=%i, retire=%i",version,besch->gewichtung,besch->intro_date/12,besch->obsolete_date/12);
  return besch;
}
