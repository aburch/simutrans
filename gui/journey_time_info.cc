#include "journey_time_info.h"
#include "../dataobj/translator.h"
#include "../dataobj/schedule.h"
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

uint32 get_latest_dep_slot(schedule_entry_t& entry, uint32 current_time) {
  const sint32 spacing_shift = (sint64)entry.spacing_shift * world()->ticks_per_world_month / world()->get_settings().get_spacing_shift_divisor();
  uint64 slot = (current_time - spacing_shift) * (uint64)entry.spacing / world()->ticks_per_world_month;
  return slot * world()->ticks_per_world_month / entry.spacing + spacing_shift;
}

gui_journey_time_stat_t::gui_journey_time_stat_t(schedule_t*, player_t* pl) {
  player = pl;
}

void gui_journey_time_stat_t::update(linehandle_t line, vector_tpl<uint32*>& journey_times, bool& empty_slot_exists) {
  schedule_t* schedule = line->get_schedule();
  scr_size size = get_size();
  remove_all();
  set_table_layout(NUM_ARRIVAL_TIME_STORED+2,0);
  uint8 depot_entry_count = 0;
  for(uint8 idx=0; idx<schedule->entries.get_count(); idx++) {
    schedule_entry_t& e = schedule->entries[idx];
    gui_label_buf_t *lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::left);
    halthandle_t const halt = haltestelle_t::get_halt(e.pos, player);
    if(  halt.is_bound()  ) {
      // show halt name
      lb->buf().printf("%s", halt->get_name());
      const bool last_slot_empty = e.get_wait_for_time() && !halt->is_departure_booked(get_latest_dep_slot(e, world()->get_ticks()), idx - depot_entry_count, line);
      lb->set_color(last_slot_empty ? color_idx_to_rgb(COL_ORANGE) : SYSCOL_TEXT);
      empty_slot_exists |= last_slot_empty;
    } else {
      // maybe a waypoint
      lb->buf().printf(translator::translate("Wegpunkt"));
      // count the entries of depot
      const grund_t* gr = world()->lookup(e.pos);
      depot_entry_count += (gr  &&  gr->get_depot());
    }
    lb->update();
    
    for(uint8 i=0; i<NUM_ARRIVAL_TIME_STORED+1; i++) {
      lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
      uint32 t = journey_times[idx][i];
      if(  t==0  ) {
        lb->buf().printf("-");
      } else {
        lb->buf().printf("%d", t);
      }
      lb->update();
      if(  t>0  &&  (sint32)t-(sint32)journey_times[idx][0]>4  ) {
        lb->set_color(color_idx_to_rgb(COL_RED));
      } else if(  t>0  &&  (sint32)journey_times[idx][0]-(sint32)t>4  ) {
        lb->set_color(color_idx_to_rgb(COL_BLUE));
      } else {
        lb->set_color(SYSCOL_TEXT);
      }
    }
  }
  
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
  } else {
    create_win( new news_img("There is nothing to copy.\n"), w_time_delete, magic_none );
  }
}

void copy_csv_format(schedule_t* schedule, player_t* player, vector_tpl<uint32*>& journey_times) {
  // copy in csv format. separator is \t.
  cbuffer_t clipboard;
  clipboard.append("Station Name\tAverage\t1st\t2nd\t3rd\t4th\t5th\n");
  for(uint8 i=0; i<schedule->entries.get_count(); i++) {
    halthandle_t const halt = haltestelle_t::get_halt(schedule->entries[i].pos, player);
    if(  halt.is_bound()  ) {
      clipboard.append(halt->get_name());
    } else {
      clipboard.append("waypoint");
    }
    for(uint8 k=0; k<NUM_ARRIVAL_TIME_STORED+1; k++) {
      clipboard.printf("\t%d", journey_times[i][k]);
    }
    clipboard.append("\n");
  }
  if(  clipboard.len()>0  ) {
    dr_copy( clipboard, clipboard.len() );
    create_win( new news_img("Station infos were copied to clipboard.\n"), w_time_delete, magic_none );
  } else {
    create_win( new news_img("There is nothing to copy.\n"), w_time_delete, magic_none );
  }
}


