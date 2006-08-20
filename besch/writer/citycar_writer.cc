#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"

#include "../stadtauto_besch.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist_writer.h"

#include "citycar_writer.h"


void citycar_writer_t::write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj)
{
    stadtauto_besch_t besch;
    int i;

    obj_node_t	node(this, 10, &parent, false);

    besch.gewichtung = obj.get_int("distributionweight", 1);

    besch.intro_date  = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
    besch.intro_date += obj.get_int("intro_month", 1) - 1;

    besch.obsolete_date  = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
    besch.obsolete_date += obj.get_int("retire_month", 1) - 1;

    besch.geschw  = obj.get_int("speed", 80) * 16;

    // new version with intro and obsolete dates
    uint16 data = 0x8002;
    node.write_data_at(fp, &data, 0, sizeof(uint16));

    data = (uint16)besch.gewichtung;
    node.write_data_at(fp, &data, 2, sizeof(uint16));

    data = (uint16)besch.geschw;
    node.write_data_at(fp, &data, 4, sizeof(uint16));

    data = besch.intro_date;
    node.write_data_at(fp, &data, 6, sizeof(uint16));

    data = besch.obsolete_date;
    node.write_data_at(fp, &data, 8, sizeof(uint16));

   write_head(fp, node, obj);

    static const char * dir_codes[] = {
	"s", "w", "sw", "se", "n", "e", "ne", "nw"
    };
    slist_tpl<cstring_t> keys;
    cstring_t str;

    for(i = 0; i < 8; i++) {
	char buf[40];

	sprintf(buf, "image[%s]", dir_codes[i]);
	str = obj.get(buf);
	keys.append(str);
    }
    imagelist_writer_t::instance()->write_obj(fp, node, keys);

    node.write(fp);
}
