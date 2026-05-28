/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * The depot window, where to buy convois
 */

#include <algorithm>
#include <stdio.h>
#include <string.h>

#include "../sys/simsys.h"

#include "../simunits.h"
#include "../simworld.h"
#include "../vehicle/simvehicle.h"
#include "../simconvoi.h"
#include "../simdepot.h"
#include "simwin.h"
#include "../simcolor.h"
#include "../simdebug.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../simline.h"
#include "../simlinemgmt.h"
#include "../simmenu.h"
#include "../simskin.h"

#include "../tpl/slist_tpl.h"

#include "schedule_gui.h"
#include "line_management_gui.h"
#include "line_item.h"
#include "convoy_item.h"
#include "components/gui_image_list.h"
#include "messagebox.h"
#include "depot_frame.h"
#include "schedule_list.h"

#include "../descriptor/goods_desc.h"
#include "../descriptor/intro_dates.h"
#include "../bauer/vehikelbauer.h"
#if CONVOI_TEMPLATE
#include "../dataobj/convoi_template.h"
#include <vector>
#endif
#include "../dataobj/schedule.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../dataobj/repositioning.h"

#include "../player/simplay.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"
#include "../utils/simrandom.h"

#include "../bauer/goods_manager.h"

#include "../boden/wege/weg.h"

#include "../unicode.h"



static int sort_by_action;

bool depot_frame_t::show_retired_vehicles = false;
bool depot_frame_t::show_all = false;


#if CONVOI_TEMPLATE
class gui_template_panel_t : public gui_action_creator_t, public gui_component_t {
public:
	struct entry_t {
		const convoi_template_t *tmpl;
		std::vector<const vehicle_desc_t *> descs;
		std::vector<const vehicle_desc_t *> compact; // descs with nulls removed
		scr_coord_val row_h;
		// stats for sorting
		sint64 cost;
		sint64 run_cost;
		sint32 min_speed;
		uint32 total_capacity;
		const goods_desc_t *primary_goods;
		uint8 veh_count;
		bool all_electric;        // true if every vehicle in this template is electric
		bool internally_valid;    // true if all originally-adjacent non-null pairs can connect
		bool compacted_valid;     // true if all pairs adjacent after null-compaction can connect
		bool mixed_waytype;       // true if the template contains vehicles of different waytypes
		waytype_t tmpl_waytype;  // common waytype of all vehicles; invalid_wt if mixed
	};

private:
	vector_tpl<entry_t> all_entries;
	vector_tpl<entry_t> entries;
	sint8 player_nr;
	sint32 last_hovered_idx;
	scr_coord_val cell_w;
	scr_coord_val cell_h;
	int cur_month_now;
	waytype_t depot_wt;
	waytype_t depot_sec_wt;

	static bool compare_entries(const entry_t &a, const entry_t &b, int sort_mode) {
		switch (sort_mode) {
			case vehicle_builder_t::sb_capacity:
				if (a.total_capacity != b.total_capacity) return a.total_capacity < b.total_capacity;
				break;
			case vehicle_builder_t::sb_price:
				if (a.cost != b.cost) return a.cost < b.cost;
				break;
			case vehicle_builder_t::sb_cost:
				if (a.run_cost != b.run_cost) return a.run_cost < b.run_cost;
				break;
			case vehicle_builder_t::sb_cost_per_unit: {
				if (a.total_capacity == 0 && b.total_capacity == 0) break;
				if (a.total_capacity == 0) return false;
				if (b.total_capacity == 0) return true;
				sint64 ra = a.run_cost * 1000 / (sint64)a.total_capacity;
				sint64 rb = b.run_cost * 1000 / (sint64)b.total_capacity;
				if (ra != rb) return ra < rb;
				break;
			}
			case vehicle_builder_t::sb_speed:
				if (a.min_speed != b.min_speed) return a.min_speed < b.min_speed;
				break;
			case vehicle_builder_t::sb_freight: {
				const char *fa = a.primary_goods ? (a.primary_goods->get_catg() == 0 ? a.primary_goods->get_name() : a.primary_goods->get_catg_name()) : "";
				const char *fb = b.primary_goods ? (b.primary_goods->get_catg() == 0 ? b.primary_goods->get_name() : b.primary_goods->get_catg_name()) : "";
				int c = strcmp(fa, fb);
				if (c != 0) return c < 0;
				break;
			}
			case vehicle_builder_t::sb_length:
				if (a.veh_count != b.veh_count) return a.veh_count < b.veh_count;
				break;
			default: // sb_name and unsupported modes
				break;
		}
		return strcmp(translator::translate(a.tmpl->name.c_str()), translator::translate(b.tmpl->name.c_str())) < 0;
	}

public:
	gui_template_panel_t() : player_nr(0), last_hovered_idx(-1), cell_w(32), cell_h(32), cur_month_now(0), depot_wt(invalid_wt), depot_sec_wt(invalid_wt) {}

	void init(const vector_tpl<convoi_template_t> &templates, sint8 onr, const depot_t *dep) {
		player_nr = onr;
		cell_w = dep->get_x_grid() * get_base_tile_raster_width() / 64 + 4
		       - dep->get_grid_dx() * get_base_tile_raster_width() / 64 / 2;
		cell_h = dep->get_y_grid() * get_base_tile_raster_width() / 64 + 6;
		depot_wt     = dep->get_waytype();
		depot_sec_wt = dep->get_secondary_waytype();
		all_entries.clear();
		for (uint i = 0; i < (uint)templates.get_count(); i++) {
			entry_t e;
			e.tmpl = &templates[i];
			e.cost = 0;
			e.run_cost = 0;
			e.min_speed = 0;
			e.total_capacity = 0;
			e.primary_goods = NULL;
			e.veh_count = 0;
			e.all_electric = false;
			bool any_speed = false;
			bool has_non_electric = false;
			const uint8 veh_count = (uint8)std::min(templates[i].vehicles.size(), (size_t)255u);
			for (uint8 j = 0; j < veh_count; j++) {
				const vehicle_desc_t *desc = vehicle_builder_t::get_info(templates[i].vehicles[j].c_str());
				if (!desc) {
					dbg->error("gui_template_panel_t::init", "Convoy template \"%s\" (%s): vehicle[%u] \"%s\" not found.",
						templates[i].name.c_str(), templates[i].source_file.c_str(), j, templates[i].vehicles[j].c_str());
				}
				e.descs.push_back(desc);
				if (desc) {
					e.cost += desc->get_price();
					e.run_cost += desc->get_running_cost();
					if (!any_speed || desc->get_topspeed() < e.min_speed) {
						e.min_speed = desc->get_topspeed();
						any_speed = true;
					}
					if (desc->get_capacity() > 0) {
						e.total_capacity += desc->get_capacity();
						if (!e.primary_goods) e.primary_goods = desc->get_freight_type();
					}
					if (desc->get_engine_type() != vehicle_desc_t::electric) {
						has_non_electric = true;
					}
					e.veh_count++;
				}
			}
			e.all_electric = (e.veh_count > 0) && !has_non_electric;
			bool mixed_waytype = false;
			waytype_t first_wt = invalid_wt;
			for (uint32 j = 0; j < (uint32)e.descs.size(); j++) {
				if (e.descs[j]) {
					waytype_t wt = e.descs[j]->get_waytype();
					if (first_wt == invalid_wt) {
						first_wt = wt;
					} else if (wt != first_wt) {
						mixed_waytype = true;
						break;
					}
				}
			}
			e.mixed_waytype = mixed_waytype;
			e.tmpl_waytype  = mixed_waytype ? invalid_wt : first_wt;
			if (mixed_waytype) {
				dbg->error("gui_template_panel_t::init", "Convoy template \"%s\" (%s) contains vehicles of mixed waytypes.",
					templates[i].name.c_str(), templates[i].source_file.c_str());
			}
			if (e.veh_count == 0) {
				dbg->error("gui_template_panel_t::init", "Convoy template \"%s\" (%s) has no valid vehicles (all descriptors missing).",
					templates[i].name.c_str(), templates[i].source_file.c_str());
			}
			bool internally_valid = true;
			for (int j = 0; j + 1 < (int)e.descs.size(); j++) {
				const vehicle_desc_t *cur  = e.descs[j];
				const vehicle_desc_t *next = e.descs[j + 1];
				if (cur && next && (!cur->can_lead(next) || !next->can_follow(cur))) {
					internally_valid = false;
					break;
				}
			}
			e.internally_valid = internally_valid;
			// Check compacted validity: after removing nulls, can all adjacent pairs connect?
			bool compacted_valid = true;
			const vehicle_desc_t *prev_non_null = NULL;
			for (uint j = 0; j < (uint)e.descs.size(); j++) {
				const vehicle_desc_t *d = e.descs[j];
				if (d) {
					if (prev_non_null && (!prev_non_null->can_lead(d) || !d->can_follow(prev_non_null))) {
						compacted_valid = false;
						break;
					}
					prev_non_null = d;
				}
			}
			e.compacted_valid = compacted_valid;
			for (size_t j = 0; j < e.descs.size(); j++) {
				if (e.descs[j]) e.compact.push_back(e.descs[j]);
			}
			e.row_h = LINESPACE + max(cell_h, (scr_coord_val)16) + 4;
			all_entries.append(e);
		}
		refresh("", vehicle_builder_t::sb_name);
	}

	// boundary_veh: the vehicle at the front (is_insert) or back (!is_insert) of the current convoy.
	// Pass NULL to skip compatibility filtering (new convoy or show_all/allow_invalid).
	// target_wt: when not invalid_wt, only templates with exactly this waytype are shown.
	void refresh(const char *name_filter, int sort_mode,
	             const vehicle_desc_t *boundary_veh = NULL, bool is_insert = false,
	             bool weg_electrified = true, bool show_all_flag = false,
	             int month_now = 0, bool show_retired = false,
	             waytype_t target_wt = invalid_wt) {
		cur_month_now = month_now;
		entries.clear();
		for (uint i = 0; i < (uint)all_entries.get_count(); i++) {
			const entry_t &e = all_entries[i];
			if (e.veh_count == 0) continue;
			if (e.tmpl_waytype == invalid_wt) continue; // mixed or no vehicles found
			if (e.tmpl_waytype != depot_wt && (depot_sec_wt == invalid_wt || e.tmpl_waytype != depot_sec_wt)) continue;
			if (target_wt != invalid_wt && e.tmpl_waytype != target_wt) continue;
			// Timeline filter: future vehicles are always hidden;
			// retired vehicles are hidden unless show_retired is true.
			if (month_now > 0) {
				bool has_future = false, has_retired = false;
				for (uint j = 0; j < (uint)e.descs.size(); j++) {
					const vehicle_desc_t *desc = e.descs[j];
					if (desc) {
						if (desc->is_future(month_now)) { has_future = true; break; }
						if (desc->is_retired(month_now)) has_retired = true;
					}
				}
				if (has_future) continue;
				if (has_retired && !show_retired) continue;
			}
			if (!show_all_flag && (!e.internally_valid || !e.compacted_valid)) {
				continue;
			}
			if (!weg_electrified && e.all_electric) {
				continue;
			}
			if (name_filter && name_filter[0] != 0) {
				if (!utf8caseutf8(e.tmpl->name.c_str(), name_filter) && !utf8caseutf8(translator::translate(e.tmpl->name.c_str()), name_filter)) {
					continue;
				}
			}
			if (boundary_veh != NULL) {
				// Find the template vehicle adjacent to the existing convoy
				const vehicle_desc_t *tmpl_adj = NULL;
				if (is_insert) {
					// Last non-null template vehicle connects to convoy front
					for (int j = (int)e.descs.size() - 1; j >= 0; j--) {
						if (e.descs[j]) { tmpl_adj = e.descs[j]; break; }
					}
					if (!tmpl_adj || !(tmpl_adj->can_lead(boundary_veh) && boundary_veh->can_follow(tmpl_adj))) {
						continue;
					}
				} else {
					// First non-null template vehicle connects to convoy back
					for (uint j = 0; j < (uint)e.descs.size(); j++) {
						if (e.descs[j]) { tmpl_adj = e.descs[j]; break; }
					}
					if (!tmpl_adj || !(tmpl_adj->can_follow(boundary_veh) && boundary_veh->can_lead(tmpl_adj))) {
						continue;
					}
				}
			}
			entries.append(e);
		}
		std::sort(entries.begin(), entries.end(), [sort_mode](const entry_t &a, const entry_t &b) {
			return compare_entries(a, b, sort_mode);
		});
		recalc_size();
	}

	void recalc_size() {
		scr_coord_val total_h = 0;
		for (uint i = 0; i < (uint)entries.get_count(); i++) {
			total_h += entries[i].row_h;
		}
		set_size(scr_size(max(get_size().w, (scr_coord_val)100), total_h));
	}

	sint32 get_count() const { return (sint32)entries.get_count(); }
	sint32 get_hovered_index() const { return last_hovered_idx; }
	const entry_t* get_entry(sint32 idx) const {
		return (idx >= 0 && (uint32)idx < entries.get_count()) ? &entries[idx] : NULL;
	}

	scr_size get_min_size() const OVERRIDE { return get_size(); }
	scr_size get_max_size() const OVERRIDE { return get_size(); }

	void draw(scr_coord offset) OVERRIDE {
		offset += get_pos();
		const int mx = get_mouse_x();
		const int my = get_mouse_y();
		last_hovered_idx = -1;
		clip_dimension const clip = display_get_clip_wh();
		scr_coord_val y = offset.y;
		for (uint i = 0; i < (uint)entries.get_count(); i++) {
			const entry_t &e = entries[i];
			const scr_coord_val rh = e.row_h;
			const bool hovered = (mx >= offset.x && mx < offset.x + get_size().w && my >= y && my < y + rh
			                      && my >= clip.y && my < clip.yy);
			if (hovered) {
				last_hovered_idx = (sint32)i;
				display_fillbox_wh_clip_rgb(offset.x, y, get_size().w, rh, SYSCOL_LIST_BACKGROUND_SELECTED_NF, true);
			}
			display_proportional_clip_rgb(offset.x + D_H_SPACE, y + 1, translator::translate(e.tmpl->name.c_str()), ALIGN_LEFT, SYSCOL_TEXT, true);
			scr_coord_val xpos = offset.x + 2;
			const scr_coord_val bar_y = y + LINESPACE + cell_h - 5;
			const std::vector<const vehicle_desc_t *> &compact = e.compact;
			const uint cn = (uint)compact.size();
			for (uint j = 0; j < cn; j++) {
				const vehicle_desc_t *desc = compact[j];
				if (desc->get_base_image() != IMG_EMPTY) {
					scr_coord_val ix, iy, iw, ih;
					display_get_base_image_offset(desc->get_base_image(), &ix, &iy, &iw, &ih);
					display_base_img(desc->get_base_image(), xpos - ix + 2, y + LINESPACE - iy + (cell_h - ih) - 6, player_nr, false, true);
				}
				// lcolor: connection from previous compacted vehicle, or terminal if first
				PIXVAL lc = (j == 0)
					? color_idx_to_rgb(desc->can_follow(NULL) ? COL_GREEN : COL_YELLOW)
					: color_idx_to_rgb((compact[j-1]->can_lead(desc) && desc->can_follow(compact[j-1])) ? COL_GREEN : COL_RED);
				// rcolor: connection to next compacted vehicle, or terminal if last
				PIXVAL rc = (j == cn - 1)
					? color_idx_to_rgb(desc->can_lead(NULL) ? COL_GREEN : COL_YELLOW)
					: color_idx_to_rgb((desc->can_lead(compact[j+1]) && compact[j+1]->can_follow(desc)) ? COL_GREEN : COL_RED);
				// Green bars turn blue for retired vehicles
				if (cur_month_now > 0 && desc->is_retired(cur_month_now)) {
					if (lc == color_idx_to_rgb(COL_GREEN)) lc = gui_theme_t::gui_color_obsolete;
					if (rc == color_idx_to_rgb(COL_GREEN)) rc = gui_theme_t::gui_color_obsolete;
				}
				display_fillbox_wh_clip_rgb(xpos + 1,          bar_y, cell_w / 2 - 1,          4, lc, true);
				display_fillbox_wh_clip_rgb(xpos + cell_w / 2, bar_y, cell_w - cell_w / 2 - 1, 4, rc, true);
				xpos += cell_w;
			}
			y += rh;
		}
	}

	bool infowin_event(event_t const *ev) OVERRIDE {
		if (IS_LEFTCLICK(ev) && last_hovered_idx >= 0) {
			call_listeners((long)last_hovered_idx);
			return true;
		}
		return false;
	}
};
#endif // CONVOI_TEMPLATE

