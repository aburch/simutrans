/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Convoi info stats, like loading status bar
 */

#ifndef gui_convoiinfo_h
#define gui_convoiinfo_h

#include "gui_aligned_container.h"
#include "gui_label.h"
#include "gui_scrolled_list.h"
#include "gui_speedbar.h"
#include "../../convoihandle_t.h"

/**
 * One element of the vehicle list display
 *
 * @author Hj. Malthaner
 */
class gui_convoiinfo_t : public gui_aligned_container_t, public gui_scrolled_list_t::scrollitem_t
{
private:
	/**
	* Handle Convois to be displayed.
	* @author Hj. Malthaner
	*/
	convoihandle_t cnv;

	gui_speedbar_t filled_bar;
	gui_label_buf_t label_name, label_profit, label_line;

public:
	/**
	* @param cnv, the handler for the Convoi to be displayed.
	* @author Hj. Malthaner
	*/
	gui_convoiinfo_t(convoihandle_t cnv);

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	* Draw the component
	* @author Hj. Malthaner
	*/
	void draw(scr_coord offset) OVERRIDE;

	void update_label();

	const char* get_text() const OVERRIDE;

	bool is_valid() const OVERRIDE { return cnv.is_bound(); }

	convoihandle_t get_cnv() const { return cnv; }

	using gui_aligned_container_t::get_min_size;
	using gui_aligned_container_t::get_max_size;
};

#endif
