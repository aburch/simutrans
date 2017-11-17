#ifndef gui_line_time_measure_h
#define gui_line_time_measure_h

#include "components/gui_textarea.h"

#include "gui_frame.h"
#include "components/gui_scrollpane.h"
#include "components/gui_label.h"

#include "../linehandle_t.h"
#include "../utils/cbuffer_t.h"

class player_t;

class line_time_measure_t : public gui_frame_t, action_listener_t
{
private:
	linehandle_t line;
	uint32 cached_schedule_count;
	const char* cached_line_name;
	uint32 update_time;

	cbuffer_t buf;

	gui_container_t cont;
	gui_scrollpane_t scrolly;
	gui_textarea_t txt_info;
	button_t bt_startstop;

	slist_tpl<button_t *> halt_buttons;
	slist_tpl<gui_label_t *> halt_name_labels;

public:
	line_time_measure_t(linehandle_t line);

	~line_time_measure_t();

	void line_time_measure_info();

	// const char * get_help_filename() const { return ".txt"; }

	// Set window size and adjust component sizes and/or positions accordingly
	virtual void set_windowsize(scr_size size);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	// only defined to update, if changed
	void draw( scr_coord pos, scr_size size );

	// this constructor is only used during loading
	line_time_measure_t();
};

#endif