depot_frame_t::depot_frame_t(depot_t* depot) :
	gui_frame_t("", NULL),
	depot(depot),
	icnv(-1),
	lb_convoi_line("Serves Line:", SYSCOL_TEXT, gui_label_t::left),
	lb_child_convoy("Child convoy:", SYSCOL_TEXT, gui_label_t::right),
	lb_sort_by("Sort by:", SYSCOL_TEXT, gui_label_t::right),
	lb_name_filter_input("Search:", SYSCOL_TEXT, gui_label_t::right),
	lb_veh_action("Fahrzeuge:", SYSCOL_TEXT, gui_label_t::right),
	convoi(&convoi_pics),
	scrolly_convoi(&cont_convoi),
	pas(&pas_vec),
	electrics(&electrics_vec),
	loks(&loks_vec),
	waggons(&waggons_vec),
	scrolly_pas(&pas),
	scrolly_electrics(&electrics),
	scrolly_loks(&loks),
	scrolly_waggons(&waggons),
	tram_pas(&tram_pas_vec),
	tram_electrics(&tram_electrics_vec),
	tram_loks(&tram_loks_vec),
	tram_waggons(&tram_waggons_vec),
	scrolly_tram_pas(&tram_pas),
	scrolly_tram_electrics(&tram_electrics),
	scrolly_tram_loks(&tram_loks),
	scrolly_tram_waggons(&tram_waggons),
	line_selector(line_scrollitem_t::compare),
	lb_vehicle_filter("Filter:", SYSCOL_TEXT, gui_label_t::right)
#if CONVOI_TEMPLATE
	,template_panel(NULL)
	,scrolly_template(NULL)
#endif
{
	if (depot) {
		init(depot);
	}
}

void depot_frame_t::init(depot_t *dep)
{
	depot = dep;
	set_name(depot->get_name());
	set_owner(depot->get_owner());
	icnv = depot->convoi_count()-1;

	scr_size size = scr_size(0,0);

DBG_DEBUG("depot_frame_t::depot_frame_t()","get_max_convoi_length()=%i",depot->get_max_convoi_length());
	last_selected_line = depot->get_last_selected_line();
	no_schedule_text     = translator::translate("<no schedule set>");
	clear_schedule_text  = translator::translate("<clear schedule>");
	unique_schedule_text = translator::translate("<individual schedule>");
	new_line_text        = translator::translate("<create new line>");
	line_seperator       = translator::translate("--------------------------------");
	new_convoy_text      = translator::translate("new convoi");
	promote_to_line_text = translator::translate("<promote to line>");
	no_child_text     = translator::translate("depart without child convoy");

	/*
	* [SELECT]:
	*/
	add_component(&lb_convois);

	convoy_selector.add_listener(this);
	add_component(&convoy_selector);

	/*
	* [SELECT ROUTE]:
	*/
	line_selector.add_listener(this);
	line_selector.set_wrapping(false);
	add_component(&line_selector);

	// goto line button
	line_button.set_typ(button_t::posbutton);
	line_button.set_targetpos3d(koord3d::invalid);
	line_button.add_listener(this);
	add_component(&line_button);

	/*
	* [CONVOI]
	*/
	convoi.set_player_nr(depot->get_owner_nr());
	convoi.add_listener(this);

	cont_convoi.add_component(&lb_convoi_number);
	cont_convoi.add_component(&convoi);

	// add_component(&convoi);

	add_component(&lb_convoi_count);
	add_component(&lb_convoi_speed);
	add_component(&lb_convoi_cost);
	add_component(&lb_convoi_value);
	add_component(&lb_convoi_line);
	add_component(&lb_convoi_power);
	add_component(&lb_convoi_weight);
	add_component(&cont_convoi_capacity);
	add_component(&lb_child_convoy);
	add_component(&lb_convoi_terminal_speed);

	sb_convoi_length.set_base( depot->get_max_convoi_length() * CARUNITS_PER_TILE / 2 - 1);
	sb_convoi_length.set_vertical(false);
	convoi_length_ok_sb = 0;
	convoi_length_slower_sb = 0;
	convoi_length_too_slow_sb = 0;
	convoi_tile_length_sb = 0;
	new_vehicle_length_sb = 0;
	if(  depot->get_max_convoi_length() > 1  ) { // no convoy length bar for ships or aircraft
		sb_convoi_length.add_color_value(&convoi_tile_length_sb, color_idx_to_rgb(COL_BROWN));
		sb_convoi_length.add_color_value(&new_vehicle_length_sb, color_idx_to_rgb(COL_DARK_GREEN));
		sb_convoi_length.add_color_value(&convoi_length_ok_sb, color_idx_to_rgb(COL_GREEN));
		sb_convoi_length.add_color_value(&convoi_length_slower_sb, color_idx_to_rgb(COL_ORANGE));
		sb_convoi_length.add_color_value(&convoi_length_too_slow_sb, color_idx_to_rgb(COL_RED));
		cont_convoi.add_component(&sb_convoi_length);
	}

	scrolly_convoi.set_scrollbar_mode(scrollbar_t::show_disabled);
	scrolly_convoi.set_size_corner(false);
	add_component(&scrolly_convoi);

	/*
	* [ACTIONS]
	*/
	bt_start.set_typ(button_t::roundbox);
	bt_start.add_listener(this);
	bt_start.set_tooltip("Start the selected vehicle(s)");
	add_component(&bt_start);

	bt_schedule.set_typ(button_t::roundbox);
	bt_schedule.add_listener(this);
	bt_schedule.set_tooltip("Give the selected vehicle(s) an individual schedule"); // translated to "Edit the selected vehicle(s) individual schedule or assigned line"
	add_component(&bt_schedule);

	bt_copy_convoi.set_typ(button_t::roundbox);
	bt_copy_convoi.add_listener(this);
	bt_copy_convoi.set_tooltip("Copy the selected convoi and its schedule or line (ctrl pressed: copy to clipboard)");
	add_component(&bt_copy_convoi);

	bt_sell.set_typ(button_t::roundbox);
	bt_sell.add_listener(this);
	bt_sell.set_tooltip("Sell the selected vehicle(s)");
	add_component(&bt_sell);
	
	bt_replacement_seed.set_typ(button_t::roundbox);
	bt_replacement_seed.add_listener(this);
	bt_replacement_seed.set_tooltip("Set as the replacement seed convoy");
	add_component(&bt_replacement_seed);

	bt_paste_convoi.set_typ(button_t::roundbox);
	bt_paste_convoi.add_listener(this);
	bt_paste_convoi.set_tooltip("Paste the copied convoi");
	add_component(&bt_paste_convoi);

	child_convoi_selector.add_listener(this);
	child_convoi_selector.set_wrapping(false);
	add_component(&child_convoi_selector);
	is_shown_convoy_coupled = false;
	is_teleport_to_another_depot = false;

	bt_uncouple.init(button_t::roundbox, "Uncouple");
	bt_uncouple.add_listener(this);
	bt_uncouple.set_tooltip("uncouple child convoy");
	add_component(&bt_uncouple);

	bt_reverse.init(button_t::square_state,"Reverse");
	bt_reverse.add_listener(this);
	bt_reverse.set_tooltip("Reverse this convoy");
	add_component(&bt_reverse);

	bt_remove_all_vehicles.init(button_t::roundbox,"remove all vehicles");
	bt_remove_all_vehicles.add_listener(this);
	bt_remove_all_vehicles.set_tooltip("remove all vehicles.");
	add_component(&bt_remove_all_vehicles);

	bt_allow_invalid_convoy.init(button_t::square_state,"allow invalid convoy");
	bt_allow_invalid_convoy.add_listener(this);
	bt_allow_invalid_convoy.set_tooltip("allow invalid coupling convoy start. If start invalid convoy, the power set as 0, and no load permitted!");
	add_component(&bt_allow_invalid_convoy);

	/*
	* [PANEL]
	*/
	pas.set_player_nr(depot->get_owner_nr());
	pas.add_listener(this);

	electrics.set_player_nr(depot->get_owner_nr());
	electrics.add_listener(this);

	loks.set_player_nr(depot->get_owner_nr());
	loks.add_listener(this);

	waggons.set_player_nr(depot->get_owner_nr());
	waggons.add_listener(this);

	tram_pas.set_player_nr(depot->get_owner_nr());
	tram_pas.add_listener(this);

	tram_electrics.set_player_nr(depot->get_owner_nr());
	tram_electrics.add_listener(this);

	tram_loks.set_player_nr(depot->get_owner_nr());
	tram_loks.add_listener(this);

	tram_waggons.set_player_nr(depot->get_owner_nr());
	tram_waggons.add_listener(this);

#if CONVOI_TEMPLATE
	// Convoy template tab setup
	if (welt->get_convoy_templates().empty()) {
		welt->load_convoy_templates();
	}
	template_panel = new gui_template_panel_t();
	template_panel->init(welt->get_convoy_templates(), (sint8)depot->get_owner_nr(), depot);
	template_panel->add_listener(this);
	scrolly_template.set_component(template_panel);
	scrolly_template.set_scrollbar_mode(scrollbar_t::show_disabled);
	scrolly_template.set_size_corner(false);
	cont_template_tab.add_component(&scrolly_template);
#endif

	tabs.add_listener(this);
	add_component(&tabs);
	add_component(&div_tabbottom);
	add_component(&lb_veh_action);
	add_component(&lb_sort_by);
	add_component(&lb_name_filter_input);
	add_component(&lb_vehicle_filter);
	add_component(&div_action_bottom);

	veh_action = va_append;
	bt_veh_action.set_typ(button_t::roundbox);
	bt_veh_action.add_listener(this);
	bt_veh_action.set_tooltip("Choose operation executed on clicking stored/new vehicles");
	add_component(&bt_veh_action);

	bt_show_all.set_typ(button_t::square_state);
	bt_show_all.set_text("Show all");
	bt_show_all.add_listener(this);
	bt_show_all.set_tooltip("Show also vehicles that do not match for current action.");
	bt_show_all.pressed = show_all;
	add_component(&bt_show_all);

	bt_show_tram.set_typ(button_t::square_state);
	bt_show_tram.set_text("Show tram");
	bt_show_tram.add_listener(this);
	bt_show_tram.set_tooltip("Switch between track and tram vehicle tabs.");
	bt_show_tram.pressed = false;
	add_component(&bt_show_tram);

	bt_obsolete.set_typ(button_t::square_state);
	bt_obsolete.set_text("Show obsolete");
	bt_obsolete.pressed = show_retired_vehicles;
	if(  welt->get_settings().get_allow_buying_obsolete_vehicles()  ) {
		bt_obsolete.add_listener(this);
		bt_obsolete.set_tooltip("Show also vehicles no longer in production.");
		add_component(&bt_obsolete);
	}

	bt_sell_all.set_typ(button_t::roundbox);
	bt_sell_all.set_text("Sell all vehicles");
	bt_sell_all.add_listener(this);
	bt_sell_all.set_tooltip("Sell all vehicles stored here.");
	add_component(&bt_sell_all);

	sort_by.add_listener(this);
	add_component(&sort_by);

	vehicle_filter.add_listener(this);
	add_component(&vehicle_filter);

	strncpy(name_filter_value,depot->get_name_filter(),sizeof(depot->get_name_filter()));
	name_filter_input.set_text(name_filter_value, 60);
	add_component(&name_filter_input);
	name_filter_input.add_listener(this);

	strncpy(depot_name, depot->get_name(), lengthof(depot_name));
	depot_name_input.set_text(depot_name, 60);
	add_component(&depot_name_input);
	depot_name_input.add_listener(this);

	build_vehicle_lists();

	// text will be translated by ourselves (after update data)!
	lb_convois.set_text_pointer( txt_convois );
	lb_convoi_number.set_text_pointer( txt_convoi_number );
	lb_convoi_count.set_text_pointer( txt_convoi_count );
	lb_convoi_speed.set_text_pointer( txt_convoi_speed );
	lb_convoi_cost.set_text_pointer( txt_convoi_cost );
	lb_convoi_value.set_text_pointer( txt_convoi_value );
	lb_convoi_power.set_text_pointer( txt_convoi_power );
	lb_convoi_weight.set_text_pointer( txt_convoi_weight );
	lb_convoi_terminal_speed.set_text_pointer( txt_convoi_terminal_speed );

	// Bolt image for electrified depots:
	add_component(&img_bolt);

	scrolly_pas.set_scrollbar_mode       ( scrollbar_t::show_disabled );
	scrolly_pas.set_size_corner(false);
	scrolly_electrics.set_scrollbar_mode ( scrollbar_t::show_disabled );
	scrolly_electrics.set_size_corner(false);
	scrolly_loks.set_scrollbar_mode      ( scrollbar_t::show_disabled );
	scrolly_loks.set_size_corner(false);
	scrolly_waggons.set_scrollbar_mode   ( scrollbar_t::show_disabled );
	scrolly_waggons.set_size_corner(false);

	scrolly_tram_pas.set_scrollbar_mode      ( scrollbar_t::show_disabled );
	scrolly_tram_pas.set_size_corner(false);
	scrolly_tram_electrics.set_scrollbar_mode( scrollbar_t::show_disabled );
	scrolly_tram_electrics.set_size_corner(false);
	scrolly_tram_loks.set_scrollbar_mode     ( scrollbar_t::show_disabled );
	scrolly_tram_loks.set_size_corner(false);
	scrolly_tram_waggons.set_scrollbar_mode  ( scrollbar_t::show_disabled );
	scrolly_tram_waggons.set_size_corner(false);

	layout(&size);
	gui_frame_t::set_windowsize(size);
	set_resizemode( diagonal_resize );

	depot->clear_command_pending();
}


// free memory: all the image_data_t
depot_frame_t::~depot_frame_t()
{
	clear_ptr_vector(pas_vec);
	clear_ptr_vector(electrics_vec);
	clear_ptr_vector(loks_vec);
	clear_ptr_vector(waggons_vec);
	clear_ptr_vector(tram_pas_vec);
	clear_ptr_vector(tram_electrics_vec);
	clear_ptr_vector(tram_loks_vec);
	clear_ptr_vector(tram_waggons_vec);
#if CONVOI_TEMPLATE
	delete template_panel;
#endif
}


// returns position of depot on the map
koord3d depot_frame_t::get_weltpos(bool)
{
	return depot->get_pos();
}


bool depot_frame_t::is_weltpos()
{
	return ( welt->get_viewport()->is_on_center( get_weltpos(false) ) );
}


