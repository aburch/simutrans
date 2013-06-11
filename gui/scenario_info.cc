/*
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */
#include "scenario_info.h"
#include "../simworld.h"
#include "../dataobj/scenario.h"
#include "../dataobj/translator.h"

void scenario_info_t::update_dynamic_texts(gui_flowtext_t &flow, dynamic_string &text, koord gr, bool init)
{
	if (text.has_changed()  ||  init) {
		flow.set_text( text );
		text.clear_changed();
		flow.set_groesse( gr );
		flow.set_groesse( flow.get_text_size() );
		set_dirty();
	}
}


scenario_info_t::scenario_info_t(karte_t *welt_) :
	gui_frame_t( translator::translate("Scenario information") ),
	scrolly_info(&info),
	scrolly_goal(&goal),
	scrolly_rule(&rule),
	scrolly_result(&result),
	scrolly_about(&about),
	scrolly_debug(&debug_msg),
	scrolly_error(&error)
{
	tabs.set_pos(koord(0,0));
	tabs.add_tab(&scrolly_info, translator::translate("Scenario Info"));
	tabs.add_tab(&scrolly_goal, translator::translate("Scenario Goal"));
	tabs.add_tab(&scrolly_rule, translator::translate("Scenario Rules"));
	tabs.add_tab(&scrolly_result, translator::translate("Scenario Result"));
	tabs.add_tab(&scrolly_about, translator::translate("About scenario"));
	add_komponente(&tabs);
	tabs.add_listener(this);

	welt = welt_;
	// fetch texts
	update_scenario_texts(true);
	// fetch possible error message
	const char *err_text = welt->get_scenario()->get_error_text();
	if (err_text) {
		tabs.add_tab(&scrolly_error, translator::translate("Scenario Error Log"));
		error.set_text( err_text );
	}

	// add debug panel
	tabs.add_tab(&scrolly_debug, translator::translate("Scenario Debug"));
	debug_msg.set_text( welt->get_scenario()->get_forbidden_text() );

	set_fenstergroesse(koord(300, D_TITLEBAR_HEIGHT + gui_tab_panel_t::HEADER_VSIZE+250));
	set_min_windowsize(koord(40,  D_TITLEBAR_HEIGHT + gui_tab_panel_t::HEADER_VSIZE+10));

	koord pane_pos(D_MARGIN_LEFT, D_MARGIN_TOP);
	gui_flowtext_t *texts[] = { &info, &goal, &rule, &result, &about, &error, &debug_msg};
	for(uint32 i=0; i<lengthof(texts); i++) {
		texts[i]->set_pos(pane_pos);
		texts[i]->add_listener(this);
	}

	gui_scrollpane_t *scrolly[] = { &scrolly_info, &scrolly_goal, &scrolly_rule, &scrolly_result, &scrolly_error, &scrolly_about, &scrolly_debug };
	for(uint32 i=0; i<lengthof(scrolly); i++) {
		scrolly[i]->set_show_scroll_x(true);
	}

	set_resizemode(diagonal_resize);
	resize(koord(0,0));
}



/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void scenario_info_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);
	koord groesse = get_fenstergroesse()-koord(0, D_TITLEBAR_HEIGHT);
	tabs.set_groesse(groesse);

	gui_flowtext_t *texts[] = { &info, &goal, &rule, &result, &about, &error, &debug_msg};
	koord gr = get_client_windowsize() - info.get_pos() - koord(D_MARGIN_RIGHT + scrollbar_t::BAR_SIZE, D_MARGIN_BOTTOM + scrollbar_t::BAR_SIZE);
	for(uint32 i=0; i<lengthof(texts); i++) {
		texts[i]->set_groesse( gr );
		texts[i]->set_groesse( texts[i]->get_text_size() );
	}
}


/**
 * fetches actualized texts and resizes flow text element
 */
void scenario_info_t::update_scenario_texts(bool init)
{
	scenario_t *scen = welt->get_scenario();
	koord border_size = get_client_windowsize() - info.get_pos() - koord(D_MARGIN_RIGHT + scrollbar_t::BAR_SIZE, D_MARGIN_BOTTOM + scrollbar_t::BAR_SIZE);
	if (init) {
		scen->update_scenario_texts();
	}
	update_dynamic_texts( info, scen->info_text, border_size, init);
	update_dynamic_texts( goal, scen->goal_text, border_size, init);
	update_dynamic_texts( rule, scen->rule_text, border_size, init);
	update_dynamic_texts( about, scen->about_text, border_size, init);
	update_dynamic_texts( result, scen->result_text, border_size, init);
}

void scenario_info_t::zeichnen(koord pos, koord gr)
{
	update_scenario_texts(false);
	gui_frame_t::zeichnen(pos, gr);
}

bool scenario_info_t::action_triggered( gui_action_creator_t *komp, value_t v)
{
	if (komp == &tabs) {
		set_dirty();
	}
	if (komp == &info  ||  komp == &goal  ||  komp ==  &rule  ||  komp ==  &result  ||  komp == &about) {
		// parse hyperlink
		const char *link = (const char*)v.p;
		if (link  && *link) {
			if (link[0]=='(') {
				// jump to coordinate
				int x=-1, y=-1;
				int n = sscanf(link, "(%i,%i)", &x, &y);
				if (n==2) {
					koord k(x,y);
					welt->get_scenario()->koord_sq2w( k );
					if (welt->is_within_limits(k)) {
						welt->change_world_position( k  );
					}
				}
			}
			else {
				const char *shorts[] = { "info", "goal", "rules", "result", "about" };
				for (uint i = 0; i<lengthof(shorts); i++) {
					if (strcmp(link, shorts[i]) == 0) {
						tabs.set_active_tab_index(i);
						resize(koord(0,0));
						set_dirty();
						return true;
					}
				}
			}
		}
	}
	return true;
}


void scenario_info_t::open_result_tab()
{
	tabs.set_active_tab_index(3);
	resize(koord(0,0));
	set_dirty();
}
