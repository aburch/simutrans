/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_HALT_LIST_STATS_H
#define GUI_HALT_LIST_STATS_H


#include "components/gui_component.h"

#include "components/gui_aligned_container.h"
#include "components/gui_colorbox.h"
#include "components/gui_button.h"
#include "components/gui_image.h"
#include "components/gui_label.h"
#include "components/gui_scrolled_list.h"
#include "../halthandle_t.h"

class gui_halt_type_images_t;


/**
 * Helper class to show small three freight category waiting indicator
 */
class gui_mini_halt_waiting_indicator_t : public gui_container_t
{
	halthandle_t halt;
public:
	gui_mini_halt_waiting_indicator_t(halthandle_t h);

	void draw(scr_coord offset);
};


class halt_list_stats_t : public gui_aligned_container_t, public gui_scrolled_list_t::scrollitem_t
{
private:
	halthandle_t halt;

public:
	gui_label_buf_t label_name, label_cargo;
	gui_image_t img_enabled[3];
	gui_halt_type_images_t *img_types;
	gui_colorbox_t indicator;
	button_t gotopos;

public:
	halt_list_stats_t(halthandle_t halt_);

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Draw the component
	 */
	void draw(scr_coord offset) OVERRIDE;

	void update_label();

	const char* get_text() const OVERRIDE;

	bool is_valid() const OVERRIDE { return halt.is_bound(); }

	halthandle_t get_halt() const { return halt; }

	using gui_aligned_container_t::get_min_size;
	using gui_aligned_container_t::get_max_size;
};

#endif
