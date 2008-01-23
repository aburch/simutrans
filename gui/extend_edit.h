#ifndef gui_extend_edit_h
#define gui_extend_edit_h

#include "gui_frame.h"
#include "gui_container.h"
#include "components/gui_textinput.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_scrollpane.h"
#include "components/gui_tab_panel.h"
#include "components/gui_divider.h"
#include "components/gui_button.h"
#include "components/gui_image.h"
#include "components/gui_textarea.h"

#include "gui_convoiinfo.h"
#include "../simline.h"
#include "../besch/haus_besch.h"
#include "../besch/fabrik_besch.h"
#include "../utils/cbuffer_t.h"
#include "../dings/baum.h"


#define NAME_COLUMN_WIDTH (int)(BUTTON_WIDTH*2.25)
#define SCL_HEIGHT (170)

/**
 * Window.
 * Will display list of schedules.
 * Contains buttons: edit new remove
 * Resizable.
 *
 * @author Niels Roest
 * @author hsiegeln: major redesign
 */


class extend_edit_gui_t : public gui_frame_t, public action_listener_t
{
protected:
	spieler_t *sp;
	karte_t* welt;

	cbuffer_t buf;
	gui_textarea_t info_text;
	gui_container_t cont;
	gui_scrollpane_t scrolly;

	gui_scrolled_list_t scl;

	gui_tab_panel_t tabs;

	//image
	gui_image_t img[4];

	button_t bt_obsolete, bt_timeline, bt_climates;

	sint16 offset_of_comp;

	bool is_show_trans_name;

	void resize(const koord delta);

	virtual void fill_list( bool /* translate */ ) {}

	virtual void change_item_info( sint32 /*entry, -1= none */ ) {}

public:
	extend_edit_gui_t(spieler_t* sp,karte_t* welt);

	/**
	* Does this window need a min size button in the title bar?
	* @return true if such a button is needed
	* @author Hj. Malthaner
	*/
	bool has_min_sizer() const {return true;}

   /**
   * Mausklicks werden hiermit an die GUI-Komponenten
   * gemeldet
   */
   void infowin_event(const event_t *ev);

	/**
	* This method is called if an action is triggered
	* @author Hj. Malthaner
	*
	* Returns true, if action is done and no more
	* components should be triggered.
	* V.Meyer
	*/
	bool action_triggered(gui_komponente_t *komp, value_t extra);

};

#endif
