#ifndef GUI_ROUTE_SEARCH_FRAME_H
#define GUI_ROUTE_SEARCH_FRAME_H

#include "../simhalt.h"
#include "gui_frame.h"
#include "components/action_listener.h"
#include "components/gui_aligned_container.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/gui_textinput.h"

/**
 * Info window for factories
 */
class route_search_frame_t : public gui_frame_t, public action_listener_t
{
 private:
 	gui_textinput_t from_halt_input, dest_halt_input;
	button_t search_button, reverse_search_button;
	gui_label_t from_halt_label, dest_halt_label;
	gui_aligned_container_t result_container;
	char from_halt_input_text[256];
	char dest_halt_input_text[256];

	void search_route();
	void append_connection_row(haltestelle_t::connection_t connection);
	void append_halt_row(halthandle_t halt);
	void swap_halt_inputs();

 public:
	route_search_frame_t();

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
