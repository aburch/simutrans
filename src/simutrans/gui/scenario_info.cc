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

void scenario_info_t::update_dynamic_texts(gui_flowtext_t &flow, dynamic_string &text, scr_size size, bool init)
{
	if (text.has_changed()  ||  init) {
		flow.set_text( text );
		text.clear_changed();
		flow.set_size( size );
		set_dirty();
	}
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

	tabs.add_tab(&info, translator::translate("Scenario Info"));
	tabs.add_tab(&goal, translator::translate("Scenario Goal"));
	tabs.add_tab(&rule, translator::translate("Scenario Rules"));
	tabs.add_tab(&result, translator::translate("Scenario Result"));
	tabs.add_tab(&about, translator::translate("About scenario"));
	add_component(&tabs);
	tabs.add_listener(this);

	// fetch texts
	update_scenario_texts(true);
	// fetch possible error message
	const char *err_text = welt->get_scenario()->get_error_text();
	if (err_text) {
		tabs.add_tab(&error, translator::translate("Scenario Error Log"));
		error.set_text( err_text );
	}

	// add debug panel
	tabs.add_tab(&debug_msg, translator::translate("Scenario Debug"));

	scr_coord pane_pos(D_MARGIN_LEFT, D_MARGIN_TOP);
	gui_flowtext_t *texts[] = { &info, &goal, &rule, &result, &about, &error, &debug_msg};
	for(uint32 i=0; i<lengthof(texts); i++) {
		texts[i]->set_pos(pane_pos);
		texts[i]->add_listener(this);
	}

	set_resizemode(diagonal_resize);
	reset_min_windowsize();
	set_windowsize(scr_size(500, D_TITLEBAR_HEIGHT + D_TAB_HEADER_HEIGHT+300));
}


/**
 * fetches actualized texts and resizes flow text element
 */
void scenario_info_t::update_scenario_texts(bool init)
{
	scenario_t *scen = welt->get_scenario();
	scr_size border_size = get_client_windowsize() - info.get_pos() - scr_size(D_MARGIN_RIGHT + D_SCROLLBAR_WIDTH, D_MARGIN_BOTTOM + D_SCROLLBAR_HEIGHT);
	if (init) {
		scen->update_scenario_texts();
	}
	update_dynamic_texts( info, scen->info_text, border_size, init);
	update_dynamic_texts( goal, scen->goal_text, border_size, init);
	update_dynamic_texts( rule, scen->rule_text, border_size, init);
	update_dynamic_texts( about, scen->about_text, border_size, init);
	update_dynamic_texts( result, scen->result_text, border_size, init);
	update_dynamic_texts( debug_msg, scen->debug_text, border_size, init);
}

void scenario_info_t::draw(scr_coord pos, scr_size size)
{
	update_scenario_texts(false);
	gui_frame_t::draw(pos, size);
}

bool scenario_info_t::action_triggered( gui_action_creator_t *comp, value_t v)
{
	if (  comp == &tabs  ) {
		set_dirty();
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
						koord3d p(x,y,z);
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
