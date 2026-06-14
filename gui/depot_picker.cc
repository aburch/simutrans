/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "depot_picker.h"
#include "gui_theme.h"
#include "minimap.h"

#include "../player/simplay.h"
#include "../vehicle/simvehicle.h"
#include "../simdepot.h"
#include "../simskin.h"
#include "../simworld.h"
#include "../simconvoi.h"
#include "../simline.h"
#include "../dataobj/translator.h"
#include "../descriptor/skin_desc.h"
#include "../utils/cbuffer_t.h"
#include "../boden/wege/weg.h"
#include "../unicode.h"


static karte_ptr_t welt;


koord3d depot_picker_item_t::sort_origin = koord3d::invalid;
depot_sort_mode_t depot_picker_item_t::sort_mode = DEPOT_SORT_NAME;


static bool is_depot_electrified(depot_t *d)
{
	waytype_t wt = d->get_waytype();
	grund_t *gr = welt->lookup(d->get_pos());
	if (!gr) return false;
	const weg_t *w = gr->get_weg(wt != tram_wt ? wt : track_wt);
	return w && w->is_electrified();
}


static void init_item_layout(depot_picker_item_t *self, depot_t *d,
                             gui_image_t &waytype_symbol,
                             gui_image_t &electrified_symbol,
                             button_t &gotopos)
{
	// layout: [posbutton] [waytype] [electrified] [label]
	gotopos.set_typ(button_t::posbutton_automatic);
	gotopos.set_targetpos3d(d->get_pos());
	self->add_component(&gotopos);

	switch (d->get_waytype()) {
		case maglev_wt:      waytype_symbol.set_image(skinverwaltung_t::maglevhaltsymbol->get_image_id(0),      true); break;
		case monorail_wt:    waytype_symbol.set_image(skinverwaltung_t::monorailhaltsymbol->get_image_id(0),    true); break;
		case track_wt:       waytype_symbol.set_image(skinverwaltung_t::zughaltsymbol->get_image_id(0),         true); break;
		case tram_wt:        waytype_symbol.set_image(skinverwaltung_t::tramhaltsymbol->get_image_id(0),        true); break;
		case narrowgauge_wt: waytype_symbol.set_image(skinverwaltung_t::narrowgaugehaltsymbol->get_image_id(0), true); break;
		case road_wt:        waytype_symbol.set_image(skinverwaltung_t::autohaltsymbol->get_image_id(0),        true); break;
		case water_wt:       waytype_symbol.set_image(skinverwaltung_t::schiffshaltsymbol->get_image_id(0),     true); break;
		case air_wt:         waytype_symbol.set_image(skinverwaltung_t::airhaltsymbol->get_image_id(0),         true); break;
		default: break;
	}
	self->add_component(&waytype_symbol);
	self->add_component(&electrified_symbol);
}


depot_picker_item_t::depot_picker_item_t(depot_t *d, convoihandle_t cnv, bool teleport)
	: depot(d), cnv(cnv), teleport(teleport)
{
	set_table_layout(4, 1);
	init_item_layout(this, d, waytype_symbol, electrified_symbol, gotopos);
	add_component(&label);
	update_label();
	update_electrified();
}


depot_picker_item_t::depot_picker_item_t(depot_t *d, linehandle_t line)
	: depot(d), teleport(true), line(line)
{
	set_table_layout(4, 1);
	init_item_layout(this, d, waytype_symbol, electrified_symbol, gotopos);
	add_component(&label);
	update_label();
	update_electrified();
}


void depot_picker_item_t::update_label()
{
	cbuffer_t &buf = label.buf();
	buf.append(translator::translate(depot->get_name()));
	koord3d p = depot->get_pos();
	buf.printf(" (%d,%d,%d)", p.x, p.y, p.z);
	label.update();
}


void depot_picker_item_t::update_electrified()
{
	const bool electrified = is_depot_electrified(depot);
	electrified_symbol.set_image(
		(electrified && skinverwaltung_t::electricity) ? skinverwaltung_t::electricity->get_image_id(0) : IMG_EMPTY,
		true);
}


void depot_picker_item_t::draw(scr_coord pos)
{
	update_label();
	update_electrified();
	gui_aligned_container_t::draw(pos);
}


bool depot_picker_item_t::is_valid() const
{
	return depot_t::get_depot_list().is_contained(depot);
}


bool depot_picker_item_t::infowin_event(const event_t *ev)
{
	bool swallowed = gui_aligned_container_t::infowin_event(ev);
	if (!swallowed && IS_LEFTRELEASE(ev)) {
		koord3d pos = depot->get_pos();
		char buf[64];
		sprintf(buf, "%d,%d,%d", pos.x, pos.y, pos.z);
		if (cnv.is_bound()) {
			cnv->call_convoi_tool(teleport ? 'Y' : 'D', buf);
		}
		else if (line.is_bound()) {
			for (size_t i = line->get_convoys().get_count(); i-- != 0;) {
				line->get_convoy(i)->call_convoi_tool('Y', buf);
			}
		}
		destroy_win(magic_depot_picker);
		swallowed = true;
	}
	return swallowed;
}


