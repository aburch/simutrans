/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_UNUSED_ADDONS_FRAME_H
#define GUI_UNUSED_ADDONS_FRAME_H

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_scrollpane.h"
#include "components/gui_aligned_container.h"
#include "components/gui_label.h"
#include "../tpl/vector_tpl.h"
#include <string>

/**
 * Dialog that lists all pak descriptors not used in the current map.
 * Provides a "Copy as CSV" button for exporting the list.
 */
class unused_addons_frame_t : public gui_frame_t, public action_listener_t
{
	struct entry_t {
		std::string name;
		const char *type; // static string, no ownership
	};

	vector_tpl<entry_t> entries;
	gui_aligned_container_t list_container;
	gui_scrollpane_t scrollpane;
	gui_label_buf_t count_label;
	button_t copy_btn;

	void build_list();

public:
	unused_addons_frame_t();

	const char *get_help_filename() const OVERRIDE { return NULL; }

	bool action_triggered(gui_action_creator_t *comp, value_t v) OVERRIDE;
};

#endif
