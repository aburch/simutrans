#include "goods_waiting_time.h"
#include "../dataobj/translator.h"
#include "../dataobj/schedule.h"
#include "../simhalt.h"
#include "../simline.h"
#include "../simworld.h"

gui_goods_waiting_time_t::gui_goods_waiting_time_t(linehandle_t line, player_t* player) :
    gui_frame_t(NULL, player),
    stat(player, line->get_schedule()),
    scrolly(&stat),
    schedule(line->get_schedule())
{
    title_buf.printf(translator::translate("Goods waiting time of %s"), line->get_name());
    set_name(title_buf);

    update();

    set_table_layout(1, 0);
    gui_aligned_container_t* container = add_table(NUM_WAITING_TIME_STORED+2,1);
    new_component<gui_label_t>("Halt name");
    const char* texts[NUM_WAITING_TIME_STORED+1] = {"Average", "1st", "2nd", "3rd", "4th", "5th"};
    gui_label_buf_t* lb;
    for(uint8 i=0; i<NUM_WAITING_TIME_STORED+1; i++) {
        lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
        lb->buf().printf("%s", texts[i]);
        lb->update();
    }
    end_table();
    scr_size min_size = get_min_size();
	container->set_size(scr_size(max(get_size().w, min_size.w), min_size.h) );

    add_component(&scrolly);
    set_resizemode(diagonal_resize);
    reset_min_windowsize();
    set_windowsize(get_min_windowsize());
}


void gui_goods_waiting_time_stat_t::update()
{
    const scr_size size = get_size();
    remove_all();
    set_table_layout(NUM_WAITING_TIME_STORED+2,0);
    for(uint8 index = 0; index < schedule->entries.get_count(); index++) {
        const schedule_entry_t& entry = schedule->entries[index];
        const halthandle_t halt = haltestelle_t::get_halt(entry.pos, player);
        gui_label_buf_t *title_lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::left);
        if(  halt.is_bound()  ) {
            // Show the halt name
            title_lb->buf().printf("%s", halt->get_name());
        } else {
            // maybe a waypoint
            title_lb->buf().printf(translator::translate("Wegpunkt"));
        }
        title_lb->update();

        gui_label_buf_t *average_lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
        average_lb->buf().printf("%d", world()->tick_to_divided_time(entry.get_average_waiting_time()));
        average_lb->update();
        for(uint8 j=0; j<NUM_WAITING_TIME_STORED; j++) {
            gui_label_buf_t *lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
            uint32 t = entry.waiting_time[j];
            if(  t==0  ) {
                lb->buf().printf("-");
            } else {
                lb->buf().printf("%d", world()->tick_to_divided_time(t));
            }
            lb->update();
        }
    }

    const scr_size min_size = get_min_size();
	set_size(scr_size(max(size.w, min_size.w), min_size.h) );
}
