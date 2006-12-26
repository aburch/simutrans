#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"
#include "obj_node.h"
#include "obj_pak_exception.h"
#include "../bruecke_besch.h"
#include "text_writer.h"
#include "imagelist_writer.h"
#include "skin_writer.h"
#include "get_waytype.h"
#include "bridge_writer.h"


void bridge_writer_t::write_obj(FILE* outfp, obj_node_t& parent, tabfileobj_t& obj)
{
	obj_node_t node(this, 20, &parent, false);

	uint8  wegtyp        = get_waytype(obj.get("waytype"));
	uint16 topspeed      = obj.get_int("topspeed", 999);
	uint32 preis         = obj.get_int("cost", 0);
	uint32 maintenance   = obj.get_int("maintenance", 1000);
	uint8  pillars_every = obj.get_int("pillar_distance",0); // distance==0 is off
	uint8  max_lenght    = obj.get_int("max_lenght",0); // max_lenght==0: unlimited

	// prissi: timeline
	uint16 intro_date = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro_date += obj.get_int("intro_month", 1) - 1;

	uint16 obsolete_date = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	obsolete_date += obj.get_int("retire_month", 1) - 1;

	sint8 number_seasons = 0;

	// Hajo: Version needs high bit set as trigger -> this is required
	//       as marker because formerly nodes were unversionend
	uint16 version = 0x8006;
	node.write_data_at(outfp, &version,        0, 2);
	node.write_data_at(outfp, &topspeed,       2, 2);
	node.write_data_at(outfp, &preis,          4, 4);
	node.write_data_at(outfp, &maintenance,    8, 4);
	node.write_data_at(outfp, &wegtyp,        12, 1);
	node.write_data_at(outfp, &pillars_every, 13, 1);
	node.write_data_at(outfp, &max_lenght,    14, 1);
	node.write_data_at(outfp, &intro_date,    15, sizeof(uint16));
	node.write_data_at(outfp, &obsolete_date, 17, sizeof(uint16));

	static const char* const names[] = {
		"image",
		"ns", "ew", NULL,
		"start",
		"n", "s", "e", "w", NULL,
		"ramp",
		"n", "s", "e", "w", NULL,
		NULL
	};
	slist_tpl<cstring_t> backkeys;
	slist_tpl<cstring_t> frontkeys;

	slist_tpl<cstring_t> cursorkeys;
	cursorkeys.append(cstring_t(obj.get("cursor")));
	cursorkeys.append(cstring_t(obj.get("icon")));

	char keybuf[40];

	cstring_t str = obj.get("backimage[ns][0]");
	if(strlen(str) == 0) {
		node.write_data_at(outfp, &number_seasons, 19, sizeof(uint8));
		const char* const * ptr = names;
		const char* keyname = *ptr++;

		do {
			const char* keyindex = *ptr++;
			do {
				cstring_t value;

				sprintf(keybuf, "back%s[%s]", keyname, keyindex);
				value = obj.get(keybuf);
				backkeys.append(value);
				//intf("BACK: %s -> %s\n", keybuf, value.chars());
				sprintf(keybuf, "front%s[%s]", keyname, keyindex);
				value = obj.get(keybuf);
				if (value.len() > 2) {
					frontkeys.append(value);
					//intf("FRNT: %s -> %s\n", keybuf, value.chars());
				} else {
					printf("WARNING: not %s specified (but might be still working)\n",keybuf);
				}
				keyindex = *ptr++;
			} while (keyindex);
			keyname = *ptr++;
		} while (keyname);

		if (pillars_every > 0) {
			backkeys.append(cstring_t(obj.get("backpillar[s]")));
			backkeys.append(cstring_t(obj.get("backpillar[w]")));
		}
		imagelist_writer_t::instance()->write_obj(outfp, node, backkeys);
		imagelist_writer_t::instance()->write_obj(outfp, node, frontkeys);
		cursorskin_writer_t::instance()->write_obj(outfp, node, obj, cursorkeys);
		backkeys.clear();
		frontkeys.clear();
	} else {
		while(number_seasons < 2) {
			sprintf(keybuf, "backimage[ns][%d]", number_seasons+1);
			cstring_t str = obj.get(keybuf);
			if(strlen(str) > 0) {
				number_seasons++;
			} else {
				break;
			}
		}

		node.write_data_at(outfp, &number_seasons, 19, sizeof(uint8));

		for(uint8 season = 0 ; season <= number_seasons ; season++) {
			const char* const * ptr = names;
			const char* keyname = *ptr++;

			do {
				const char* keyindex = *ptr++;
				do {
					cstring_t value;

					sprintf(keybuf, "back%s[%s][%d]", keyname, keyindex, season);
					value = obj.get(keybuf);
					backkeys.append(value);
					//intf("BACK: %s -> %s\n", keybuf, value.chars());
					sprintf(keybuf, "front%s[%s][%d]", keyname, keyindex, season);
					value = obj.get(keybuf);
					if (value.len() > 2) {
						frontkeys.append(value);
						//intf("FRNT: %s -> %s\n", keybuf, value.chars());
					} else {
						printf("WARNING: not %s specified (but might be still working)\n",keybuf);
					}
					keyindex = *ptr++;
				} while (keyindex);
				keyname = *ptr++;
			} while (keyname);

			if (pillars_every > 0) {
				sprintf(keybuf, "backpillar[s][%d]",season);
				backkeys.append(cstring_t(obj.get(keybuf)));
				sprintf(keybuf, "backpillar[w][%d]",season);
				backkeys.append(cstring_t(obj.get(keybuf)));
			}
			imagelist_writer_t::instance()->write_obj(outfp, node, backkeys);
			imagelist_writer_t::instance()->write_obj(outfp, node, frontkeys);
			if(season == 0 ) {
				cursorskin_writer_t::instance()->write_obj(outfp, node, obj, cursorkeys);
			}
			backkeys.clear();
			frontkeys.clear();
		}
	}

	cursorkeys.clear();

	// node.write_data(outfp, &besch);
	node.write(outfp);
}
