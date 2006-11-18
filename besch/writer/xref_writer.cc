#include <string.h>

#include "../text_besch.h"
#include "obj_node.h"

#include "xref_writer.h"


void xref_writer_t::write_obj(FILE *outfp, obj_node_t &parent, obj_type type, const char *text, bool fatal)
{
    if(!text) {
	text = "";
    }
    int len = strlen(text);

    obj_node_t	node(this,
	sizeof(char) +		     // Fatal-Flag
	sizeof(obj_type) +	     // type of dest node
	len + 1,		     // 0-terminated name of dest node
	&parent,
	false);

    char c = fatal ? 1 : 0;

    node.write_data_at(outfp, &type, 0, sizeof(obj_type));
    node.write_data_at(outfp, &c, sizeof(obj_type), sizeof(char));
    node.write_data_at(outfp, text, sizeof(obj_type) + sizeof(char), len + 1);
    node.write(outfp);
}


void xref_writer_t::dump_node(FILE *infp, const obj_node_info_t &node)
{
    obj_writer_t::dump_node(infp, node);

    char *buf = new char[node.size];

    fread(buf, node.size, 1, infp);
    printf(" -> %4.4s-node '%s'", buf, buf + 4);

    delete buf;
}
