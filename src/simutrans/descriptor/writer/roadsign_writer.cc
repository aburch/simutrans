/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include <vector>
#include "../../dataobj/tabfile.h"
#include "../roadsign_desc.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist_writer.h"
#include "roadsign_writer.h"
#include "get_waytype.h"
#include "skin_writer.h"


static const char* private_sign_directions[] = {"ns", "ew"};
static const char* traffic_light_directions[] = {"n", "s", "e", "w", "ne", "sw", "se", "nw"};
static const char* general_sign_directions[] = {"n", "s", "e", "w"};

// parse "image[direction][state]" syntax
void parse_images_2d(slist_tpl<std::string>& keys, tabfileobj_t& obj, roadsign_desc_t::types flags)
{
	const char** directions;
	uint8 dir_cnt; // how many directions are there?
	if(  flags&roadsign_desc_t::PRIVATE_ROAD  ) {
		directions = private_sign_directions;
		dir_cnt = lengthof(private_sign_directions);
	}
	else if(  *obj.get("image[ne][0]")  ) {
		// Assume this is a traffic light.
		directions = traffic_light_directions;
		dir_cnt = lengthof(traffic_light_directions);
	}
	else {
		// Normal road sign or railway signal
		directions = general_sign_directions;
		dir_cnt = lengthof(general_sign_directions);
	}

	for(  uint8 state=0;  state<8;  state++  ) {
		for(  uint8 idx = 0;  idx < dir_cnt;  idx++  ) {
			char buf[64];
			sprintf(buf, "image[%s][%i]", directions[idx], state);
			const char* img = obj.get(buf);
			if(  !*img  ){
				if(  state>(dir_cnt==2)  &&  idx==0  ) {
					// Assume all further state numbers are invalid.
					return;
				}
				// image in the middle is missing => fatal error
				dbg->fatal("roadsign_writer", "%s is missing!", buf);
			}
			// append image number
			keys.append(img);
		}
	}
}


// parse "image[number]" syntax
void parse_images_numbered(slist_tpl<std::string>& keys, tabfileobj_t& obj)
{
	for (int i = 0; i < 32; i++) {
		char buf[40];
		sprintf(buf, "image[%i]", i);
		const char *str = obj.get(buf);
		// make sure, there are always 4, 8, 12, ... images (for all directions)
		if(  !*str  ) {
			if(  i % 4  ) {
				dbg->fatal("roadsign_writer", "image count is %d but must be multiple of 4!", i);
			}
			break;
		}
		keys.append(str);
	}
}

void roadsign_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	obj_node_t node(this, 16, &parent);

	uint32                 const price       = obj.get_int("cost",        500) * 100;
	uint16                 const min_speed   = obj.get_int("min_speed",     0);
	sint8                  const offset_left = obj.get_int("offset_left",  14);
	uint8                  const wtyp        = get_waytype(obj.get("waytype"));
	roadsign_desc_t::types       flags       = roadsign_desc_t::NONE;

	if(  obj.get_int("is_signal",0)   ) {
		flags = roadsign_desc_t::SIGN_SIGNAL;
		if(  obj.get_int("free_route",0)  ) {
			flags |= roadsign_desc_t::CHOOSE_SIGN;
		}
	}
	else if(  obj.get_int("is_presignal",0)   ) {
		flags = roadsign_desc_t::SIGN_PRE_SIGNAL;
	}
	else if(  obj.get_int("is_prioritysignal",0)   ) {
		flags = roadsign_desc_t::SIGN_PRIORITY_SIGNAL;
	}
	else if(  obj.get_int("is_longblocksignal",0)   ) {
		flags = roadsign_desc_t::SIGN_LONGBLOCK_SIGNAL;
	}
	else {
		// road or airsigns ...
		flags =
			(obj.get_int("single_way",         0) > 0 ? roadsign_desc_t::ONE_WAY               : roadsign_desc_t::NONE) |
			(obj.get_int("free_route",         0) > 0 ? roadsign_desc_t::CHOOSE_SIGN           : roadsign_desc_t::NONE) |
			(obj.get_int("is_private",         0) > 0 ? roadsign_desc_t::PRIVATE_ROAD          : roadsign_desc_t::NONE) |
			(obj.get_int("no_foreground",      0) > 0 ? roadsign_desc_t::ONLY_BACKIMAGE        : roadsign_desc_t::NONE) |
			(obj.get_int("end_of_choose",      0) > 0 ? roadsign_desc_t::END_OF_CHOOSE_AREA    : roadsign_desc_t::NONE);
	}
	// this causes unused entries to give a warning that they are ignored

	// write version data
	node.write_uint16(fp, 0x8005,      0); // version 5
	node.write_uint16(fp, min_speed,   2);
	node.write_uint32(fp, price,       4);
	node.write_uint16(fp, flags,       8);
	node.write_uint8 (fp, offset_left,10);
	node.write_uint8 (fp, wtyp,       11);

	uint16 intro_date = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro_date += obj.get_int("intro_month", 1) - 1;
	node.write_uint16(fp, intro_date, 12);

	uint16 retire_date = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	retire_date += obj.get_int("retire_month", 1) - 1;
	node.write_uint16(fp, retire_date, 14);

	write_head(fp, node, obj);

	// add the images
	slist_tpl<std::string> keys;

	if(  *obj.get("image[0]")  ) {
		// image[0] is defined.
		// assume that images are defined in image[number] syntax.
		parse_images_numbered(keys, obj);
	}
	else {
		// image[0] is not defined.
		// assume that images are defined in image[direction][state] syntax.
		parse_images_2d(keys, obj, flags);
	}
	imagelist_writer_t::instance()->write_obj(fp, node, keys);

	// probably add some icons, if defined
	slist_tpl<std::string> cursorkeys;

	const char *c = obj.get("cursor"), *i=obj.get("icon");
	cursorkeys.append(c);
	cursorkeys.append(i);
	if (*c || *i) {
		cursorskin_writer_t::instance()->write_obj(fp, node, obj, cursorkeys);
	}

	node.write(fp);
}
