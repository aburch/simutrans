/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "scenario_info.h"
#include "../world/simworld.h"
#include "../tool/simtool.h"
#include "../display/viewport.h"
#include "../obj/zeiger.h"
#include "../dataobj/scenario.h"
#include "../dataobj/translator.h"
#include "../utils/simstring.h"


static dynamic_string* tab2dyn[scenario_t::SCRIPT_DEBUG + 1];

bool scenario_info_t::update_dynamic_texts(gui_flowtext_t *flow, dynamic_string *text, scr_size size, bool init)
{
	if (text->has_changed()  ||  init) {

		flow->set_text( *text );
		text->clear_changed();
		flow->set_size( size );
		set_dirty();

		return true;
	}
	return false;
}


uint16 scenario_info_t::get_tab_index(const char* which)
{
	const char *shorts[] = { "info", "goal", "rules", "result", "about", "debug" };
	for (uint i = 0; i<lengthof(shorts); i++) {
		if (strcmp(which, shorts[i]) == 0) {
			return i+1 < lengthof(shorts) ? i : tabs.get_count() - 1;
		}
	}
	return 0;
}


scenario_info_t::scenario_info_t() :
	gui_frame_t( translator::translate("Scenario information") )
{
	set_table_layout(1,0);

	int i = 0;
	scenario_t* scen = welt->get_scenario();

	tabs.add_tab(&info, translator::translate("Scenario Info"));
	tab2dyn[i++] = &(scen->info_text);
	tabs.add_tab(&goal, translator::translate("Scenario Goal"));
	tab2dyn[i++] = &(scen->goal_text);
	tabs.add_tab(&rule, translator::translate("Scenario Rules"));
	tab2dyn[i++] = &(scen->rule_text);
	tabs.add_tab(&result, translator::translate("Scenario Result"));
	tab2dyn[i++] = &(scen->result_text);
	tabs.add_tab(&about, translator::translate("About scenario"));
	tab2dyn[i++] = &(scen->about_text);
	tabs.add_tab(&debug_msg, translator::translate("Scenario Debug"));
	tab2dyn[i++] = &(scen->debug_text);

	// cannot update text => possibly opened from sync_step
	hash_text = 0;

	// fetch possible error message
	const char *err_text = welt->get_scenario()->get_error_text();
	if (err_text) {
		tabs.add_tab(&error, translator::translate("Scenario Error Log"));
		error.set_text( err_text );
	}

	add_component(&tabs);
	tabs.add_listener(this);

	scr_coord pane_pos(D_MARGIN_LEFT, D_MARGIN_TOP);
	gui_flowtext_t *texts[] = { &info, &goal, &rule, &result, &about, &error, &debug_msg};
	for(uint32 i=0; i<lengthof(texts); i++) {
		texts[i]->set_pos(pane_pos);
		texts[i]->add_listener(this);
	}

	set_resizemode(diagonal_resize);
	scr_size ms(min(display_get_width() / 2, 500), min(display_get_height() / 2, 300));
	set_min_windowsize(ms);
	set_windowsize(ms);
}


/**
 * fetches actualized texts and resizes flow text element
 */
void scenario_info_t::update_scenario_texts()
{
	// we only update the current tab ...
	scenario_t *scen = welt->get_scenario();
	scr_size border_size = get_client_windowsize() - info.get_pos() - scr_size(D_MARGIN_RIGHT + D_SCROLLBAR_WIDTH, D_MARGIN_BOTTOM + D_SCROLLBAR_HEIGHT);
	int active = tabs.get_active_tab_index();
	gui_flowtext_t* ft = dynamic_cast<gui_flowtext_t*>(tabs.get_aktives_tab());

	// since we do not update the scroll position if the top text did not changed
	uint32 new_hash_text = string_to_hash(*tab2dyn[active], 64);
	int x = ft->get_scroll_x();
	int y = ft->get_scroll_y();
	bool changed = update_dynamic_texts(ft, tab2dyn[active], border_size, hash_text==0);

	const char* d = scen->debug_text;
	debug_msg.set_visible(d && *d);

	if (changed  ||  new_hash_text != hash_text) {
		set_windowsize(get_windowsize());
	}

	if (new_hash_text == hash_text) {
		// first 256 bytes the same => keep scroll position
		ft->set_scroll_position(x, y);
	}

	hash_text = new_hash_text;
}

void scenario_info_t::draw(scr_coord pos, scr_size size)
{
	update_scenario_texts();
	gui_frame_t::draw(pos, size);
}

bool scenario_info_t::action_triggered( gui_action_creator_t *comp, value_t v)
{
	if (  comp == &tabs  ) {
		set_dirty();
		hash_text = 0; // forces redraw
	}
	if (  comp == &info  ||  comp == &goal  ||  comp ==  &rule  ||  comp ==  &result  ||  comp == &about  ||  comp == &debug_msg  ) {
		// parse hyperlink
		const char *link = (const char*)v.p;
		if (link  && *link) {
			if (link[0]=='(') {
				// jump to coordinate
				int x=-1, y=-1, z=-1;
				// try 3d coordinates first
				int n = sscanf(link, "(%i,%i,%i)", &x, &y, &z);
				if (n < 3) { // now try 2d
					n = sscanf(link, "(%i,%i)", &x, &y);
				}
				if (n >= 2) { // at least 2d coordinates supplied
					koord k(x,y);
					welt->get_scenario()->koord_sq2w( k );
					if (welt->is_within_limits(k)) {
						koord3d p(k,z);
						if (n < 3) {
							// take z coordinate from ground
							p = welt->lookup_kartenboden(k)->get_pos();
						}
						welt->get_viewport()->change_world_position( p );
						welt->get_zeiger()->change_pos( p );
						const char* err = welt->get_scenario()->jump_to_link_executed(p);
						if (err) {
							open_error_msg_win(err);
						}
					}
				}
			}
			else if (const char* func = strstart(link, "script:") ) {
				const char* err = welt->get_scenario()->eval_string(func);
				if (err  &&  strcmp(err, "suspended") != 0) {
					dbg->warning("scenario_info_t::action_triggered", "error `%s' when evaluating: %s", err, func);
				}
			}
			else {
				open_tab(link);
			}
		}
	}
	return true;
}


void scenario_info_t::open_tab(const char* which)
{
	tabs.set_active_tab_index(get_tab_index(which));
	hash_text = 0;
	resize(scr_coord(0,0));
	set_dirty();
}


void scenario_info_t::rdwr( loadsave_t *file )
{
	// window size
	scr_size size = get_windowsize();
	size.rdwr( file );

	tabs.rdwr(file);

	if(  file->is_loading()  ) {
		reset_min_windowsize();
		set_windowsize(size);
	}
}
