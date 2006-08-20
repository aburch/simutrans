#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"
#include "../../dataobj/ribi.h"

#include "../tunnel_besch.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist_writer.h"
#include "skin_writer.h"

#include "tunnel_writer.h"


void tunnel_writer_t::write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj)
{
    tunnel_besch_t besch;
    int pos, i;

    obj_node_t	node(this, sizeof(besch), &parent, true);

    write_head(fp, node, obj);

    static const char * indices[] = {
	"n", "s", "e", "w"
    };
    slist_tpl<cstring_t> backkeys;
    slist_tpl<cstring_t> frontkeys;

    for(pos = 0; pos < 2; pos++) {
	for(i = 0; i < 4; i++) {
	    char buf[40];

	    sprintf(buf, "%simage[%s]", pos ? "back" : "front", indices[i]);
	    cstring_t str = obj.get(buf);
	    (pos ? &backkeys : &frontkeys)->append(str);
	}
    }

    slist_tpl<cstring_t> cursorkeys;
    cursorkeys.append(cstring_t(obj.get("cursor")));
    cursorkeys.append(cstring_t(obj.get("icon")));

    imagelist_writer_t::instance()->write_obj(fp, node, backkeys);
    imagelist_writer_t::instance()->write_obj(fp, node, frontkeys);
    cursorskin_writer_t::instance()->write_obj(fp, node, obj, cursorkeys);

    cursorkeys.clear();
    backkeys.clear();
    frontkeys.clear();

    node.write_data(fp, &besch);
    node.write(fp);
}
