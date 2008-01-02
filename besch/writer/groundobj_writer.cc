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
	obj_node_t node(this, 16, &parent, false);	/* false, because we write this ourselves */
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
finish_images:
	imagelist2d_writer_t::instance()->write_obj(fp, node, keys);

	// Hajo: temp vars of appropriate size
	sint32 s32;
	uint16 v16;
	uint8 v8;

	// Hajo: write version data
	v16 = 0x8001;
	node.write_data_at(fp, &v16, 0, sizeof(uint16));

	v16 = (uint16) besch.allowed_climates;
	node.write_data_at(fp, &v16, 2, sizeof(uint16));

	v16 = (uint16)besch.distribution_weight;
	node.write_data_at(fp, &v16, 4, sizeof(uint16));

	v8 = (uint8) besch.number_of_seasons;
	node.write_data_at(fp, &v8, 6, sizeof(uint8));

	v8 = (uint8) besch.trees_on_top;
	node.write_data_at(fp, &v8, 7, sizeof(uint8));

	v16 = (uint16) besch.speed;
	node.write_data_at(fp, &v16, 8, sizeof(uint16));

	v16 = (uint16)besch.waytype;
	node.write_data_at(fp, &v16, 10, sizeof(uint16));

	s32 = (sint32)besch.cost_removal;
	node.write_data_at(fp, &s32, 12, sizeof(sint32));

	node.write(fp);
}
