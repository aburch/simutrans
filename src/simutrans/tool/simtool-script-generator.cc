
#include "simtool.h"

#include "../tpl/vector_tpl.h"
#include "../simhalt.h"

#include "../dataobj/environment.h"
#include "../dataobj/koord3d.h"

#include "../descriptor/building_desc.h"
#include "../descriptor/ground_desc.h"

#include "../gui/messagebox.h"
#include "../gui/script_generator_frame.h"
#include "../gui/simwin.h"

#include "../obj/wayobj.h"
#include "../obj/bruecke.h"
#include "../obj/depot.h"
#include "../obj/gebaeude.h"
#include "../obj/signal.h"
#include "../obj/zeiger.h"

#include "../sys/simsys.h"

#include "../world/simcity.h"
#include "../world/simworld.h"
#include "../world/simplan.h"


void tool_generate_script_t::mark_tiles(player_t*, const koord3d& start, const koord3d& end)
{
	koord k1, k2;
	k1.x = start.x < end.x ? start.x : end.x;
	k1.y = start.y < end.y ? start.y : end.y;
	k2.x = start.x + end.x - k1.x;
	k2.y = start.y + end.y - k1.y;
	koord k;
	for (k.x = k1.x; k.x <= k2.x; k.x++) {
		for (k.y = k1.y; k.y <= k2.y; k.y++) {
			if (planquadrat_t* plan = welt->access(k.x, k.y)) {
				for (uint8 i = 0; i < plan->get_boden_count(); i++) {
					if (grund_t* gr = plan->get_boden_bei(i)) {
						if (gr->ist_karten_boden() || gr->get_pos().z > plan->get_boden_bei(0)->get_pos().z) {
							zeiger_t* marker = new zeiger_t(gr->get_pos(), NULL);
							const uint8 grund_hang = gr->get_grund_hang();
#if 0
							// this would use the way slope, not the ground slope
							const uint8 weg_hang = gr->get_weg_hang();
							const uint8 hang = max(corner_sw(grund_hang), corner_sw(weg_hang)) +
								3 * max(corner_se(grund_hang), corner_se(weg_hang)) +
								9 * max(corner_ne(grund_hang), corner_ne(weg_hang)) +
								27 * max(corner_nw(grund_hang), corner_nw(weg_hang));
							uint8 back_hang = (hang % 3) + 3 * ((uint8)(hang / 9)) + 27;
#else
							uint8 back_hang = (grund_hang % 3) + 3 * ((uint8)(grund_hang / 9)) + 27;
#endif
							marker->set_foreground_image(ground_desc_t::marker->get_image(grund_hang % 27));
							marker->set_image(ground_desc_t::marker->get_image(back_hang));
							marker->mark_image_dirty(marker->get_image(), 0);
							gr->obj_add(marker);
							marked.insert(marker);
						}
					}
				}
			}
		}
	}
}


static void write_owner_at(player_t* pl, cbuffer_t& buf, const koord3d pos, const koord3d origin)
{
	if (pl->is_public_service()) {
		if (const grund_t* gr = world()->lookup(pos)) {
			if (obj_t* obj = gr->obj_bei(0)) {
				if (obj->get_owner() != pl) {
					koord3d diff = pos - origin;
					buf.printf("\thm_set_owner_tl(%hu,[%d,%d,%d])\n", obj->get_owner_nr(), diff.x, diff.y, diff.z);
				}
			}
		}
	}
}


static void write_city_at(player_t* pl, cbuffer_t& buf, const koord3d pos, const koord3d origin)
{
	if (pl->is_public_service()) {
		if (const grund_t* gr = world()->lookup(pos)) {
			if (const gebaeude_t* gb = gr->find<gebaeude_t>()) {
				sint16 rotation = gb->get_tile()->get_layout();
				if (gb->get_tile()->get_offset() == koord(0, 0)) {
					if (gb->is_townhall()) {
						koord3d diff = pos - origin;
						buf.printf("\thm_city_set_population_tl(%ld,[%d,%d,%d])\n", gb->get_stadt()->get_finance_history_month(0, HIST_CITIZENS), diff.x, diff.y, diff.z);
					}
				}
			}
		}
	}
}


