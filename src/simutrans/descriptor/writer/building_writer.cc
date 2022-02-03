/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include "../../utils/simstring.h"
#include "../../dataobj/tabfile.h"
#include "../building_desc.h"
#include "obj_pak_exception.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist2d_writer.h"
#include "get_waytype.h"
#include "get_climate.h"
#include "building_writer.h"
#include "skin_writer.h"

using std::string;

void tile_writer_t::write_obj(FILE* fp, obj_node_t& parent, int index, int seasons,
	slist_tpl<slist_tpl<slist_tpl<string> > >& backkeys,
	slist_tpl<slist_tpl<slist_tpl<string> > >& frontkeys
)
{
	obj_node_t node(this, 7, &parent);

	uint8 phases = 0;
	for (int i = 0; i < seasons; i++) {
		for(slist_tpl<string>  const& s : backkeys.at(i)) {
			if (phases < s.get_count()) {
				phases = s.get_count();
			}
		}
		for(slist_tpl<string>  const& s : frontkeys.at(i)) {
			if (phases < s.get_count()) {
				phases = s.get_count();
			}
		}
	}

	for (int i = 0; i < seasons; i++) {
		imagelist2d_writer_t::instance()->write_obj(fp, node, backkeys.at(i));
		imagelist2d_writer_t::instance()->write_obj(fp, node, frontkeys.at(i));
	}

	// write version data
	uint16 v16 = 0x8002;
	node.write_uint16(fp, v16, 0);

	v16 = phases;
	node.write_uint16(fp, v16, 2);

	v16 = index;
	node.write_uint16(fp, v16, 4);

	uint8 uv8 = seasons;
	node.write_uint8(fp, uv8, 6);

	node.write(fp);
}


// Subroutine for write_obj, to avoid duplicated code
static uint32 get_cluster_data(tabfileobj_t& obj)
{
	uint32 clusters = 0;
	vector_tpl<int> ints = obj.get_ints("clusters");

	for(  uint32 i = 0;  i < ints.get_count();  i++  ) {
		if(  ints[i] >= 1  &&  ints[i] <= 32  ) { // Sanity check
			clusters |= 1<<(ints[i]-1);
		}
	}
	return clusters;
}