void depot_frame_t::layout(scr_size *size)
{
	scr_coord placement;
	scr_coord grid;
	scr_coord_val grid_dx;
	scr_coord_val placement_dx;


	/*
	* These parameter are adjusted to resolution.
	* - Some extra space looks nicer.
	*/
	grid.x = depot->get_x_grid() * get_base_tile_raster_width() / 64 + 4;
	grid.y = depot->get_y_grid() * get_base_tile_raster_width() / 64 + 6;
	placement.x = depot->get_x_placement() * get_base_tile_raster_width() / 64 + 2;
	placement.y = depot->get_y_placement() * get_base_tile_raster_width() / 64 + 2;
	grid_dx = depot->get_grid_dx() * get_base_tile_raster_width() / 64 / 2;
	placement_dx = depot->get_placement_dx() * get_base_tile_raster_width() / 64 / 4;

	// calculate some useful default width first (since everything depends on width)
	scr_size win_size = (size != NULL) ? *size : get_windowsize();
	if (win_size.w <= 0) {
		win_size.w = max(D_DEFAULT_WIDTH,(grid.x - grid_dx)*18);
		win_size.h = 0;
	}
	if (size  &&  size->w == 0) {
		size->w = D_DEFAULT_WIDTH;
	}

	/*
	 * Dialog format:
	 *
	 * Main structure are these parts from top to bottom:
	 *
	 *     [SELECT]    convoi-selector
	 *     [CONVOI]    current convoi
	 *     [ACTIONS]   convoi action buttons
	 *     [PANEL]     vehicle panel
	 *     [VINFO]     vehicle infotext
	 *
	 * Structure of [SELECT] is:
	 *
	 *     [Info]
	 *     [PREV][LABEL][NEXT]
	 *
	 *  PREV and NEXT are small buttons - Label is adjusted to total width.
	 */
	const scr_coord_val SELECT_HEIGHT = D_BUTTON_HEIGHT;
	const scr_coord_val selector_x = max(max(max(max(max(102, proportional_string_width(translator::translate("no convois")) + 4),
		proportional_string_width(translator::translate("1 convoi")) + 4),
		proportional_string_width(translator::translate("%d convois")) + 4),
		proportional_string_width(translator::translate("convoi %d of %d")) + 4),
		line_button.get_size().w + 2 + proportional_string_width(translator::translate(lb_convoi_line.get_text_pointer())) + 4
		);

	const scr_coord_val BUTTON_WIDTH_DEPOT = max(D_BUTTON_WIDTH,(win_size.w - D_MARGIN_LEFT - D_MARGIN_RIGHT - 4*D_H_SPACE) / 4);

	/*
	 * Structure of [CONVOI] is a image_list and an infos:
	 *
	 *     [List]
	 *     [Info]
	 *
	 * The image list is horizontally "condensed".
	 */
	const scr_coord_val CLIST_WIDTH = depot->get_max_convoi_length() * (grid.x - grid_dx) + 2 * gui_image_list_t::BORDER;
	const scr_coord_val CLIST_HEIGHT = grid.y + 2 * gui_image_list_t::BORDER + 5;
	const scr_coord_val CINFO_HEIGHT = LINESPACE * 3 + D_BUTTON_HEIGHT * 2 + 1;

	/*
	 * Structure of [ACTIONS] is a row of buttons:
	 *
	 *     [Start][Schedule][Destroy][Sell]
	 *     [new Route][change Route][delete Route]
	 */
	/*
	 * Structure of [VINFO] is one multiline text.
	 */
	const scr_coord_val VINFO_HEIGHT =  7*LINESPACE + D_BUTTON_HEIGHT + D_EDIT_HEIGHT + 5*D_V_SPACE;

	/*
	 *  Now we can do the first vertical adjustment:
	 *  Calculate position of each element to tabs.
	 */
	const scr_coord_val SELECT_VSTART = D_MARGIN_TOP;
	const scr_coord_val CONVOI_VSTART = SELECT_VSTART + LINESPACE + (D_BUTTON_HEIGHT + D_V_SPACE)*2;
	const scr_coord_val CINFO_VSTART = CONVOI_VSTART + CLIST_HEIGHT +  D_SCROLLBAR_HEIGHT*(CLIST_WIDTH >= win_size.w-D_MARGIN_LEFT-D_MARGIN_RIGHT) + D_BUTTON_HEIGHT + D_V_SPACE;
	const scr_coord_val ACTIONS_VSTART = CINFO_VSTART + CINFO_HEIGHT;
	const scr_coord_val PANEL_VSTART = ACTIONS_VSTART + D_BUTTON_HEIGHT;

	/*
	 * Now we determine the row/col layout for the panel and the total panel
	 * size.
	 * build_vehicle_lists() fills loks_vec and waggon_vec.
	 * Total width will be expanded to match complete columns in panel.
	 */
	const scr_coord_val TAB_HEADER_HEIGHT = tabs.get_required_size().h;
	const scr_coord_val total_h = PANEL_VSTART + VINFO_HEIGHT + D_TITLEBAR_HEIGHT + TAB_HEADER_HEIGHT + 2 * gui_image_list_t::BORDER + D_MARGIN_BOTTOM + 1;
	scr_coord_val PANEL_ROWS = max(1, ((win_size.h - total_h) / grid.y));
	if (size  &&  size->h == 0) {
		PANEL_ROWS = 3;
	}
	const scr_coord_val PANEL_HEIGHT = PANEL_ROWS * grid.y + TAB_HEADER_HEIGHT + 2 * gui_image_list_t::BORDER;
	const scr_coord_val MIN_PANEL_HEIGHT = grid.y + TAB_HEADER_HEIGHT + 2 * gui_image_list_t::BORDER;
	const scr_coord_val INFO_VSTART = PANEL_VSTART + PANEL_HEIGHT + div_tabbottom.get_size().h;

	/*
	 *  Now we can do the complete vertical adjustment:
	 */
	const scr_coord_val TOTAL_HEIGHT = PANEL_VSTART + PANEL_HEIGHT + D_V_SPACE + VINFO_HEIGHT + D_TITLEBAR_HEIGHT + 1 + D_MARGIN_BOTTOM;
	const scr_coord_val MIN_TOTAL_HEIGHT = PANEL_VSTART + MIN_PANEL_HEIGHT + D_V_SPACE + VINFO_HEIGHT + D_TITLEBAR_HEIGHT + 1 + D_MARGIN_BOTTOM;

	// now we know also the height, so we can set the size of the whole dialogue
	if(  win_size.h <= 0  ) {
		// set default size on first call
		win_size.h = TOTAL_HEIGHT;
	}
	win_size.h = max(win_size.h, MIN_TOTAL_HEIGHT);
	if(  size  ) {
		*size = win_size;
	}
	gui_frame_t::set_windowsize(win_size);
	set_min_windowsize(scr_size(D_DEFAULT_WIDTH, MIN_TOTAL_HEIGHT));
	const waytype_t wt = depot->get_waytype();
	const bool should_show_child_convoi_selector = (wt != road_wt && wt != air_wt && wt != water_wt);

	/*
	 * DONE with layout planning - now build everything.
	 */

	/*
	 * [NAME OF DEPOT]:
	 */
	depot_name_input.set_pos(scr_coord(D_MARGIN_LEFT, SELECT_VSTART));
	depot_name_input.set_size(scr_size(win_size.w - D_MARGIN_RIGHT - D_MARGIN_LEFT, D_BUTTON_HEIGHT));

	/*
	 * [SELECT]:
	 */
	lb_convois.set_pos(scr_coord(D_MARGIN_LEFT, SELECT_VSTART + D_BUTTON_HEIGHT + D_V_SPACE));
	lb_convois.set_width(selector_x - D_H_SPACE);

	convoy_selector.set_pos(scr_coord(D_MARGIN_LEFT + selector_x, SELECT_VSTART + D_BUTTON_HEIGHT + D_V_SPACE));
	convoy_selector.set_size(scr_size(win_size.w - D_MARGIN_RIGHT - D_MARGIN_LEFT - selector_x, D_BUTTON_HEIGHT));
	convoy_selector.set_max_size(scr_size(win_size.w - D_MARGIN_RIGHT - D_MARGIN_LEFT - selector_x, LINESPACE * 13 + 2 + 16));

	/*
	 * [SELECT ROUTE]:
	 */
	line_button.set_pos(scr_coord(D_MARGIN_LEFT, SELECT_VSTART + (D_BUTTON_HEIGHT + D_V_SPACE)*2));
	lb_convoi_line.set_pos(scr_coord(D_MARGIN_LEFT + line_button.get_size().w + 2, SELECT_VSTART + (D_BUTTON_HEIGHT + D_V_SPACE)*2));
	lb_convoi_line.set_width(selector_x - line_button.get_size().w - 2 - D_H_SPACE);

	line_selector.set_pos(scr_coord(D_MARGIN_LEFT + selector_x, SELECT_VSTART + (D_BUTTON_HEIGHT + D_V_SPACE)*2));
	line_selector.set_size(scr_size(win_size.w - D_MARGIN_RIGHT - D_MARGIN_LEFT - selector_x, D_BUTTON_HEIGHT));
	line_selector.set_max_size(scr_size(win_size.w - D_MARGIN_RIGHT - D_MARGIN_LEFT - selector_x, LINESPACE * 13 + 2 + 16));

	line_button.align_to(&line_selector, ALIGN_CENTER_V);
	lb_convoi_line.align_to(&line_selector, ALIGN_CENTER_V);

	/*
	 * [CONVOI]
	 */
	convoi.set_grid(scr_coord(grid.x - grid_dx, grid.y));
	convoi.set_placement(scr_coord(placement.x - placement_dx, placement.y));
	convoi.set_pos(scr_coord(0, 0));
	convoi.set_size(scr_size(CLIST_WIDTH, CLIST_HEIGHT));

	sb_convoi_length.set_pos(scr_coord(5, grid.y + 5));
	sb_convoi_length.set_size(scr_size(CLIST_WIDTH - 10, 4));

	cont_convoi.set_size(scr_size(CLIST_WIDTH, grid.y + 5 + 4));
	scrolly_convoi.set_size(scr_size(win_size.w - D_MARGIN_LEFT - D_MARGIN_RIGHT, cont_convoi.get_size().h + (3+D_SCROLLBAR_HEIGHT)*(CLIST_WIDTH >= win_size.w-D_MARGIN_LEFT-D_MARGIN_RIGHT) ));
	scrolly_convoi.set_show_scroll_x(true);
	scrolly_convoi.set_show_scroll_y(false);
	scrolly_convoi.set_scroll_discrete_x(false);
	scrolly_convoi.set_size_corner(false);
	scrolly_convoi.set_pos(scr_coord(D_MARGIN_LEFT,CONVOI_VSTART));

	lb_convoi_number.set_width(30);
	lb_convoi_number.set_color(COL_WHITE);

	bt_remove_all_vehicles.set_pos(scr_size(D_MARGIN_LEFT, CONVOI_VSTART + cont_convoi.get_size().h + (3+D_SCROLLBAR_HEIGHT)*(CLIST_WIDTH >= win_size.w-D_MARGIN_LEFT-D_MARGIN_RIGHT) + D_V_SPACE));
	bt_remove_all_vehicles.set_width(BUTTON_WIDTH_DEPOT);

	bt_allow_invalid_convoy.set_pos(scr_size(D_MARGIN_LEFT+D_H_SPACE+BUTTON_WIDTH_DEPOT, CONVOI_VSTART + cont_convoi.get_size().h + (3+D_SCROLLBAR_HEIGHT)*(CLIST_WIDTH >= win_size.w-D_MARGIN_LEFT-D_MARGIN_RIGHT) + D_V_SPACE));
	bt_allow_invalid_convoy.set_width(BUTTON_WIDTH_DEPOT);
	// invalid convoy can not go alone!
	bt_allow_invalid_convoy.set_visible(should_show_child_convoi_selector);

	// place for description text
	second_column_x = D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT+D_H_SPACE*2)*2;
	second_column_w = (BUTTON_WIDTH_DEPOT+D_H_SPACE)*2;

	lb_convoi_count.set_pos(scr_coord(D_MARGIN_LEFT, CINFO_VSTART));
	lb_convoi_count.set_width(second_column_w);

	cont_convoi_capacity.set_pos(scr_coord(second_column_x, CINFO_VSTART));
	cont_convoi_capacity.set_width(second_column_w);

	lb_convoi_cost.set_pos(scr_coord(D_MARGIN_LEFT, CINFO_VSTART + LINESPACE * 1));
	lb_convoi_cost.set_width(second_column_w);
	lb_convoi_value.set_pos(scr_coord(second_column_x, CINFO_VSTART + LINESPACE * 1));
	lb_convoi_value.set_width(second_column_w);

	lb_convoi_power.set_pos(scr_coord(D_MARGIN_LEFT, CINFO_VSTART + LINESPACE * 2));
	lb_convoi_power.set_width(second_column_w);
	lb_convoi_weight.set_pos(scr_coord(second_column_x, CINFO_VSTART + LINESPACE * 2));
	lb_convoi_weight.set_width(second_column_w);

	lb_convoi_speed.set_pos(scr_coord(D_MARGIN_LEFT, CINFO_VSTART + LINESPACE * 3));
	lb_convoi_speed.set_width(win_size.w - D_MARGIN_RIGHT - D_MARGIN_LEFT);
	lb_convoi_terminal_speed.set_pos(scr_coord(second_column_x, CINFO_VSTART + LINESPACE * 3));
	lb_convoi_terminal_speed.set_width(second_column_w);
	lb_convoi_terminal_speed.set_tooltip(translator::translate("terminal speed: to compare acceleration."));

	/*
	 * [ACTIONS]
	 */
	lb_child_convoy.set_visible(should_show_child_convoi_selector);
	child_convoi_selector.set_visible(should_show_child_convoi_selector);
	lb_child_convoy.set_pos(scr_coord(D_MARGIN_LEFT, ACTIONS_VSTART - D_BUTTON_HEIGHT ));
	lb_child_convoy.set_width(BUTTON_WIDTH_DEPOT);
	child_convoi_selector.set_pos(scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT + D_H_SPACE) , ACTIONS_VSTART - D_BUTTON_HEIGHT)); // D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT + D_H_SPACE)*2
	child_convoi_selector.set_size(scr_size(win_size.w - D_MARGIN_RIGHT - ( D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT + D_H_SPACE)*3 + D_H_SPACE ), D_BUTTON_HEIGHT));
	child_convoi_selector.set_max_size(scr_size(win_size.w - D_MARGIN_RIGHT - ( D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT + D_H_SPACE)*3 + D_H_SPACE ), LINESPACE * 13 + 2 + 16));
	bt_uncouple.set_visible(should_show_child_convoi_selector);
	bt_uncouple.set_pos(scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT + D_H_SPACE)*2 ,ACTIONS_VSTART - D_BUTTON_HEIGHT));
	bt_uncouple.set_width(BUTTON_WIDTH_DEPOT);
	bt_reverse.set_pos(scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT + D_H_SPACE)*3 ,ACTIONS_VSTART - D_BUTTON_HEIGHT));
	bt_reverse.set_width(BUTTON_WIDTH_DEPOT);
	bt_reverse.set_visible(env_t::reversible_waytype(wt));

	bt_start.set_pos(scr_coord(D_MARGIN_LEFT, ACTIONS_VSTART));
	bt_start.set_size(scr_size(BUTTON_WIDTH_DEPOT, D_BUTTON_HEIGHT));

	bt_schedule.set_pos(scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT + D_H_SPACE), ACTIONS_VSTART));
	bt_schedule.set_size(scr_size(BUTTON_WIDTH_DEPOT, D_BUTTON_HEIGHT));
	bt_schedule.set_text("Fahrplan");

	bt_copy_convoi.set_pos(scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT + D_H_SPACE)*2, ACTIONS_VSTART));
	bt_copy_convoi.set_size(scr_size(BUTTON_WIDTH_DEPOT, D_BUTTON_HEIGHT));
	bt_copy_convoi.set_text("Copy Convoi");

	bt_sell.set_pos(scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT + D_H_SPACE)*3, ACTIONS_VSTART));
	bt_sell.set_size(scr_size(BUTTON_WIDTH_DEPOT, D_BUTTON_HEIGHT));
	bt_sell.set_text("verkaufen");

	bt_paste_convoi.set_pos(scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT + D_H_SPACE)*2, ACTIONS_VSTART + D_BUTTON_HEIGHT));
	bt_paste_convoi.set_size(scr_size(BUTTON_WIDTH_DEPOT, D_BUTTON_HEIGHT));
	bt_paste_convoi.set_text("Paste Convoi");
	
	bt_replacement_seed.set_pos(scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT + D_H_SPACE)*3, ACTIONS_VSTART + D_BUTTON_HEIGHT));
	bt_replacement_seed.set_size(scr_size(BUTTON_WIDTH_DEPOT, D_BUTTON_HEIGHT));
	bt_replacement_seed.set_text("Replacement seed");

	/*
	* [PANEL]
	*/
	tabs.set_pos(scr_coord(0, PANEL_VSTART));
	tabs.set_size(scr_size(win_size.w, PANEL_HEIGHT));

	pas.set_grid(grid);
	pas.set_placement(placement);
	pas.set_size(tabs.get_size() - scr_size(D_SCROLLBAR_WIDTH, 0));
	pas.recalc_size();
	pas.set_pos(scr_coord(0, 0));
	scrolly_pas.set_size(scrolly_pas.get_size());
	scrolly_pas.set_scroll_amount_y(grid.y);
	scrolly_pas.set_scroll_discrete_y(false);
	scrolly_pas.set_size_corner(false);

	electrics.set_grid(grid);
	electrics.set_placement(placement);
	electrics.set_size(tabs.get_size() - scr_size(D_SCROLLBAR_WIDTH, 0));
	electrics.recalc_size();
	electrics.set_pos(scr_coord(0, 0));
	scrolly_electrics.set_size(scrolly_electrics.get_size());
	scrolly_electrics.set_scroll_amount_y(grid.y);
	scrolly_electrics.set_scroll_discrete_y(false);
	scrolly_electrics.set_size_corner(false);

	loks.set_grid(grid);
	loks.set_placement(placement);
	loks.set_size(tabs.get_size() - scr_size(D_SCROLLBAR_WIDTH, 0));
	loks.recalc_size();
	loks.set_pos(scr_coord(0, 0));
	scrolly_loks.set_size(scrolly_loks.get_size());
	scrolly_loks.set_scroll_amount_y(grid.y);
	scrolly_loks.set_scroll_discrete_y(false);
	scrolly_loks.set_size_corner(false);

	waggons.set_grid(grid);
	waggons.set_placement(placement);
	waggons.set_size(tabs.get_size() - scr_size(D_SCROLLBAR_WIDTH, 0));
	waggons.recalc_size();
	waggons.set_pos(scr_coord(0, 0));
	scrolly_waggons.set_size(scrolly_waggons.get_size());
	scrolly_waggons.set_scroll_amount_y(grid.y);
	scrolly_waggons.set_scroll_discrete_y(false);
	scrolly_waggons.set_size_corner(false);

	tram_pas.set_grid(grid);
	tram_pas.set_placement(placement);
	tram_pas.set_size(tabs.get_size() - scr_size(D_SCROLLBAR_WIDTH, 0));
	tram_pas.recalc_size();
	tram_pas.set_pos(scr_coord(0, 0));
	scrolly_tram_pas.set_size(scrolly_tram_pas.get_size());
	scrolly_tram_pas.set_scroll_amount_y(grid.y);
	scrolly_tram_pas.set_scroll_discrete_y(false);
	scrolly_tram_pas.set_size_corner(false);

	tram_electrics.set_grid(grid);
	tram_electrics.set_placement(placement);
	tram_electrics.set_size(tabs.get_size() - scr_size(D_SCROLLBAR_WIDTH, 0));
	tram_electrics.recalc_size();
	tram_electrics.set_pos(scr_coord(0, 0));
	scrolly_tram_electrics.set_size(scrolly_tram_electrics.get_size());
	scrolly_tram_electrics.set_scroll_amount_y(grid.y);
	scrolly_tram_electrics.set_scroll_discrete_y(false);
	scrolly_tram_electrics.set_size_corner(false);

	tram_loks.set_grid(grid);
	tram_loks.set_placement(placement);
	tram_loks.set_size(tabs.get_size() - scr_size(D_SCROLLBAR_WIDTH, 0));
	tram_loks.recalc_size();
	tram_loks.set_pos(scr_coord(0, 0));
	scrolly_tram_loks.set_size(scrolly_tram_loks.get_size());
	scrolly_tram_loks.set_scroll_amount_y(grid.y);
	scrolly_tram_loks.set_scroll_discrete_y(false);
	scrolly_tram_loks.set_size_corner(false);

	tram_waggons.set_grid(grid);
	tram_waggons.set_placement(placement);
	tram_waggons.set_size(tabs.get_size() - scr_size(D_SCROLLBAR_WIDTH, 0));
	tram_waggons.recalc_size();
	tram_waggons.set_pos(scr_coord(0, 0));
	scrolly_tram_waggons.set_size(scrolly_tram_waggons.get_size());
	scrolly_tram_waggons.set_scroll_amount_y(grid.y);
	scrolly_tram_waggons.set_scroll_discrete_y(false);
	scrolly_tram_waggons.set_size_corner(false);

