#ifndef times_history_container_h
#define times_history_container_h

#include "components/gui_component.h"
#include "gui_frame.h"
#include "components/gui_label.h"
#include "times_history_entry.h"

// @author: suitougreentea
class times_history_container_t :
	public gui_container_t,
	public action_listener_t
{
private:
	const player_t *owner;
	schedule_t *schedule;
	times_history_map *map;

	bool mirrored;
	bool reversed;

	gui_label_t lbl_order;
	gui_label_t lbl_name_header;
	gui_label_t lbl_time_header;
	gui_label_t lbl_average_header;

	slist_tpl<button_t *> buttons;
	slist_tpl<gui_label_t *> name_labels;
	slist_tpl<times_history_entry_t *> entry_labels;

	static class karte_ptr_t welt;

	void delete_components();

public:
	times_history_container_t() {}
	times_history_container_t(const player_t *owner, schedule_t *schedule, times_history_map *map, bool mirrored, bool reversed);
	~times_history_container_t();

	void update_container();

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void draw(scr_coord offset);
};

#endif
