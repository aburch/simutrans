
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

    obj_node_t	node(this, sizeof(besch), &parent, true);

    besch.gewichtung = obj.get_int("distributionweight", 1);

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

    node.write_data(fp, &besch);
    node.write(fp);
}

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
