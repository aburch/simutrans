#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"
#include "obj_node.h"
#include "obj_pak_exception.h"
#include "../kreuzung_besch.h"
#include "text_writer.h"
#include "image_writer.h"
#include "get_waytype.h"
#include "crossing_writer.h"


void crossing_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	kreuzung_besch_t besch;

	obj_node_t node(this, 4, &parent, false);

	write_head(fp, node, obj);

	const char* waytype = obj.get("waytype[ns]");
	besch.wegtyp_ns = get_waytype(obj.get("waytype[ns]"));
	besch.wegtyp_ow = get_waytype(obj.get("waytype[ew]"));

	if (besch.wegtyp_ns == 0 || besch.wegtyp_ow == 0) {
		cstring_t reason;
		reason.printf("invalid waytype for crossing %s\n", obj.get("name"));
		throw new obj_pak_exception_t("crossing_writer_t", reason);
	}
	image_writer_t::instance()->write_obj(fp, node, obj.get("image"));

	uint16 v16 = besch.wegtyp_ns;
	node.write_data_at(fp, &v16, 0, sizeof(uint16));
	v16 = besch.wegtyp_ow;
	node.write_data_at(fp, &v16, 2, sizeof(uint16));
    node.write(fp);
}
