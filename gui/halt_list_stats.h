/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 * Written (w) 2001 Markus Weber
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef halt_list_stats_h
#define halt_list_stats_h

#include "components/gui_komponente.h"

#include "components/gui_aligned_container.h"
#include "components/gui_colorbox.h"
#include "components/gui_image.h"
#include "components/gui_label.h"
#include "components/gui_scrolled_list.h"
#include "../halthandle_t.h"

class gui_halt_type_images_t;
/**
 * @author Hj. Malthaner
 */
class halt_list_stats_t : public gui_aligned_container_t, public gui_scrolled_list_t::scrollitem_t
{
private:
	halthandle_t halt;

public:
	gui_label_buf_t label_name, label_cargo;
	gui_image_t img_enabled[3];
	gui_halt_type_images_t *img_types;
	gui_colorbox_t indicator;

public:
	halt_list_stats_t(halthandle_t halt_);

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Draw the component
	 * @author Hj. Malthaner
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
