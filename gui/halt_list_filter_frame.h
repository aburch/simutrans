/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_HALT_LIST_FILTER_FRAME_H
#define GUI_HALT_LIST_FILTER_FRAME_H


#include "gui_frame.h"
#include "components/action_listener.h"
#include "components/gui_aligned_container.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/gui_scrollpane.h"
#include "components/gui_tab_panel.h"
#include "components/gui_textinput.h"
#include "halt_list_frame.h"

class player_t;

/**
 * Stations/stops list filter dialog
 * Displays filter settings for the halt list
 */
class halt_list_filter_frame_t : public gui_frame_t , private action_listener_t
{
private:
	/**
	* Helper class for the entries of the scrollable list of goods.
	* Needed since a button_t does not know its parent.
	*/
	class ware_item_t : public button_t {
		const goods_desc_t *ware_ab;
		const goods_desc_t *ware_an;
		halt_list_filter_frame_t *parent;
	public:
		ware_item_t(halt_list_filter_frame_t *parent, const goods_desc_t *ware_ab, const goods_desc_t *ware_an)
		{
			this->ware_ab = ware_ab;
			this->ware_an = ware_an;
			this->parent = parent;
		}

		bool infowin_event(event_t const* const ev) OVERRIDE
		{
			bool swallow = button_t::infowin_event( ev );
			if(  swallow  &&  IS_LEFTRELEASE(ev)  ) {	// only handle, if we are hit!
				parent->ware_item_triggered( ware_ab, ware_an );
			}
			return swallow;
		}
		void draw(scr_coord offset) OVERRIDE {
			if(ware_ab) {
				pressed = parent->get_ware_filter_ab(ware_ab);
			}
			if(ware_an) {
				pressed = parent->get_ware_filter_an(ware_an);
			}
			button_t::draw(offset);
		}
	};

	/**
	 * As long we do not have resource scripts, we display make
	 * some tables for the main attributes of each button.
	 */
	enum {
		FILTER_BUTTONS = 5
	};

	static halt_list_frame_t::filter_flag_t filter_buttons_types[FILTER_BUTTONS];
	static const char *filter_buttons_text[FILTER_BUTTONS];

	/**
	 * We are bound to this window. All filter states are stored in main_frame.
	 */
	halt_list_frame_t *main_frame;

	/**
	 * All gui elements of this dialog:
	 */
	button_t filter_buttons[FILTER_BUTTONS];

	gui_aligned_container_t special_filter, accepts_filter, sends_filter;
	gui_tab_panel_t tabs;

	gui_textinput_t name_filter_input;

	button_t typ_filter_enable;

	button_t ware_alle_ab;
	button_t ware_keine_ab;
	button_t ware_invers_ab;

	gui_aligned_container_t ware_cont_ab;
	gui_scrollpane_t ware_scrolly_ab;

	button_t ware_alle_an;
	button_t ware_keine_an;
	button_t ware_invers_an;

	gui_aligned_container_t ware_cont_an;
	gui_scrollpane_t ware_scrolly_an;

public:
	halt_list_filter_frame_t(player_t *player, halt_list_frame_t *main_frame);
	~halt_list_filter_frame_t();

	/**
	 * Propagate function from main_frame for ware_item_t
	 */
	bool get_ware_filter_ab(const goods_desc_t *ware) const { return main_frame->get_ware_filter_ab(ware); }
	bool get_ware_filter_an(const goods_desc_t *ware) const { return main_frame->get_ware_filter_an(ware); }

	/**
	 * Handler for ware_item_t event.
	 */
	void ware_item_triggered(const goods_desc_t *ware_ab, const goods_desc_t *ware_an);

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
	 */
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 */
	const char * get_help_filename() const OVERRIDE {return "haltlist_filter.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