void building_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	// take care, hardcoded size of node on disc here!
	obj_node_t node(this, 39, &parent);

	write_head(fp, node, obj);

	koord size(1, 1);
	uint8 layouts = 0;

	vector_tpl<int> ints = obj.get_ints("dims");

	switch (ints.get_count()) {
		default:
		case 3: layouts = ints[2]; /* FALLTHROUGH */
		case 2: size.y  = ints[1]; /* FALLTHROUGH */
		case 1: size.x  = ints[0]; /* FALLTHROUGH */
		case 0: break;
	}
	if (layouts == 0) {
		layouts = size.x == size.y ? 1 : 2;
	}
	if (size.x*size.y == 0) {
		dbg->fatal("building_writer_t::write_obj", "Cannot create a building with zero size (%i,%i)", size.x, size.y);
	}

	building_desc_t::btype     type             = building_desc_t::unknown;
	uint32                     extra_data       = 0;
	uint8                      enables          = 0;
	uint16                     level            = obj.get_int("level", 1) - 1;

	// get the allowed area for this building
	climate_bits allowed_climates = all_but_water_climate; // all but water
	const char* climate_str = obj.get("climates");
	if (climate_str && strlen(climate_str) > 4) {
		allowed_climates = get_climate_bits(climate_str);
	}

	building_desc_t::flag_t const flags =
		(obj.get_int("noinfo",         0) > 0 ? building_desc_t::FLAG_NO_INFO  : building_desc_t::FLAG_NULL) |
		(obj.get_int("noconstruction", 0) > 0 ? building_desc_t::FLAG_NO_PIT : building_desc_t::FLAG_NULL) |
		(obj.get_int("needs_ground",   0) > 0 ? building_desc_t::FLAG_NEED_GROUND : building_desc_t::FLAG_NULL);

	uint16 const animation_time = obj.get_int("animation_time", 300);

	const char* type_name = obj.get("type");
	if (!STRICMP(type_name, "res")) {
		extra_data = get_cluster_data(obj);
		type = building_desc_t::city_res;
	}
	else if (!STRICMP(type_name, "com")) {
		extra_data = get_cluster_data(obj);
		type = building_desc_t::city_com;
	}
	else if (!STRICMP(type_name, "ind")) {
		extra_data = get_cluster_data(obj);
		type = building_desc_t::city_ind;
	}
	else if (!STRICMP(type_name, "cur")) {
		extra_data = obj.get_int("build_time", 0);
		level      = obj.get_int("passengers",  level);
		type       = extra_data == 0 ? building_desc_t::attraction_land : building_desc_t::attraction_city;
	}
	else if (!STRICMP(type_name, "mon")) {
		type = building_desc_t::monument;
		level = obj.get_int("passengers",  level);
	}
	else if (!STRICMP(type_name, "tow")) {
		level      = obj.get_int("passengers",  level);
		extra_data = obj.get_int("build_time", 0);
		type = building_desc_t::townhall;
	}
	else if (!STRICMP(type_name, "hq")) {
		level      = obj.get_int("passengers",  level);
		extra_data = obj.get_int("hq_level", 0);
		type = building_desc_t::headquarters;
	}
	else if (!STRICMP(type_name, "habour")  ||  !STRICMP(type_name, "harbour")) {
		// buildable only on sloped shores
		type       = building_desc_t::dock;
		extra_data = water_wt;
	}
	else if (!STRICMP(type_name, "dock")) {
		// buildable only on flat shores
		type       = building_desc_t::flat_dock;
		extra_data = water_wt;
	}
	else if (!STRICMP(type_name, "fac")) {
		type     = building_desc_t::factory;
		enables |= 4;
	}
	else if (!STRICMP(type_name, "stop")) {
		type       = building_desc_t::generic_stop;
		extra_data = get_waytype(obj.get("waytype"));
	}
	else if (!STRICMP(type_name, "extension")) {
		type = building_desc_t::generic_extension;
		const char *wt = obj.get("waytype");
		if(wt  &&  *wt>' ') {
			// no waytype => just a generic extension that fits all
			extra_data = get_waytype(wt);
		}
	}
	else if (!STRICMP(type_name, "depot")) {
		type       = building_desc_t::depot;
		extra_data = get_waytype(obj.get("waytype"));
	}
	else if (!STRICMP(type_name, "any") || *type_name == '\0') {
		// for instance "MonorailGround"
		type = building_desc_t::others;
	}
	else if (
		!STRICMP(type_name, "station")  ||
		!STRICMP(type_name, "railstop")  ||
		!STRICMP(type_name, "monorailstop")  ||
		!STRICMP(type_name, "busstop")  ||
		!STRICMP(type_name, "carstop")  ||
		!STRICMP(type_name, "airport")  ||
		!STRICMP(type_name, "wharf")
	) {
		dbg->fatal("building_writer_t::write_obj()","%s is obsolete type for %s; use stop/extension and waytype!", type_name, obj.get("name") );
	}
	else if (!STRICMP(type_name, "hall")  ||  !STRICMP(type_name, "post")  ||  !STRICMP(type_name, "shed")  ) {
		dbg->fatal("building_writer_t::write_obj()","%s is obsolete type for %s; use extension and waytype!", type_name, obj.get("name") );
	}
	else {
		dbg->fatal( "building_writer_t::write_obj()","%s is obsolete type for %s", type_name, obj.get("name") );
	}

	// is is an station extension building?
	if (obj.get_int("extension_building", 0) > 0) {
		dbg->fatal("building_writer_t::write_obj()","extension_building is obsolete keyword for %s; use stop/extension and waytype!", obj.get("name") );
	}

	if (obj.get_int("enables_pax", 0) > 0) {
		enables |= 1;
	}
	if (obj.get_int("enables_post", 0) > 0) {
		enables |= 2;
	}
	if(  type  == building_desc_t::factory  ||  obj.get_int("enables_ware", 0) > 0  ) {
		enables |= 4;
	}

	if(  type == building_desc_t::generic_extension  ||  type == building_desc_t::generic_stop  ||  type == building_desc_t::dock  ||  type == building_desc_t::depot  ||  type == building_desc_t::factory  ) {
		// since level was reduced by one beforehand ...
		++level;
	}

	// read chance - default is 100% chance to be built
	uint8 const chance = obj.get_int("chance", 100);

	// timeline for buildings
	uint16 const intro_date =
		obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12 +
		obj.get_int("intro_month", 1) - 1;

	uint16 const retire_date =
		obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12 +
		obj.get_int("retire_month", 1) - 1;

	uint16 const preservation_date =
		obj.get_int("preservation_year", DEFAULT_RETIRE_DATE) * 12 +
		obj.get_int("preservation_month", 1) - 1;

	// capacity and price information.
	// Stands in place of the "level" setting, but uses "level" data by default.

	//NOTE: Default for maintenance and price must be set when loading so use magic default here
	//also check for "station_xx" for experimental compatibility

	sint32 capacity = obj.get_int("capacity", level * 32);
	if(  capacity == level * 32  ) {
		capacity = obj.get_int("station_capacity", level * 32);
	}

	sint32 maintenance = obj.get_int("maintenance", PRICE_MAGIC);
	if(  maintenance == PRICE_MAGIC  ) {
		maintenance = obj.get_int("station_maintenance", PRICE_MAGIC);
	}

	sint32 price = obj.get_int("cost", PRICE_MAGIC);
	if(  price == PRICE_MAGIC  ) {
		price = obj.get_int("station_price", PRICE_MAGIC);
	}

	uint8 allow_underground = obj.get_int("allow_underground", 2);

	if(allow_underground > 2) {
		// Prohibit illegal values here.
		allow_underground = 2;
	}

	// scan for most number of seasons
	int seasons = 1;
	for (int l = 0; l < layouts; l++) { // each layout
		int const h = l & 1 ? size.x : size.y;
		int const w = l & 1 ? size.y : size.x;
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) { // each tile
				for (int pos = 0; pos < 2; pos++) {
					for (int season = seasons; season < 12; season++) {
						char buf[40];
						sprintf(buf, "%simage[%d][%d][%d][%d][%d][%d]", pos ? "back" : "front", l, y, x, 0, 0, season);
						string str = obj.get(buf);
						if (str.size() != 0) {
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
	for (int l = 0; l < layouts; l++) { // each layout
		int const h = l & 1 ? size.x : size.y;
		int const w = l & 1 ? size.y : size.x;
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) { // each tile
				slist_tpl<slist_tpl<slist_tpl<string> > > backkeys;
				slist_tpl<slist_tpl<slist_tpl<string> > > frontkeys;

				for (int season = 0; season < seasons; season++) {
					backkeys.append();
					frontkeys.append();

					for (int pos = 0; pos < 2; pos++) {
						slist_tpl<slist_tpl<slist_tpl<string> > >& keys = pos ? backkeys : frontkeys;

						for (unsigned int h = 0; ; h++) { // each height
							for (int phase = 0; ; phase++) { // each animation
								char buf[40];

								sprintf(buf, "%simage[%d][%d][%d][%d][%d][%d]", pos ? "back" : "front", l, y, x, h, phase, season);
								string str = obj.get(buf);

								// if no string check to see whether using format without seasons parameter
								if (str.empty() && seasons == 1) {
									sprintf(buf, "%simage[%d][%d][%d][%d][%d]", pos ? "back" : "front", l, y, x, h, phase);
									str = obj.get(buf);
								}
								if (str.empty()) {
#if 0
									printf("Not found: %s\n", buf);
									fflush(NULL);
#endif
									break;
								}
								else {
									// no higher front images
									if (h > 0 && pos == 0) {
										dbg->error( obj_writer_t::last_name, "Frontimage height MUST be one tile only!");
										break;
									}
								}
								if (phase == 0) {
									keys.at(season).append();
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

	// write version data
	node.write_uint16(fp, 0x800A,            0);

	// write desc data
	node.write_uint8 (fp, 0,                 2); // was gtyp
	node.write_uint8 (fp, type,              3);
	node.write_uint16(fp, level,             4);
	node.write_uint32(fp, extra_data,        6);
	node.write_uint16(fp, size.x,           10);
	node.write_uint16(fp, size.y,           12);
	node.write_uint8 (fp, layouts,          14);
	node.write_uint16(fp, allowed_climates, 15);
	node.write_uint8 (fp, enables,          17);
	node.write_uint8 (fp, flags,            18);
	node.write_uint8 (fp, chance,           19);
	node.write_uint16(fp, intro_date,       20);
	node.write_uint16(fp, retire_date,    22);
	node.write_uint16(fp, animation_time,   24);
	node.write_uint16(fp, capacity,         26);
	node.write_sint32(fp, maintenance,      28);
	node.write_sint32(fp, price,            32);
	node.write_uint8 (fp, allow_underground,36);
	node.write_uint16(fp, preservation_date,37);

	// probably add some icons, if defined
	slist_tpl<string> cursorkeys;

	string c = string(obj.get("cursor"));
	string i = string(obj.get("icon"));
	cursorkeys.append(c);
	cursorkeys.append(i);
	if (!c.empty() || !i.empty()) {
		cursorskin_writer_t::instance()->write_obj(fp, node, obj, cursorkeys);
	}
	node.write(fp);
}
