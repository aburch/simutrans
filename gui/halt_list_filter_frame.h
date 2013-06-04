/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Stations/stops list filter dialog
 * Displays filter settings for the halt list
 * @author V. Meyer
 */

#include "gui_frame.h"
#include "components/gui_label.h"
#include "components/gui_scrollpane.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "halt_list_frame.h"
#include "components/gui_textinput.h"

class spieler_t;

class halt_list_filter_frame_t : public gui_frame_t , private action_listener_t
{
private:
	/**
	* Helper class for the entries of the scrollable list of goods.
	* Needed since a button_t does not know its parent.
	*/
	class ware_item_t : public button_t {
		const ware_besch_t *ware_ab;
		const ware_besch_t *ware_an;
		halt_list_filter_frame_t *parent;
	public:
		ware_item_t(halt_list_filter_frame_t *parent, const ware_besch_t *ware_ab, const ware_besch_t *ware_an)
		{
			this->ware_ab = ware_ab;
			this->ware_an = ware_an;
			this->parent = parent;
		}

		bool infowin_event(event_t const* const ev) OVERRIDE
		{
			if(IS_LEFTRELEASE(ev)) {
				parent->ware_item_triggered(ware_ab, ware_an);
			}
			return button_t::infowin_event(ev);
		}
		virtual void zeichnen(koord offset) {
			if(ware_ab) {
				pressed = parent->get_ware_filter_ab(ware_ab);
			}
			if(ware_an) {
				pressed = parent->get_ware_filter_an(ware_an);
			}
			button_t::zeichnen(offset);
		}
	};

	/**
	 * As long we do not have resource scripts, we display make
	 * some tables for the main attributes of each button.
	 */
	enum { FILTER_BUTTONS=16 };

	static koord filter_buttons_pos[FILTER_BUTTONS];
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

	gui_textinput_t name_filter_input;

	button_t typ_filter_enable;

	button_t ware_alle_ab;
	button_t ware_keine_ab;
	button_t ware_invers_ab;

	gui_scrollpane_t ware_scrolly_ab;
	gui_container_t ware_cont_ab;

	button_t ware_alle_an;
	button_t ware_keine_an;
	button_t ware_invers_an;

	gui_scrollpane_t ware_scrolly_an;
	gui_container_t ware_cont_an;

public:
	halt_list_filter_frame_t(spieler_t *sp, halt_list_frame_t *main_frame);
	~halt_list_filter_frame_t();

	/**
	 * Propagate function from main_frame for ware_item_t
	 * @author V. Meyer
	 */
	bool get_ware_filter_ab(const ware_besch_t *ware) const { return main_frame->get_ware_filter_ab(ware); }
	bool get_ware_filter_an(const ware_besch_t *ware) const { return main_frame->get_ware_filter_an(ware); }

	/**
	 * Handler for ware_item_t event.
	 * @author V. Meyer
	 */
	void ware_item_triggered(const ware_besch_t *ware_ab, const ware_besch_t *ware_an);

	/**
	 * Does this window need a min size button in the title bar?
	 * @return true if such a button is needed
	 */
	bool has_min_sizer() const {return true;}

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
	 * @author V. Meyer
	 */
	void zeichnen(koord pos, koord gr);

    /**
     * resize window in response to a resize event
     */
	void resize(const koord delta);

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author V. Meyer
	 */
	const char * get_hilfe_datei() const {return "haltlist_filter.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};