bool depot_picker_item_t::compare(const gui_component_t *aa, const gui_component_t *bb)
{
	const depot_picker_item_t *fa = dynamic_cast<const depot_picker_item_t*>(aa);
	const depot_picker_item_t *fb = dynamic_cast<const depot_picker_item_t*>(bb);
	assert(fa && fb);

	switch (sort_mode) {
		case DEPOT_SORT_ORIGINAL: {
			int cmp = koord_distance(fa->depot->get_pos(), koord(0, 0))
			        - koord_distance(fb->depot->get_pos(), koord(0, 0));
			if (cmp == 0) cmp = fa->depot->get_pos().x - fb->depot->get_pos().x;
			return cmp < 0;
		}
		case DEPOT_SORT_NEAREST:
			if (sort_origin != koord3d::invalid) {
				int da = koord_distance(fa->depot->get_pos().get_2d(), sort_origin.get_2d());
				int db = koord_distance(fb->depot->get_pos().get_2d(), sort_origin.get_2d());
				if (da != db) return da < db;
			}
			break;
		case DEPOT_SORT_ELECTRIFIED: {
			bool ea = is_depot_electrified(fa->depot);
			bool eb = is_depot_electrified(fb->depot);
			if (ea != eb) return ea > eb;  // electrified first
			break;
		}
		case DEPOT_SORT_NAME:
		default:
			break;
	}
	// Secondary / default: sort by name
	return strcmp(translator::translate(fa->depot->get_name()),
	              translator::translate(fb->depot->get_name())) < 0;
}


void depot_picker_t::init_ui(const char *title)
{
	set_name(title);
	set_table_layout(1, 0);

	// Filter row
	add_table(2, 1);
	{
		new_component<gui_label_t>("Filter:");
		filter_input.set_text(name_filter, sizeof(name_filter));
		filter_input.add_listener(this);
		add_component(&filter_input);
	}
	end_table();

	// Sort row
	add_table(2, 1);
	{
		new_component<gui_label_t>("Sort:");
		sort_combo.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Original"), SYSCOL_TEXT);
		sort_combo.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Name"), SYSCOL_TEXT);
		if (has_nearest_sort) {
			sort_combo.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Nearest"), SYSCOL_TEXT);
		}
		sort_combo.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Electrified"), SYSCOL_TEXT);
		sort_combo.set_selection(0);
		sort_combo.add_listener(this);
		add_component(&sort_combo);
	}
	end_table();

	scrolly.set_maximize(true);
	add_component(&scrolly);
	fill_list();
	set_resizemode(diagonal_resize);
	reset_min_windowsize();
}


depot_picker_t::depot_picker_t(convoihandle_t cnv, bool teleport)
	: gui_frame_t("", cnv->get_owner()),
	  scrolly(gui_scrolled_list_t::windowskin, depot_picker_item_t::compare),
	  cnv(cnv), wt(cnv->front()->get_desc()->get_waytype()), owner(cnv->get_owner()),
	  teleport(teleport), has_nearest_sort(true)
{
	depot_picker_item_t::sort_origin = cnv->get_pos();
	depot_picker_item_t::sort_mode   = DEPOT_SORT_ORIGINAL;
	name_filter[0] = '\0';
	init_ui(translator::translate(teleport ? "Teleport to Depot" : "Go to depot"));
}


depot_picker_t::depot_picker_t(linehandle_t line, waytype_t wt, player_t *owner)
	: gui_frame_t("", owner),
	  scrolly(gui_scrolled_list_t::windowskin, depot_picker_item_t::compare),
	  line(line), wt(wt), owner(owner),
	  teleport(true), has_nearest_sort(false)
{
	depot_picker_item_t::sort_origin = koord3d::invalid;
	depot_picker_item_t::sort_mode   = DEPOT_SORT_ORIGINAL;
	name_filter[0] = '\0';
	init_ui(translator::translate("Teleport All to Depot"));
}


bool depot_picker_t::action_triggered(gui_action_creator_t *comp, value_t)
{
	if (comp == &sort_combo) {
		int sel = sort_combo.get_selection();
		if (has_nearest_sort) {
			// combo order: 0=Original, 1=Name, 2=Nearest, 3=Electrified — matches enum directly
			depot_picker_item_t::sort_mode = (depot_sort_mode_t)sel;
		}
		else {
			// combo order without "Nearest": 0=Original, 1=Name, 2=Electrified
			static const depot_sort_mode_t no_nearest_map[] = {
				DEPOT_SORT_ORIGINAL, DEPOT_SORT_NAME, DEPOT_SORT_ELECTRIFIED
			};
			depot_picker_item_t::sort_mode = no_nearest_map[sel];
		}
		fill_list();
	}
	else if (comp == &filter_input) {
		fill_list();
	}
	return true;
}


void depot_picker_t::fill_list()
{
	scrolly.clear_elements();
	vector_tpl<koord> positions;

	FOR(slist_tpl<depot_t*>, const depot, depot_t::get_depot_list()) {
		if (depot->get_owner() != owner || !depot->can_accept_waytype(wt)) {
			continue;
		}
		// Name filter (case-insensitive substring match)
		if (name_filter[0] != '\0') {
			const char *dname = translator::translate(depot->get_name());
			if (!utf8caseutf8(dname, name_filter)) {
				continue;
			}
		}
		positions.append(depot->get_pos().get_2d());
		if (cnv.is_bound()) {
			scrolly.new_component<depot_picker_item_t>(depot, cnv, teleport);
		}
		else if (line.is_bound()) {
			scrolly.new_component<depot_picker_item_t>(depot, line);
		}
	}
	scrolly.sort(0);
	scrolly.set_size(scrolly.get_size());

	if (minimap_t *mm = minimap_t::get_instance()) {
		mm->set_highlighted_depots(positions);
	}
}


depot_picker_t::~depot_picker_t()
{
	if (minimap_t *mm = minimap_t::get_instance()) {
		mm->clear_highlighted_depots();
	}
}
