/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_LINE_CLASS_MANAGER_H
#define GUI_LINE_CLASS_MANAGER_H


#include "gui_frame.h"
#include "components/gui_container.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"
#include "components/gui_speedbar.h"
#include "components/gui_button.h"
#include "components/gui_label.h"                  // 09-Dec-2001      Markus Weber    Added
#include "components/gui_combobox.h"
#include "components/action_listener.h"
#include "../linehandle_t.h"
#include "../gui/simwin.h"

class scr_coord;

/*
 * Line class manager window
 */
class line_class_manager_t : public gui_frame_t, private action_listener_t
{
private:

	linehandle_t line;
	button_t reset_all_pass_button, reset_all_mail_button;

	vector_tpl<char> class_indices;

	slist_tpl<gui_combobox_t *> pass_class_sel;
	slist_tpl<gui_combobox_t *> mail_class_sel;

	sint16 button_width = 190;

	gui_container_t cont;

	uint16 current_pass_entries;
	uint16 pass_entries;

	uint32 text_height;

	uint8 highest_catering;
	uint8 lowest_catering;
	uint32 catering_cars_amount;
	bool is_tpo;
	uint32 tpo_amount;

	uint8 vehicle_count;
	uint8 old_vehicle_count;

	uint32 overcrowded_capacity;

	char *pass_class_name_untranslated[32];
	char *mail_class_name_untranslated[32];

	uint32 pass_capacity_at_class[255] = { 0 };
	uint32 mail_capacity_at_class[255] = { 0 };

	uint32 pass_capacity_at_accommodation[255] = { 0 };
	uint32 mail_capacity_at_accommodation[255] = { 0 };

	bool show_pass = false;
	bool show_mail = false;

	bool need_to_update_comboboxes = false;

public:

	line_class_manager_t(linehandle_t line);

	/**
	* Do the dynamic component layout
	*/
	void layout();

	/**
	* Build the class lists
	*/

	void build_class_entries();

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
	 * @author Hj. Malthaner
	 */
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author V. Meyer
	 */
	const char * get_help_filename() const OVERRIDE {return "line_class_manager.txt"; }

	/**
	 * Set window size and adjust component sizes and/or positions accordingly
	 * @author Hj. Malthaner
	 */
	virtual void set_windowsize(scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	/**
	 * called when convoi was renamed
	 */
	void update_data() { set_dirty(); }

	// this constructor is only used during loading
	line_class_manager_t();

	void rdwr( loadsave_t *file ) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_line_class_manager; }

	~line_class_manager_t();
};

#endif