#if CONVOI_TEMPLATE
	if (template_panel) {
		const scr_coord_val template_content_h = PANEL_HEIGHT - TAB_HEADER_HEIGHT;
		template_panel->set_size(scr_size(win_size.w, template_panel->get_size().h));
		scrolly_template.set_pos(scr_coord(0, 0));
		scrolly_template.set_size(scr_size(win_size.w, template_content_h));
		cont_template_tab.set_size(scr_size(win_size.w, template_content_h));
	}
#endif

	div_tabbottom.set_pos(scr_coord(0, PANEL_VSTART + PANEL_HEIGHT));
	div_tabbottom.set_width(win_size.w);

	/*
	* [BOTTOM]
	*/

	// 1st line
	bt_sell_all.set_pos(scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT+D_H_SPACE)*2-proportional_string_width(translator::translate("Vehicels:"))-D_H_SPACE, INFO_VSTART));
	bt_sell_all.set_size(scr_size(BUTTON_WIDTH_DEPOT, D_BUTTON_HEIGHT));

	bt_veh_action.set_pos(scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT+D_H_SPACE)*3, INFO_VSTART));
	bt_veh_action.set_size(scr_size(BUTTON_WIDTH_DEPOT, D_BUTTON_HEIGHT));
	lb_veh_action.align_to(&bt_veh_action, ALIGN_RIGHT | ALIGN_EXTERIOR_H | ALIGN_CENTER_V, scr_coord(D_H_SPACE, 0));

	// 2nd line
	vehicle_filter.set_pos(scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT+D_H_SPACE)*2 - proportional_string_width(translator::translate("Search:")), INFO_VSTART + D_BUTTON_HEIGHT + D_V_SPACE));
	vehicle_filter.set_size(scr_size(BUTTON_WIDTH_DEPOT, D_BUTTON_HEIGHT));
	vehicle_filter.set_max_size(scr_size(D_BUTTON_WIDTH, LINESPACE*7));
	lb_vehicle_filter.align_to(&vehicle_filter, ALIGN_RIGHT | ALIGN_EXTERIOR_H | ALIGN_TOP, scr_coord(2,D_GET_CENTER_ALIGN_OFFSET(LINESPACE,D_BUTTON_HEIGHT)));

	name_filter_input.set_pos( scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT+D_H_SPACE)*3, INFO_VSTART + D_BUTTON_HEIGHT + D_V_SPACE) );
	name_filter_input.set_size( scr_size( BUTTON_WIDTH_DEPOT, D_EDIT_HEIGHT ) );
	lb_name_filter_input.align_to(&name_filter_input, ALIGN_RIGHT | ALIGN_EXTERIOR_H | ALIGN_TOP, scr_coord(D_H_SPACE,D_GET_CENTER_ALIGN_OFFSET(LINESPACE,D_BUTTON_HEIGHT)));

	bt_obsolete.set_pos(scr_coord(D_MARGIN_LEFT, INFO_VSTART + (D_BUTTON_HEIGHT + D_V_SPACE)*2));
	bt_obsolete.align_to(&name_filter_input, ALIGN_CENTER_V);

	//3rd line
	sort_by.set_pos(scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT+D_H_SPACE)*3, INFO_VSTART + (D_BUTTON_HEIGHT + D_V_SPACE)*2));
	sort_by.set_size(scr_size(BUTTON_WIDTH_DEPOT, D_BUTTON_HEIGHT));
	sort_by.set_max_size(scr_size(D_BUTTON_WIDTH, LINESPACE * 7));
	lb_sort_by.align_to(&sort_by, ALIGN_RIGHT | ALIGN_EXTERIOR_H | ALIGN_TOP, scr_coord(D_H_SPACE,D_GET_CENTER_ALIGN_OFFSET(LINESPACE,D_BUTTON_HEIGHT)));

	bt_show_all.set_pos(scr_coord(D_MARGIN_LEFT, INFO_VSTART + (D_BUTTON_HEIGHT + D_V_SPACE)*2));
//	bt_show_all.align_to(&sort_by, ALIGN_CENTER_TOP); Comboboxes change height when openen!!!

	bt_show_tram.set_pos(scr_coord(D_MARGIN_LEFT + BUTTON_WIDTH_DEPOT + D_H_SPACE, INFO_VSTART + (D_BUTTON_HEIGHT + D_V_SPACE)*2));
	bt_show_tram.set_visible(depot->get_secondary_waytype() != invalid_wt);

	div_action_bottom.set_pos(scr_coord(0, INFO_VSTART + (D_BUTTON_HEIGHT + D_V_SPACE) * 3));
	div_action_bottom.set_width(win_size.w);

	const scr_coord_val margin = 4;
	img_bolt.set_pos(scr_coord(get_windowsize().w - skinverwaltung_t::electricity->get_image(0)->get_pic()->w - margin, margin));

}


void depot_frame_t::set_windowsize( scr_size size )
{
	if (depot) {
		update_data();
		layout(&size);
	}
	gui_frame_t::set_windowsize(size);
}


void depot_frame_t::activate_convoi( convoihandle_t c )
{
	icnv = -1; // deselect
	for(  uint i = 0;  i < depot->convoi_count();  i++  ) {
		if(  c == depot->get_convoi(i)  ) {
			icnv = i;
			break;
		}
	}
	build_vehicle_lists();
}


// true if already stored here
bool depot_frame_t::is_in_vehicle_list(const vehicle_desc_t *info)
{
	FOR(slist_tpl<vehicle_t*>, const v, depot->get_vehicle_list()) {
		if(  v->get_desc() == info  ) {
			return true;
		}
	}
	return false;
}


// add a single vehicle (helper function). is_secondary=true routes to tram tabs.
void depot_frame_t::add_to_vehicle_list(const vehicle_desc_t *info, bool is_secondary)
{
	// Check if vehicle should be filtered
	const goods_desc_t *freight = info->get_freight_type();
	// Only filter when required and never filter engines
	if (depot->selected_filter > 0 && info->get_capacity() > 0) {
		if (depot->selected_filter == VEHICLE_FILTER_RELEVANT) {
			if(freight->get_catg_index() >= 3) {
				bool found = false;
				FOR(vector_tpl<goods_desc_t const*>, const i, welt->get_goods_list()) {
					if (freight->get_catg_index() == i->get_catg_index()) {
						found = true;
						break;
					}
				}

				// If no current goods can be transported by this vehicle, don't display it
				if (!found) return;
			}
		}
		else if (depot->selected_filter > VEHICLE_FILTER_RELEVANT) {
			// Filter on specific selected good
			uint32 goods_index = depot->selected_filter - VEHICLE_FILTER_GOODS_OFFSET;
			if (goods_index < welt->get_goods_list().get_count()) {
				const goods_desc_t *selected_good = welt->get_goods_list()[goods_index];
				if (freight->get_catg_index() != selected_good->get_catg_index()) {
					return; // This vehicle can't transport the selected good
				}
			}
		}
	}

	gui_image_list_t::image_data_t* img_data = new gui_image_list_t::image_data_t(info->get_name(), info->get_base_image());

	if(  is_secondary  ) {
		// Route to tram (secondary waytype) tabs
		if(  info->get_engine_type() == vehicle_desc_t::electric  &&  (info->get_freight_type()==goods_manager_t::passengers  ||  info->get_freight_type()==goods_manager_t::mail)  ) {
			tram_electrics_vec.append(img_data);
		}
		else if(info->get_freight_type() == goods_manager_t::passengers  ||  info->get_freight_type() == goods_manager_t::mail) {
			tram_pas_vec.append(img_data);
		}
		else if(info->get_power() > 0  ||  info->get_capacity()==0) {
			tram_loks_vec.append(img_data);
		}
		else {
			tram_waggons_vec.append(img_data);
		}
		tram_vehicle_map.set(info, img_data);
	}
	else {
		if(  info->get_engine_type() == vehicle_desc_t::electric  &&  (info->get_freight_type()==goods_manager_t::passengers  ||  info->get_freight_type()==goods_manager_t::mail)  ) {
			electrics_vec.append(img_data);
		}
		// since they come "pre-sorted" from the vehikelbauer, we have to do nothing to keep them sorted
		else if(info->get_freight_type() == goods_manager_t::passengers  ||  info->get_freight_type() == goods_manager_t::mail) {
			pas_vec.append(img_data);
		}
		else if(info->get_power() > 0  ||  info->get_capacity()==0) {
			loks_vec.append(img_data);
		}
		else {
			waggons_vec.append(img_data);
		}
		// add reference to map
		vehicle_map.set(info, img_data);
	}
}

// add all current vehicles
void depot_frame_t::build_vehicle_lists()
{
	if (depot->get_vehicle_type().empty()) {
		// there are tracks etc. but no vehicles => do nothing
		// at least initialize some data
		update_data();
		update_tabs();
		return;
	}

	const int month_now = welt->get_timeline_year_month();

	// free vectors
	clear_ptr_vector(pas_vec);
	clear_ptr_vector(electrics_vec);
	clear_ptr_vector(loks_vec);
	clear_ptr_vector(waggons_vec);
	clear_ptr_vector(tram_pas_vec);
	clear_ptr_vector(tram_electrics_vec);
	clear_ptr_vector(tram_loks_vec);
	clear_ptr_vector(tram_waggons_vec);
	// clear maps
	vehicle_map.clear();
	tram_vehicle_map.clear();

	// we do not allow to built electric vehicle in a depot without electrification
	const waytype_t wt = depot->get_waytype();
	const weg_t *w = welt->lookup(depot->get_pos())->get_weg(wt!=tram_wt ? wt : track_wt);
	const bool weg_electrified = w ? w->is_electrified() : false;

	img_bolt.set_image( weg_electrified ? skinverwaltung_t::electricity->get_image_id(0) : IMG_EMPTY );

	sort_by_action = depot->selected_sort_by;

	vector_tpl<const vehicle_desc_t*> typ_list;

	if(!show_all  &&  veh_action==va_sell) {
		// show only sellable vehicles
		FOR(slist_tpl<vehicle_t*>, const v, depot->get_vehicle_list()) {
			vehicle_desc_t const* const d = v->get_desc();
			typ_list.append(d);
		}
	}
	else {
		slist_tpl<const vehicle_desc_t*> const& tmp_list = depot->get_vehicle_type(sort_by_action);
		for(slist_tpl<const vehicle_desc_t*>::const_iterator itr = tmp_list.begin(); itr != tmp_list.end(); ++itr) {
			typ_list.append(*itr);
		}
	}

	const waytype_t sec_wt = depot->get_secondary_waytype();

	// use this to show only sellable vehicles
	if(!show_all  &&  veh_action==va_sell) {
		// just list the one to sell
		FOR(vector_tpl<vehicle_desc_t const*>, const info, typ_list) {
			if (vehicle_map.get(info)) continue;
			if (tram_vehicle_map.get(info)) continue;
			add_to_vehicle_list(info, info->get_waytype() == sec_wt);
		}
	}
	else {
		// list only matching ones
		FOR(vector_tpl<vehicle_desc_t const*>, const info, typ_list) {
			const vehicle_desc_t *veh = NULL;
			convoihandle_t cnv = depot->get_convoi(icnv);
			if(cnv.is_bound() && cnv->get_vehicle_count()>0) {
				veh = (veh_action == va_insert ? cnv->front() : cnv->back())->get_desc();
			}

			// current vehicle
			if( is_in_vehicle_list(info)  ||
				((weg_electrified  ||  info->get_engine_type()!=vehicle_desc_t::electric)  &&
					 ((!info->is_future(month_now))  &&  (show_retired_vehicles  ||  (!info->is_retired(month_now)) )  ) )) {
				// check, if allowed
				bool append = true;
				if(!show_all) {
					if(veh_action == va_insert) {
						append = info->can_lead(veh)  &&  (veh==NULL  ||  veh->can_follow(info));
					} else if(veh_action == va_append) {
						append = info->can_follow(veh)  &&  (veh==NULL  ||  veh->can_lead(info));
					}
				}
				if(append) {
					// name filter. Try to check both object name and translation name (case sensitive though!)
					if(  depot->get_name_filter()[0]==0  ||  (utf8caseutf8(info->get_name(), name_filter_value)  ||  utf8caseutf8(translator::translate(info->get_name()), name_filter_value))  ) {
						add_to_vehicle_list( info );
					}
				}
			}
		}
	}

	// Load secondary waytype vehicles (e.g. tram vehicles in track depot)
	if(  sec_wt != invalid_wt  ) {
		convoihandle_t cnv = depot->get_convoi(icnv);
		const vehicle_desc_t *veh = NULL;
		if(  cnv.is_bound()  &&  cnv->get_vehicle_count() > 0  ) {
			veh = (veh_action == va_insert ? cnv->front() : cnv->back())->get_desc();
		}

		if(  show_all  ||  veh_action != va_sell  )  {
			slist_tpl<const vehicle_desc_t*> const& tram_list = vehicle_builder_t::get_info(sec_wt, sort_by_action);
			for(  slist_tpl<const vehicle_desc_t*>::const_iterator itr = tram_list.begin();  itr != tram_list.end();  ++itr  ) {
				const vehicle_desc_t *info = *itr;
				if(  tram_vehicle_map.get(info)  ) continue;
				if(  is_in_vehicle_list(info)  ||
					((weg_electrified  ||  info->get_engine_type() != vehicle_desc_t::electric)  &&
					 (!info->is_future(month_now))  &&  (show_retired_vehicles  ||  !info->is_retired(month_now))  )  ) {
					bool append = true;
					if(  !show_all  ) {
						if(  veh_action == va_insert  ) {
							append = info->can_lead(veh)  &&  (veh==NULL  ||  veh->can_follow(info));
						}
						else if(  veh_action == va_append  ) {
							append = info->can_follow(veh)  &&  (veh==NULL  ||  veh->can_lead(info));
						}
					}
					if(  append  ) {
						if(  depot->get_name_filter()[0]==0  ||  (utf8caseutf8(info->get_name(), name_filter_value)  ||  utf8caseutf8(translator::translate(info->get_name()), name_filter_value))  ) {
							add_to_vehicle_list(info, true);
						}
					}
				}
			}
		}
	}

DBG_DEBUG("depot_frame_t::build_vehicle_lists()","finally %i passenger vehicle, %i  engines, %i good wagons",pas_vec.get_count(),loks_vec.get_count(),waggons_vec.get_count());
#if CONVOI_TEMPLATE
	if (template_panel) {
		// Determine the boundary vehicle for template compatibility filtering.
		// NULL means no filtering (new convoy, show_all, or allow_invalid mode).
		const vehicle_desc_t *tmpl_boundary_veh = NULL;
		convoihandle_t cnv_tmpl = depot->get_convoi(icnv);
		const bool tmpl_is_insert = (veh_action == va_insert);
		if (!show_all && !bt_allow_invalid_convoy.pressed
		    && cnv_tmpl.is_bound() && cnv_tmpl->get_vehicle_count() > 0
		    && (veh_action == va_append || veh_action == va_insert)) {
			tmpl_boundary_veh = (tmpl_is_insert ? cnv_tmpl->front() : cnv_tmpl->back())->get_desc();
		}
		// Determine which waytype templates to show.
		// Existing convoy: use its waytype. Rail+tram depot with no convoy: use bt_show_tram.
		waytype_t tmpl_target_wt = invalid_wt;
		if (cnv_tmpl.is_bound() && cnv_tmpl->get_vehicle_count() > 0) {
			tmpl_target_wt = cnv_tmpl->front()->get_desc()->get_waytype();
		} else if (depot->get_secondary_waytype() != invalid_wt) {
			tmpl_target_wt = bt_show_tram.pressed ? depot->get_secondary_waytype() : depot->get_waytype();
		}
		template_panel->refresh(name_filter_value, sort_by_action, tmpl_boundary_veh, tmpl_is_insert, weg_electrified, show_all, month_now, show_retired_vehicles, tmpl_target_wt);
	}
#endif
	update_data();
	update_tabs();
}



