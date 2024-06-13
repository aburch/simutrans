/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_CONVOIINFO_H
#define GUI_COMPONENTS_GUI_CONVOIINFO_H


#include "gui_aligned_container.h"
#include "gui_label.h"
#include "gui_scrolled_list.h"
#include "gui_speedbar.h"
#include "../../convoihandle.h"

/**
 * Convoi info stats, like loading status bar
 * One element of the vehicle list display
 */
class gui_convoiinfo_t : public gui_aligned_container_t, public gui_scrolled_list_t::scrollitem_t
{
private:
	sint64 old_income; // for size updates ...

	/**
	* Handle Convois to be displayed.
	*/
	convoihandle_t cnv;

	gui_speedbar_t filled_bar;
	gui_routebar_t route_bar;
	sint32 cnv_route_index, next_reservation_index;

	gui_label_t label_name, label_next_halt;
	gui_label_buf_t label_line, label_profit;
	button_t pos_next_halt;

public:
	/**
	* @param cnv the handle for the Convoi to be displayed.
	*/
	gui_convoiinfo_t(convoihandle_t cnv);

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	* Draw the component
	*/
	void draw(scr_coord offset) OVERRIDE;

	void update_label();

	const char* get_text() const OVERRIDE;

	bool is_valid() const OVERRIDE { return cnv.is_bound(); }

	convoihandle_t get_cnv() const { return cnv; }

	using gui_aligned_container_t::get_min_size;
	scr_size get_max_size() const OVERRIDE { return scr_size(scr_size::inf.w, get_min_size().h); }
};

#endif
