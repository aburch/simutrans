#include "../../utils/simstring.h"
#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"
#include "../sound_besch.h"
#include "obj_node.h"
#include "obj_pak_exception.h"
#include "text_writer.h"
#include "image_writer.h"
#include "get_waytype.h"
#include "imagelist_writer.h"
#include "crossing_writer.h"


void crossing_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	int total_len = 17;

	// prissi: must be done here, since it may affect the len of the header!
	cstring_t sound_str = ltrim( obj.get("sound") );
	sint8 sound_id=NO_SOUND;
	if (sound_str.len() > 0) {
		// ok, there is some sound
		sound_id = atoi(sound_str);
		if (sound_id == 0 && sound_str[0] == '0') {
			sound_id = 0;
			sound_str = "";
		} else if (sound_id != 0) {
			// old style id
			sound_str = "";
		}
		if (sound_str.len() > 0) {
			sound_id = LOAD_SOUND;
			total_len += sound_str.len() + 1;
		}
	}

	// ok, node can be allocated now
	obj_node_t	node(this, total_len, &parent, false);
	write_head(fp, node, obj);

	// Hajo: version number
	// Hajo: Version needs high bit set as trigger -> this is required
	//       as marker because formerly nodes were unversionend
	uint16 uv16 = 0x8001;
	node.write_data_at(fp, &uv16, 0, sizeof(uint16));

	// waytypes, waytype 2 will be on top
	uint8 wegtyp1 = get_waytype(obj.get("waytype[0]"));
	uint8 wegtyp2 = get_waytype(obj.get("waytype[1]"));
	if(wegtyp1==wegtyp2) {
		printf("*** FATAL ***:\nIdentical ways cannot cross (check waytypes)!\n");
		exit(0);
	}
	node.write_data_at(fp, &wegtyp1, 2, sizeof(uint8));
	node.write_data_at(fp, &wegtyp2, 3, sizeof(uint8));

	// Top speed of this way
	uv16 = obj.get_int("speed[0]", 0);
	node.write_data_at(fp, &uv16, 4, sizeof(uint16));
	uv16 = obj.get_int("speed[1]", 0);
	node.write_data_at(fp, &uv16, 6, sizeof(uint16));

	// time between frames for animation
	uint32 uv32 = obj.get_int("animation_time_open", 0);
	node.write_data_at(fp, &uv32, 8, sizeof(sint32));
	uv32 = obj.get_int("animation_time_closed", 0);
	node.write_data_at(fp, &uv32, 12, sizeof(sint32));

	node.write_data_at(fp, &sound_id, 16, sizeof(uint8));

	if(sound_str.len() > 0) {
		sint8 sv8 = sound_str.len();
		node.write_data_at(fp, &sv8, 17, sizeof(sint8));
		node.write_data_at(fp, sound_str, 18, sound_str.len());
	}

	// now the image stuff
	slist_tpl<cstring_t> openkeys_ns;
	slist_tpl<cstring_t> openkeys_ew;
	slist_tpl<cstring_t> closekeys_ns;
	slist_tpl<cstring_t> closekeys_ew;

	int i=0;
	cstring_t str;

	// open crossings ...
	while(1) {
		char buf[40];

		sprintf(buf, "openimage[ns][%s]", i);
		str = obj.get(buf);
		if (str.len() <= 0) {
			break;
		}
		// ok, we have this direction
		openkeys_ns.append(str);
		// MUST exist then also for the other direction
		sprintf(buf, "openimage[ew][%s]", i);
		str = obj.get(buf);
		openkeys_ew.append(str);
	}
	if(openkeys_ns.count()!=openkeys_ew.count()  &&  openkeys_ns.count()>0) {
		printf("*** FATAL ***:\nMissing images (must be same number for both open directions, and at least one! (but %i and %i found)!)\n", openkeys_ns.count(), openkeys_ew.count());
		exit(0);
	}
	imagelist_writer_t::instance()->write_obj(fp, node, openkeys_ns);
	imagelist_writer_t::instance()->write_obj(fp, node, openkeys_ew);

	// closed crossings ...
	while(1) {
		char buf[40];

		sprintf(buf, "closedimage[ns][%s]", i);
		str = obj.get(buf);
		if (str.len() <= 0) {
			break;
		}
		// ok, we have this direction
		closekeys_ns.append(str);
		// MUST exist then also for the other direction
		sprintf(buf, "closedimage[ew][%s]", i);
		str = obj.get(buf);
		if (str.len() > 0) {
			closekeys_ew.append(str);
		}
	}
	if(closekeys_ns.count()!=closekeys_ew.count()) {
		printf("*** FATAL ***:\nMissing images (must be same number for both closed directions (but %i and %i found)!)\n", closekeys_ns.count(), closekeys_ew.count());
		exit(0);
	}
	imagelist_writer_t::instance()->write_obj(fp, node, closekeys_ns);
	imagelist_writer_t::instance()->write_obj(fp, node, closekeys_ew);

	node.write(fp);
}
