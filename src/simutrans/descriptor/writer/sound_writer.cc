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

	write_name_and_copyright(fp, node, obj);

	// Version needs high bit set as trigger -> this is required
	// as marker because formerly nodes were unversioned
	node.write_version(fp, 2);

	// for compatibility reasons; the old nr of a sound
	node.write_uint16(fp, (uint16)obj.get_int("sound_nr", NO_SOUND));

	node.write_uint16(fp, len);
	node.write_bytes(fp, len + 1, str.c_str());

	node.check_and_write_header(fp);
}