void depot_frame_t::update_data()
{
	static const char *txt_veh_action[5] = { "anhaengen", "voranstellen", "verkaufen", "set offset", "cancel offset" };

	// change green into blue for retired vehicles
	const int month_now = welt->get_timeline_year_month();

	bt_veh_action.set_text(txt_veh_action[veh_action]);

	txt_convois.clear();
	switch(  depot->convoi_count()  ) {
		case 0: {
			txt_convois.append( translator::translate("no convois") );
			break;
		}
		case 1: {
			if(  icnv == -1  ) {
				txt_convois.append( translator::translate("1 convoi") );
			}
			else {
				txt_convois.printf( translator::translate("convoi %d of %d"), icnv + 1, depot->convoi_count() );
			}
			break;
		}
		default: {
			if(  icnv == -1  ) {
				txt_convois.printf( translator::translate("%d convois"), depot->convoi_count() );
			}
			else {
				txt_convois.printf( translator::translate("convoi %d of %d"), icnv + 1, depot->convoi_count() );
			}
			break;
		}
	}

	/*
	* Reset counts and check for valid vehicles
	*/
	convoihandle_t cnv = depot->get_convoi( icnv );

	// update convoy selector
	convoy_selector.clear_elements();
	convoy_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( new_convoy_text, SYSCOL_TEXT ) ;
	convoy_selector.set_selection(0);
	child_convoi_selector.clear_elements();
	child_convoi_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( no_child_text, SYSCOL_TEXT ) ;
	child_convoi_selector.set_selection(0);
	// This flag is to prohibit child convoy departures without parental permission
	is_shown_convoy_coupled = false;
	is_teleport_to_another_depot = false;

	// check all matching convoys
	FOR(slist_tpl<convoihandle_t>, const c, depot->get_convoy_list()) {
		convoy_selector.new_component<convoy_scrollitem_t>(c) ;
		if(  cnv.is_bound()  &&  c == cnv  ) {
			// this convoy
			convoy_selector.set_selection( convoy_selector.count_elements() - 1 );
			if(  cnv->get_vehicle_count()>0  ) {
				bt_show_tram.pressed = cnv->front()->get_desc()->get_waytype()==tram_wt;
			}
		} 
	}

	// add choices for coupling, find selected child convoy, and find parent convoy of coupling.
	FOR(slist_tpl<convoihandle_t>, const c, depot->get_convoy_list()) {
		if (  !cnv.is_bound()  ||  c == cnv  ) {
			continue;
		}
		// add choices for coupling
		child_convoi_selector.new_component<convoy_scrollitem_t>(c) ;
		if( cnv.is_bound() && c == cnv->get_coupling_convoi() ) {
			// this convoy is cnv's child convoy, cnv will couple with this convoy when departure. 
			child_convoi_selector.set_selection( child_convoi_selector.count_elements() - 1 );
		}
		if( cnv.is_bound() && cnv == c->get_coupling_convoi() ) {
			// this convoy is cnv's parent convoy.
			// therefore, this convoy can not start without parent's permission
			is_shown_convoy_coupled = true;
		}
	}
	if( cnv.is_bound() && !is_shown_convoy_coupled && cnv->get_schedule() && cnv->get_schedule()->get_count()==1) {
		if( grund_t *gr_depot = welt->lookup(cnv->get_schedule()->at(0).pos) ) {
			if( depot_t *dep=gr_depot->get_depot() ) {
				if(dep->can_accept_waytype(cnv->front()->get_desc()->get_waytype())) {
					// this convoy will be teleported to another depot
					is_teleport_to_another_depot = true;
				}
			}
		}
	}
	
	
	// update the description of start/move_to_parent_convoy button
	// if this convoy is child convoy, start button is changed to "move to parent convoy" button.
	if(  !is_shown_convoy_coupled  ) {
		if( is_teleport_to_another_depot ){
			bt_start.set_text("Teleport to Depot");
			bt_start.set_tooltip("Teleport this convoy to another depot(defined in schedule)");
		} else {
			bt_start.set_text("Start");
			bt_start.set_tooltip("Start the selected vehicle(s)");
		}
	} else {
		bt_start.set_tooltip("Move to Parent Convoy");
		bt_start.set_text("Move to Parent Convoy");
	}

	const vehicle_desc_t *veh = NULL;

	clear_ptr_vector( convoi_pics );
	if(  cnv.is_bound()  &&  cnv->get_vehicle_count() > 0  ) {
		for(  unsigned i=0;  i < cnv->get_vehicle_count();  i++  ) {
			// just make sure, there is this vehicle also here!
			const vehicle_desc_t *info=cnv->get_vehikel(i)->get_desc();
			if(  vehicle_map.get( info ) == NULL  &&  tram_vehicle_map.get( info ) == NULL  ) {
				// Add to appropriate list based on waytype
				if(  depot->get_secondary_waytype() != invalid_wt  &&  info->get_waytype() == depot->get_secondary_waytype()  ) {
					add_to_vehicle_list( info, true );
				}
				else {
					add_to_vehicle_list( info );
				}
			}

			gui_image_list_t::image_data_t* img_data = new gui_image_list_t::image_data_t(info->get_name(), info->get_base_image());
			convoi_pics.append(img_data);
		}

		/* color bars for current convoi: */
		convoi_pics[0]->lcolor = color_idx_to_rgb(cnv->front()->get_desc()->can_follow(NULL) ? COL_GREEN : COL_YELLOW);
		{
			unsigned i;
			for(  i = 1;  i < cnv->get_vehicle_count();  i++  ) {
				convoi_pics[i - 1]->rcolor = color_idx_to_rgb(cnv->get_vehikel(i-1)->get_desc()->can_lead(cnv->get_vehikel(i)->get_desc()) ? COL_GREEN : COL_RED);
				convoi_pics[i]->lcolor     = color_idx_to_rgb(cnv->get_vehikel(i)->get_desc()->can_follow(cnv->get_vehikel(i-1)->get_desc()) ? COL_GREEN : COL_RED);
			}
			convoi_pics[i - 1]->rcolor = color_idx_to_rgb(cnv->get_vehikel(i-1)->get_desc()->can_lead(NULL) ? COL_GREEN : COL_YELLOW);
		}

		// change green into blue for vehicles that are not available
		for(  unsigned i = 0;  i < cnv->get_vehicle_count();  i++  ) {
			if(  !cnv->get_vehikel(i)->get_desc()->is_available(month_now)  ) {
				if(  convoi_pics[i]->lcolor == color_idx_to_rgb(COL_GREEN)  ) {
					convoi_pics[i]->lcolor = gui_theme_t::gui_color_obsolete;
				}
				if(  convoi_pics[i]->rcolor == color_idx_to_rgb(COL_GREEN)  ) {
					convoi_pics[i]->rcolor = gui_theme_t::gui_color_obsolete;
				}
			}
		}

		veh = (veh_action == va_insert ? cnv->front() : cnv->back())->get_desc();
		bt_reverse.enable();
		bt_reverse.pressed=cnv->is_reversing_needed();
		bt_uncouple.enable();
		bt_remove_all_vehicles.enable();
		bt_allow_invalid_convoy.pressed|=cnv->is_invalid_convoy();
	}

	repositioning_t& rep = repositioning_t::get_instance();
	
	FOR(vehicle_image_map, const& i, vehicle_map) {
		vehicle_desc_t const* const    info = i.key;
		gui_image_list_t::image_data_t& img  = *i.value;
		const PIXVAL ok_color = info->is_available(month_now) ? color_idx_to_rgb(COL_GREEN) : gui_theme_t::gui_color_obsolete;

		img.count = 0;
		img.lcolor = ok_color;
		img.rcolor = ok_color;

		/*
		* color bars for current convoi:
		*  green/green  okay to append/insert
		*  red/red      cannot be appended/inserted
		*  green/yellow append okay, cannot be end of train
		*  yellow/green insert okay, cannot be start of train
		*/

		if(veh_action == va_insert) {
			if(!info->can_lead(veh)  ||  (veh  &&  !veh->can_follow(info))) {
				img.lcolor = color_idx_to_rgb(COL_RED);
				img.rcolor = color_idx_to_rgb(COL_RED);
			}
			else if(!info->can_follow(NULL)) {
				img.lcolor = color_idx_to_rgb(COL_YELLOW);
			}
		}
		else if(veh_action == va_append) {
			if(!info->can_follow(veh)  ||  (veh  &&  !veh->can_lead(info))) {
				img.lcolor = color_idx_to_rgb(COL_RED);
				img.rcolor = color_idx_to_rgb(COL_RED);
			}
			else if(!info->can_lead(NULL)) {
				img.rcolor = color_idx_to_rgb(COL_YELLOW);
			}
		}
		else if( veh_action == va_sell ) {
			img.lcolor = color_idx_to_rgb(COL_RED);
			img.rcolor = color_idx_to_rgb(COL_RED);
		}
		else if(  veh_action == va_set_offset  ) {
			if(  rep.get_offset(info->get_name())==rep.get_default_offset()  ) {
				// default offset is set.
				img.lcolor = color_idx_to_rgb(COL_RED);
				img.rcolor = color_idx_to_rgb(COL_RED);
			} else if(  rep.get_offset(info->get_name())!=koord(0,0)  ) {
				// offset is other than default but not zero.
				img.lcolor = color_idx_to_rgb(COL_YELLOW);
				img.rcolor = color_idx_to_rgb(COL_YELLOW);
			}
		}
		else if(  veh_action == va_cancel_offset  ) {
			if(  rep.get_offset(info->get_name())==koord(0,0)  ) {
				img.lcolor = color_idx_to_rgb(COL_RED);
				img.rcolor = color_idx_to_rgb(COL_RED);
			}
		}
	}

	// Waytype guard: if the active convoy has tram vehicles, all track vehicles become red (no mixing)
	if(  depot->get_secondary_waytype() != invalid_wt  &&  veh  &&  veh->get_waytype() == depot->get_secondary_waytype()  ) {
		FOR(vehicle_image_map, const& i, vehicle_map) {
			i.value->lcolor = color_idx_to_rgb(COL_RED);
			i.value->rcolor = color_idx_to_rgb(COL_RED);
		}
	}

	// Color bars for tram (secondary waytype) vehicles
	if(  depot->get_secondary_waytype() != invalid_wt  ) {
		const bool track_convoy_active = veh  &&  veh->get_waytype() != depot->get_secondary_waytype();
		FOR(vehicle_image_map, const& i, tram_vehicle_map) {
			vehicle_desc_t const* const    info = i.key;
			gui_image_list_t::image_data_t& img  = *i.value;
			const PIXVAL ok_color = info->is_available(month_now) ? color_idx_to_rgb(COL_GREEN) : gui_theme_t::gui_color_obsolete;

			img.count = 0;
			img.lcolor = ok_color;
			img.rcolor = ok_color;

			// No mixing: if a track convoy is active, all tram vehicles are red
			// Also: replacement seed must only contain vehicles of the depot's primary waytype
			if(  track_convoy_active  ||  (cnv.is_bound()  &&  cnv == depot->get_replacement_seed())  ) {
				img.lcolor = color_idx_to_rgb(COL_RED);
				img.rcolor = color_idx_to_rgb(COL_RED);
			}
			else if(veh_action == va_insert) {
				if(!info->can_lead(veh)  ||  (veh  &&  !veh->can_follow(info))) {
					img.lcolor = color_idx_to_rgb(COL_RED);
					img.rcolor = color_idx_to_rgb(COL_RED);
				}
				else if(!info->can_follow(NULL)) {
					img.lcolor = color_idx_to_rgb(COL_YELLOW);
				}
			}
			else if(veh_action == va_append) {
				if(!info->can_follow(veh)  ||  (veh  &&  !veh->can_lead(info))) {
					img.lcolor = color_idx_to_rgb(COL_RED);
					img.rcolor = color_idx_to_rgb(COL_RED);
				}
				else if(!info->can_lead(NULL)) {
					img.rcolor = color_idx_to_rgb(COL_YELLOW);
				}
			}
			else if( veh_action == va_sell ) {
				img.lcolor = color_idx_to_rgb(COL_RED);
				img.rcolor = color_idx_to_rgb(COL_RED);
			}
			else if(  veh_action == va_set_offset  ) {
				if(  rep.get_offset(info->get_name())==rep.get_default_offset()  ) {
					img.lcolor = color_idx_to_rgb(COL_RED);
					img.rcolor = color_idx_to_rgb(COL_RED);
				} else if(  rep.get_offset(info->get_name())!=koord(0,0)  ) {
					img.lcolor = color_idx_to_rgb(COL_YELLOW);
					img.rcolor = color_idx_to_rgb(COL_YELLOW);
				}
			}
			else if(  veh_action == va_cancel_offset  ) {
				if(  rep.get_offset(info->get_name())==koord(0,0)  ) {
					img.lcolor = color_idx_to_rgb(COL_RED);
					img.rcolor = color_idx_to_rgb(COL_RED);
				}
			}
		}
	}

	FOR(slist_tpl<vehicle_t*>, const v, depot->get_vehicle_list()) {
		// can fail, if currently not visible
		if (gui_image_list_t::image_data_t* const imgdat = vehicle_map.get(v->get_desc())) {
			imgdat->count++;
			if(veh_action == va_sell) {
				imgdat->lcolor = color_idx_to_rgb(COL_GREEN);
				imgdat->rcolor = color_idx_to_rgb(COL_GREEN);
			}
		}
		// also update tram vehicle storage counts
		if (gui_image_list_t::image_data_t* const imgdat = tram_vehicle_map.get(v->get_desc())) {
			imgdat->count++;
			if(veh_action == va_sell) {
				imgdat->lcolor = color_idx_to_rgb(COL_GREEN);
				imgdat->rcolor = color_idx_to_rgb(COL_GREEN);
			}
		}
	}

	// update the line selector
	line_selector.clear_elements();

	// Determine the effective line type based on the current convoy's waytype.
	// A tram convoy in a track depot should see tram lines, not train lines.
	simline_t::linetype effective_line_type = depot->get_line_type();
	if(  cnv.is_bound()  &&  cnv->get_vehicle_count() > 0  ) {
		effective_line_type = simline_t::waytype_to_linetype( cnv->front()->get_desc()->get_waytype() );
	}

	if(  !last_selected_line.is_bound()  ) {
		// new line may have a valid line now
		last_selected_line = selected_line;
		// if still nothing, resort to line management dialoge
		if(  !last_selected_line.is_bound()  ) {
			// try last specific line
			last_selected_line = schedule_list_gui_t::selected_line[ depot->get_owner()->get_player_nr() ][ effective_line_type ];
		}
		if(  !last_selected_line.is_bound()  ) {
			// try last general line
			last_selected_line = schedule_list_gui_t::selected_line[ depot->get_owner()->get_player_nr() ][ 0 ];
			if(  last_selected_line.is_bound()  &&  last_selected_line->get_linetype() != effective_line_type  ) {
				last_selected_line = linehandle_t();
			}
		}
	}
	if(  cnv.is_bound()  &&  cnv->get_schedule()  &&  !cnv->get_schedule()->empty()  ) {
		if(  cnv->get_line().is_bound()  ) {
			line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( clear_schedule_text, SYSCOL_TEXT ) ;
			line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( new_line_text, SYSCOL_TEXT ) ;
		}
		else {
			line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( unique_schedule_text, SYSCOL_TEXT ) ;
			line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( promote_to_line_text, SYSCOL_TEXT ) ;
		}
	}
	else {
		line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( no_schedule_text, SYSCOL_TEXT ) ;
		line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( new_line_text, SYSCOL_TEXT ) ;
	}
	if(  last_selected_line.is_bound()  ) {
		line_selector.new_component<line_scrollitem_t>( last_selected_line ) ;
	}
	if(  !selected_line.is_bound()  ) {
		// select "create new schedule"
		line_selector.set_selection( 0 );
	}
	line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( line_seperator, SYSCOL_TEXT ) ;

	// check all matching lines
	if(  cnv.is_bound()  ) {
		selected_line = cnv->get_line();
	}
	vector_tpl<linehandle_t> lines;
	depot->get_owner()->simlinemgmt.get_lines(effective_line_type, &lines);
	line_selector.set_selection( 0 );
	FOR(  vector_tpl<linehandle_t>,  const line,  lines  ) {
		line_selector.new_component<line_scrollitem_t>(line) ;
		if(  selected_line.is_bound()  &&  selected_line == line  ) {
			line_selector.set_selection( line_selector.count_elements() - 1 );
		}
	}
	if(  line_selector.get_selection() == 0  ) {
		// no line selected
		selected_line = linehandle_t();
	}
	line_selector.sort( last_selected_line.is_bound()+3 );

	// Update vehicle filter
	vehicle_filter.clear_elements();
	vehicle_filter.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("All"), SYSCOL_TEXT);
	vehicle_filter.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Relevant"), SYSCOL_TEXT);

	FOR(vector_tpl<goods_desc_t const*>, const i, welt->get_goods_list()) {
		vehicle_filter.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(i->get_name()), SYSCOL_TEXT);
	}

	if(  depot->selected_filter > vehicle_filter.count_elements()  ) {
		depot->selected_filter = VEHICLE_FILTER_RELEVANT;
	}
	vehicle_filter.set_selection(depot->selected_filter);

	sort_by.clear_elements();
#if CONVOI_TEMPLATE
	// On the Templates tab, power/weight/intro/retire sort modes are not meaningful
	const bool tmpl_tab_active = (tabs.get_aktives_tab() == &cont_template_tab);
