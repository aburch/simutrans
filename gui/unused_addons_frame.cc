/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "unused_addons_frame.h"

#include "../simworld.h"
#include "../simplan.h"
#include "../simconvoi.h"
#include "../simfab.h"
#include "../boden/grund.h"
#include "../boden/wege/weg.h"
#include "../obj/gebaeude.h"
#include "../obj/bruecke.h"
#include "../obj/tunnel.h"
#include "../obj/wayobj.h"
#include "../obj/roadsign.h"
#include "../vehicle/simvehicle.h"
#include "../descriptor/vehicle_desc.h"
#include "../descriptor/tunnel_desc.h"
#include "../bauer/hausbauer.h"
#include "../bauer/vehikelbauer.h"
#include "../bauer/wegbauer.h"
#include "../bauer/brueckenbauer.h"
#include "../bauer/tunnelbauer.h"
#include "../bauer/fabrikbauer.h"
#include "../dataobj/translator.h"
#include "../tpl/stringhashtable_tpl.h"
#include "../utils/cbuffer_t.h"
#include "../sys/simsys.h"

#include "components/gui_divider.h"


unused_addons_frame_t::unused_addons_frame_t() :
	gui_frame_t(translator::translate("Unused addons")),
	scrollpane(&list_container)
{
	build_list();

	set_table_layout(1, 0);

	count_label.buf().printf(translator::translate("%d unused addons found"), (int)entries.get_count());
	count_label.update();
	add_component(&count_label);

	new_component<gui_divider_t>();

	// populate list container before adding to scrollpane
	list_container.set_table_layout(2, 0);
	list_container.set_margin(scr_size(D_H_SPACE, D_V_SPACE), scr_size(D_H_SPACE, D_V_SPACE));
	list_container.set_spacing(scr_size(D_H_SPACE, 1));
	for (uint32 i = 0; i < entries.get_count(); i++) {
		list_container.new_component<gui_label_t>(entries[i].name.c_str());
		list_container.new_component<gui_label_t>(entries[i].type);
	}

	scrollpane.set_scroll_amount_y(D_BUTTON_HEIGHT);
	add_component(&scrollpane);

	new_component<gui_divider_t>();

	copy_btn.init(button_t::roundbox | button_t::flexible, "Copy as CSV");
	copy_btn.add_listener(this);
	add_component(&copy_btn);

	set_resizemode(diagonal_resize);
	reset_min_windowsize();
	set_windowsize(scr_size(D_DEFAULT_WIDTH, D_DEFAULT_HEIGHT));
	resize(scr_coord(0, 0));
}


