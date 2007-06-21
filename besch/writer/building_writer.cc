#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"
#include "../haus_besch.h"
#include "obj_pak_exception.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist2d_writer.h"
#include "get_waytype.h"
#include "get_climate.h"
#include "building_writer.h"
#include "skin_writer.h"


void tile_writer_t::write_obj(FILE* fp, obj_node_t& parent, int index, int seasons,
	slist_tpl<slist_tpl<slist_tpl<cstring_t> > >& backkeys,
	slist_tpl<slist_tpl<slist_tpl<cstring_t> > >& frontkeys
)
{
	haus_tile_besch_t besch;

	obj_node_t node(this, 7, &parent, false);

	besch.phasen = 0;
	for (int i = 0; i < seasons; i++) {
		slist_iterator_tpl<slist_tpl<cstring_t> > iter(backkeys.at(i));
		while (iter.next()) {
			if (iter.get_current().count() > besch.phasen) {
				besch.phasen = iter.get_current().count();
			}
		}
		iter = slist_iterator_tpl<slist_tpl<cstring_t> >(frontkeys.at(i));
		while (iter.next()) {
			if (iter.get_current().count() > besch.phasen) {
				besch.phasen = iter.get_current().count();
			}
		}
	}
	besch.index = index;
	besch.seasons = seasons;

	for (int i = 0; i < seasons; i++) {
		imagelist2d_writer_t::instance()->write_obj(fp, node, backkeys.at(i));
		imagelist2d_writer_t::instance()->write_obj(fp, node, frontkeys.at(i));
	}

	// Hajo: temp vars of appropriate size
	uint16 v16;

	// Hajo: write version data
	v16 = 0x8002;
	node.write_data_at(fp, &v16, 0, sizeof(uint16));

	v16 = besch.phasen;
	node.write_data_at(fp, &v16, 2, sizeof(uint16));

	v16 = besch.index;
	node.write_data_at(fp, &v16, 4, sizeof(uint16));

	uint8 uv8 = besch.seasons;
	node.write_data_at(fp, &uv8, 6, sizeof(uint8));

	node.write(fp);
}