#endif
	for(int i = 0; i < vehicle_builder_t::sb_length; i++) {
#if CONVOI_TEMPLATE
		if(  tmpl_tab_active
		  && (i == vehicle_builder_t::sb_power || i == vehicle_builder_t::sb_weight
		      || i == vehicle_builder_t::sb_intro_date || i == vehicle_builder_t::sb_retire_date)  ) {
			continue; // not meaningful for convoy templates
		}
#endif
		sort_by.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(vehicle_builder_t::vehicle_sort_by[i]), SYSCOL_TEXT);
	}
	if(  depot->selected_sort_by > sort_by.count_elements()  ) {
		depot->selected_sort_by = vehicle_builder_t::sb_name;
	}
	sort_by.set_selection(depot->selected_sort_by);

	// finally: update text
	uint32 total_pax = 0;
	uint32 total_mail = 0;
	uint32 total_goods = 0;

	uint64 total_power = 0;
	uint32 total_empty_weight = 0;
	uint32 total_selected_weight = 0;
	uint32 total_max_weight = 0;
	uint32 total_min_weight = 0;
	bool use_sel_weight = true;

	if(  cnv.is_bound()  ) {
		if(  cnv->get_vehicle_count() > 0  ) {
			uint8 selected_good_index = 0;
			if(  depot->selected_filter > VEHICLE_FILTER_RELEVANT  ) {
				// Filter is set to specific good
				const uint32 goods_index = depot->selected_filter - VEHICLE_FILTER_GOODS_OFFSET;
				if(  goods_index < welt->get_goods_list().get_count()  ) {
					selected_good_index = welt->get_goods_list()[goods_index]->get_index();
				}
			}

			for(  unsigned i = 0;  i < cnv->get_vehicle_count();  i++  ) {
				const vehicle_desc_t *desc = cnv->get_vehikel(i)->get_desc();

				total_power += desc->get_power()*desc->get_gear();

				uint32 sel_weight = 0; // actual weight using vehicle filter selected good to fill
				uint32 max_weight = 0;
				uint32 min_weight = 100000;
				bool sel_found = false;
				for(  uint32 j=0;  j<goods_manager_t::get_count();  j++  ) {
					const goods_desc_t *ware = goods_manager_t::get_info(j);

					if(  desc->get_freight_type()->get_catg_index() == ware->get_catg_index()  ) {
						max_weight = max(max_weight, (uint32)ware->get_weight_per_unit());
						min_weight = min(min_weight, (uint32)ware->get_weight_per_unit());

						// find number of goods in in this category. TODO: gotta be a better way...
						uint8 catg_count = 0;
						FOR(vector_tpl<goods_desc_t const*>, const i, welt->get_goods_list()) {
							if(  ware->get_catg_index() == i->get_catg_index()  ) {
								catg_count++;
							}
						}

						if(  ware->get_index() == selected_good_index  ||  catg_count < 2  ) {
							sel_found = true;
							sel_weight = ware->get_weight_per_unit();
						}
					}
				}
				if(  !sel_found  ) {
					// vehicle carries more than one good, but not the selected one
					use_sel_weight = false;
				}

				total_empty_weight += desc->get_weight();
				total_selected_weight += desc->get_weight() + sel_weight * desc->get_capacity();
				total_max_weight += desc->get_weight() + max_weight * desc->get_capacity();
				total_min_weight += desc->get_weight() + min_weight * desc->get_capacity();

				const goods_desc_t* const ware = desc->get_freight_type();
				switch(  ware->get_catg_index()  ) {
					case goods_manager_t::INDEX_PAS: {
						total_pax += desc->get_capacity();
						break;
					}
					case goods_manager_t::INDEX_MAIL: {
						total_mail += desc->get_capacity();
						break;
					}
					default: {
						total_goods += desc->get_capacity();
						break;
					}
				}
			}

			sint32 empty_kmh, sel_kmh, max_kmh, min_kmh, balance_kmh;
			if(  cnv->front()->get_waytype() == air_wt  ) {
				// flying aircraft have 0 friction --> speed not limited by power, so just use top_speed
				empty_kmh = sel_kmh = max_kmh = min_kmh = speed_to_kmh( cnv->get_min_top_speed() );
			}
			else {
				uint64 coupled_total_power=total_power;
				uint64 coupled_total_empty_weight=total_empty_weight;
				uint64 coupled_total_selected_weight=total_selected_weight;
				uint64 coupled_total_max_weight=total_max_weight;
				if(!is_shown_convoy_coupled) {
					convoihandle_t c = cnv->get_coupling_convoi();
					while(c.is_bound()&&c!=cnv) {
						coupled_total_power+=c->get_sum_gear_and_power();
						for(uint8 i=0; i<c->get_vehicle_count(); i++) {
							const vehicle_desc_t *desc = c->get_vehikel(i)->get_desc();
							bool sel_found = false;
							uint32 max_weight=0;
							uint32 sel_weight=0;
							for(  uint32 j=0;  j<goods_manager_t::get_count();  j++  ) {
								const goods_desc_t *ware = goods_manager_t::get_info(j);

								if(  desc->get_freight_type()->get_catg_index() == ware->get_catg_index()  ) {
									max_weight = max(max_weight, (uint32)ware->get_weight_per_unit());

									// find number of goods in in this category. TODO: gotta be a better way...
									uint8 catg_count = 0;
									FOR(vector_tpl<goods_desc_t const*>, const i, welt->get_goods_list()) {
										if(  ware->get_catg_index() == i->get_catg_index()  ) {
											catg_count++;
										}
									}

									if(  ware->get_index() == selected_good_index  ||  catg_count < 2  ) {
										sel_found = true;
										sel_weight = ware->get_weight_per_unit();
									}
								}
							}
							if(  !sel_found  ) {
								// vehicle carries more than one good, but not the selected one
								use_sel_weight = false;
							}
							coupled_total_empty_weight += desc->get_weight();
							coupled_total_selected_weight += desc->get_weight() + sel_weight * desc->get_capacity();
							coupled_total_max_weight += desc->get_weight() + max_weight * desc->get_capacity();
						}
						c = c->get_coupling_convoi();
					}
				}
				empty_kmh = speed_to_kmh(convoi_t::calc_max_speed(coupled_total_power, coupled_total_empty_weight, cnv->get_min_top_speed()));
				sel_kmh =   speed_to_kmh(convoi_t::calc_max_speed(coupled_total_power, coupled_total_selected_weight, cnv->get_min_top_speed()));
				max_kmh =   speed_to_kmh(cnv->get_min_top_speed());
				min_kmh =   speed_to_kmh(convoi_t::calc_max_speed(coupled_total_power, coupled_total_max_weight,   cnv->get_min_top_speed()));
				balance_kmh = speed_to_kmh(convoi_t::calc_max_speed(coupled_total_power, use_sel_weight? coupled_total_selected_weight: coupled_total_max_weight, kmh_to_speed(test_balance_kmh)));
			}

			const sint32 convoi_length = (cnv->get_vehicle_count()) * CARUNITS_PER_TILE / 2 - 1;
			convoi_tile_length_sb = convoi_length + (cnv->get_tile_length() * CARUNITS_PER_TILE - cnv->get_length());

			txt_convoi_count.clear();
			if(  cnv->get_vehicle_count()>1  ) {
				txt_convoi_count.printf( translator::translate("%i car(s),"), cnv->get_vehicle_count() );
			}
			txt_convoi_count.append( translator::translate("Station tiles:") );
			txt_convoi_count.append( (double)cnv->get_tile_length(), 0 );
			char txt_count_real_value[12];
			snprintf(txt_count_real_value, 11, "(%.4f)", ( (double)cnv->get_length() / CARUNITS_PER_TILE ) );
			txt_convoi_count.append(  txt_count_real_value  );

			txt_convoi_speed.clear();
			if(  empty_kmh < 4  ||  empty_kmh != (use_sel_weight ? sel_kmh : min_kmh)  ) {
				// convoi way too slow
				convoi_length_ok_sb = 0;
				if(  max_kmh != min_kmh  &&  !use_sel_weight  ) {
					txt_convoi_speed.printf("%s %d km/h, %d-%d km/h %s", translator::translate("Max. speed:"), empty_kmh, min_kmh, max_kmh, translator::translate("loaded") );
					if(  max_kmh != empty_kmh  || empty_kmh < 4  ) {
						convoi_length_slower_sb = 0;
						convoi_length_too_slow_sb = convoi_length;
					}
					else {
						convoi_length_slower_sb = convoi_length;
						convoi_length_too_slow_sb = 0;
					}
				}
				else {
					txt_convoi_speed.printf("%s %d km/h, %d km/h %s", translator::translate("Max. speed:"), empty_kmh, use_sel_weight ? sel_kmh : min_kmh, translator::translate("loaded") );
					convoi_length_slower_sb = 0;
					convoi_length_too_slow_sb = convoi_length;
				}
				txt_convoi_terminal_speed.printf("%s %d");
			}
			else {
					txt_convoi_speed.printf("%s %d km/h", translator::translate("Max. speed:"), empty_kmh );
					convoi_length_ok_sb = convoi_length;
					convoi_length_slower_sb = 0;
					convoi_length_too_slow_sb = 0;
			}

			{
				char buf[128];
				txt_convoi_value.clear();
				money_to_string(  buf, cnv->calc_restwert() / 100.0, false );
				txt_convoi_value.printf("%s %8s", translator::translate("Restwert:"), buf );

				txt_convoi_cost.clear();
				if(  sint64 fix_cost = cnv->get_fixed_cost()  ) {
					money_to_string(  buf, (double)cnv->get_purchase_cost() / 100.0, false );
					if(env_t::show_yen){
						txt_convoi_cost.printf( translator::translate("Cost: %8s (%d$/km %d$/m)\n"), buf, cnv->get_running_cost(), fix_cost );
					}
					else{
						txt_convoi_cost.printf( translator::translate("Cost: %8s (%.2f$/km %.2f$/m)\n"), buf, (double)cnv->get_running_cost()/100.0, (double)fix_cost/100.0 );
					}
				}
				else {
					money_to_string(  buf, cnv->get_purchase_cost() / 100.0, false );
					if(env_t::show_yen){
						txt_convoi_cost.printf( translator::translate("Cost: %8s (%d$/km)\n"), buf, cnv->get_running_cost() );
					}
					else{
						txt_convoi_cost.printf( translator::translate("Cost: %8s (%.2f$/km)\n"), buf, (double)cnv->get_running_cost() / 100.0 );
					}
				}
			}

			txt_convoi_power.clear();
			txt_convoi_power.printf( translator::translate("Power: %4d kW\n"), cnv->get_sum_power() );

			txt_convoi_weight.clear();
			if(  total_empty_weight != (use_sel_weight ? total_selected_weight : total_max_weight)  ) {
				if(  total_min_weight != total_max_weight  &&  !use_sel_weight  ) {
					txt_convoi_weight.printf("%s %.1ft, %.1f-%.1ft", translator::translate("Weight:"), total_empty_weight / 1000.0, total_min_weight / 1000.0, total_max_weight / 1000.0 );
				}
				else {
					txt_convoi_weight.printf("%s %.1ft, %.1ft", translator::translate("Weight:"), total_empty_weight / 1000.0, (use_sel_weight ? total_selected_weight : total_max_weight) / 1000.0 );
				}
			}
			else {
					txt_convoi_weight.printf("%s %.1ft", translator::translate("Weight:"), total_empty_weight / 1000.0 );
			}
			txt_convoi_terminal_speed.clear();
			if(  cnv->front()->get_waytype() != air_wt  ) {
				if(  balance_kmh<test_balance_kmh  ) {
					txt_convoi_terminal_speed.printf("%s %d km/h", translator::translate("terminal speed:"), balance_kmh);
				} else {
					txt_convoi_terminal_speed.printf(translator::translate("terminal speed: UNLIMITED"));
				}
			}
		}
		else {
			txt_convoi_count.clear();
			txt_convoi_count.append(translator::translate("keine Fahrzeuge"));
			txt_convoi_value.clear();
			txt_convoi_cost.clear();
			txt_convoi_power.clear();
			txt_convoi_weight.clear();
			txt_convoi_terminal_speed.clear();
		}

		sb_convoi_length.set_visible(true);
		cont_convoi_capacity.set_totals( total_pax, total_mail, total_goods );
		cont_convoi_capacity.set_visible(true);
	}
	else {
		txt_convoi_count.clear();
		txt_convoi_speed.clear();
		txt_convoi_value.clear();
		txt_convoi_cost.clear();
		txt_convoi_power.clear();
		txt_convoi_weight.clear();
		txt_convoi_terminal_speed.clear();
		sb_convoi_length.set_visible(false);
		cont_convoi_capacity.set_visible(false);
		bt_reverse.disable();
		bt_uncouple.disable();
		bt_remove_all_vehicles.disable();
	}
}


sint64 depot_frame_t::calc_restwert(const vehicle_desc_t *veh_type)
{
	sint64 wert = 0;
	FOR(slist_tpl<vehicle_t*>, const v, depot->get_vehicle_list()) {
		if(  v->get_desc() == veh_type  ) {
			wert += v->calc_sale_value();
		}
	}
	return wert;
}


void depot_frame_t::image_from_storage_list(gui_image_list_t::image_data_t *image_data)
{
	if(  (image_data->lcolor != color_idx_to_rgb(COL_RED)  &&  image_data->rcolor != color_idx_to_rgb(COL_RED))  ||  (bt_allow_invalid_convoy.pressed)) {
		if(  veh_action == va_set_offset  ) {
			repositioning_t::get_instance().set_offset(image_data->text);
			welt->set_dirty();
		}
		else if(  veh_action == va_cancel_offset  ) {
			repositioning_t::get_instance().cancel_offset(image_data->text);
			welt->set_dirty();
		}
		else if(  veh_action == va_sell  ) {
			depot->call_depot_tool('s', convoihandle_t(), image_data->text );
		}
		else {
			convoihandle_t cnv = depot->get_convoi( icnv );
			if(  !cnv.is_bound()  &&   !depot->get_owner()->is_locked()  ) {
				// adding new convoi, block depot actions until command executed
				// otherwise in multiplayer it's likely multiple convois get created
				// rather than one new convoi with multiple vehicles
				depot->set_command_pending();
			}
			depot->call_depot_tool( veh_action == va_insert ? 'i' : bt_allow_invalid_convoy.pressed? 'A' : 'a', cnv, image_data->text );
		}
	}
}


void depot_frame_t::image_from_convoi_list(uint nr, bool to_end)
{
	const convoihandle_t cnv = depot->get_convoi( icnv );
	if(  cnv.is_bound()  &&  nr < cnv->get_vehicle_count()  ) {
		// we remove all connected vehicles together!
		// find start
		unsigned start_nr = nr;
		while(  start_nr > 0  ) {
			start_nr--;
			const vehicle_desc_t *info = cnv->get_vehikel(start_nr)->get_desc();
			if(  info->get_trailer_count() != 1 || cnv->is_invalid_convoy()  ) {
				start_nr++;
				break;
			}
		}

		cbuffer_t start;
		start.printf("%u", start_nr);

		const char tool = to_end ? 'R' : 'r';
		depot->call_depot_tool( tool, cnv, start );
	}
}


