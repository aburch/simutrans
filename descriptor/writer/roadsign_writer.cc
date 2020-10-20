/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include "../../dataobj/tabfile.h"
#include "../roadsign_desc.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist_writer.h"
#include "roadsign_writer.h"
#include "get_waytype.h"
#include "skin_writer.h"
#include "cluster_writer.h"
#include "../../utils/simstring.h"

using std::string;

void roadsign_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	obj_node_t node(this, 45, &parent);

	uint32                 const price      = obj.get_int("cost",      500) * 100;
	uint16                 const min_speed = obj.get_int("min_speed",   0);
	sint8                  const offset_left = obj.get_int("offset_left", 14);
	uint8                  const wtyp = get_waytype(obj.get("waytype"));

	roadsign_desc_t::types const flags     =
		(obj.get_int("single_way",         0) > 0 ? roadsign_desc_t::ONE_WAY               : roadsign_desc_t::NONE) |
		(obj.get_int("free_route",         0) > 0 ? roadsign_desc_t::CHOOSE_SIGN           : roadsign_desc_t::NONE) |
		(obj.get_int("is_private",         0) > 0 ? roadsign_desc_t::PRIVATE_ROAD          : roadsign_desc_t::NONE) |
		(obj.get_int("is_signal",          0) > 0 ? roadsign_desc_t::SIGN_SIGNAL           : roadsign_desc_t::NONE) |
		(obj.get_int("is_presignal",       0) > 0 ? roadsign_desc_t::SIGN_PRE_SIGNAL       : roadsign_desc_t::NONE) |
		(obj.get_int("no_foreground",      0) > 0 ? roadsign_desc_t::ONLY_BACKIMAGE        : roadsign_desc_t::NONE) |
		(obj.get_int("is_longblocksignal", 0) > 0 ? roadsign_desc_t::SIGN_LONGBLOCK_SIGNAL : roadsign_desc_t::NONE) |
		(obj.get_int("end_of_choose",      0) > 0 ? roadsign_desc_t::END_OF_CHOOSE_AREA    : roadsign_desc_t::NONE);

	uint8 allow_underground = obj.get_int("allow_underground", 2);

	if(allow_underground > 2)
	{
		// Prohibit illegal values here.
		allow_underground = 2;
	}

	uint32 signal_group = cluster_writer_t::get_cluster_data(obj, "signal_groups");
	uint32 maintenance = obj.get_int("maintenance", 0);
	uint32 max_distance_to_signalbox = obj.get_int("max_distance_to_signalbox", 1000);
	uint8 aspects = obj.get_int("aspects", 2);
	sint8 has_call_on = obj.get_int("has_call_on", 0);
	sint8 has_selective_choose = obj.get_int("has_selective_choose", 0);
	sint8 permissive = obj.get_int("permissive", 0);
	uint32 max_speed_kmh = obj.get_int("max_speed", 160);
	const char* working_method_string = obj.get_string("working_method", "track_circuit_block");

	working_method_t working_method = track_circuit_block;

	if(!STRICMP(working_method_string, "drive_by_sight"))
	{
		working_method = drive_by_sight;
	}

	if(!STRICMP(working_method_string, "time_interval"))
	{
		working_method = time_interval;
	}

	if(!STRICMP(working_method_string, "time_interval_with_telegraph"))
	{
		working_method = time_interval_with_telegraph;
	}

	if(!STRICMP(working_method_string, "absolute_block"))
	{
		working_method = absolute_block;
	}

	if(!STRICMP(working_method_string, "token_block"))
	{
		working_method = token_block;
	}

	if(!STRICMP(working_method_string, "cab_signalling"))
	{
		working_method = cab_signalling;
	}

	if(!STRICMP(working_method_string, "moving_block"))
	{
		working_method = moving_block;
	}

	if(!STRICMP(working_method_string, "one_train_staff"))
	{
		working_method = one_train_staff;
	}

	uint32 signal_upgrade_cost = obj.get_int("signal_upgrade_cost", price);
	uint8 upgrade_group = obj.get_int("upgrade_group", 0);
	uint8 intermediate_block = obj.get_int("intermediate_block", 0);
	uint8 normal_danger = obj.get_int("normal_danger", 0);
	uint8 double_block = obj.get_int("double_block", 0);

	uint16 version = 0x8004; // version 4

	// This is the overlay flag for Simutrans-Extended
	// This sets the *second* highest bit to 1.
	version |= EX_VER;

	// Finally, this is the extended version number. This is *added*
	// to the standard version number, to be subtracted again when read.
	// Start at 0x100 and increment in hundreds (hex).
	// 0x100: Allow underground
	// 0x200: New signalling system (major version 12)
	// 0x300: Double block signals
	version += 0x300;

	// write version data
	node.write_uint16(fp, version,     0);
	node.write_uint16(fp, min_speed,   2);
	node.write_uint32(fp, price,       4);
	node.write_uint8 (fp, flags,       8);
	node.write_uint8 (fp, offset_left, 9);
	node.write_uint8 (fp, wtyp,        10);

	uint16 intro_date = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro_date += obj.get_int("intro_month", 1) - 1;
	node.write_uint16(fp, intro_date,           11);

	uint16 retire_date = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	retire_date += obj.get_int("retire_month", 1) - 1;
	node.write_uint16(fp, retire_date,          13);

	node.write_uint8(fp, allow_underground, 15);
	node.write_uint32(fp, signal_group, 16);
	node.write_uint32(fp, maintenance, 20);
	node.write_uint32(fp, max_distance_to_signalbox, 24);
	node.write_uint8(fp, aspects, 28);
	node.write_sint8(fp, has_call_on, 29);
	node.write_sint8(fp, has_selective_choose, 30);
	node.write_uint8(fp, working_method, 31);
	node.write_sint8(fp, permissive, 32);
	node.write_uint32(fp, max_speed_kmh, 33);
	node.write_uint32(fp, signal_upgrade_cost, 37);
	node.write_uint8(fp, upgrade_group, 41);
	node.write_uint8(fp, intermediate_block, 42);
	node.write_uint8(fp, normal_danger, 43);
	node.write_uint8(fp, double_block, 44);

	write_head(fp, node, obj);

	// add the images
	slist_tpl<string> keys;
	string str;

	for (int i = 0; i < 48; i++) {
		char buf[40];

		sprintf(buf, "image[%i]", i);
		str = obj.get(buf);
		// make sure, there are always 4, 8, 12, ... images (for all directions)
		if (str.empty() && i % 4 == 0) {
			break;
		}
		keys.append(str);
	}
	imagelist_writer_t::instance()->write_obj(fp, node, keys);

	// probably add some icons, if defined
	slist_tpl<string> cursorkeys;

	string c = string(obj.get("cursor")), i=string(obj.get("icon"));
	cursorkeys.append(c);
	cursorkeys.append(i);
	if (!c.empty() || !i.empty()) {
		cursorskin_writer_t::instance()->write_obj(fp, node, obj, cursorkeys);
	}

	node.write(fp);
}
