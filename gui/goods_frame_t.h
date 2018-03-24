/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Goods list dialog
 */

#ifndef goods_frame_t_h
#define goods_frame_t_h

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_numberinput.h"
#include "components/gui_fixedwidth_textarea.h"
#include "components/gui_combobox.h"
#include "components/gui_scrollpane.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "goods_stats_t.h"
#include "../utils/cbuffer_t.h"

/**
 * Shows statistics. Only goods so far.
 * @author Hj. Malthaner
 */
class goods_frame_t : public gui_frame_t, private action_listener_t
{
private:
	enum sort_mode_t { unsortiert=0, nach_name=1, nach_gewinn=2, nach_bonus=3, nach_catg=4, SORT_MODES=5 };
	static const char *sort_text[SORT_MODES];

	// static, so we remember the last settings
	static sint16 relative_speed_change;
	static bool sortreverse;
	static sort_mode_t sortby;
	static bool filter_goods;

	char	speed_bonus[6];
	cbuffer_t	speed_message, speed_message2;
	uint16 good_list[256];

	gui_label_t sort_label;
	button_t	sortedby;
	button_t	sorteddir;

	gui_label_t tile_speed;
	gui_numberinput_t	speed;
	gui_combobox_t	scheduletype;

	gui_fixedwidth_textarea_t speed_text;

	button_t	filter_goods_toggle;

	goods_stats_t goods_stats;
	gui_scrollpane_t scrolly;

	// creates the list and pass it to the child function good_stats, which does the display stuff ...
	static bool compare_goods(uint16, uint16);
	void sort_list();

public:
	goods_frame_t();

	// yes we can reload
	uint32 get_rdwr_id();
	void rdwr( loadsave_t *file );

	/**
	* resize window in response to a resize event
	* @author Hj. Malthaner
	* @date   16-Oct-2003
	*/
	void resize(const scr_coord delta);

	bool has_min_sizer() const {return true;}

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author V. Meyer
	 */
	const char * get_help_filename() const {return "goods_filter.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