static void write_townhall_at(player_t */*pl*/, cbuffer_t& buf, const koord3d pos, const koord3d origin)
{
	if (const grund_t* gr = world()->lookup(pos)) {
		if (const gebaeude_t* gb = gr->find<gebaeude_t>()) {
			if (gb->get_tile()->get_offset() == koord(0, 0)) {
				// we have a start tile here => more checks
				const building_desc_t* desc = gb->get_tile()->get_desc();
				if (gb->is_townhall()) {
					koord3d diff = pos - origin;
					buf.printf("\thm_city_tl(0,\"%s\",[%d,%d,%d],%d)\n", desc->get_name(), diff.x, diff.y, diff.z, gb->get_tile()->get_layout());
				}
			}
		}
	}
}


static void write_house_at(player_t* pl, cbuffer_t& buf, const koord3d pos, const koord3d origin)
{
	if (const grund_t* gr = world()->lookup(pos)) {
		if (const gebaeude_t* gb = gr->find<gebaeude_t>()) {
			if (gb->get_tile()->get_offset() == koord(0, 0)) {
				// we have a start tile here => more checks
				const building_desc_t* desc = gb->get_tile()->get_desc();
				if (!desc->is_transport_building()) {
					sint16 rotation = gb->get_tile()->get_layout();
					if (pl->is_public_service()) {
						if (desc->is_headquarters()) {
							// skipping
						}
						else if (desc->is_factory()) {
							// koord3d diff = pos - origin;
							// buf.printf("\thm_factor_tl(0,\"%s\",[%d,%d,%d],%d)\n", desc->get_name(), diff.x, diff.y, diff.z, rotation);
						}
						else if (desc->is_attraction()) {
							koord3d diff = pos - origin;
							buf.printf("\thm_house_tl(\"%s\",[%d,%d,%d],%d)\n", desc->get_name(), diff.x, diff.y, diff.z, rotation);
						}
						else if (gb->is_townhall()) {
							// has been hopefully already written ...
						}
						else if (desc->is_connected_with_town()) {
							koord3d diff = pos - origin;
							buf.printf("\thm_house_tl(\"%s\",[%d,%d,%d],%d)\n", desc->get_name(), diff.x, diff.y, diff.z, rotation);
						}
					}
					else if (gb->get_owner() == pl) {
						koord3d diff = pos - origin;
						if (desc->is_headquarters()) {
							// headquarter has a level instead desc
							buf.printf("\thm_headquarter_tl(%d,[%d,%d,%d],%d)\n", pl->get_headquarter_level(), diff.x, diff.y, diff.z, rotation);
						}
						else {
							buf.printf("\thm_house_tl(\"%s\",[%d,%d,%d],%d)\n", desc->get_name(), diff.x, diff.y, diff.z, rotation);
						}
					}
				}
			}
		}
	}
}


static void write_depot_at(player_t* pl, cbuffer_t& buf, const koord3d pos, const koord3d origin)
{
	if (const grund_t* gr = world()->lookup(pos)) {
		if (const depot_t* obj = gr->get_depot()) {
			if (obj->get_owner() == pl) {
				const building_desc_t* desc = obj->get_tile()->get_desc();
				koord3d diff = pos - origin;
				buf.printf("\thm_depot_tl(\"%s\",[%d,%d,%d],%d)\n", desc->get_name(), diff.x, diff.y, diff.z, desc->get_finance_waytype());
			}
		}
	}
}


