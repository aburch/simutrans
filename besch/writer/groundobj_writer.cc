#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"
#include "../groundobj_besch.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist2d_writer.h"
#include "get_climate.h"
#include "get_waytype.h"
#include "groundobj_writer.h"


void groundobj_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	obj_node_t node(this, 16, &parent);
	write_head(fp, node, obj);

	// Hajodoc: Preferred height of this tree type
	// Hajoval: int (useful range: 0-14)
	groundobj_besch_t besch;
	const char *climate_str = obj.get("climates");
	if (climate_str) {
		besch.allowed_climates = get_climate_bits(climate_str);
	} else {
		printf("WARNING: without climates!\n");
		besch.allowed_climates = all_but_arctic_climate;
	}
	// seasons = 1: no seasons
	// otherwise the year will be devided by the (number_of_seasons-1)
	// The last image is alsway the snow image!
	besch.number_of_seasons = obj.get_int("seasons", 1);

	// distribution probabiltion for all of this set
	besch.distribution_weight = obj.get_int("distributionweight", 3);

	// how much for removal
	besch.cost_removal = obj.get_int("cost", 0);

	// !=0 for moving objects (sheeps, birds)
	besch.speed = obj.get_int("speed", 0);

	// 1 for to allow trees on this objects
	besch.trees_on_top = obj.get_int("trees_on_top", 1)!=0;

	// waytype for moving stuff; meaningful air for birds, water for fish, does not matter for everything else
	const char* waytype = obj.get("waytype");
	besch.waytype = (waytype==0  ||  waytype[0]==0) ? ignore_wt : (waytype_t)get_waytype(waytype);

	// now for the images
	slist_tpl<slist_tpl<cstring_t> > keys;
	if(besch.speed==0) {
		// fixed stuff
		for (unsigned int phase = 0; 1; phase++) {
			keys.append(slist_tpl<cstring_t>());

			for (int seasons = 0; seasons < besch.number_of_seasons; seasons++) {
				char buf[40];

				// Images of the tree
				// age is 1..5 (usually five stages, seasons is the seaons
				sprintf(buf, "image[%d][%d]", phase, seasons);
				cstring_t str = obj.get(buf);
				if (str.len() == 0) {
					if(seasons==0) {
						goto finish_images;
					}
					else {
						dbg->fatal("groundobj_writer_t","Season image for season %i missing!",seasons);
					}
				}
				keys.at(phase).append(str);
			}
		}
	}
	else {
		// moving stuff
		const char* const dir_codes[] = {
			"s", "w", "sw", "se", "n", "e", "ne", "nw"
		};
		for (unsigned int dir = 0; dir<8; dir++) {
			keys.append(slist_tpl<cstring_t>());

			for (int seasons = 0; seasons < besch.number_of_seasons; seasons++) {
				char buf[40];

				// Images of the tree
				// age is 1..5 (usually five stages, seasons is the seaons
				sprintf(buf, "image[%s][%d]", dir_codes[dir], seasons);
				cstring_t str = obj.get(buf);
				if (str.len() == 0) {
					printf("Missing images in moving groundobj (expected %s)!\n", buf );
					dbg->fatal("groundobj_writer_t","Season image for season %i missing (%s)!",seasons);
				}
				keys.at(dir).append(str);
			}
		}
	}
finish_images:
	imagelist2d_writer_t::instance()->write_obj(fp, node, keys);

	// Hajo: write version data
	node.write_uint16(fp, 0x8001,                              0);

	node.write_uint16(fp, (uint16) besch.allowed_climates,     2);
	node.write_uint16(fp, (uint16) besch.distribution_weight,  4);
	node.write_uint8 (fp, (uint8)  besch.number_of_seasons,    6);
	node.write_uint8 (fp, (uint8)  besch.trees_on_top,         7);
	node.write_uint16(fp, (uint16) besch.speed,                8);
	node.write_uint16(fp, (uint16) besch.waytype,             10);
	node.write_sint32(fp, (sint32) besch.cost_removal,        12);

	node.write(fp);
}
