
#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"

#include "../stadtauto_besch.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist_writer.h"

#include "citycar_writer.h"

#ifdef _MSC_VER
#define STRICMP stricmp
#else
#define STRICMP strcasecmp
#endif

/////////////////////////////////////////////////////////////////////////////
//
//  static data
//
/////////////////////////////////////////////////////////////////////////////


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      citycar_writer_t::write_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      FILE *fp
//      obj_node_t &parent
//      tabfileobj_t &obj
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void citycar_writer_t::write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj)
{
    stadtauto_besch_t besch;
    int i;

    obj_node_t	node(this, 8, &parent, true);

    besch.gewichtung = obj.get_int("distributionweight", 1);

    besch.intro_date  = obj.get_int("intro_year", 1900) * 16;
    besch.intro_date += obj.get_int("intro_month", 1) - 1;

    besch.obsolete_date  = obj.get_int("retire_year", 2999) * 16;
    besch.obsolete_date += obj.get_int("retire_month", 1) - 1;

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

    // new version with intro and obsolete dates
    uint16 data = 0x8001;
    uint32 data32;
    node.write_data_at(fp, &data, 0, sizeof(uint16));

    data = (uint16)besch.gewichtung;
    node.write_data_at(fp, &data, 2, sizeof(uint16));

    data = besch.intro_date;
    node.write_data_at(fp, &data, 4, sizeof(uint16));

    data = besch.obsolete_date;
    node.write_data_at(fp, &data, 6, sizeof(uint16));

    node.write(fp);
}

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