void unused_addons_frame_t::build_list()
{
	karte_t *welt = world();

	// --- Step 1: collect all descriptor names used on the current map ---
	stringhashtable_tpl<bool> used;

	// Scan all tiles
	for (sint16 y = 0; y < welt->get_size().y; y++) {
		for (sint16 x = 0; x < welt->get_size().x; x++) {
			const planquadrat_t *plan = welt->access(x, y);
			if (!plan) continue;
			for (uint8 b = 0; b < plan->get_boden_count(); b++) {
				const grund_t *gr = plan->get_boden_bei(b);

				// Ways (up to 2 per tile)
				for (uint8 wi = 0; wi < 2; wi++) {
					if (const weg_t *w = gr->get_weg_nr(wi)) {
						used.put(w->get_desc()->get_name(), true);
					}
				}

				// Buildings
				if (const gebaeude_t *gb = gr->find<gebaeude_t>()) {
					used.put(gb->get_tile()->get_desc()->get_name(), true);
				}

				// Bridges
				if (const bruecke_t *brk = gr->find<bruecke_t>()) {
					used.put(brk->get_desc()->get_name(), true);
				}

				// Tunnels
				if (const tunnel_t *tun = gr->find<tunnel_t>()) {
					used.put(tun->get_desc()->get_name(), true);
				}

				// Way objects (catenary etc.)
				if (const wayobj_t *wo = gr->find<wayobj_t>()) {
					used.put(wo->get_desc()->get_name(), true);
				}

				// Road signs / signals
				if (const roadsign_t *rs = gr->find<roadsign_t>()) {
					used.put(rs->get_desc()->get_name(), true);
				}
			}
		}
	}

	// Convoys -> vehicles
	for (uint32 i = 0; i < welt->convoys().get_count(); i++) {
		const convoihandle_t c = welt->convoys()[i];
		for (uint8 vi = 0; vi < c->get_vehicle_count(); vi++) {
			used.put(c->get_vehikel(vi)->get_desc()->get_name(), true);
		}
	}

	// Factories
	FOR(slist_tpl<fabrik_t*>, const fab, welt->get_fab_list()) {
		used.put(fab->get_desc()->get_name(), true);
	}

	// --- Step 2: collect all registered descriptors and find unused ones ---

	// Buildings
	FOR(stringhashtable_tpl<building_desc_t const*>, const& i, hausbauer_t::get_desc_table()) {
		if (!used.get(i.value->get_name())) {
			entry_t e;
			e.name = i.value->get_name();
			e.type = "Building";
			entries.append(e);
		}
	}

	// Vehicles
	FOR(stringhashtable_tpl<vehicle_desc_t const*>, const& i, vehicle_builder_t::get_desc_table()) {
		if (!used.get(i.value->get_name())) {
			entry_t e;
			e.name = i.value->get_name();
			e.type = "Vehicle";
			entries.append(e);
		}
	}

	// Ways
	FOR(stringhashtable_tpl<way_desc_t const*>, const& i, way_builder_t::get_desc_table()) {
		if (!used.get(i.value->get_name())) {
			entry_t e;
			e.name = i.value->get_name();
			e.type = "Way";
			entries.append(e);
		}
	}

	// Bridges
	FOR(stringhashtable_tpl<bridge_desc_t const*>, const& i, bridge_builder_t::get_desc_table()) {
		if (!used.get(i.value->get_name())) {
			entry_t e;
			e.name = i.value->get_name();
			e.type = "Bridge";
			entries.append(e);
		}
	}

	// Tunnels
	FOR(stringhashtable_tpl<tunnel_desc_t*>, const& i, tunnel_builder_t::get_desc_table()) {
		if (!used.get(i.value->get_name())) {
			entry_t e;
			e.name = i.value->get_name();
			e.type = "Tunnel";
			entries.append(e);
		}
	}

	// Way objects
	FOR(stringhashtable_tpl<way_obj_desc_t const*>, const& i, wayobj_t::get_list()) {
		if (!used.get(i.value->get_name())) {
			entry_t e;
			e.name = i.value->get_name();
			e.type = "Wayobj";
			entries.append(e);
		}
	}

	// Road signs
	FOR(stringhashtable_tpl<roadsign_desc_t const*>, const& i, roadsign_t::get_desc_table()) {
		if (!used.get(i.value->get_name())) {
			entry_t e;
			e.name = i.value->get_name();
			e.type = "Roadsign";
			entries.append(e);
		}
	}

	// Factories
	FOR(stringhashtable_tpl<factory_desc_t const*>, const& i, factory_builder_t::get_factory_table()) {
		if (!used.get(i.value->get_name())) {
			entry_t e;
			e.name = i.value->get_name();
			e.type = "Factory";
			entries.append(e);
		}
	}
}


bool unused_addons_frame_t::action_triggered(gui_action_creator_t *comp, value_t)
{
	if (comp == &copy_btn) {
		cbuffer_t buf;
		buf.append("name,type\n");
		for (uint32 i = 0; i < entries.get_count(); i++) {
			buf.append(entries[i].name.c_str());
			buf.append(",");
			buf.append(entries[i].type);
			buf.append("\n");
		}
		if (buf.len() > 0) {
			dr_copy(buf, buf.len());
		}
	}
	return true;
}