// taking all roadsigns for now
static void write_sign_at(player_t* , cbuffer_t& buf, const koord3d pos, const koord3d origin)
{
	const grund_t* gr = world()->lookup(pos);
	const weg_t* weg = gr ? gr->get_weg_nr(0) : NULL;
	if (!weg || (!weg->has_sign() && !weg->has_signal())) {
		return;
	}
	// now this tile should have a sign.
	roadsign_t* sign = NULL;
	if (signal_t* s = gr->find<signal_t>()) {
		sign = s;
	}
	else {
		// a sign should exists.
		sign = gr->find<roadsign_t>();
	}
	if (sign) {
		// now this pos has a stop.
		uint8 cnt = 1;
		const ribi_t::ribi d = sign->get_dir();
		const bool ow = sign->get_desc()->is_single_way();
		if (!ribi_t::is_single(d)) {
			cnt = 1;
		}
		else if (ribi_t::is_straight(weg->get_ribi_unmasked())) {
			cnt = (d == ribi_t::north || d == ribi_t::east) ^ ow ? 2 : 3;
		}
		else if (ribi_t::is_bend(weg->get_ribi_unmasked())) {
			cnt = (d == ribi_t::north || d == ribi_t::south) ^ ow ? 2 : 3;
		}
		koord3d diff = pos - origin;
		buf.printf("\thm_sign_tl(\"%s\",%d,[%d,%d,%d],%d,%u)\n", sign->get_desc()->get_name(), cnt, diff.x, diff.y, diff.z,
			sign->get_desc()->get_waytype(),
			sign->get_desc()->is_traffic_light() ? 512 : sign->get_desc()->get_flags());
	}
}


static void write_slope_at(player_t* pl, cbuffer_t& buf, const koord3d pos, const koord3d origin)
{
	const grund_t* gr = world()->lookup(pos);
	if (!gr  ||  gr->is_water()  ||  gr->ist_auf_bruecke()  ||  gr->ist_im_tunnel()) {
		return;
	}
	if(!pl->is_public_service()) {
		// only save used tiles unless public service
		if (gr->ist_natur()  &&  gr->get_typ()==grund_t::boden) {
			// do not touch
			return;
		}
		if (origin.z > pos.z  &&  (gr->obj_count()==0  ||  gr->obj_bei(0)->get_owner() != pl)) {
			// do not save tiles below the start unless is mine
			return;
		}
	}

	const koord3d pb = pos - origin;
	sint8 diff = pb.z;
	while (diff != 0) {
		if (diff > 0) {
			// raise the land
			buf.printf("\thm_slope_tl(hm_slope.UP,[%d,%d,%d])\n", pb.x, pb.y, pb.z - diff);
			diff -= 1;
		}
		else {
			// lower the land
			buf.printf("\thm_slope_tl(hm_slope.DOWN,[%d,%d,%d])\n", pb.x, pb.y, pb.z - diff);
			diff += 1;
		}
	}
	// check slopes
	const slope_t::type slp = gr->get_grund_hang();
	if (slp > 0) {
		buf.printf("\thm_slope_tl(%d,[%d,%d,%d])\n", slp, pb.x, pb.y, pb.z);
	}
}


static void write_ground_at(player_t* pl, cbuffer_t& buf, const koord3d pos, const koord3d origin)
{
	const grund_t* gr = world()->lookup(pos);
	if (!gr || gr->is_water() || gr->ist_auf_bruecke() || gr->ist_im_tunnel()) {
		return;
	}
	if (!pl->is_public_service()) {
		// only save used tiles unless public service
		if (gr->ist_natur() && gr->get_typ() == grund_t::boden) {
			// do not touch
			return;
		}
		if (origin.z > pos.z && (gr->obj_count() == 0 || gr->obj_bei(0)->get_owner() != pl)) {
			// do not save tiles below the start unless is mine
			return;
		}
	}

	const koord3d pb = pos - origin;
	buf.printf("\thm_ground_tl(%d,[%d,%d,%d])\n", gr->get_grund_hang(), pb.x, pb.y, pb.z);
}


