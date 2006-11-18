#include <string.h>

#include "../text_besch.h"
#include "obj_node.h"

#include "text_writer.h"


void text_writer_t::write_obj(FILE *outfp, obj_node_t &parent, const char *text)
{
    if(!text) {
	text = "";
    }
    int len = strlen(text);

    obj_node_t	node(this, len + 1, &parent, false);

    node.write_data(outfp, text);
    node.write(outfp);
}


void text_writer_t::dump_node(FILE *infp, const obj_node_info_t &node)
{
    obj_writer_t::dump_node(infp, node);

    char *buf = new char[node.size];

    fread(buf, node.size, 1, infp);
    printf(" '%s'", buf);
    delete buf;
}
