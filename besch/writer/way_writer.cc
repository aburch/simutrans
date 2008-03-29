#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"
#include "obj_node.h"
#include "obj_pak_exception.h"
#include "../weg_besch.h"
#include "text_writer.h"
#include "imagelist_writer.h"
#include "skin_writer.h"
#include "get_waytype.h"
#include "way_writer.h"


/**
 * Write a waytype description node
 */
void way_writer_t::write_obj(FILE* outfp, obj_node_t& parent, tabfileobj_t& obj)
{
	static char* const ribi_codes[16] = {
		"-", "n",  "e",  "ne",  "s",  "ns",  "se",  "nse",
		"w", "nw", "ew", "new", "sw", "nsw", "sew", "nsew"
	};
	int ribi, hang;

	// Hajo: node size is 25 bytes
	obj_node_t node(this, 26, &parent, false);


	// Hajo: Version needs high bit set as trigger -> this is required
	//       as marker because formerly nodes were unversionend
	uint16 version     = 0x8004;
	uint32 price       = obj.get_int("cost",        100);
	uint32 maintenance = obj.get_int("maintenance", 100);
	uint32 topspeed    = obj.get_int("topspeed",    999);
	uint32 max_weight  = obj.get_int("max_weight",  999);

	uint16 intro  = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro += obj.get_int("intro_month", 1) - 1;

	uint16 retire  = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	retire += obj.get_int("retire_month", 1) - 1;

	uint8 wtyp = get_waytype(obj.get("waytype"));
	uint8 styp = obj.get_int("system_type", 0);
	// compatibility conversions
	if (wtyp == track_wt && styp == 5) {
		wtyp = monorail_wt;
	} else if (wtyp == track_wt && styp == 7) {
		wtyp = tram_wt;
	}

	// true to draw as foregrund and not much earlier (default)
	uint8 draw_as_ding = (obj.get_int("draw_as_ding", 0) == 1);
	sint8 number_seasons = 0;

	node.write_data_at(outfp, &version,       0, 2);
	node.write_data_at(outfp, &price,         2, 4);
	node.write_data_at(outfp, &maintenance,   6, 4);
	node.write_data_at(outfp, &topspeed,     10, 4);
	node.write_data_at(outfp, &max_weight,   14, 4);
	node.write_data_at(outfp, &intro,        18, 2);
	node.write_data_at(outfp, &retire,       20, 2);
	node.write_data_at(outfp, &wtyp,         22, 1);
	node.write_data_at(outfp, &styp,         23, 1);
	node.write_data_at(outfp, &draw_as_ding, 24, 1);

	slist_tpl<cstring_t> keys;
	char buf[40];
	sprintf(buf, "image[%s][0]", ribi_codes[0]);
	cstring_t str = obj.get(buf);
	if (strlen(str) == 0) {
		node.write_data_at(outfp, &number_seasons, 25, 1);
		write_head(outfp, node, obj);

		sprintf(buf, "image[%s]", ribi_codes[0]);
		cstring_t str = obj.get(buf);
		if(strlen(str) > 0) {
			// way images defined without seasons
			for (ribi = 0; ribi < 16; ribi++) {
				char buf[40];

				sprintf(buf, "image[%s]", ribi_codes[ribi]);
				cstring_t str = obj.get(buf);
				keys.append(str);
			}
			imagelist_writer_t::instance()->write_obj(outfp, node, keys);

			keys.clear();
			for (hang = 3; hang <= 12; hang += 3) {
				char buf[40];

				sprintf(buf, "imageup[%d]", hang);
				cstring_t str = obj.get(buf);
				keys.append(str);
			}
			imagelist_writer_t::instance()->write_obj(outfp, node, keys);

			keys.clear();
			for (ribi = 3; ribi <= 12; ribi += 3) {
				char buf[40];

				sprintf(buf, "diagonal[%s]", ribi_codes[ribi]);
				cstring_t str = obj.get(buf);
				keys.append(str);
			}
			imagelist_writer_t::instance()->write_obj(outfp, node, keys);
			keys.clear();


			slist_tpl<cstring_t> cursorkeys;

			cursorkeys.append(cstring_t(obj.get("cursor")));
			cursorkeys.append(cstring_t(obj.get("icon")));

			cursorskin_writer_t::instance()->write_obj(outfp, node, obj, cursorkeys);

			// skip new write code
			number_seasons = -1;
		} else {
			// no images - should complain probably...
		}
	} else {
		while(number_seasons < 2) {
			sprintf(buf, "image[%s][%d]", ribi_codes[0], number_seasons+1);
			cstring_t str = obj.get(buf);
			if(strlen(str) > 0) {
				number_seasons++;
			} else {
				break;
			}
		}
		node.write_data_at(outfp, &number_seasons, 25, 1);
		write_head(outfp, node, obj);

		for (uint8 season = 0; season <= number_seasons ; season++) {
			for (ribi = 0; ribi < 16; ribi++) {
				char buf[40];

				sprintf(buf, "image[%s][%d]", ribi_codes[ribi], season);
				cstring_t str = obj.get(buf);
				keys.append(str);
			}
			imagelist_writer_t::instance()->write_obj(outfp, node, keys);

			keys.clear();
			for (hang = 3; hang <= 12; hang += 3) {
				char buf[40];

				sprintf(buf, "imageup[%d][%d]", hang, season);
				cstring_t str = obj.get(buf);
				keys.append(str);
			}
			imagelist_writer_t::instance()->write_obj(outfp, node, keys);

			keys.clear();
			for (ribi = 3; ribi <= 12; ribi += 3) {
				char buf[40];

				sprintf(buf, "diagonal[%s][%d]", ribi_codes[ribi], season);
				cstring_t str = obj.get(buf);
				keys.append(str);
			}
			imagelist_writer_t::instance()->write_obj(outfp, node, keys);

			keys.clear();
			if(season == 0) {
				slist_tpl<cstring_t> cursorkeys;

				cursorkeys.append(cstring_t(obj.get("cursor")));
				cursorkeys.append(cstring_t(obj.get("icon")));

				cursorskin_writer_t::instance()->write_obj(outfp, node, obj, cursorkeys);
			}
		}
	}

	node.write(outfp);
}
