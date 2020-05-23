#include "journey_time_info.h"
#include "../dataobj/translator.h"
#include "../dataobj/schedule.h"
#include "../dataobj/schedule_entry.h"
#include "../simhalt.h"
#include "../simworld.h"


uint16 tick_to_divided_time(uint32 tick) {
  const uint16 divisor = world()->get_settings().get_spacing_shift_divisor();
  return (uint16)((uint64)tick * divisor / world()->ticks_per_world_month);
}

class gui_journey_time_entry_t : public gui_aligned_container_t {
  cbuffer_t buf[5];
public:
  gui_journey_time_entry_t(schedule_entry_t e, player_t* player) {
    set_table_layout(1,0);
    halthandle_t const halt = haltestelle_t::get_halt(e.pos, player);
    if(  halt.is_bound()  ) {
      // show halt name
      buf[0].printf("%s", halt->get_name());
    } else {
      // maybe a waypoint
      buf[0].printf("waypoint");
    }
    // show last 3 actual journey time
    // calc average
    uint32 sum_ticks = 0;
    uint8 cnt = 0;
    for(uint8 i=0; i<3; i++) {
      uint32 t = e.journey_time[(i+e.jtime_index)%3];
      if(  t==0  ) {
        buf[i+1].printf("-");
      } else {
        buf[i+1].printf("%d", tick_to_divided_time(t));
        cnt += 1;
        sum_ticks += t;
      }
    }
    if(  cnt==0  ) {
      buf[4].printf("-");
    } else {
      buf[4].printf("%d", tick_to_divided_time(sum_ticks/cnt));
    }
    
    add_table(5,1);
    for(uint8 i=0; i<5; i++) {
      /*gui_label_t* label = */new_component<gui_label_t>(buf[i]);
    }
    end_table();
  }
};

gui_journey_time_stat_t::gui_journey_time_stat_t(schedule_t* schedule, player_t* pl) {
  player = pl;
  set_table_layout(1,0);
  update(schedule);
}

void gui_journey_time_stat_t::update(schedule_t* schedule) {
  remove_all();
  FOR(minivec_tpl<schedule_entry_t>, const& e, schedule->entries) {
    new_component<gui_journey_time_entry_t>(e, player);
  }
  set_size(get_min_size());
}


gui_journey_time_info_t::gui_journey_time_info_t(schedule_t* schedule, player_t* player) : 
  gui_frame_t( translator::translate("Journey Time"), player ),
  stat(schedule, player),
  scrolly(&stat),
  schedule(schedule)
{
  set_table_layout(1,0);
  add_table(5,0)->set_force_equal_columns(true);
  new_component<gui_label_t>("Stop");
  new_component<gui_label_t>("Time 1");
  new_component<gui_label_t>("Time 2");
  new_component<gui_label_t>("Time 3");
  new_component<gui_label_t>("Average");
  end_table();
  
  add_component(&scrolly);
  
  set_resizemode(diagonal_resize);

	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
}

bool gui_journey_time_info_t::action_triggered(gui_action_creator_t*, value_t) {
  return true;
}
