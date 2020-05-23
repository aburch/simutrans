/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_JOURNEY_TIME_INFO_T_H
#define GUI_JOURNEY_TIME_INFO_T_H


#include "components/gui_aligned_container.h"
#include "components/gui_scrollpane.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "gui_frame.h"

class schedule_t;
class player_t;

class gui_journey_time_stat_t : public gui_aligned_container_t {
  player_t* player;
  
public:
  gui_journey_time_stat_t(schedule_t*, player_t*);
  
  void update(schedule_t*);
};


class gui_journey_time_info_t : public gui_frame_t, private action_listener_t
{
private:
  gui_journey_time_stat_t stat;
	gui_scrollpane_t scrolly;
  schedule_t* schedule;
  
public:
	gui_journey_time_info_t(schedule_t*, player_t*);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void update() { stat.update(schedule); }

	//void rdwr( loadsave_t *file );
};

#endif
