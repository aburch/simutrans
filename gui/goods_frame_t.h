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

class goods_desc_t;

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

	cbuffer_t	speed_message;
	vector_tpl<const goods_desc_t*> good_list;

	button_t	sortedby;
	button_t	sorteddir;

	gui_numberinput_t	speed;
	gui_combobox_t	scheduletype;

	gui_fixedwidth_textarea_t speed_text;
	gui_aligned_container_t *sort_row;

	button_t	filter_goods_toggle;

	goods_stats_t goods_stats;
	gui_scrollpane_t scrolly;

	// creates the list and pass it to the child function good_stats, which does the display stuff ...
	static bool compare_goods(goods_desc_t const* const w1, goods_desc_t const* const w2);
	void sort_list();

public:
	goods_frame_t();

	// yes we can reload
	uint32 get_rdwr_id() OVERRIDE;
	void rdwr( loadsave_t *file ) OVERRIDE;

	bool has_min_sizer() const OVERRIDE {return true;}

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author V. Meyer
	 */
	const char * get_help_filename() const OVERRIDE {return "goods_filter.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