// we only write bridges inside the marked area (ignoring ownership)
static void write_command_bridges(player_t*, cbuffer_t& buf, const koord start, const koord end, const koord3d origin)
{
	vector_tpl<koord3d> all_bridgepos;;
	karte_t* welt = world();
	for (sint16 x = start.x; x <= end.x; x++) {
		for (sint16 y = start.y; y <= end.y; y++) {
			if (grund_t* gr = welt->lookup_kartenboden(x, y)) {
				if (gr->ist_bruecke()) {
					koord3d bstart = gr->get_pos();
					bruecke_t* br = gr->find<bruecke_t>();
					assert(br);
					if (all_bridgepos.is_contained(bstart)) {
						// already handled
						continue;
					}
					all_bridgepos.append(bstart);

					// find end of bridge
					koord zv = (gr->get_grund_hang() != slope_t::flat) ? -koord(gr->get_grund_hang()) : koord(gr->get_weg_hang());
					koord3d checkpos(zv,slope_t::max_diff(max(gr->get_weg_hang(),gr->get_grund_hang())));
					checkpos += bstart;
					bstart -= origin;
					while(checkpos.x>=start.x && checkpos.y>=start.y && checkpos.x<=end.x && checkpos.y<=end.y) {
						gr = welt->lookup(checkpos);
						if (!gr) {
							// nothing here => must be the end
							gr = welt->lookup_kartenboden(checkpos.get_2d());
							if (gr) {
								koord3d bend = gr->get_pos();
								all_bridgepos.append(bend);
								bend -= origin;
								// write bridge building command
								buf.printf("\thm_bridge_tl(\"%s\",[%d,%d,%d],[%d,%d,%d],%d)\n", br->get_desc()->get_name(), bstart.x, bstart.y, bstart.z, bend.x, bend.y, bend.z, br->get_waytype());
							}
							// or the end of the map ...
							break;
						}
						else if (!gr->ist_auf_bruecke()) {
							koord3d bend = gr->get_pos();
							all_bridgepos.append(bend);
							bend -= origin;
							// write bridge building command
							buf.printf("\thm_bridge_tl(\"%s\",[%d,%d,%d],[%d,%d,%d],%d)\n", br->get_desc()->get_name(), bstart.x, bstart.y, bstart.z, bend.x, bend.y, bend.z, br->get_waytype());
							break;
						}
						checkpos += zv;
					}
				}
			}
		}
	}
}


// since the building order is important, we build the station in the same tile order as the original
// May be still fail if there have been connecting tiles that have been deleted ...
static void write_command_halt(player_t* pl, cbuffer_t& buf, const koord start, const koord end, const koord3d origin)
{
	vector_tpl<halthandle_t> all_halt;
	karte_t *welt = world();
	for (sint16 x = start.x; x <= end.x; x++) {
		for (sint16 y = start.y; y <= end.y; y++) {
			if (planquadrat_t* plan = welt->access(x, y)) {
				for (uint8 i = 0; i < plan->get_boden_count(); i++) {
					halthandle_t h = plan->get_boden_bei(i)->get_halt();
					if (h.is_bound()  &&  (h->get_owner()==pl  ||  h->get_owner()->get_player_nr()==PLAYER_PUBLIC_NR)) {
						all_halt.append_unique(h);
					}
				}
			}
		}
	}
	for (const halthandle_t &halt : all_halt) {
		for (const haltestelle_t::tile_t& t : halt->get_tiles()) {
			koord p = t.grund->get_pos().get_2d();
			if (start.x <= p.x && p.x <= end.x && start.y <= p.y && p.y <= end.y) {
				if (const gebaeude_t* gb = t.grund->find<gebaeude_t>()) {
					sint16 rotation = gb->get_tile()->get_layout();
					if (gb->get_tile()->get_offset() == koord(0, 0)) {
						// only for left top tile save
						const building_desc_t* desc = gb->get_tile()->get_desc();
						koord3d diff = t.grund->get_pos() - origin;
						buf.printf("\thm_station_tl(\"%s\",[%d,%d,%d],%d,%d)\n", desc->get_name(), diff.x, diff.y, diff.z, gb->get_waytype(), rotation);
					}
				}
			}
		}
	}
}


