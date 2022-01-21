/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_EXTEND_EDIT_H
#define GUI_EXTEND_EDIT_H


#include "gui_frame.h"
#include "components/gui_textinput.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_scrollpane.h"
#include "components/gui_tab_panel.h"
#include "components/gui_button.h"
#include "components/gui_image.h"
#include "components/gui_fixedwidth_textarea.h"
#include "components/gui_building.h"
#include "components/gui_combobox.h"
#include "components/gui_numberinput.h"
#include "components/gui_divider.h"

#include "../utils/cbuffer_t.h"
#include "../simtypes.h"

class player_t;

/**
 * Entries for rotation selection.
 */
class gui_rotation_item_t : public gui_scrolled_list_t::const_text_scrollitem_t
{
private:
	const char *text;
	uint8 rotation; ///< 0-3, 255 = random

public:
	enum special_rotations_t {
		automatic = 254,
		random    = 255
	};

	gui_rotation_item_t(uint8 r);

	char const* get_text () const OVERRIDE { return text; }

	sint8 get_rotation() const { return rotation; }
};

/**
 * Entries for climate selection.
 */
class gui_climates_item_t : public gui_scrolled_list_t::const_text_scrollitem_t
{
private:
	const char *text;
	uint8 climate_;

public:
	gui_climates_item_t(uint8 r);

	char const* get_text () const OVERRIDE { return text; }

	sint8 get_climate() const { return climate_; }
};

/**
 * Entries for sorting selection.
 */
class gui_sorting_item_t : public gui_scrolled_list_t::const_text_scrollitem_t
{
private:
	const char *text;
	uint8 sorted_by;

public:
	enum sorting_options {
		BY_NAME_TRANSLATED,
		BY_NAME_OBJECT,
		BY_LEVEL_PAX,
		BY_LEVEL_MAIL,
		BY_DATE_INTRO,
		BY_DATE_RETIRE,
		BY_SIZE,
		BY_COST,
		BY_GOODS_NUMBER,
		BY_REMOVAL
	};

	gui_sorting_item_t(uint8 r);

	char const* get_text () const OVERRIDE { return text; }

	sint8 get_sortedby() const { return sorted_by; }
};




/**
 * Base class map editor dialogues to select object to place on map.
 */
class extend_edit_gui_t :
	public gui_frame_t,
	public action_listener_t
{
protected:
	player_t *player;
	/// cont_left: left column, cont_right: right column
	gui_aligned_container_t cont_left, cont_right;
	/// cont_filter: Settings about the content of the list (above list)
	/// cont_timeline: timeline-related filter settings
	/// cont_options: Settings about the active object (eg. rotation)
	/// cont_scrolly: the scrollable container (image + desc)
	gui_aligned_container_t cont_filter, cont_options, cont_timeline, cont_scrolly;



	cbuffer_t buf;
	gui_fixedwidth_textarea_t info_text;
	//container for object description
	gui_scrollpane_t scrolly;
	gui_scrolled_list_t scl;

	//image
	gui_building_t building_image;

	button_t bt_obsolete, bt_timeline, bt_climates, bt_timeline_custom, sort_order;

	// we make this available for child classes
	gui_combobox_t cb_rotation, cb_climates, cb_sortedby;

	gui_numberinput_t ni_timeline_year, ni_timeline_month;

	virtual void fill_list() {}

	virtual void change_item_info( sint32 /*entry, -1= none */ ) {}

	/**
	 * @returns selected rotation of cb_rotation.
	 * assumes that cb_rotation only contains gui_rotation_item_t-items.
	 * defaults to zero.
	 */
	uint8 get_rotation() const;

	/**
	 * @returns selected climate of cb_climates.
	 */
	uint8 get_climate() const;

	/**
	 * @returns selected sorting method.
	 */
	uint8 get_sortedby() const;

public:
	extend_edit_gui_t(const char *name, player_t* player_);

	void set_windowsize( scr_size s ) OVERRIDE;

	bool infowin_event(event_t const*) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
