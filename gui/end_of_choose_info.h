#ifndef end_of_choose_info_t_h
#define end_of_choose_info_t_h

#include "obj_info.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_container.h"

class roadsign_t;

/**
 * Info window for railway signals
 */
class end_of_choose_info_t : public obj_infowin_t, public action_listener_t
{
	private:
		roadsign_t* signal;
		button_t bt_remove_signal, bt_end_of_choose, bt_end_of_guide;

	public:
		end_of_choose_info_t(roadsign_t*);

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
