/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_CONVOIINFO_H
#define GUI_COMPONENTS_GUI_CONVOIINFO_H


#include "gui_aligned_container.h"
#include "gui_label.h"
#include "gui_scrolled_list.h"
#include "gui_speedbar.h"
#include "../../convoihandle_t.h"
#include "gui_convoy_formation.h"
#include "gui_convoy_payloadinfo.h"
#include "gui_image.h"

/**
 * Convoi info stats, like loading status bar
 * One element of the vehicle list display
 */
class gui_convoiinfo_t : public gui_aligned_container_t, public gui_scrolled_list_t::scrollitem_t
{
private:
	/**
	* Handle Convois to be displayed.
	*/
	convoihandle_t cnv;

	gui_loadingbar_t loading_bar;
	gui_label_buf_t label_name, label_line, switchable_label_title, switchable_label_value;
	gui_image_t img_alert, img_operation;

	gui_convoy_formation_t formation;

	uint8 switch_label = 0;
	bool show_line_name = true;

public:
	/**
	* @param cnv, the handler for the Convoi to be displayed.
	*/
	gui_convoiinfo_t(convoihandle_t cnv, bool show_line_name = true);

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	* Draw the component
	*/
	void draw(scr_coord offset) OVERRIDE;

	void set_mode(uint8 mode);
	enum cl_sort_mode_t { by_name = 0, by_profit, by_type, by_id, by_power, CL_SORT_MODES };
	//enum sort_mode_t { by_name = 0, by_schedule, by_profit, by_loading_lvl, by_max_speed, /*by_power,*/ by_value, by_age, SORT_MODES };

	void update_label();

	const char* get_text() const OVERRIDE;

	virtual bool is_valid() const OVERRIDE { return cnv.is_bound(); }

	convoihandle_t get_cnv() const { return cnv; }

	using gui_aligned_container_t::get_min_size;
	using gui_aligned_container_t::get_max_size;
};


#endif
