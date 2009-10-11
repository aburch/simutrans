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

	obj_node_t node(this, 7, &parent);

	besch.phasen = 0;
	for (int i = 0; i < seasons; i++) {
		slist_iterator_tpl<slist_tpl<cstring_t> > iter(backkeys.at(i));
		while (iter.next()) {
			if (iter.get_current().get_count() > besch.phasen) {
				besch.phasen = iter.get_current().get_count();
			}
		}
		iter = slist_iterator_tpl<slist_tpl<cstring_t> >(frontkeys.at(i));
		while (iter.next()) {
			if (iter.get_current().get_count() > besch.phasen) {
				besch.phasen = iter.get_current().get_count();
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

	// Set version data
	v16 = 0x8002;

	// This is the overlay flag for Simutrans-Experimental
	// This sets the *second* highest bit to 1. 
	v16 |= EXP_VER;

	// Finally, this is the experimental version number. This is *added*
	// to the standard version number, to be subtracted again when read.
	// Start at 0x100 and increment in hundreds (hex).
	v16 += 0x100;

	// Write version data
	node.write_uint16(fp, v16, 0);

	v16 = besch.phasen;
	node.write_uint16(fp, v16, 2);

	v16 = besch.index;
	node.write_uint16(fp, v16, 4);

	uint8 uv8 = besch.seasons;
	node.write_uint8(fp, uv8, 6);

	node.write(fp);
}


void building_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	haus_besch_t besch;

	// Hajo: take care, hardocded size of node on disc here!
	obj_node_t node(this, 36, &parent);

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
	besch.utype            = haus_besch_t::unbekannt;
	besch.extra_data          = 0;
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
		besch.extra_data = obj.get_int("build_time", 0);
		besch.level = obj.get_int("passengers",  besch.level);
		besch.utype = besch.extra_data == 0 ? haus_besch_t::attraction_land : haus_besch_t::attraction_city;
	} else if (!STRICMP(type_name, "mon")) {
		besch.utype = haus_besch_t::denkmal;
		besch.level = obj.get_int("passengers",  besch.level);
	} else if (!STRICMP(type_name, "tow")) {
		besch.level = obj.get_int("passengers",  besch.level);
		besch.extra_data = obj.get_int("build_time", 0);
		besch.utype = haus_besch_t::rathaus;
	} else if (!STRICMP(type_name, "hq")) {
		besch.level = obj.get_int("passengers",  besch.level);
		besch.extra_data = obj.get_int("hq_level", 0);
		besch.utype = haus_besch_t::firmensitz;
	} else if (!STRICMP(type_name, "habour")) {
		besch.utype = haus_besch_t::hafen;
		besch.extra_data = water_wt;
	} else if (!STRICMP(type_name, "fac")) {
		besch.utype = haus_besch_t::fabrik;
		besch.enables |= 4;
	} else if (!STRICMP(type_name, "stop")) {
		besch.utype = haus_besch_t::generic_stop;
		besch.extra_data = get_waytype(obj.get("waytype"));
	} else if (!STRICMP(type_name, "extension")) {
		besch.utype = haus_besch_t::generic_extension;
		const char *wt = obj.get("waytype");
		if(wt  &&  *wt>' ') {
			// not waytype => just a generic exten that fits all
			besch.extra_data = get_waytype(wt);
		}
	} else if (!STRICMP(type_name, "depot")) {
		besch.utype = haus_besch_t::depot;
		besch.extra_data = get_waytype(obj.get("waytype"));
	} else if (!STRICMP(type_name, "any") || *type_name == '\0') {
		// for instance "MonorailGround"
		besch.utype = haus_besch_t::weitere;
	} else if (
		!STRICMP(type_name, "station")  ||
		!STRICMP(type_name, "railstop")  ||
		!STRICMP(type_name, "monorailstop")  ||
		!STRICMP(type_name, "busstop")  ||
		!STRICMP(type_name, "carstop")  ||
		!STRICMP(type_name, "airport")  ||
		!STRICMP(type_name, "wharf")
	) {
		dbg->fatal("building_writer_t::write_obj()","%s is obsolete type for %s; use stop/extension and waytype!", type_name, obj.get("name") );
	} else if (!STRICMP(type_name, "hall")  ||  !STRICMP(type_name, "post")  ||  !STRICMP(type_name, "shed")  ) {
		dbg->fatal("building_writer_t::write_obj()","%s is obsolete type for %s; use extension and waytype!", type_name, obj.get("name") );
	} else {
		dbg->fatal( "building_writer_t::write_obj()","%s is obsolete type for %s", type_name, obj.get("name") );
	}

	// is is an station extension building?
	if (obj.get_int("extension_building", 0) > 0) {
		dbg->fatal("building_writer_t::write_obj()","extension_building is obsolete keyword for %s; use stop/extension and waytype!", obj.get("name") );
	}

	if (obj.get_int("enables_pax", 0) > 0) {
		besch.enables |= 1;
	}
	if (obj.get_int("enables_post", 0) > 0) {
		besch.enables |= 2;
	}
	if (besch.utype == haus_besch_t::fabrik || obj.get_int("enables_ware", 0) > 0) {
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

	// @author: jamespetts
	// Station-specific capacity and price information.
	// Stands in place of the "level" setting, but uses "level" data by default.

	besch.station_capacity = obj.get_int("station_capacity", besch.level * 32);
	besch.station_maintenance = obj.get_int("station_maintenance", 2147483647); //NOTE: Default cannot be set because it depends on a world factor. Set default in the *reader*.
	besch.station_price = obj.get_int("station_price", 2147483647);

	// scan for most number of seasons
	int seasons = 1;
	for (int l = 0; l < besch.layouts; l++) { // each layout
		for (int y = 0; y < besch.get_h(l); y++) {
			for (int x = 0; x < besch.get_b(l); x++) { // each tile
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
		for (int y = 0; y < besch.get_h(l); y++) {
			for (int x = 0; x < besch.get_b(l); x++) { // each tile
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
							if (keys.at(season).get_count() <= h) {
								break;
							}
						}
					}
				}
				tile_writer_t::instance()->write_obj(fp, node, tile_index++, seasons, backkeys, frontkeys);
			}
		}
	}

	// Hajo: write version data
	node.write_uint16(fp, 0x8005,                           0);

	// Hajo: write besch data
	node.write_uint8 (fp, (uint8) besch.gtyp,               2);
	node.write_uint8 (fp, (uint8) besch.utype,              3);
	node.write_uint16(fp, (uint16) besch.level,             4);
	node.write_uint32(fp, (uint32) besch.extra_data,        6);
	node.write_uint16(fp, besch.groesse.x,                 10);
	node.write_uint16(fp, besch.groesse.y,                 12);
	node.write_uint8 (fp, (uint8) besch.layouts,           14);
	node.write_uint16(fp, (uint16) besch.allowed_climates, 15);
	node.write_uint8 (fp, (uint8) besch.enables,           17);
	node.write_uint8 (fp, (uint8) besch.flags,             18);
	node.write_uint8 (fp, (uint8) besch.chance,            19);
	node.write_uint16(fp, besch.intro_date,                20);
	node.write_uint16(fp, besch.obsolete_date,             22);
	node.write_uint16(fp, besch.animation_time,            24);
	node.write_uint16(fp, besch.station_capacity,		   26);
	node.write_sint32(fp, besch.station_maintenance        28);
	node.write_sint32(fp, besch.station_price			   32);

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
