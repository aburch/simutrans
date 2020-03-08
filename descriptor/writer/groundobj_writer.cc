/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include "../../dataobj/tabfile.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist2d_writer.h"
#include "get_climate.h"
#include "get_waytype.h"
#include "groundobj_writer.h"

using std::string;

void groundobj_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	obj_node_t node(this, 16, &parent);
	write_head(fp, node, obj);

	climate_bits allowed_climates;
	const char *climate_str = obj.get("climates");
	if (climate_str) {
		allowed_climates = get_climate_bits(climate_str);
	}
	else {
		dbg->warning( obj_writer_t::last_name, "No climates (using default)!");
		allowed_climates = all_but_arctic_climate;
	}
	// seasons = 1: no seasons
	// otherwise the year will be divided by the (number_of_seasons-1)
	// The last image is always the snow image!
	uint8 const number_of_seasons = obj.get_int("seasons", 1);

	// distribution probability for all of this set
	uint16 const distribution_weight = obj.get_int("distributionweight", 3);

	// how much for removal
	sint32 const price = obj.get_int("cost", 0);

	// !=0 for moving objects (sheeps, birds)
	uint16 const speed = obj.get_int("speed", 0);

	// 1 for to allow trees on this objects
	bool const trees_on_top = obj.get_int("trees_on_top", 1) != 0;

	// waytype for moving stuff; meaningful air for birds, water for fish, does not matter for everything else
	char const* const waytype_txt = obj.get("waytype");
	waytype_t   const waytype     = waytype_txt && waytype_txt[0] != '\0' ? get_waytype(waytype_txt) : ignore_wt;

	// now for the images
	slist_tpl<slist_tpl<string> > keys;
	if (speed == 0) {
		// fixed stuff
		for (unsigned int phase = 0; 1; phase++) {
			keys.append();

			for (int seasons = 0; seasons < number_of_seasons; seasons++) {
				char buf[40];

				// Images of the tree
				// age is 1..5 (usually five stages, seasons is the seasons
				sprintf(buf, "image[%d][%d]", phase, seasons);
				string str = obj.get(buf);
				if (str.empty()) {
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
			keys.append();

			for (int seasons = 0; seasons < number_of_seasons; seasons++) {
				char buf[40];

				// Images of the tree
				// age is 1..5 (usually five stages, seasons is the seasons
				sprintf(buf, "image[%s][%d]", dir_codes[dir], seasons);
				string str = obj.get(buf);
				if(  str.empty()  ) {
					dbg->fatal("groundobj_writer_t","Season image for season %i missing (expected %s)!", seasons, buf );
				}
				keys.at(dir).append(str);
			}
		}
	}
finish_images:
	imagelist2d_writer_t::instance()->write_obj(fp, node, keys);

	// write version data
	node.write_uint16(fp, 0x8001,               0);
	node.write_uint16(fp, allowed_climates,     2);
	node.write_uint16(fp, distribution_weight,  4);
	node.write_uint8 (fp, number_of_seasons,    6);
	node.write_uint8 (fp, trees_on_top,         7);
	node.write_uint16(fp, speed,                8);
	node.write_uint16(fp, waytype,             10);
	node.write_sint32(fp, price,        12);

	node.write(fp);
}
