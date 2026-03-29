#ifndef signal_info_t_h
#define signal_info_t_h

#include "obj_info.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_container.h"
#include "components/gui_numberinput.h"
#include "components/gui_label.h"

class signal_t;

/**
 * Info window for railway signals
 */
class signal_info_t : public obj_infowin_t, public action_listener_t
{
	private:
		signal_t* signal;
		button_t bt_require_parent, bt_remove_signal, bt_advance_to_end, bt_choose_signal, bt_stop_before_check, bt_skip_default_route;

		gui_numberinput_t numinp_tiles_margin;	
		gui_label_t lb_tiles_margin;

	public:
		signal_info_t(signal_t*);

		/**
		 * Set the window associated helptext
		 * @return the filename for the helptext, or NULL
		 * @author Hj. Malthaner
		 */
		const char *get_help_filename() const OVERRIDE {return "signal_info.txt";}

		bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

		// called, after external change
		void update_data();
};

#endif