void building_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	haus_besch_t besch;

	// Hajo: take care, hardocded size of node on disc here!
	obj_node_t node(this, 26, &parent, false);

	write_head(fp, node, obj);

	besch.groesse = koord(1, 1);
	besch.layouts = 0;

	int* ints = obj.get_ints("dims");

	switch (ints[0]) {
		default:
		case 3: besch.layouts   = ints[3]; /* FALLTHROUGH */
		case 2: besch.groesse.y = ints[2]; /* FALLTHROUGH */
		case 1: besch.groesse.x = ints[1]; /* FALLTHROUGH */
		case 0: break;
	}
	if (besch.layouts == 0) {
		besch.layouts = besch.groesse.x == besch.groesse.y ? 1 : 2;
	}
	delete [] ints;

	besch.gtyp             = gebaeude_t::unbekannt;
	besch.utyp             = hausbauer_t::unbekannt;
	besch.bauzeit          = 0;
	besch.allowed_climates = all_but_water_climate; // all but water
	besch.enables          = 0;
	besch.level            = obj.get_int("level", 1) - 1;
	besch.flags            = haus_besch_t::flag_t(
		(obj.get_int("noinfo",         0) > 0 ? haus_besch_t::FLAG_KEINE_INFO  : 0) |
		(obj.get_int("noconstruction", 0) > 0 ? haus_besch_t::FLAG_KEINE_GRUBE : 0) |
		(obj.get_int("needs_ground",   0) > 0 ? haus_besch_t::FLAG_NEED_GROUND : 0)
	);
	besch.animation_time = obj.get_int("animation_time", 300);

	// get the allowed area for this building
	const char* climate_str = obj.get("climates");
	if (climate_str && strlen(climate_str) > 4) {
		besch.allowed_climates = get_climate_bits(climate_str);
	}

	const char* type_name = obj.get("type");
	if (!STRICMP(type_name, "res")) {
		besch.gtyp = gebaeude_t::wohnung;
	} else if (!STRICMP(type_name, "com")) {
		besch.gtyp = gebaeude_t::gewerbe;
	} else if (!STRICMP(type_name, "ind")) {
		besch.gtyp = gebaeude_t::industrie;
	} else if (!STRICMP(type_name, "cur")) {
		besch.bauzeit = obj.get_int("build_time", 0);
		besch.level = obj.get_int("passengers",  besch.level);
		besch.utyp = besch.bauzeit == 0 ? hausbauer_t::sehenswuerdigkeit : hausbauer_t::special;
	} else if (!STRICMP(type_name, "mon")) {
		besch.utyp = hausbauer_t::denkmal;
		besch.level = obj.get_int("passengers",  besch.level);
	} else if (!STRICMP(type_name, "tow")) {
		besch.level = obj.get_int("passengers",  besch.level);
		besch.bauzeit = obj.get_int("build_time", 0);
		besch.utyp = hausbauer_t::rathaus;
	} else if (!STRICMP(type_name, "hq")) {
		besch.level = obj.get_int("passengers",  besch.level);
		besch.bauzeit = obj.get_int("build_time", 0);
		besch.utyp = hausbauer_t::firmensitz;
	} else if (!STRICMP(type_name, "station")) {
		besch.utyp = hausbauer_t::bahnhof;
	} else if (!STRICMP(type_name, "railstop")) {
		besch.utyp = hausbauer_t::bahnhof;
	} else if (!STRICMP(type_name, "monorailstop")) {
		besch.utyp = hausbauer_t::monorailstop;
	} else if (!STRICMP(type_name, "busstop")) {
		besch.utyp = hausbauer_t::bushalt;
	} else if (!STRICMP(type_name, "carstop")) {
		besch.utyp = hausbauer_t::ladebucht;
	} else if (!STRICMP(type_name, "habour")) {
		besch.utyp = hausbauer_t::hafen;
	} else if (!STRICMP(type_name, "wharf")) {
		besch.utyp = hausbauer_t::binnenhafen;
	} else if (!STRICMP(type_name, "airport")) {
		besch.utyp = hausbauer_t::airport;
	} else if (!STRICMP(type_name, "hall")) {
		besch.utyp = hausbauer_t::wartehalle;
	} else if (!STRICMP(type_name, "post")) {
		besch.utyp = hausbauer_t::post;
	} else if (!STRICMP(type_name, "shed")) {
		besch.utyp = hausbauer_t::lagerhalle;
	} else if (!STRICMP(type_name, "fac")) {
		besch.utyp = hausbauer_t::fabrik;
	} else if (!STRICMP(type_name, "any") || *type_name == '\0') {
		besch.utyp = hausbauer_t::weitere;
	} else {
		cstring_t reason;
		reason.printf("invalid type %s for building %s\n", type_name, obj.get("name"));
		throw obj_pak_exception_t("building_writer_t", reason);
	}

	// if it is a station (building), or a water factory
	if (besch.utyp >= hausbauer_t::bahnhof && besch.utyp <= hausbauer_t::monorailstop+1) {
		// is is an station extension building?
		if (obj.get_int("extension_building", 0) > 0) {
			besch.utyp = (enum hausbauer_t::utyp)(8 + (int)besch.utyp);
		}
	}

	if (obj.get_int("enables_pax", 0) > 0) {
		besch.enables |= 1;
	}
	if (obj.get_int("enables_post", 0) > 0) {
		besch.enables |= 2;
	}
	if (besch.utyp == hausbauer_t::fabrik || obj.get_int("enables_ware", 0) > 0) {
		besch.enables |= 4;
	}

	// some station thing ...
	if (besch.enables) {
		besch.level ++;
	}

	// Hajo: read chance - default is 100% chance to be built
	besch.chance = obj.get_int("chance", 100) ;

	// prissi: timeline for buildings
	besch.intro_date  = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	besch.intro_date += obj.get_int("intro_month", 1) - 1;

	besch.obsolete_date  = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	besch.obsolete_date += obj.get_int("retire_month", 1) - 1;

	// scan for most number of seasons
	int seasons = 1;
	for (int l = 0; l < besch.layouts; l++) { // each layout
		for (int y = 0; y < besch.gib_h(l); y++) {
			for (int x = 0; x < besch.gib_b(l); x++) { // each tile
				for (int pos = 0; pos < 2; pos++) {
					for (int season = seasons; season < 12; season++) {
						char buf[40];
						sprintf(buf, "%simage[%d][%d][%d][%d][%d][%d]", pos ? "back" : "front", l, y, x, 0, 0, season);
						cstring_t str = obj.get(buf);
						if (str.len() != 0) {
							seasons = season + 1;
						} else {
							break;
						}
					}
				}
			}
		}
	}

	int tile_index = 0;
	for (int l = 0; l < besch.layouts; l++) { // each layout
		for (int y = 0; y < besch.gib_h(l); y++) {
			for (int x = 0; x < besch.gib_b(l); x++) { // each tile
				slist_tpl<slist_tpl<slist_tpl<cstring_t> > > backkeys;
				slist_tpl<slist_tpl<slist_tpl<cstring_t> > > frontkeys;

				for (int season = 0; season < seasons; season++) {
					backkeys.append(slist_tpl<slist_tpl<cstring_t> >());
					frontkeys.append(slist_tpl<slist_tpl<cstring_t> >());

					for (int pos = 0; pos < 2; pos++) {
						slist_tpl<slist_tpl<slist_tpl<cstring_t> > >& keys = pos ? backkeys : frontkeys;

						for (unsigned int h = 0; ; h++) { // each height
							for (int phase = 0; ; phase++) { // each animation
								char buf[40];

								sprintf(buf, "%simage[%d][%d][%d][%d][%d][%d]", pos ? "back" : "front", l, y, x, h, phase, season);
								cstring_t str = obj.get(buf);

								// if no string check to see whether using format without seasons parameter
								if (str.len() == 0 && seasons == 1) {
									sprintf(buf, "%simage[%d][%d][%d][%d][%d]", pos ? "back" : "front", l, y, x, h, phase);
									str = obj.get(buf);
								}
								if (str.len() == 0) {
#if 0
									printf("Not found: %s\n", buf);
									fflush(NULL);
#endif
									break;
								} else {
									// no higher front images
									if (h > 0 && pos == 0) {
										printf("WARNING: frontimage height MUST be one tile only!\n");
										fflush(NULL);
										break;
									}
								}
								if (phase == 0) {
									keys.at(season).append(slist_tpl<cstring_t>());
								}
								keys.at(season).at(h).append(str);
							}
							if (keys.at(season).count() <= h) {
								break;
							}
						}
					}
				}
				tile_writer_t::instance()->write_obj(fp, node, tile_index++, seasons, backkeys, frontkeys);
			}
		}
	}

	// Hajo: old code
	// node.write_data(fp, &besch);

	// Hajo: temp vars of appropriate size
	uint32 v32;
	uint16 v16;
	uint8  v8;

	// Hajo: write version data
	v16 = 0x8005;
	node.write_data_at(fp, &v16, 0, sizeof(uint16));

	// Hajo: write besch data

	v8 = (uint8) besch.gtyp;
	node.write_data_at(fp, &v8, 2, sizeof(uint8));

	v8 = (uint8)besch.utyp;
	node.write_data_at(fp, &v8, 3, sizeof(uint8));

	v16 = (uint16)besch.level;
	node.write_data_at(fp, &v16, 4, sizeof(uint16));

	v32 = (uint32)besch.bauzeit;
	node.write_data_at(fp, &v32, 6, sizeof(uint32));

	v16 = besch.groesse.x;
	node.write_data_at(fp, &v16, 10, sizeof(uint16));

	v16 = besch.groesse.y;
	node.write_data_at(fp, &v16, 12, sizeof(uint16));

	v8 = (uint8)besch.layouts;
	node.write_data_at(fp, &v8, 14, sizeof(uint8));

	v16 = (uint16)besch.allowed_climates;
	node.write_data_at(fp, &v16, 15, sizeof(uint16));

	v8 = (uint8)besch.enables;
	node.write_data_at(fp, &v8, 17, sizeof(uint8));

	v8 = (uint8)besch.flags;
	node.write_data_at(fp, &v8, 18, sizeof(uint8));

	v8 = (uint8)besch.chance;
	node.write_data_at(fp, &v8, 19, sizeof(uint8));

	v16 = besch.intro_date;
	node.write_data_at(fp, &v16, 20, sizeof(uint16));

	v16 = besch.obsolete_date;
	node.write_data_at(fp, &v16, 22, sizeof(uint16));

	v16 = besch.animation_time;
	node.write_data_at(fp, &v16, 24, sizeof(uint16));

	// probably add some icons, if defined
	slist_tpl<cstring_t> cursorkeys;

	cstring_t c = cstring_t(obj.get("cursor"));
	cstring_t i = cstring_t(obj.get("icon"));
	cursorkeys.append(c);
	cursorkeys.append(i);
	if (c.len() > 0 || i.len() > 0) {
		cursorskin_writer_t::instance()->write_obj(fp, node, obj, cursorkeys);
	}
	node.write(fp);
}