static void write_command(player_t *pl, cbuffer_t& buf, void (*func)(player_t*, cbuffer_t&, const koord3d, const koord3d), const koord start, const koord end, const koord3d origin)
{
	karte_t* welt = world();
	for (sint16 x = start.x; x <= end.x; x++) {
		for (sint16 y = start.y; y <= end.y; y++) {
			if (planquadrat_t* plan = welt->access(x, y)) {
				for (uint8 i = 0; i < plan->get_boden_count(); i++) {
					if (grund_t* gr = plan->get_boden_bei(i)) {
						if (!gr->ist_im_tunnel()) {
							func(pl, buf, plan->get_boden_bei(i)->get_pos(), origin);
						}
					}
				}
			}
		}
	}
}


// for functions which need concatenation
class write_path_command_t {
protected:
	struct {
		koord3d start;
		koord3d end;
		const char* desc_name;
		waytype_t way_type;
		sint16 system_type;
	} typedef script_cmd;
	vector_tpl<script_cmd> commands;

	const player_t* player;

	cbuffer_t& buf;

	const char* cmd_str;

	koord start, end;

	koord3d origin;

	virtual void append_command(koord3d pos, const ribi_t::ribi(&dirs)[2]) = 0;
	virtual bool can_concatnate(script_cmd& a, script_cmd& b) = 0;

public:
	write_path_command_t(player_t* pl, cbuffer_t& b, koord s, koord e, koord3d o) :
		player(pl), buf(b), start(s), end(e), origin(o) { };

	void write() {
		for (sint8 z = -128; z < 127; z++) { // iterate for all height
			for (sint16 x = start.x; x <= end.x; x++) {
				for (sint16 y = start.y; y <= end.y; y++) {
					ribi_t::ribi dirs[2];
					dirs[0] = x > start.x ? ribi_t::west : ribi_t::none;
					dirs[1] = y > start.y ? ribi_t::north : ribi_t::none;
					append_command(koord3d(x, y, z), dirs);
				}
			}
		}
		// concatenate the command
		while (!commands.empty()) {
			script_cmd cmd = commands.pop_back();
			bool adjacent_found = true;
			while (adjacent_found) {
				adjacent_found = false;
				for (uint32 i = 0; i < commands.get_count(); i++) {
					if (!can_concatnate(commands[i], cmd)) {
						continue;
					}
					if (cmd.end == commands[i].start) {
						cmd.end = commands[i].end;
						adjacent_found = true;
					}
					else if (cmd.start == commands[i].end) {
						cmd.start = commands[i].start;
						adjacent_found = true;
					}
					if (adjacent_found) {
						commands.remove_at(i);
						break;
					}
				}
			}
			// all adjacent entries were concatenated.
			if (cmd.system_type >= 0) {
				buf.printf("\t%s(\"%s\",[%d,%d,%d],[%d,%d,%d],%d,%d)\n", cmd_str, cmd.desc_name, cmd.start.x, cmd.start.y, cmd.start.z, cmd.end.x, cmd.end.y, cmd.end.z, cmd.way_type, cmd.system_type);
			}
			else {
				buf.printf("\t%s(\"%s\",[%d,%d,%d],[%d,%d,%d],%d)\n", cmd_str, cmd.desc_name, cmd.start.x, cmd.start.y, cmd.start.z, cmd.end.x, cmd.end.y, cmd.end.z, cmd.way_type);
			}
		}
	}
};

class write_way_command_t : public write_path_command_t {
protected:
	bool first_pass;