bool depot_frame_t::action_triggered( gui_action_creator_t *comp, value_t p)
{
	convoihandle_t cnv = depot->get_convoi( icnv );

	if(  depot->is_command_pending()  ) {
		// block new commands until last command is executed
		return true;
	}

	if(  comp != NULL  ) { // message from outside!
		if(  comp == &bt_start  ) {
			if(  cnv.is_bound()  ) {
				// Move to Parent Convoy (Not Start Button!)
				if(  is_shown_convoy_coupled  ) {
					for( uint32 i=0; i<depot->get_convoy_list().get_count(); i++ ) {
						convoihandle_t c = depot->get_convoy_list().at(i);
						if( c.is_bound() && cnv.is_bound() && cnv == c->get_coupling_convoi() ) {
							// switch convoy to parent convoy
							icnv = i;
						}
					}
				} else {
					//first: close schedule (will update schedule on clients)
					destroy_win( (ptrdiff_t)cnv->get_schedule() );
					// only then call the tool to start
					char tool = event_get_last_control_shift() == 2 ? 'B' : 'b'; // start all with CTRL-click
					depot->call_depot_tool( tool, cnv, NULL);
				}
			}
		}
		else if(  comp == &bt_schedule  ) {
			if(  line_selector.get_selection() == 1  &&  !line_selector.is_dropped()  ) { // create new line
				// promote existing individual schedule to line
				cbuffer_t buf;
				if(  cnv.is_bound()  &&  !selected_line.is_bound()  ) {
					schedule_t* schedule = cnv->get_schedule();
					if(  schedule  ) {
						schedule->sprintf_schedule( buf );
					}
				}
				depot->call_depot_tool('l', convoihandle_t(), buf);
				return true;
			}
			else {
				open_schedule_editor();
				return true;
			}
		}
		else if(  comp == &line_button  ) {
			if(  cnv.is_bound()  ) {
				cnv->get_owner()->simlinemgmt.show_lineinfo( cnv->get_owner(), cnv->get_line() );
				welt->set_dirty();
			}
		}
		else if(  comp == &bt_sell  ) {
			depot->call_depot_tool(event_get_last_control_shift()==2?'V':'v', cnv, NULL);
		}
		else if(  comp == &bt_replacement_seed  ) {
			depot->call_depot_tool('e', cnv, NULL);
		}
		else if(  comp == &bt_reverse  ) {
			depot->call_depot_tool('t', cnv, NULL);
			bt_reverse.pressed = !bt_reverse.pressed;
			return true;
		}
		else if(  comp == &bt_allow_invalid_convoy  ) {
			bt_allow_invalid_convoy.pressed = !bt_allow_invalid_convoy.pressed;
			return true;
		}
		// image list selection here ...
		else if(  comp == &convoi  ) {
			image_from_convoi_list( p.i, last_meta_event_get_class() == EVENT_DOUBLE_CLICK);
		}
		else if(  comp == &pas  &&  last_meta_event_get_class() != EVENT_DOUBLE_CLICK  ) {
			image_from_storage_list(pas_vec[p.i]);
		}
		else if(  comp == &electrics  &&  last_meta_event_get_class() != EVENT_DOUBLE_CLICK  ) {
			image_from_storage_list(electrics_vec[p.i]);
		}
		else if(  comp == &loks  &&  last_meta_event_get_class() != EVENT_DOUBLE_CLICK  ) {
			image_from_storage_list(loks_vec[p.i]);
		}
		else if(  comp == &waggons  &&  last_meta_event_get_class() != EVENT_DOUBLE_CLICK  ) {
			image_from_storage_list(waggons_vec[p.i]);
		}
		else if(  comp == &tram_pas  &&  last_meta_event_get_class() != EVENT_DOUBLE_CLICK  ) {
			image_from_storage_list(tram_pas_vec[p.i]);
		}
		else if(  comp == &tram_electrics  &&  last_meta_event_get_class() != EVENT_DOUBLE_CLICK  ) {
			image_from_storage_list(tram_electrics_vec[p.i]);
		}
		else if(  comp == &tram_loks  &&  last_meta_event_get_class() != EVENT_DOUBLE_CLICK  ) {
			image_from_storage_list(tram_loks_vec[p.i]);
		}
		else if(  comp == &tram_waggons  &&  last_meta_event_get_class() != EVENT_DOUBLE_CLICK  ) {
			image_from_storage_list(tram_waggons_vec[p.i]);
		}
		else if(  comp == &bt_remove_all_vehicles  ) {
			depot->call_depot_tool(event_get_last_control_shift()==2?'D':'d', cnv, NULL);
		}
		// convoi filters
		else if(  comp == &bt_obsolete  ) {
			show_retired_vehicles = (show_retired_vehicles == 0);
			bt_obsolete.pressed = show_retired_vehicles;
			depot_t::update_all_win();
		}
		else if(  comp == &bt_show_all  ) {
			show_all = (show_all == 0);
			bt_show_all.pressed = show_all;
			depot_t::update_all_win();
		}
		else if(  comp == &bt_show_tram  ) {
			if(  !cnv.is_bound()  ) {
				bt_show_tram.pressed = !bt_show_tram.pressed;
				build_vehicle_lists();
			}
			return true;
		}
		else if(  comp == &bt_sell_all  ) {
			depot->call_depot_tool('S', convoihandle_t(), NULL);
		}
		else if(  comp == &name_filter_input  ) {
			depot->set_name_filter(name_filter_input.get_text());
			depot_t::update_all_win();
		}
		else if(  comp == &tabs  ) {
			// Rebuild sort dropdown to add/remove modes that are meaningless on Templates tab.
			// Reset forbidden sort selection exactly once, here on tab switch to Templates.
#if CONVOI_TEMPLATE
			if(  tabs.get_aktives_tab() == &cont_template_tab  ) {
				using vb = vehicle_builder_t;
				if(  depot->selected_sort_by == vb::sb_power || depot->selected_sort_by == vb::sb_weight
				  || depot->selected_sort_by == vb::sb_intro_date || depot->selected_sort_by == vb::sb_retire_date  ) {
					depot->selected_sort_by = vb::sb_name;
				}
			}
#endif
			update_data();
		}
		else if(  comp == &bt_veh_action  ) {
#if CONVOI_TEMPLATE
			if(  tabs.get_aktives_tab() == &cont_template_tab  ) {
				// Only append/insert are valid when Templates tab is active
				veh_action = (veh_action == va_append) ? va_insert : va_append;
			}
			else
#endif
			if(  veh_action == va_cancel_offset  ||  (get_base_tile_raster_width()!=128  &&  veh_action ==va_sell)  ) {
				veh_action = va_append;
			}
			else {
				veh_action++;
			}
		}
		else if(  comp == &sort_by  ) {
			depot->selected_sort_by = sort_by.get_selection();
		}
		else if(  comp == &bt_copy_convoi  ) {
			if(  cnv.is_bound()  ) {
				if(  !welt->use_timeline()  ||  welt->get_settings().get_allow_buying_obsolete_vehicles()  ||  depot->check_obsolete_inventory( cnv )  ) {
					if(  event_get_last_control_shift() == 2  ) {
						// ctrl pressed -> copy convoy to game clipboard, and also copy vehicle list
						// as convoy template format to system clipboard
						welt->set_copy_convoi(cnv);
						cbuffer_t buf;
						for(  uint16 i = 0;  i < cnv->get_vehicle_count();  i++  ) {
							buf.printf("vehicle[%u]=%s\n", i, cnv->get_vehikel(i)->get_desc()->get_name());
						}
						if(  buf.len() > 0  ) {
							dr_copy(buf, buf.len());
						}
					} else {
						depot->call_depot_tool('c', cnv, NULL);
					}
				}
				else {
					create_win( new news_img("Can't buy obsolete vehicles!"), w_time_delete, magic_none );
				}
			}
			update_data();
			return true;
		}
		else if(  comp == &convoy_selector  ) {
			icnv = p.i - 1;
		}
		else if(  comp == &line_selector  ) {
			const int selection = p.i;
			if(  selection == 0  ) { // unique
				if(  selected_line.is_bound()  ) {
					selected_line = linehandle_t();
					apply_line();
				}
				update_data();
				return true;
			}
			else if(  selection == 1  ) { // create new line
				if(  line_selector.is_dropped()  ) { // but not from next/prev buttons
					// promote existing individual schedule to line
					cbuffer_t buf;
					if(  cnv.is_bound()  &&  !selected_line.is_bound()  ) {
						schedule_t* schedule = cnv->get_schedule();
						if(  schedule  ) {
							schedule->sprintf_schedule( buf );
						}
					}

					last_selected_line = linehandle_t(); // clear last selected line so we can get a new one ...
					depot->call_depot_tool('l', convoihandle_t(), buf);
				}
				update_data();
				return true;
			}
			if(  last_selected_line.is_bound()  ) {
				if(  selection == 2  ) { // last selected line
					selected_line = last_selected_line;
					apply_line();
					update_data();
					return true;
				}
			}

			// access the selected element to get selected line
			line_scrollitem_t *item = dynamic_cast<line_scrollitem_t*>(line_selector.get_element(selection));
			if(  item  ) {
				selected_line = item->get_line();
				depot->set_last_selected_line( selected_line );
				last_selected_line = selected_line;
				apply_line();
				update_data();
				return true;
			}
		}
		else if(  comp == &vehicle_filter  ) {
			depot->selected_filter = vehicle_filter.get_selection();
		}
		else if(  comp == &bt_paste_convoi  ) {
			if(  cnv.is_bound()  &&  (event_get_last_control_shift() == 2)  ) {
				// paste vehicles after this convoi
				convoihandle_t c = welt->get_copy_convoi();
				if(  c.is_bound()  ) {
					const uint8 vehicle_count = c->get_vehicle_count();// avoid infinity loop, we get vehicle length before paste vehicles.
					for (uint8 i=0; i<vehicle_count; i++) {
						depot->call_depot_tool( 'a', cnv, c->get_vehikel(i)->get_desc()->get_name() );
					}
				}
				return true;
			}
			else {
				if(  welt->get_copy_convoi().is_bound()  ) {
					if(  !welt->use_timeline()  ||  welt->get_settings().get_allow_buying_obsolete_vehicles()  ||  depot->check_obsolete_inventory( welt->get_copy_convoi() )  ) {
						depot->call_depot_tool('p', welt->get_copy_convoi(), NULL);
					}
					else {
						create_win( new news_img("Can't buy obsolete vehicles!"), w_time_delete, magic_none );
					}
				}
			}
			return true;
		}
#if CONVOI_TEMPLATE
		else if (comp == template_panel) {
			if (veh_action == va_sell || veh_action == va_set_offset || veh_action == va_cancel_offset) {
				return true;
			}
			const sint32 idx = p.i;
			const gui_template_panel_t::entry_t *entry = template_panel->get_entry(idx);
			if (entry && !entry->tmpl->vehicles.empty()) {
				cbuffer_t veh_buf;
				// Format: <convoi_name>=<prefix>=<veh1>[=<veh2>...]
				// '=' is used as the field separator throughout.
				// convoi_name is applied only when creating a new convoy.
				veh_buf.append(translator::translate(entry->tmpl->name.c_str()));
				veh_buf.append("=");
				const std::vector<std::string> &vehs = entry->tmpl->vehicles;
				if (veh_action == va_insert) {
					veh_buf.append("i");
					for (int i = (int)vehs.size() - 1; i >= 0; i--) {
						veh_buf.append("=");
						veh_buf.append(vehs[i].c_str());
					}
				}
				else {
					veh_buf.append("a");
					for (uint i = 0; i < (uint)vehs.size(); i++) {
						veh_buf.append("=");
						veh_buf.append(vehs[i].c_str());
					}
				}
				if (!cnv.is_bound() && !depot->get_owner()->is_locked()) {
					depot->set_command_pending();
				}
				depot->call_depot_tool('T', cnv, veh_buf);
			}
			return true;
		}
#endif // CONVOI_TEMPLATE
		else if(  comp == &child_convoi_selector  ) {
			if( !cnv.is_bound() ) {
				// this is not convoy.
				return true;
			}
			cbuffer_t couple_buf;
			// selection number should be modified because child_convoi_selector(0) is "departing alone"
			const int selection = p.i <= icnv? p.i-1: p.i;
			uint32 child_convoy_id = selection < 0? 0: depot->get_convoi(selection).get_id();
			// check the ouroboros-like coupling setting:
			// If this convoy's connecting convoy contains itself, this convoy don't start coupling!
			convoihandle_t check_cnv = depot->get_convoi(selection);
			while( check_cnv.is_bound() ) {
				if(  check_cnv->get_coupling_convoi() == cnv  ) {
					// loop found!
					// no coupling
					child_convoy_id = 0;
					child_convoi_selector.set_selection(0);
					create_win( new news_img("This convoy cannot be coupled because it is a parent of shown convoy!"), w_time_delete, magic_none );
					break;
				}
				check_cnv = check_cnv->get_coupling_convoi();
			}
			couple_buf.printf("%u", child_convoy_id);
			depot->call_depot_tool('u',cnv,couple_buf);
			update_data();
			return true;
		}
		else if(  comp == &bt_uncouple  ) {
			if( !cnv.is_bound() ) {
				// this is not convoy.
				return true;
			}
			depot->call_depot_tool('u',cnv,"0");
			update_data();
			return true;
		}
		else if(  comp == &depot_name_input  ) {
			cbuffer_t buf;
			buf.printf(depot_name_input.get_text());
			depot->call_depot_tool('N',convoihandle_t(),buf);
			update_data();
			return true;
		}
		else {
			update_data();
			return false;
		}
		build_vehicle_lists();
		update_data();
		update_tabs();
	}
	else {
		update_data();
		update_tabs();
	}
	layout(NULL);
	return true;
}


bool depot_frame_t::infowin_event(const event_t *ev)
{
	// enable disable button actions
	if(  ev->ev_class < INFOWIN  &&  (depot == NULL  ||  welt->get_active_player() != depot->get_owner()) ) {
		return false;
	}

	const bool swallowed = gui_frame_t::infowin_event(ev);

	if(IS_WINDOW_CHOOSE_NEXT(ev)) {

		bool dir = (ev->ev_code==NEXT_WINDOW);
		depot_t *next_dep = depot_t::find_depot( depot->get_pos(), depot->get_typ(), depot->get_owner(), dir == NEXT_WINDOW );
		if(next_dep == NULL) {
			if(dir == NEXT_WINDOW) {
				// check the next from start of map
				next_dep = depot_t::find_depot( koord3d(-1,-1,0), depot->get_typ(), depot->get_owner(), true );
			}
			else {
				// respective end of map
				next_dep = depot_t::find_depot( koord3d(8192,8192,127), depot->get_typ(), depot->get_owner(), false );
			}
		}

		if(next_dep  &&  next_dep!=this->depot) {
			//  Replace our depot_frame_t with a new at the same position.
			scr_coord const pos = win_get_pos(this);
			destroy_win( this );

			next_dep->show_info();
			win_set_pos(win_get_magic((ptrdiff_t)next_dep), pos.x, pos.y);
			welt->get_viewport()->change_world_position(next_dep->get_pos());
		}
		else {
			// recenter on current depot
			welt->get_viewport()->change_world_position(depot->get_pos());
		}

		return true;
	}

	if (ev->ev_code == WIN_TOP) {
		update_data();
	}

	if(  swallowed  &&  get_focus()==&name_filter_input  &&  (ev->ev_class == EVENT_KEYBOARD  ||  ev->ev_class == EVENT_STRING)  ) {
		depot_t::update_all_win();
	}

	return swallowed;
}


void depot_frame_t::draw(scr_coord pos, scr_size size)
{
	const bool action_allowed = welt->get_active_player() == depot->get_owner();
	convoihandle_t cnv = depot->get_convoi(icnv);

	bt_new_line.enable( action_allowed );
	bt_change_line.enable( action_allowed );
	bt_copy_convoi.enable( action_allowed );
	bt_apply_line.enable( action_allowed );
	bt_start.enable( action_allowed  &&  cnv!=depot->get_replacement_seed() );	
	bt_schedule.enable( action_allowed );
	bt_destroy.enable( action_allowed );
	bt_sell.enable( action_allowed );
	bt_replacement_seed.enable( action_allowed );
	bt_obsolete.enable( action_allowed );
	bt_show_all.enable( action_allowed );
	bt_veh_action.enable( action_allowed );
	bt_sell_all.enable( action_allowed );
	line_button.enable( action_allowed );

	bt_paste_convoi.enable( action_allowed );
	
	bt_replacement_seed.set_text(cnv==depot->get_replacement_seed() ? "Unregister replacement" : "Replacement seed");

	// check for data inconsistencies (can happen with withdraw-all and vehicle in depot)
	if(  !cnv.is_bound()  &&  !convoi_pics.empty()  ) {
		icnv=0;
		update_data();
		cnv = depot->get_convoi(icnv);
	}

	gui_frame_t::draw(pos, size);
	draw_vehicle_info_text(pos);
}


void depot_frame_t::apply_line()
{
	if(  icnv > -1  ) {
		convoihandle_t cnv = depot->get_convoi( icnv );
		// if no convoi is selected, do nothing
		if(  !cnv.is_bound()  ) {
			return;
		}

		if(  selected_line.is_bound()  ) {
			// set new route only, a valid route is selected:
			char id[16];
			sprintf( id, "%i", selected_line.get_id() );
			cnv->call_convoi_tool('l', id );
		}
		else {
			// sometimes the user might wish to remove convoy from line
			// => we clear the schedule completely
			schedule_t *dummy = cnv->create_schedule()->copy();
			dummy->remove_all();

			cbuffer_t buf;
			dummy->sprintf_schedule(buf);
			cnv->call_convoi_tool('g', (const char*)buf );

			delete dummy;
		}
	}
}


void depot_frame_t::open_schedule_editor()
{
	convoihandle_t cnv = depot->get_convoi( icnv );

	if(  cnv.is_bound()  &&  cnv->get_vehicle_count() > 0  ) {
		if(  selected_line.is_bound()  &&  event_get_last_control_shift() == 2  ) { // update line with CTRL-click
			create_win( new line_management_gui_t( selected_line, depot->get_owner() ), w_info, (ptrdiff_t)selected_line.get_rep() );
		}
		else { // edit individual schedule
			// this can happen locally, since any update of the schedule is done during closing window
			schedule_t *schedule = cnv->create_schedule();
			assert(schedule!=NULL);
			gui_frame_t *schedulewin = win_get_magic( (ptrdiff_t)schedule );
			if(  schedulewin == NULL  ) {
				cnv->open_schedule_window( welt->get_active_player() == cnv->get_owner() );
			}
			else {
				top_win( schedulewin );
			}
		}
	}
	else {
		create_win( new news_img("Please choose vehicles first\n"), w_time_delete, magic_none );
	}
}