gui_journey_time_info_t::gui_journey_time_info_t(linehandle_t line, player_t* player) : 
  gui_frame_t( NULL, player ),
  stat(line->get_schedule(), player),
  scrolly(&stat),
  player(player),
  schedule(line->get_schedule()),
  line(line)
{
  update();
  
  title_buf = new cbuffer_t();
  title_buf->printf(translator::translate("Journey time of %s"), line->get_name());
  set_name(*title_buf);
  
  set_table_layout(1,0);
  insufficient_cnv_label.set_color(color_idx_to_rgb(COL_ORANGE));
  insufficient_cnv_label.set_text("The latest departure slots were not used.");
  add_component(&insufficient_cnv_label);
  
  gui_aligned_container_t* container = add_table(NUM_ARRIVAL_TIME_STORED+2,1);
  new_component<gui_label_t>("Halt name");
  const char* texts[NUM_ARRIVAL_TIME_STORED+1] = {"Average", "1st", "2nd", "3rd", "4th", "5th"};
  gui_label_buf_t* lb;
  for(uint8 i=0; i<NUM_ARRIVAL_TIME_STORED+1; i++) {
    lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
    lb->buf().printf("%s", texts[i]);
    lb->update();
  }
  end_table();
  scr_size min_size = get_min_size();
	container->set_size(scr_size(max(get_size().w, min_size.w), min_size.h) );
  
  add_component(&scrolly);
  
  add_table(2,2)->set_force_equal_columns(true);
  {
    bt_copy_names.init(button_t::roundbox | button_t::flexible, translator::translate("Copy names"));
    bt_copy_names.set_tooltip("Copy station names to clipboard.");
    bt_copy_names.add_listener(this);
    add_component(&bt_copy_names);
    
    bt_copy_stations.init(button_t::roundbox | button_t::flexible, translator::translate("Copy stations"));
    bt_copy_stations.set_tooltip("Copy station names and their position to clipboard.");
    bt_copy_stations.add_listener(this);
    add_component(&bt_copy_stations);
    
    bt_copy_csv.init(button_t::roundbox | button_t::flexible, translator::translate("Copy csv format"));
    bt_copy_csv.set_tooltip("Copy station names and journey time to clipboard in csv format.");
    bt_copy_csv.add_listener(this);
    add_component(&bt_copy_csv);
    
    new_component<gui_fill_t>();
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
  else if(  comp==&bt_copy_csv  ) {
    copy_csv_format(schedule, player,  journey_times);
  }
  return true;
}

gui_journey_time_info_t::~gui_journey_time_info_t() {
  delete title_buf;
  for(uint8 i=0; i<journey_times.get_count(); i++) {
    delete[] journey_times[i];
  }
}


void gui_journey_time_info_t::update() {
  // append journey_times entries if required.
  for(uint8 i=journey_times.get_count(); i<schedule->entries.get_count(); i++) {
    journey_times.append(new uint32[NUM_ARRIVAL_TIME_STORED+1]);
  }
  
  // calculate journey time and average time
  journey_time_sum = 0;
  for(uint8 i=0; i<schedule->entries.get_count(); i++) {
    uint32 sum = 0;
    uint8 cnt = 0;
    const uint8 kc = (schedule->entries[i].at_index + NUM_ARRIVAL_TIME_STORED - 1) % NUM_ARRIVAL_TIME_STORED;
    for(uint8 k=0; k<NUM_ARRIVAL_TIME_STORED; k++) {
      uint32* ca = schedule->entries[i].journey_time;
      uint8 ica = (kc + NUM_ARRIVAL_TIME_STORED - k) % NUM_ARRIVAL_TIME_STORED;
      if(  ca[ica]>0  ) {
        journey_times[i][k+1] = tick_to_divided_time(ca[ica]);
        sum += journey_times[i][k+1];
        cnt += 1;
      } else {
        journey_times[i][k+1] = 0;
      }
    }
    
    if(  cnt>0  ) {
      journey_times[i][0] = sum/cnt;
      journey_time_sum += sum/cnt;
    } else {
      journey_times[i][0] = 0;
    }
  }
  
  // disable copy buttons if schedule is empty
  bt_copy_names.enable(!schedule->entries.empty());
  bt_copy_stations.enable(!schedule->entries.empty());
  bt_copy_csv.enable(!schedule->entries.empty());
  
  // update stat
  bool empty_slot_exists = false;
  stat.update(line, journey_times, empty_slot_exists);
  insufficient_cnv_label.set_visible(empty_slot_exists);
  reset_min_windowsize();
}