	void append_command(koord3d pos, const ribi_t::ribi(&dirs)[2]) OVERRIDE
	{
		const grund_t* gr = world()->lookup(pos);
		if (!gr) {
			return;
		}
		bool weg_nr = 0;
		if (!first_pass  &&  gr->get_weg_nr(1)) {
			weg_nr = 1;
		}
		const weg_t* weg0 = gr->get_weg_nr(weg_nr);
		if (!weg0) {
			return;
		}
		systemtype_t start_styp = weg0->get_desc()->get_styp();
		koord3d pb = pos - origin; // relative base pos
		if (gr->get_typ() == grund_t::monorailboden) {
			pb.z -= world()->get_settings().get_way_height_clearance();
		}
		for (uint8 i = 0; i < 2; i++) {
			if (dirs[i] == ribi_t::none) {
				continue;
			}
			grund_t* to = NULL;
			gr->get_neighbour(to, weg0->get_waytype(), dirs[i]);
			if (to) {
				const weg_t* to_weg = to->get_weg_nr(0);
				bool to_weg_nr = 0;
				if (to_weg->get_waytype() != weg0->get_waytype()) {
					to_weg = to->get_weg_nr(1);
					to_weg_nr = 1;
					if (to_weg->get_waytype() != weg0->get_waytype()) {
						continue;
					}
				}
				if (!to_weg_nr  &&  !weg_nr) {
					// check system type (for airplanes etc.) if there is only one way
					if (start_styp == to_weg->get_desc()->get_styp()) {
						if (first_pass && start_styp != 0) {
							// we connect in this round only to other system types for one step
							continue;
						}
						if (!first_pass && start_styp == 0) {
							// we connect in this round only to other system types
							continue;
						}
					}
					else if (!first_pass) {
						continue;
					}
				}
				koord3d tp = to->get_pos() - origin;
				if (to->get_typ() == grund_t::monorailboden) {
					tp = tp - koord3d(0, 0, world()->get_settings().get_way_height_clearance());
				}
				const way_desc_t* d = (weg0->get_desc()->get_styp()==0 && first_pass) ? weg0->get_desc() : to_weg->get_desc();
				if (weg_nr > 0) {
					d = weg0->get_desc();
				}
				if (to_weg_nr > 0) {
					d = to_weg->get_desc();
				}
				commands.append(script_cmd{ pb, tp, d->get_name(), d->get_waytype(), (sint16)d->get_styp() });
			}
		}
	}

	bool can_concatnate(script_cmd& a, script_cmd& b) OVERRIDE
	{
		return strcmp(a.desc_name, b.desc_name) == 0  &&  ribi_type(a.start, a.end) == ribi_type(b.start, b.end);
	}

public:
	write_way_command_t(player_t* pl, cbuffer_t& b, koord s, koord e, koord3d o, bool fp) :
		write_path_command_t(pl, b, s, e, o)
	{
		cmd_str = "hm_way_tl";
		first_pass = fp;
	}
};


class write_wayobj_command_t : public write_path_command_t {
protected:
	void append_command(koord3d pos, const ribi_t::ribi(&dirs)[2]) OVERRIDE
	{
		const grund_t* gr = world()->lookup(pos);
		const weg_t* weg0 = gr ? gr->get_weg_nr(0) : NULL;
		const wayobj_t* wayobj = weg0 ? gr->get_wayobj(weg0->get_waytype()) : NULL;
		if (!wayobj) {
			return;
		}
		const waytype_t type = weg0->get_waytype();
		for (uint8 i = 0; i < 2; i++) {
			if (dirs[i] == ribi_t::none) {
				continue;
			}
			grund_t* to = NULL;
			gr->get_neighbour(to, type, dirs[i]);
			const wayobj_t* t_obj = to ? to->get_wayobj(type) : NULL;
			if (t_obj && t_obj->get_desc() == wayobj->get_desc()) {
				commands.append(script_cmd{ pos - origin, to->get_pos() - origin, wayobj->get_desc()->get_name(), wayobj->get_waytype(),-1 });
			}
		}
	}

	bool can_concatnate(script_cmd& a, script_cmd& b) OVERRIDE
	{
		return strcmp(a.desc_name, b.desc_name) == 0;
	}

public:
	write_wayobj_command_t(player_t *pl, cbuffer_t& b, koord s, koord e, koord3d o) :
		write_path_command_t(pl, b, s, e, o)
	{
		cmd_str = "hm_wayobj_tl";
	}
};


