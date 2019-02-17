#ifndef gui_extend_edit_h
#define gui_extend_edit_h

#include "gui_frame.h"
#include "components/gui_textinput.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_scrollpane.h"
#include "components/gui_tab_panel.h"
#include "components/gui_button.h"
#include "components/gui_image.h"
#include "components/gui_fixedwidth_textarea.h"
#include "components/gui_building.h"

#include "../utils/cbuffer_t.h"
#include "../simtypes.h"

class player_t;


class extend_edit_gui_t :
	public gui_frame_t,
	public action_listener_t
{
protected:
	player_t *player;
	/// cont_left: left column, right: between obsolete-button and text-area
	gui_aligned_container_t cont_left, cont_right;

	cbuffer_t buf;
	gui_fixedwidth_textarea_t info_text;
	gui_scrollpane_t scrolly;
	gui_scrolled_list_t scl;
	gui_tab_panel_t tabs;

	//image
	gui_building_t building_image;

	button_t bt_obsolete, bt_timeline, bt_climates;

	/// show translated names
	bool is_show_trans_name;

	virtual void fill_list( bool /* translate */ ) {}

	virtual void change_item_info( sint32 /*entry, -1= none */ ) {}

public:
	extend_edit_gui_t(const char *name, player_t* player_);

	/**
	* Does this window need a min size button in the title bar?
	* @return true if such a button is needed
	* @author Hj. Malthaner
	*/
	bool has_min_sizer() const OVERRIDE {return true;}

	bool infowin_event(event_t const*) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
