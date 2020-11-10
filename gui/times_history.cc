/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simworld.h"
#include "../simhalt.h"
#include "../simline.h"
#include "../dataobj/schedule.h"
#include "../display/viewport.h"
#include "../dataobj/translator.h"

#include "times_history.h"


times_history_t::times_history_t(linehandle_t line_, convoihandle_t convoi_) :
	gui_frame_t(NULL, line_.is_bound() ? line_->get_owner() : convoi_->get_owner()),
	owner(line_.is_bound() ? line_->get_owner() : convoi_->get_owner()),
	line(line_),
	convoi(convoi_),
	scrolly(&cont)
{
	register_containers();
	update_components();
	set_name(title_buf);

	scrolly.set_pos(scr_coord(0, 0));
	scrolly.set_show_scroll_x(true);
	add_component(&scrolly);

	set_windowsize(scr_size(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+4+22*(LINESPACE)+D_SCROLLBAR_HEIGHT+2));
	set_min_windowsize(scr_size(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+4+3*(LINESPACE)+D_SCROLLBAR_HEIGHT+2));

	set_resizemode(diagonal_resize);
	resize(scr_coord(0,0));
}



times_history_t::~times_history_t()
{
	while (!history_conts.empty()) {
		times_history_container_t *c = history_conts.remove_first();
		cont.remove_component(c);
		delete c;
	}

	delete last_schedule;
}

void times_history_t::register_containers()
{
	while (!history_conts.empty()) {
		times_history_container_t *c = history_conts.remove_first();
		cont.remove_component(c);
		delete c;
	}

	bool mirrored;
	bool reversed;

	schedule_t *schedule;
	times_history_map *map;
	if (line.is_bound()) {
		schedule = line->get_schedule();
		map = &line->get_journey_times_history();

		mirrored = schedule->is_mirrored();
		reversed = false;
		if (mirrored) {
			times_history_container_t *history_cont = new times_history_container_t(owner, schedule, map, true, false);
			history_conts.append(history_cont);
			cont.add_component(history_cont);
		}
		else {
			times_history_container_t *history_cont_normal = new times_history_container_t(owner, schedule, map, false, false);
			history_conts.append(history_cont_normal);
			cont.add_component(history_cont_normal);
			times_history_container_t *history_cont_reversed = new times_history_container_t(owner, schedule, map, false, true);
			history_conts.append(history_cont_reversed);
			cont.add_component(history_cont_reversed);
		}
	}
	else if (convoi.is_bound()) {
		schedule = convoi->get_schedule();
		map = &convoi->get_journey_times_history();

		mirrored = schedule->is_mirrored();
		reversed = convoi->get_reverse_schedule();
		times_history_container_t *history_cont = new times_history_container_t(owner, schedule, map, mirrored, reversed);
		history_conts.append(history_cont);
		cont.add_component(history_cont);
	}
	else return;

	last_convoy_reversed = reversed;
	if (last_schedule) delete last_schedule;
	last_schedule = schedule->copy();
}

void times_history_t::update_components()
{
	schedule_t *schedule;
	if (line.is_bound()) schedule = line->get_schedule();
	else if (convoi.is_bound()) schedule = convoi->get_schedule();
	else return;

	const char *name = NULL;
	title_buf.clear();
	if (line.is_bound()) {
		title_buf.append(translator::translate("line_times_history"));
		name = line->get_name();
	}
	else if (convoi.is_bound()) {
		title_buf.append(translator::translate("convoy_times_history"));
		name = convoi->get_name();
	}
	title_buf.append(name);

	FOR(slist_tpl<times_history_container_t *>, c, history_conts) {
		c->set_schedule(schedule);
		c->update_container();
	}

	cached_name = name;
	update_time = welt->get_ticks();
}

bool times_history_t::action_triggered(gui_action_creator_t *, value_t)
{
	return true;
}

void times_history_t::draw(scr_coord pos, scr_size size)
{
	 if(line.is_bound()) {
		if ((!line->get_schedule()->empty() && !line->get_schedule()->matches(welt, last_schedule))) {
			register_containers();
		}
		if (line->get_name() != cached_name ||
		    welt->get_ticks() - update_time > 10000) {
			update_components();
		}
	}
	else if (convoi.is_bound()) {
		if ((!convoi->get_schedule()->empty() && !convoi->get_schedule()->matches(welt, last_schedule)) ||
			convoi->get_reverse_schedule() != last_convoy_reversed) {
			register_containers();
		}
		if (convoi->get_name() != cached_name ||
			welt->get_ticks() - update_time > 10000) {
			update_components();
		}
	}

	scr_coord_val y = D_MARGIN_TOP;
	FOR(slist_tpl<times_history_container_t *>, &history_cont, history_conts) {
		history_cont->set_pos(scr_coord(D_MARGIN_LEFT, y));
		history_cont->set_width(get_windowsize().w - D_MARGIN_LEFT - D_MARGIN_RIGHT - D_SCROLLBAR_WIDTH);
		y += history_cont->get_size().h + 2 * D_V_SPACE;
	}

	cont.set_height(y - 2 * D_V_SPACE + D_MARGIN_BOTTOM);

	gui_frame_t::draw( pos, size );
}



void times_history_t::set_windowsize(scr_size size)
{
	gui_frame_t::set_windowsize(size);
	scrolly.set_size(get_client_windowsize()-scrolly.get_pos());
}



times_history_t::times_history_t():
	gui_frame_t("", NULL),
	scrolly(&cont)
{
	// just a dummy
}
