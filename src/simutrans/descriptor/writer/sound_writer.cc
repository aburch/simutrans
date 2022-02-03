/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include "../../dataobj/tabfile.h"
#include "obj_node.h"
#include "../sound_desc.h"
#include "text_writer.h"
#include "sound_writer.h"


void sound_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	// eventual direct name input
	std::string str = obj.get("sound_name");
	uint16 len = str.size();

	obj_node_t node(this, 6+len+1, &parent);

	write_head(fp, node, obj);

	// Version needs high bit set as trigger -> this is required
	// as marker because formerly nodes were unversioned
	uint16 uv16 = 0x8002;
	node.write_uint16(fp, uv16, 0);

	uv16 = obj.get_int("sound_nr", NO_SOUND); // for compatibility reasons; the old nr of a sound
	node.write_uint16(fp, uv16, 2);

	node.write_uint16(fp, len, 4);
	node.write_data_at(fp, str.c_str(), 6, len + 1);

	node.write(fp);
}
