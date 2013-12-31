#ifndef gui_extend_edit_h
#define gui_extend_edit_h

#include "gui_frame.h"
#include "components/gui_container.h"
#include "components/gui_textinput.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_scrollpane.h"
#include "components/gui_tab_panel.h"
#include "components/gui_divider.h"
#include "components/gui_button.h"
#include "components/gui_image.h"
#include "components/gui_fixedwidth_textarea.h"

#include "components/gui_convoiinfo.h"
#include "../utils/cbuffer_t.h"
#include "../simtypes.h"

class spieler_t;

#define COLUMN_WIDTH (int)(D_BUTTON_WIDTH*2.25)
#define SCL_HEIGHT (15*LINESPACE-1)
#define LINE_NAME_COLUMN_WIDTH (int)((D_BUTTON_WIDTH*2.25)+11)
#define N_BUTTON_WIDTH  (int)(D_BUTTON_WIDTH*1.5)
#define MARGIN (10)

/**
 * Window.
 * Will display list of schedules.
 * Contains buttons: edit new remove
 * Resizable.
 *
 * @author Niels Roest
 * @author hsiegeln: major redesign
 */


class extend_edit_gui_t :
	public gui_frame_t,
	public action_listener_t
{
private:
	sint16 tab_panel_width;

protected:
	spieler_t *sp;

	cbuffer_t buf;
	gui_fixedwidth_textarea_t info_text;
	gui_container_t cont;
	gui_scrollpane_t scrolly;

	gui_scrolled_list_t scl;

	gui_tab_panel_t tabs;

	//image
	gui_image_t img[4];

	button_t bt_obsolete, bt_timeline, bt_climates;

	sint16 offset_of_comp;

	sint16 get_tab_panel_width() const { return tab_panel_width; }

	bool is_show_trans_name;

	void resize(const scr_coord delta);

	virtual void fill_list( bool /* translate */ ) {}

	virtual void change_item_info( sint32 /*entry, -1= none */ ) {}

public:
	extend_edit_gui_t(const char *name, spieler_t* sp);

	/**
	* Does this window need a min size button in the title bar?
	* @return true if such a button is needed
	* @author Hj. Malthaner
	*/
	bool has_min_sizer() const {return true;}

	bool infowin_event(event_t const*) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
