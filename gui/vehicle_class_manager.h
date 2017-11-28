/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Convoi details window
 */

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
#include "../convoihandle_t.h"
#include "../gui/simwin.h"

class scr_coord;


class gui_class_vehicleinfo_t : public gui_container_t/*, private action_listener_t*/
{
private:
	/**
	 * Handle the convoi to be displayed.
	 * @author Hj. Malthaner
	 */
	convoihandle_t cnv;


	// When comboboxes eventually makes it to this part of the window....
	//slist_tpl<gui_combobox_t *> pass_class_sel;
	//slist_tpl<gui_combobox_t *> mail_class_sel;

	gui_container_t cont;

	// below a duplication of code, for the moment until I can get it automatically
	//char *class_name;
	//char *pass_class_name_untranslated[32];
	//char *mail_class_name_untranslated[32];

public:
	/**
	 * @param cnv, the handler for displaying the convoi.
	 * @author Hj. Malthaner
	 */
	gui_class_vehicleinfo_t(convoihandle_t cnv);



	//bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void set_cnv( convoihandle_t c ) { cnv = c; }

	/**
	 * Draw the component
	 * @author Hj. Malthaner
	 */
	void draw(scr_coord offset);
};



class vehicle_class_manager_t : public gui_frame_t , private action_listener_t
{
public:
	enum sort_mode_t { by_destination=0, by_via=1, by_amount_via=2, by_amount=3, SORT_MODES=4 };

private:

	gui_scrollpane_t scrolly;
	gui_class_vehicleinfo_t veh_info;

	convoihandle_t cnv;
	button_t reset_all_classes_button;

	vector_tpl<char> class_indices;

	slist_tpl<gui_combobox_t *> pass_class_sel;
	slist_tpl<gui_combobox_t *> mail_class_sel;

	sint16 button_width = 190;

	gui_container_t cont;

	uint16 current_number_of_classes;
	uint16 old_number_of_classes;

	uint16 current_number_of_accommodations;
	uint16 old_number_of_accommodations;

	uint32 current_number_of_vehicles;
	uint32 old_number_of_vehicles;

	uint8 highest_catering;
	bool is_tpo;

	uint8 vehicle_count;
	uint8 old_vehicle_count;

	uint16 header_height;

	int longest_class_name;

	uint32 overcrowded_capacity;

	bool convoy_bound = false;

	char *pass_class_name_untranslated[32];
	char *mail_class_name_untranslated[32];

	uint32 pass_capacity_at_class[255] = { 0 };
	uint32 mail_capacity_at_class[255] = { 0 };

	uint32 pass_capacity_at_accommodation[255] = { 0 };
	uint32 mail_capacity_at_accommodation[255] = { 0 };

	bool any_pass = false;
	bool any_mail = false;



public:

	vehicle_class_manager_t(convoihandle_t cnv);
	
	/**
	* Do the dynamic component layout
	*/
	void layout(scr_coord pos);

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
	void draw(scr_coord pos, scr_size size);

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author V. Meyer
	 */
	const char * get_help_filename() const {return "vehicle_class_manager.txt"; }

	/**
	 * Set window size and adjust component sizes and/or positions accordingly
	 * @author Hj. Malthaner
	 */
	virtual void set_windowsize(scr_size size);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	/**
	 * called when convoi was renamed
	 */
	void update_data() { set_dirty(); }

	// this constructor is only used during loading
	vehicle_class_manager_t();

	void rdwr( loadsave_t *file );

	uint32 get_rdwr_id() { return magic_class_manager; }

	vehicle_class_manager_t::~vehicle_class_manager_t();
};