void depot_frame_t::draw_vehicle_info_text(scr_coord pos)
{
	cbuffer_t buf;

	gui_component_t const* const tab = tabs.get_aktives_tab();
	gui_image_list_t const* lst;
	if      (tab == &scrolly_pas)            lst = &pas;
	else if (tab == &scrolly_electrics)      lst = &electrics;
	else if (tab == &scrolly_loks)           lst = &loks;
	else if (tab == &scrolly_tram_pas)       lst = &tram_pas;
	else if (tab == &scrolly_tram_electrics) lst = &tram_electrics;
	else if (tab == &scrolly_tram_loks)      lst = &tram_loks;
	else if (tab == &scrolly_tram_waggons)   lst = &tram_waggons;
	else                                     lst = &waggons;

#if CONVOI_TEMPLATE
	if (tab == &cont_template_tab) {
		// Show vehicle count in depot
		{
			const char *c;
			switch (const uint32 count = depot->get_vehicle_list().get_count()) {
				case 0: c = translator::translate("Keine Einzelfahrzeuge im Depot"); break;
				case 1: c = translator::translate("1 Einzelfahrzeug im Depot"); break;
				default: buf.printf(translator::translate("%d Einzelfahrzeuge im Depot"), count); c = buf; break;
			}
			display_proportional_clip_rgb(pos.x + D_MARGIN_LEFT, pos.y + D_TITLEBAR_HEIGHT + div_tabbottom.get_pos().y + div_tabbottom.get_size().h + 1, c, ALIGN_LEFT, SYSCOL_TEXT, true);
		}
		const int tmpl_mx = get_mouse_x();
		const int tmpl_my = get_mouse_y();
		const bool over_tabs = tabs.getroffen(tmpl_mx - pos.x, tmpl_my - pos.y - D_TITLEBAR_HEIGHT);
		const int rel_y_in_tabs = tmpl_my - pos.y - D_TITLEBAR_HEIGHT - tabs.get_pos().y;
		const bool over_tab_content = over_tabs && (rel_y_in_tabs >= tabs.get_required_size().h);
		const sint32 hi = (template_panel && over_tab_content) ? template_panel->get_hovered_index() : -1;
		const gui_template_panel_t::entry_t *entry = template_panel ? template_panel->get_entry(hi) : NULL;
		if (entry) {
			sint64 cost = 0;
			uint64 run_cost = 0;
			sint32 min_speed = 0;
			bool any_speed = false;
			uint8 veh_count = 0;
			uint32 total_len_carunits = 0;
			// per-goods capacity (catg==0 grouped by goods pointer, catg>0 grouped by catg)
			struct catg_cap_t { uint8 catg; uint32 cap; const goods_desc_t *goods; };
			std::vector<catg_cap_t> caps;
			for (uint32 j = 0; j < (uint32)entry->descs.size(); j++) {
				const vehicle_desc_t *desc = entry->descs[j];
				if (!desc) continue;
				veh_count++;
				total_len_carunits += desc->get_length();
				cost += desc->get_price();
				run_cost += desc->get_running_cost();
				if (!any_speed || desc->get_topspeed() < min_speed) { min_speed = desc->get_topspeed(); any_speed = true; }
				if (desc->get_capacity() > 0) {
					const goods_desc_t *goods = desc->get_freight_type();
					uint8 catg = goods->get_catg();
					bool found = false;
					for (uint32 k = 0; k < (uint32)caps.size(); k++) {
						bool same = (catg == 0) ? (caps[k].goods == goods) : (caps[k].catg == catg);
						if (same) { caps[k].cap += desc->get_capacity(); found = true; break; }
					}
					if (!found) {
						caps.push_back({ catg, desc->get_capacity(), goods });
					}
				}
			}
			buf.clear();
			// vehicle count and platform length line
			buf.printf(translator::translate("%i car(s),"), veh_count);
			buf.printf("%s %u(%.4f)\n", translator::translate("Station tiles:"),
				(total_len_carunits + CARUNITS_PER_TILE - 1) / CARUNITS_PER_TILE,
				(double)total_len_carunits / CARUNITS_PER_TILE);
			char tmp[128];
			money_to_string(tmp, cost / 100.0, false);
			if (env_t::show_yen) {
				buf.printf(translator::translate("Cost: %8s (%llu$/km)\n"), tmp, run_cost);
			}
			else {
				buf.printf(translator::translate("Cost: %8s (%.2f$/km)\n"), tmp, run_cost / 100.0);
			}
			if (!caps.empty()) {
				buf.printf("%s", translator::translate("Capacity:"));
				for (uint32 k = 0; k < (uint32)caps.size(); k++) {
					const char *catg_name = caps[k].catg == 0
						? translator::translate(caps[k].goods->get_name())
						: translator::translate(caps[k].goods->get_catg_name());
					if (k > 0) buf.printf(",");
					buf.printf(" %u %s", caps[k].cap, catg_name);
				}
				buf.printf("\n");
			}
			buf.printf("%s %d km/h\n", translator::translate("Max. speed:"), min_speed);
			int yyy = pos.y + D_TITLEBAR_HEIGHT + div_action_bottom.get_pos().y + div_action_bottom.get_size().h + 2;
			display_multiline_text_rgb(pos.x + D_MARGIN_LEFT, yyy, buf, SYSCOL_TEXT);
		}
		new_vehicle_length_sb = 0;
		txt_convoi_number.clear();
		return;
	}
#endif // CONVOI_TEMPLATE

	int x = get_mouse_x();
	int y = get_mouse_y();
	double resale_value = -1.0;
	const vehicle_desc_t *veh_type = NULL;
	bool new_vehicle_length_sb_force_zero = false;
	sint16 convoi_number = -1;
	scr_coord relpos = scr_coord(0, ((gui_scrollpane_t *)tabs.get_aktives_tab())->get_scroll_y());
	int sel_index = lst->index_at( pos + tabs.get_pos() - relpos, x, y - D_TITLEBAR_HEIGHT - tabs.get_required_size().h);

	if(  (sel_index != -1)  &&  (tabs.getroffen(x - pos.x, y - pos.y - D_TITLEBAR_HEIGHT))  ) {
		// cursor over a vehicle in the selection list
		const vector_tpl<gui_image_list_t::image_data_t*>& vec =
			lst == &electrics      ? electrics_vec      :
			lst == &pas            ? pas_vec            :
			lst == &loks           ? loks_vec           :
			lst == &tram_pas       ? tram_pas_vec       :
			lst == &tram_electrics ? tram_electrics_vec :
			lst == &tram_loks      ? tram_loks_vec      :
			lst == &tram_waggons   ? tram_waggons_vec   :
			waggons_vec;
		veh_type = vehicle_builder_t::get_info( vec[sel_index]->text );
		if(  vec[sel_index]->lcolor == color_idx_to_rgb(COL_RED)  ||  veh_action == va_sell  ) {
			// don't show new_vehicle_length_sb when can't actually add the highlighted vehicle, or selling from inventory
			new_vehicle_length_sb_force_zero = true;
		}
		if(  vec[sel_index]->count > 0  ) {
			resale_value = (double)calc_restwert( veh_type );
		}
	}
	else {
		// cursor over a vehicle in the convoi
		relpos = scr_coord(scrolly_convoi.get_scroll_x(), 0);

		convoi_number = sel_index = convoi.index_at(pos - relpos + scrolly_convoi.get_pos(), x, y - D_TITLEBAR_HEIGHT);
		if(  sel_index != -1  ) {
			convoihandle_t cnv = depot->get_convoi( icnv );
			veh_type = cnv->get_vehikel( sel_index )->get_desc();
			resale_value = cnv->get_vehikel( sel_index )->calc_sale_value();
			new_vehicle_length_sb_force_zero = true;
		}
	}

	{
		const char *c;
		switch(  const uint32 count = depot->get_vehicle_list().get_count()  ) {
			case 0: {
				c = translator::translate("Keine Einzelfahrzeuge im Depot");
				break;
			}
			case 1: {
				c = translator::translate("1 Einzelfahrzeug im Depot");
				break;
			}
			default: {
				buf.printf( translator::translate("%d Einzelfahrzeuge im Depot"), count );
				c = buf;
				break;
			}
		}
		display_proportional_clip_rgb( pos.x + D_MARGIN_LEFT, pos.y + D_TITLEBAR_HEIGHT + div_tabbottom.get_pos().y + div_tabbottom.get_size().h + 1, c, ALIGN_LEFT, SYSCOL_TEXT, true );
	}

	if(  veh_type  ) {

		// column 1
		buf.clear();
		buf.printf( "%s", translator::translate( veh_type->get_name(), welt->get_settings().get_name_language_id() ) );

		if(  veh_type->get_power() > 0  ) { // LOCO
			buf.printf( " (%s)\n", translator::translate( vehicle_builder_t::engine_type_names[veh_type->get_engine_type()+1] ) );
		}
		else {
			buf.append( "\n" );
		}

		if(  sint64 fix_cost = welt->scale_with_month_length( veh_type->get_fixed_cost() )  ) {
			char tmp[128];
			money_to_string( tmp, veh_type->get_price() / 100.0, false );
			if(env_t::show_yen){
				buf.printf( translator::translate("Cost: %8s (%d$/km %d$/m)\n"), tmp, veh_type->get_running_cost(), fix_cost );
			}
			else{
				buf.printf( translator::translate("Cost: %8s (%.2f$/km %.2f$/m)\n"), tmp, veh_type->get_running_cost()/100.0, fix_cost/100.0 );
			}
		}
		else {
			char tmp[128];
			money_to_string(  tmp, veh_type->get_price() / 100.0, false );
			if(env_t::show_yen){
				buf.printf( translator::translate("Cost: %8s (%d$/km)\n"), tmp, veh_type->get_running_cost() );
			}
			else{
				buf.printf( translator::translate("Cost: %8s (%.2f$/km)\n"), tmp, veh_type->get_running_cost()/100.0 );
			}
		}

		if(  veh_type->get_capacity() > 0  ) { // must translate as "Capacity: %3d%s %s\n"
			buf.printf( translator::translate("Capacity: %d%s %s\n"),
				veh_type->get_capacity(),
				translator::translate( veh_type->get_freight_type()->get_mass() ),
				veh_type->get_freight_type()->get_catg()==0 ? translator::translate( veh_type->get_freight_type()->get_name() ) : translator::translate( veh_type->get_freight_type()->get_catg_name() )
				);
		}
		else {
			buf.append( "\n" );
		}

		buf.printf( "%s %3d km/h\n", translator::translate("Max. speed:"), veh_type->get_topspeed() );

		if(  veh_type->get_power() > 0  ) {
			if(  veh_type->get_gear() != 64  ){
				buf.printf( "%s %4d kW (x%0.2f)\n", translator::translate("Power:"), veh_type->get_power(), veh_type->get_gear() / 64.0 );
			}
			else {
				buf.printf( translator::translate("Power: %4d kW\n"), veh_type->get_power() );
			}
		}

		int yyy = pos.y + D_TITLEBAR_HEIGHT + div_action_bottom.get_pos().y + div_action_bottom.get_size().h + 2;
		display_multiline_text_rgb( pos.x + D_MARGIN_LEFT, yyy, buf, SYSCOL_TEXT);

		// column 2
		buf.clear();
		buf.printf( "%s %4.1ft\n", translator::translate("Weight:"), veh_type->get_weight() / 1000.0 );
		buf.printf( "%s %s - ", translator::translate("Available:"), translator::get_short_date(veh_type->get_intro_year_month() / 12, veh_type->get_intro_year_month() % 12) );

		if(  veh_type->get_retire_year_month() != DEFAULT_RETIRE_DATE * 12  ) {
			buf.printf( "%s\n", translator::get_short_date(veh_type->get_retire_year_month() / 12, veh_type->get_retire_year_month() % 12) );
		}
		else {
			buf.append( "*\n" );
		}

		if(  char const* const copyright = veh_type->get_copyright()  ) {
			buf.printf( translator::translate("Constructed by %s"), copyright );
		}
		buf.append( "\n" );

		if(  resale_value != -1.0  ) {
			char tmp[128];
			money_to_string(  tmp, resale_value / 100.0, false );
			buf.printf( "%s %8s", translator::translate("Restwert:"), tmp );
		}

		display_multiline_text_rgb( pos.x + second_column_x, yyy + LINESPACE, buf, SYSCOL_TEXT);

		// update speedbar
		new_vehicle_length_sb = new_vehicle_length_sb_force_zero ? 0 : convoi_length_ok_sb + convoi_length_slower_sb + convoi_length_too_slow_sb + veh_type->get_length();

		txt_convoi_number.clear();
		if (convoi_number>-1){
			txt_convoi_number.printf("%d", convoi_number + 1);
			lb_convoi_number.set_pos(scr_coord(((depot->get_x_num_grid() * get_base_tile_raster_width() / 64 + 4) - (depot->get_x_num_grid() * get_base_tile_raster_width() / 64 / 2))*convoi_number + 4, 4));
		}
	}
	else {
		txt_convoi_number.clear();
		new_vehicle_length_sb = 0;
	}
}


void depot_frame_t::update_tabs()
{
	gui_component_t *old_tab = tabs.get_aktives_tab();
	tabs.clear();

	bool one = false;
	const bool show_tram_tabs = depot->get_secondary_waytype() != invalid_wt  &&  bt_show_tram.pressed;

	if(  !show_tram_tabs  ) {
		// Track (primary) vehicle tabs
		if(  !electrics_vec.empty()  ) {
			tabs.add_tab(&scrolly_electrics, translator::translate( depot->get_electrics_name() ) );
			one = true;
		}
		if(  !pas_vec.empty()  ) {
			tabs.add_tab(&scrolly_pas, translator::translate( depot->get_passenger_name() ) );
			one = true;
		}
		if(  !loks_vec.empty()  ||  !waggons_vec.empty()  ) {
			tabs.add_tab(&scrolly_loks, translator::translate( depot->get_zieher_name() ) );
			one = true;
		}
		if(  !waggons_vec.empty()  ) {
			tabs.add_tab(&scrolly_waggons, translator::translate( depot->get_haenger_name() ) );
			one = true;
		}
	}
	else {
		// Tram (secondary waytype) tabs
		if(  !tram_electrics_vec.empty()  ) {
			tabs.add_tab(&scrolly_tram_electrics, translator::translate("Tram Electrics"));
			one = true;
		}
		if(  !tram_pas_vec.empty()  ) {
			tabs.add_tab(&scrolly_tram_pas, translator::translate("Tram Passengers"));
			one = true;
		}
		if(  !tram_loks_vec.empty()  ||  !tram_waggons_vec.empty()  ) {
			tabs.add_tab(&scrolly_tram_loks, translator::translate("Trams"));
			one = true;
		}
		if(  !tram_waggons_vec.empty()  ) {
			tabs.add_tab(&scrolly_tram_waggons, translator::translate("Tram Trailers"));
			one = true;
		}
	}

	if(  !one  ) {
		// add passenger as default
		tabs.add_tab(&scrolly_pas, translator::translate( depot->get_passenger_name() ) );
	}

#if CONVOI_TEMPLATE
	// Templates tab is not meaningful in sell/offset modes
	if (template_panel && template_panel->get_count() > 0
	    && veh_action != va_sell && veh_action != va_set_offset && veh_action != va_cancel_offset) {
		tabs.add_tab(&cont_template_tab, translator::translate("Templates"));
	}
#endif

	// Look, if there is our old tab present again (otherwise it will be 0 by tabs.clear()).
	for(  uint8 i = 0;  i < tabs.get_count();  i++  ) {
		if(  old_tab == tabs.get_tab(i)  ) {
			// Found it!
			tabs.set_active_tab_index(i);
			break;
		}
	}
}


depot_convoi_capacity_t::depot_convoi_capacity_t()
{
	total_pax = 0;
	total_mail = 0;
	total_goods = 0;
}


void depot_convoi_capacity_t::set_totals(uint32 pax, uint32 mail, uint32 goods)
{
	total_pax = pax;
	total_mail = mail;
	total_goods = goods;
}


void depot_convoi_capacity_t::draw(scr_coord off)
{
	cbuffer_t cbuf;

	scr_coord_val w = 0;
	cbuf.clear();
	cbuf.printf("%s %d", translator::translate("Capacity:"), total_pax );
	w += display_proportional_clip_rgb( pos.x+off.x + w, pos.y+off.y , cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
	display_color_img( skinverwaltung_t::passengers->get_image_id(0), pos.x + off.x + w, pos.y + off.y, 0, false, false);

	w += 16;
	cbuf.clear();
	cbuf.printf("%d", total_mail );
	w += display_proportional_clip_rgb( pos.x+off.x + w, pos.y+off.y, cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
	display_color_img( skinverwaltung_t::mail->get_image_id(0), pos.x + off.x + w, pos.y + off.y, 0, false, false);

	w += 16;
	cbuf.clear();
	cbuf.printf("%d", total_goods );
	w += display_proportional_clip_rgb( pos.x+off.x + w, pos.y+off.y, cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
	display_color_img( skinverwaltung_t::goods->get_image_id(0), pos.x + off.x + w, pos.y + off.y, 0, false, false);
}


uint32 depot_frame_t::get_rdwr_id()
{
	return magic_depot;
}

void  depot_frame_t::rdwr( loadsave_t *file)
{
	if (file->is_version_less(120, 9)) {
		destroy_win(this);
		return;
	}

	// depot position
	koord3d pos;
	if(  file->is_saving()  ) {
		pos = depot->get_pos();
	}
	pos.rdwr( file );
	// window size
	scr_size size = get_windowsize();
	size.rdwr( file );

	if(  file->is_loading()  ) {
		depot_t *dep = welt->lookup(pos)->get_depot();
		if (dep) {
			init(dep);
		}
	}
	tabs.rdwr(file);
	vehicle_filter.rdwr(file);
	file->rdwr_byte(veh_action);
	file->rdwr_long(icnv);
	if(  file->get_OTRP_version()==51  ) {
		file->rdwr_str(name_filter_value, sizeof(name_filter_value));
		strncpy(name_filter_value,depot->get_name_filter(),sizeof(depot->get_name_filter()));
	}
	sort_by.rdwr(file);
	simline_t::rdwr_linehandle_t(file, selected_line);

	if(  depot  &&  file->is_loading()  ) {
		build_vehicle_lists();
		reset_min_windowsize();
		set_windowsize(size);

		win_set_magic(this, (ptrdiff_t)depot);

		strncpy(name_filter_value,depot->get_name_filter(),sizeof(depot->get_name_filter()));
		name_filter_input.set_text(name_filter_value,sizeof(name_filter_value));
	}

	if (depot == NULL) {
		destroy_win(this);
	}
}