char const* tool_generate_script_t::do_work(player_t* pl, const koord3d& start, const koord3d& end)
{
	koord3d e = end == koord3d::invalid ? start : end;
	koord k1 = koord(min(start.x, e.x), min(start.y, e.y));
	koord k2 = koord(max(start.x, e.x), max(start.y, e.y));
	koord area( k2.x-k1.x+1, k2.y-k1.y+1 );

	cbuffer_t generated_script_buf;
	generated_script_buf.printf("// automated rebuild scrip with size (%d,%d)\n\n", area.x, area.y); // comment
	generated_script_buf.append("include(\"hm_toolkit_v3\")\n\nfunction hm_build() {\n"); // header

	int cmdlen = generated_script_buf.len();

	koord3d begin(k1, start.z);
	if (pl->is_public_service()) {
		// ensure this is run by public service
		generated_script_buf.append("\tif(this.player.nr != 1) {\n\t\treturn \"Must be run as public player!\"\n\t}\n\n");
	}
	write_command(pl, generated_script_buf, write_ground_at, k1, k2, begin); // write all used tiles
	if (pl->is_public_service()) {
		write_command(pl, generated_script_buf, write_townhall_at, k1, k2, begin);
	}
	write_command_bridges(pl, generated_script_buf, k1, k2, begin);
	write_way_command_t(pl, generated_script_buf, k1, k2, begin, true).write();
	write_way_command_t(pl, generated_script_buf, k1, k2, begin, false).write();
	write_wayobj_command_t(pl, generated_script_buf, k1, k2, begin).write();
	write_command_halt(pl, generated_script_buf, k1, k2, begin);
	write_command(pl, generated_script_buf, write_depot_at, k1, k2, begin);
	write_command(pl, generated_script_buf, write_sign_at, k1, k2, begin);
	write_command(pl, generated_script_buf, write_house_at, k1, k2, begin);
	if (pl->is_public_service()) {
		write_command(pl, generated_script_buf, write_owner_at, k1, k2, begin);
//		write_command(pl, generated_script_buf, write_city_at, k1, k2, begin);
	}

	if (cmdlen == generated_script_buf.len()) {
		return NULL;
	}

	generated_script_buf.append("}\n"); // footer

	cbuffer_t dir_buf;
	dir_buf.printf("%saddons%s%stool%s", env_t::user_dir, PATH_SEPARATOR, env_t::pak_name.c_str(), PATH_SEPARATOR);
	dr_mkdir(dir_buf.get_str());

	create_win(new script_generator_frame_t(this, dir_buf.get_str(), generated_script_buf, area ), w_info, magic_script_generator);

	if (can_use_gui()  &&  pl == welt->get_active_player()) {
		welt->set_tool(general_tool[TOOL_QUERY], pl);
	}

	return NULL;
}


bool tool_generate_script_t::save_script(const char* fullpath, const char *command, koord area) const
{
	dr_mkdir(fullpath);
	cbuffer_t fname;
	cbuffer_t short_name;
	if (const char* p = strrchr(fullpath, *PATH_SEPARATOR)) {
		short_name.append(p + 1);
	}

	fname.printf("%s%stool.nut",fullpath, PATH_SEPARATOR);
	if (FILE* file = dr_fopen(fname, "w")) {
		fprintf(file, "%s", command);
		fclose(file);
		fname.clear();
		fname.printf("%s%sdescription.tab", fullpath, PATH_SEPARATOR);
		if ((file = dr_fopen(fname, "w"))) {
			fprintf(file, "title=Building %s\n", short_name.get_str());
			fprintf(file, "cursor_area=%d,%d\n", area.x, area.y);
			fprintf(file, "type=one_click\ntooltip=Building %s created by Simutrans\nrestart=1\nicon=BuilderScript\n", short_name.get_str());
			fclose(file);
		}
		create_win(new news_img("The generated script was saved!\n"), w_time_delete, magic_none);
		return true;
	}
	dbg->error("tool_generate_script_t::save_script()", "cannot save file %s", fullpath);
	create_win(new news_img("The script cannot be saved!\n"), w_time_delete, magic_none);
	return false;
}
