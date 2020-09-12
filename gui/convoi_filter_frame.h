/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CONVOI_FILTER_FRAME_H
#define GUI_CONVOI_FILTER_FRAME_H


#include "gui_frame.h"
#include "components/gui_label.h"
#include "components/gui_scrollpane.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_textinput.h"

class convoi_frame_t;
class player_t;
class goods_desc_t;

/**
 * Displays a filter settings dialog for the convoi list
 */
class convoi_filter_frame_t : public gui_frame_t , private action_listener_t
{
public:

	enum filter_flag_t {
		any_filter     =1,
		name_filter    =2,
		typ_filter     =4,
		ware_filter    =8,
		spezial_filter =16,
		lkws_filter        = 1 << 5,
		zuege_filter       = 1 << 6,
		schiffe_filter     = 1 << 7,
		aircraft_filter    = 1 << 8,
		noroute_filter     = 1 << 9,
		noschedule_filter  = 1 << 10,
		noincome_filter    = 1 << 11,
		indepot_filter     = 1 << 12,
		noline_filter      = 1 << 13,
		stucked_filter     = 1 << 14,
		monorail_filter    = 1 << 15,
		maglev_filter      = 1 << 16,
		narrowgauge_filter = 1 << 17,
		tram_filter        = 1 << 18,
		obsolete_filter    = 1 << 19,
		// number of first special filter
		sub_filter         = lkws_filter
	};
	enum { FILTER_BUTTONS=19 };


private:
	static uint32 filter_flags;

	bool get_filter(convoi_filter_frame_t::filter_flag_t filter) { return (filter_flags & filter) != 0; }
	void set_filter(convoi_filter_frame_t::filter_flag_t filter, bool on) { filter_flags = (on ? (filter_flags | filter) : (filter_flags & ~filter) ); }

	/*
	* Helper class for the entries of the scrollable list of goods.
	* Needed since a button_t does not know its parent.
	*/
	class ware_item_t : public button_t
	{
	public:
		const goods_desc_t *ware;
		convoi_filter_frame_t *parent;

		ware_item_t(convoi_filter_frame_t *parent, const goods_desc_t *ware)
		{
			this->ware = ware;
			this->parent = parent;
		}

		bool infowin_event(event_t const* const ev) OVERRIDE
		{
			bool swallow = button_t::infowin_event(ev);
			if(  IS_LEFTRELEASE(ev)  &&  swallow   ) {
				pressed ^= 1;
				parent->sort_list();
			}
			return swallow;
		}
	};

	slist_tpl<ware_item_t *>all_ware;
	static slist_tpl<const goods_desc_t *>active_ware;

	static filter_flag_t filter_buttons_types[FILTER_BUTTONS];
	static const char *filter_buttons_text[FILTER_BUTTONS];

	/*
	 * We are bound to this window. All filter states are stored in main_frame.
	 */
	convoi_frame_t *main_frame;

	/*
	 * All gui elements of this dialog:
	 */
	button_t filter_buttons[FILTER_BUTTONS];

	static char name_filter_text[64];
	gui_textinput_t name_filter_input;

	button_t typ_filter_enable;

	button_t ware_alle;
	button_t ware_keine;
	button_t ware_invers;

	gui_aligned_container_t ware_cont;
	gui_scrollpane_t ware_scrolly;

public:
	void sort_list();

	/**
	 * Constructor. Generates all necessary Subcomponents.
	 */
	convoi_filter_frame_t(player_t *player, convoi_frame_t *parent);

	/**
	 * Does this window need a min size button in the title bar?
	 * @return true if such a button is needed
	 */
	bool has_min_sizer() const OVERRIDE {return true;}

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 */
	const char *get_help_filename() const OVERRIDE {return "convoi_filter.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
