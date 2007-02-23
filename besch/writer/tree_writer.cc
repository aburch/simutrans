#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"
#include "../baum_besch.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist2d_writer.h"
#include "get_climate.h"
#include "tree_writer.h"


void tree_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	obj_node_t node(this, 6, &parent, false);	/* false, because we write this ourselves */
	write_head(fp, node, obj);

	// Hajodoc: Preferred height of this tree type
	// Hajoval: int (useful range: 0-14)
	baum_besch_t besch;
	const char *climate_str = obj.get("climates");
	if (climate_str) {
		besch.allowed_climates = get_climate_bits(climate_str);
	} else {
		printf("WARNING: old syntax without climates!\n");
		besch.allowed_climates = all_but_arctic_climate;
	}
	// seasons = 1: no seasons
	// seasons = 2: 0=summer, 1=winter
	// seasons = 4, normal four seasons, starting with summer
	// seasons = 5, normal for seasons and snowy image
	besch.number_of_seasons = obj.get_int("seasons", 1);
	besch.distribution_weight = obj.get_int("distributionweight", 3);

	slist_tpl<slist_tpl<cstring_t> > keys;
	for (unsigned int age = 0; age < 5; age++) {
		keys.append(slist_tpl<cstring_t>());

		for (int seasons = 0; seasons < besch.number_of_seasons; seasons++) {
			char buf[40];

			// Images of the tree
			// age is 1..5 (usually five stages, seasons is the seaons
			sprintf(buf, "image[%d][%d]", age, seasons);

			cstring_t str = obj.get(buf);
			if (str.len() == 0) {
				// else missing image
				printf("*** FATAL ***:\nMissing %s!\n", buf); fflush(NULL);
				exit(0);
			}
			keys.at(age).append(str);
		}
	}
	imagelist2d_writer_t::instance()->write_obj(fp, node, keys);

	// Hajo: temp vars of appropriate size
	uint16 v16;
	uint8 v8;

	// Hajo: write version data
	v16 = 0x8002;
	node.write_data_at(fp, &v16, 0, sizeof(uint16));

	v16 = (uint16) besch.allowed_climates;
	node.write_data_at(fp, &v16, 2, sizeof(uint16));

	v8 = (uint8)besch.distribution_weight;
	node.write_data_at(fp, &v8, 4, sizeof(uint8));

	v8 = (uint8) besch.number_of_seasons;
	node.write_data_at(fp, &v8, 5, sizeof(uint8));

	node.write(fp);
}
