#include "journey_time_info.h"
#include "../dataobj/translator.h"
#include "../dataobj/schedule.h"
#include "../dataobj/schedule_entry.h"
#include "../simhalt.h"
#include "../simworld.h"
#include "../simline.h"
#include "../sys/simsys.h"
#include "messagebox.h"
#include "simwin.h"


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
    end_table();
  }
};

gui_journey_time_stat_t::gui_journey_time_stat_t(schedule_t* schedule, player_t* pl) {
  player = pl;
  update(schedule);
}

void gui_journey_time_stat_t::update(schedule_t* schedule) {
  scr_size size = get_size();
  remove_all();
  set_table_layout(5,0);
  uint32 journey_time_sum = 0;
  FOR(minivec_tpl<schedule_entry_t>, const& e, schedule->entries) {
    //new_component<gui_journey_time_entry_t>(e, player);
    gui_label_buf_t *lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::left);
    halthandle_t const halt = haltestelle_t::get_halt(e.pos, player);
    if(  halt.is_bound()  ) {
      // show halt name
      lb->buf().printf("%s", halt->get_name());
    } else {
      // maybe a waypoint
      lb->buf().printf("waypoint");
    }
    lb->update();
    
    // show last 3 actual journey time
    // calc average
    uint32 sum_ticks = 0;
    uint8 cnt = 0;
    for(uint8 i=0; i<3; i++) {
      lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
      uint32 t = e.journey_time[(i+e.jtime_index)%3];
      if(  t==0  ) {
        lb->buf().printf("-");
      } else {
        lb->buf().printf("%d", tick_to_divided_time(t));
        cnt += 1;
        sum_ticks += t;
      }
      lb->update();
    }
    
    lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
    if(  cnt==0  ) {
      lb->buf().printf("-");
    } else {
      lb->buf().printf("%d", tick_to_divided_time(sum_ticks/cnt));
      journey_time_sum += (sum_ticks/cnt);
    }
    lb->update();
  }
  
  // show total journey time
  gui_label_buf_t *lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::left);
  lb->buf().printf("Total");
  lb->update();
  
  for(uint8 i=0; i<3; i++) {
    lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
    lb->buf().printf("");
    lb->update();
  }
  lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
  lb->buf().printf("%d", tick_to_divided_time(journey_time_sum)); 
  lb->update();
  
  scr_size min_size = get_min_size();
	set_size(scr_size(max(size.w, min_size.w), min_size.h) );
}


void copy_stations_to_clipboard(schedule_t* schedule, player_t* player, bool name_only) {
  cbuffer_t clipboard;
  FOR(minivec_tpl<schedule_entry_t>, const& e, schedule->entries) {
    halthandle_t const halt = haltestelle_t::get_halt(e.pos, player);
    if(  !halt.is_bound()  ) {
      // do not export waypoint
      continue;
    }
    clipboard.append(halt->get_name());
    if(  !name_only  ) {
      clipboard.append(",");
      clipboard.append(e.pos.get_str());
    }
    clipboard.append("\n");
  }
  // copy, if there was anything ...
  if( clipboard.len() > 0 ) {
    dr_copy( clipboard, clipboard.len() );
    create_win( new news_img("Station infos were copied to clipboard.\n"), w_time_delete, magic_none );
  }
}


gui_journey_time_info_t::gui_journey_time_info_t(linehandle_t line, player_t* player) : 
  gui_frame_t( NULL, player ),
  stat(line->get_schedule(), player),
  scrolly(&stat),
  player(player),
  schedule(line->get_schedule())
{
  title_buf = new cbuffer_t();
  title_buf->printf(translator::translate("Journey time of %s"), line->get_name());
  set_name(*title_buf);
  
  set_table_layout(1,0);
  gui_aligned_container_t* container = add_table(5,1);
  new_component<gui_label_t>("Stop");
  const char* texts[4] = {"1st", "2nd", "3rd", "Ave."};
  gui_label_buf_t* lb;
  for(uint8 i=0; i<4; i++) {
    lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
    lb->buf().printf("%s", texts[i]);
    lb->update();
  }
  end_table();
  scr_size min_size = get_min_size();
	container->set_size(scr_size(max(get_size().w, min_size.w), min_size.h) );
  
  scrolly.set_maximize(true);
  add_component(&scrolly);
  
  add_table(3,1)->set_force_equal_columns(true);
  {
    bt_copy_names.init(button_t::roundbox | button_t::flexible, translator::translate("Copy names"));
    bt_copy_names.set_tooltip("Copy station names to clipboard.");
    bt_copy_names.add_listener(this);
    add_component(&bt_copy_names);
    
    bt_copy_stations.init(button_t::roundbox | button_t::flexible, translator::translate("Copy stations"));
    bt_copy_stations.set_tooltip("Copy station names and their position to clipboard.");
    bt_copy_stations.add_listener(this);
    add_component(&bt_copy_stations);
    
    bt_copy_oudia.init(button_t::roundbox | button_t::flexible, translator::translate("Copy oudia format"));
    bt_copy_oudia.set_tooltip("Copy station names and journey time to clipboard in oudia format.");
    bt_copy_oudia.add_listener(this);
    add_component(&bt_copy_oudia);
  }
  end_table();
  
  set_resizemode(diagonal_resize);

	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
}

bool gui_journey_time_info_t::action_triggered(gui_action_creator_t* comp, value_t) {
  if(  comp==&bt_copy_names  ) {
    copy_stations_to_clipboard(schedule, player, true);
  }
  else if(  comp==&bt_copy_stations  ) {
    copy_stations_to_clipboard(schedule, player, false);
  }
  return true;
}

gui_journey_time_info_t::~gui_journey_time_info_t() {
  delete title_buf;
}
