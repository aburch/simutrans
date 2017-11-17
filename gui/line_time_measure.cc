#include "../simworld.h"
#include "../simhalt.h"
#include "../simline.h"
#include "../dataobj/schedule.h"
#include "../display/viewport.h"

#include "line_time_measure.h"


line_time_measure_t::line_time_measure_t(linehandle_t line_) :
	gui_frame_t("Line Time Measurement", line_->get_owner()),
	line(line_),
	scrolly(&cont),
	txt_info(&buf)
{
	bt_startstop.init(button_t::roundbox_state, "Start / Stop", scr_coord(D_MARGIN_LEFT, D_MARGIN_TOP), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	bt_startstop.add_listener(this);
	cont.add_component(&bt_startstop);
	bt_startstop.pressed = false;

	cont.add_component(&txt_info);

	line_time_measure_info();
	txt_info.set_pos(scr_coord(D_MARGIN_LEFT, D_MARGIN_TOP + D_BUTTON_HEIGHT));

	scrolly.set_pos(scr_coord(0, 0));
	scrolly.set_show_scroll_x(true);
	add_component(&scrolly);

	set_windowsize(scr_size(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+4+22*(LINESPACE)+D_SCROLLBAR_HEIGHT+2));
	set_min_windowsize(scr_size(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+4+3*(LINESPACE)+D_SCROLLBAR_HEIGHT+2));

	set_resizemode(diagonal_resize);
	resize(scr_coord(0,0));
}



line_time_measure_t::~line_time_measure_t()
{
	while(!halt_buttons.empty()) {
		button_t *b = halt_buttons.remove_first();
		cont.remove_component( b );
		delete b;
	}
	while(!halt_name_labels.empty()) {
		gui_label_t *l = halt_name_labels.remove_first();
		cont.remove_component( l );
		delete l;
	}
}

void line_time_measure_t::line_time_measure_info()
{
	if (!line.is_bound()) {
		return;
	}

	while(!halt_buttons.empty()) {
		button_t *b = halt_buttons.remove_first();
		cont.remove_component( b );
		delete b;
	}
	while(!halt_name_labels.empty()) {
		gui_label_t *l = halt_name_labels.remove_first();
		cont.remove_component( l );
		delete l;
	}
	printf("%d", welt->get_ticks());

	buf.clear();
	sint16 offset_y = D_MARGIN_TOP + D_BUTTON_HEIGHT;

	buf.append("Line: ");
	buf.append(line->get_name());
	buf.append("\n");
	offset_y += LINESPACE;
	if (line->is_in_journey_times_measurement()) buf.append("In measurement");
	else buf.append("Not in measurement");
	buf.append("\n\n");
	offset_y += 2 * LINESPACE;

	const minivec_tpl<schedule_entry_t>& entries = line->get_schedule()->entries;
	const uint8 size = entries.get_count();

	for (int i = 0; i < size; i++) {
		halthandle_t const halt = haltestelle_t::get_halt(entries[i].pos, line->get_owner());
		halthandle_t const halt_next = haltestelle_t::get_halt(entries[(i + 1) % size].pos, line->get_owner());
		const id_pair pair(halt.get_id(), halt_next.get_id());

		button_t *b = new button_t();
		b->init(button_t::posbutton, NULL, scr_coord(D_MARGIN_LEFT, offset_y));
		b->set_targetpos(entries[i].pos.get_2d());
		b->add_listener(this);
		cont.add_component(b);
		halt_buttons.append(b);

		gui_label_t *l = new gui_label_t(halt.is_bound() ? halt->get_name() : "Wegpunkt");
		l->set_pos(scr_coord(D_MARGIN_LEFT+D_BUTTON_HEIGHT+D_H_SPACE, offset_y));
		cont.add_component(l);
		halt_name_labels.append(l);
		buf.append("\n");

		buf.append(" | ");
		journey_times_map &map = line->get_average_journey_times_for_measurement();
		const average_tpl<uint32> *average = line->get_average_journey_times_for_measurement().access(pair);
		if (average) {
			const uint32 average_time = average->get_average();
			char time_as_clock[32];
			welt->sprintf_time_tenths(time_as_clock, sizeof(time_as_clock), average_time );
			buf.append(time_as_clock);
		}
		buf.append("\n");
		offset_y += 2 * LINESPACE;
	}

	txt_info.recalc_size();
	scr_size text_size = txt_info.get_size();
	scr_size cont_size = scr_size(text_size.w, D_MARGIN_TOP + D_BUTTON_HEIGHT + text_size.h + D_MARGIN_BOTTOM);
	cont.set_size(cont_size);
	txt_info.set_size(txt_info.get_size());

	cached_schedule_count = line->get_schedule()->get_count();
	cached_line_name = line->get_name();
	update_time = welt->get_ticks();
}



bool line_time_measure_t::action_triggered(gui_action_creator_t *comp, value_t extra)
{
	if(comp == &bt_startstop) {
		if(line.is_bound()) {
			bt_startstop.pressed = !bt_startstop.pressed;
			if (bt_startstop.pressed) {
				line->start_journey_time_measurement();
			}
			else {
				line->stop_journey_time_measurement();
			}
			line_time_measure_info();
		}
	}
	else if (extra.i & ~1) {
		koord k = *(const koord *)extra.p;
		welt->get_viewport()->change_world_position(koord3d(k, welt->max_hgt(k)));
	}

	return true;
}



void line_time_measure_t::draw(scr_coord pos, scr_size size)
{
	if(line.is_bound()) {
		if( line->get_schedule()->get_count()!=cached_schedule_count  || 
			line->get_name()!=cached_line_name || 
		    welt->get_ticks() - update_time > 10000) {
			line_time_measure_info();
			cached_schedule_count = line->get_schedule()->get_count();
			cached_line_name = line->get_name();
			update_time = welt->get_ticks();
		}
	}
	gui_frame_t::draw( pos, size );
}



void line_time_measure_t::set_windowsize(scr_size size)
{
	gui_frame_t::set_windowsize(size);
	scrolly.set_size(get_client_windowsize()-scrolly.get_pos());
}



line_time_measure_t::line_time_measure_t():
	gui_frame_t("", NULL),
	scrolly(&cont),
	txt_info(&buf)
{
	// just a dummy
}
