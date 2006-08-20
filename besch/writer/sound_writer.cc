#include "../../dataobj/tabfile.h"

#include "obj_node.h"
#include "../sound_besch.h"
#include "text_writer.h"

#include "sound_writer.h"


void sound_writer_t::write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj)
{
    obj_node_t	node(this, 4, &parent, true);

    write_head(fp, node, obj);

    // Hajo: version number
    // Hajo: Version needs high bit set as trigger -> this is required
    //       as marker because formerly nodes were unversionend
    uint16 uv16 = 0x8001;
    node.write_data_at(fp, &uv16, 0, sizeof(uint16));

    uv16 = obj.get_int("sound_nr", NO_SOUND);	// for compatibility reasons; the old nr of a sound
    node.write_data_at(fp, &uv16, 2, sizeof(uint16));

    node.write(fp);
}
