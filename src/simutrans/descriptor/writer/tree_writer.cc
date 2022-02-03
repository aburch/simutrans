/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include <stdlib.h>
#include "../../dataobj/tabfile.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist2d_writer.h"
#include "get_climate.h"
#include "tree_writer.h"

using std::string;

void tree_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	obj_node_t node(this, 6, &parent);
	write_head(fp, node, obj);

	climate_bits allowed_climates;
	const char *climate_str = obj.get("climates");
	if (climate_str) {
		allowed_climates = get_climate_bits(climate_str);
	} else {
		printf("WARNING: old syntax without climates!\n");
		allowed_climates = all_but_arctic_climate;
	}
	// seasons = 1: no seasons
	// seasons = 2: 0=summer, 1=winter
	// seasons = 4, normal four seasons, starting with summer
	// seasons = 5, normal four seasons and snowy image
	uint8 const number_of_seasons   = obj.get_int("seasons", 1);
	uint8 const distribution_weight = obj.get_int("distributionweight", 3);

	slist_tpl<slist_tpl<string> > keys;
	for (unsigned int age = 0; age < 5; age++) {
		keys.append();

		for (int seasons = 0; seasons < number_of_seasons; seasons++) {
			char buf[40];

			// Images of the tree
			// age is 1..5 (usually five stages, seasons is the seaons
			sprintf(buf, "image[%d][%d]", age, seasons);

			string str = obj.get(buf);
			if (str.empty()) {
				// else missing image
				dbg->fatal( "Tree", "Missing %s!", buf);
			}
			keys.at(age).append(str);
		}
	}
	imagelist2d_writer_t::instance()->write_obj(fp, node, keys);

	// write version data
	node.write_uint16(fp, 0x8002,              0);
	node.write_uint16(fp, allowed_climates,    2);
	node.write_uint8( fp, distribution_weight, 4);
	node.write_uint8( fp, number_of_seasons,   5);

	node.write(fp);
}
