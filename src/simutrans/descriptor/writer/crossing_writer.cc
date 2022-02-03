/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include <stdio.h>
#include <stdlib.h>

#include "../../utils/simstring.h"
#include "../../dataobj/tabfile.h"
#include "../sound_desc.h"
#include "obj_node.h"
#include "obj_pak_exception.h"
#include "text_writer.h"
#include "image_writer.h"
#include "get_waytype.h"
#include "imagelist_writer.h"
#include "crossing_writer.h"
#include "xref_writer.h"

using std::string;

static void make_list(tabfileobj_t &obj, slist_tpl<string>& list, char const* const key)
{
	for (int i = 0;; ++i) {
		char buf[40];
		sprintf(buf, "%s[%i]", key, i);
		string str(obj.get(buf));
		if (str.empty()) {
			break;
		}
		// We have this direction
		list.append(str);
	}
}

static void write_list(FILE* const fp, obj_node_t& node, slist_tpl<string> const& list)
{
	if (list.empty()) {
		xref_writer_t::instance()->write_obj(fp, node, obj_imagelist, "", false);
	} else {
		imagelist_writer_t::instance()->write_obj(fp, node, list);
	}
}

void crossing_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	int total_len = 21;

	// must be done here, since it may affect the len of the header!
	string sound_str = ltrim( obj.get("sound") );
	sint8 sound_id=NO_SOUND;
	if (!sound_str.empty()) {
		// ok, there is some sound
		sound_id = atoi(sound_str.c_str());
		if (sound_id == 0 && sound_str[0] == '0') {
			sound_id = 0;
			sound_str = "";
		}
		else if (sound_id != 0) {
			// old style id
			sound_str = "";
		}
		if (!sound_str.empty()) {
			sound_id = LOAD_SOUND;
			total_len += sound_str.size() + 1;
		}
	}

	// ok, node can be allocated now
	obj_node_t node(this, total_len, &parent);
	write_head(fp, node, obj);

	// Version needs high bit set as trigger -> this is required
	// as marker because formerly nodes were unversionend
	uint16 uv16 = 0x8002;
	node.write_uint16(fp, uv16, 0);

	// waytypes, waytype 2 will be on top
	uint8 waytype1 = get_waytype(obj.get("waytype[0]"));
	uint8 waytype2 = get_waytype(obj.get("waytype[1]"));
	if(waytype1==waytype2) {
		dbg->fatal( "Crossing", "Identical ways (%s) cannot cross (check waytypes)!", obj.get("waytype[0]") );
	}
	node.write_uint8(fp, waytype1, 2);
	node.write_uint8(fp, waytype2, 3);

	// Top speed of this way
	uv16 = obj.get_int("speed[0]", 0);
	if(uv16==0) {
		dbg->fatal( "Crossing", "A maxspeed MUST be given for both ways!");
	}
	node.write_uint16(fp, uv16, 4);
	uv16 = obj.get_int("speed[1]", 0);
	if(uv16==0) {
		dbg->fatal( "Crossing", "A maxspeed MUST be given for both ways!");
	}
	node.write_uint16(fp, uv16, 6);

	// time between frames for animation
	uint32 uv32 = obj.get_int("animation_time_open", 0);
	node.write_uint32(fp, uv32, 8);
	uv32 = obj.get_int("animation_time_closed", 0);
	node.write_uint32(fp, uv32, 12);

	node.write_uint8(fp, sound_id, 16);
	uint8 index = 17;

	if (!sound_str.empty()) {
		sint8 sv8 = sound_str.size();
		node.write_data_at(fp, &sv8, 17, sizeof(sint8));
		node.write_data_at(fp, sound_str.c_str(), 18, sound_str.size());
		index += 1 + sound_str.size();
	}

	uint16 intro_date  = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro_date += obj.get_int("intro_month", 1) - 1;

	uint16 retire_date  = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	retire_date += obj.get_int("retire_month", 1) - 1;

	node.write_uint16(fp, intro_date, index);
	index += 2;
	node.write_uint16(fp, retire_date, index);
	index += 2;

	// now the image stuff
	slist_tpl<string> openkeys_ns;
	slist_tpl<string> openkeys_ew;
	slist_tpl<string> front_openkeys_ns;
	slist_tpl<string> front_openkeys_ew;
	slist_tpl<string> closekeys_ns;
	slist_tpl<string> closekeys_ew;
	slist_tpl<string> front_closekeys_ns;
	slist_tpl<string> front_closekeys_ew;

	// open crossings ...
	make_list(obj, openkeys_ns, "openimage[ns]");
	make_list(obj, openkeys_ew, "openimage[ew]");
	// these must exists!
	if (openkeys_ns.empty() || openkeys_ew.empty()) {
		dbg->fatal( "Crossing", "Missing images (at least one openimage! (but %i and %i found)!)", openkeys_ns.get_count(), openkeys_ew.get_count() );
	}
	write_list(fp, node, openkeys_ns);
	write_list(fp, node, openkeys_ew);

	// foreground
	make_list(obj, front_openkeys_ns, "front_openimage[ns]");
	make_list(obj, front_openkeys_ew, "front_openimage[ew]");
	// the following lists are optional
	write_list(fp, node, front_openkeys_ns);
	write_list(fp, node, front_openkeys_ew);

	// closed crossings ...
	make_list(obj, closekeys_ns, "closedimage[ns]");
	make_list(obj, closekeys_ew, "closedimage[ew]");
	write_list(fp, node, closekeys_ns);
	write_list(fp, node, closekeys_ew);

	// foreground
	make_list(obj, front_closekeys_ns, "front_closedimage[ns]");
	make_list(obj, front_closekeys_ew, "front_closedimage[ew]");
	write_list(fp, node, front_closekeys_ns);
	write_list(fp, node, front_closekeys_ew);

	node.write(fp);
}
