/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../../simline.h"
#include "../../simcolor.h"
#include "../../simhalt.h"
#include "../../simworld.h"
#include "../../simmenu.h"
#include "../../simconvoi.h"
#include "../../display/simgraph.h"
#include "../../display/viewport.h"

#include "../../utils/simstring.h"
#include "../../utils/cbuffer_t.h"

#include "../../boden/grund.h"

#include "../../obj/zeiger.h"

#include "../../dataobj/schedule.h"
#include "../../dataobj/loadsave.h"
#include "../../dataobj/translator.h"
#include "../../dataobj/environment.h"

#include "../../player/simplay.h"

#include "../../tpl/vector_tpl.h"

#include "../simwin.h"
#include "../depot_frame.h"
#include "../minimap.h"

#include "gui_button.h"
#include "gui_image.h"
#include "gui_textarea.h"
#include "gui_component.h"

#include "gui_schedule.h"

static karte_ptr_t welt;

char *gui_schedule_t::bt_mode_text[MAX_MODE] = { "Add Stop","Ins Stop", "Del Stop" };
char *gui_schedule_t::bt_mode_tooltip[MAX_MODE] = { "Appends stops at the end of the schedule","Insert stop before the current stop", "Delete the current stop" };

/**
 * One entry in the list of schedule entries.
 */
class gui_schedule_entry_t : public gui_aligned_container_t, public gui_action_creator_t
{
	schedule_entry_t entry;
	bool is_current;
	uint number;
	player_t* player;
	gui_image_t arrow;
	gui_label_buf_t stop;

public:
	gui_schedule_entry_t(player_t* pl, schedule_entry_t e, uint n)
	{
		player = pl;
		entry  = e;
		number = n;
		is_current = false;
		set_table_layout(2,1);

		add_component(&arrow);
		arrow.set_image(gui_theme_t::pos_button_img[0], true);

		add_component(&stop);
		update_label();
	}

	void update_label()
	{
		stop.buf().printf("%i) ", number+1);
		schedule_t::gimme_stop_name(stop.buf(), welt, player, entry, -1);
		stop.update();
	}

	void draw(scr_coord offset) OVERRIDE
	{
		update_label();
		if (is_current) {
			display_fillbox_wh_clip_rgb(pos.x + offset.x, pos.y + offset.y, size.w, size.h, SYSCOL_LIST_BACKGROUND_SELECTED_F, false);
		}
		gui_aligned_container_t::draw(offset);
	}

	void set_active(bool yesno)
	{
		is_current = yesno;
		arrow.set_image(gui_theme_t::pos_button_img[yesno ? 1: 0], true);
		stop.set_color(yesno ? SYSCOL_TEXT_HIGHLIGHT : SYSCOL_TEXT);
	}

	bool infowin_event(const event_t *ev) OVERRIDE
	{
		if( ev->ev_class == EVENT_CLICK ) {
			if(  IS_RIGHTCLICK(ev)  ||  ev->mx < stop.get_pos().x) {
				// just center on it
				welt->get_viewport()->change_world_position( entry.pos );
			}
			else {
				call_listeners(number);
			}
			return true;
		}
		return false;
	}
};

/**
 * List of displayed schedule entries.
 */
class schedule_gui_stats_t : public gui_aligned_container_t, action_listener_t, public gui_action_creator_t
{
	static cbuffer_t buf;

	vector_tpl<gui_schedule_entry_t*> entries;
	schedule_t *last_schedule; ///< last displayed schedule
	zeiger_t *current_stop_mark; ///< mark current stop on map
public:
	schedule_t *schedule;      ///< schedule under editing
	player_t*  player;

	schedule_gui_stats_t()
	{
		set_table_layout(1,0);
		last_schedule = schedule = NULL;

		current_stop_mark = new zeiger_t(koord3d::invalid, NULL );
		current_stop_mark->set_image( tool_t::general_tool[TOOL_SCHEDULE_ADD]->cursor );
	}

	~schedule_gui_stats_t()
	{
		delete current_stop_mark;
		delete last_schedule;
	}

	// shows/deletes highlighting of tiles
	void highlight_schedule(bool marking)
	{
		marking &= env_t::visualize_schedule;
		FOR(minivec_tpl<schedule_entry_t>, const& i, schedule->entries) {
			if (grund_t* const gr = welt->lookup(i.pos)) {
				for(  uint idx=0;  idx<gr->get_top();  idx++  ) {
					obj_t *obj = gr->obj_bei(idx);
					if(  marking  ) {
						if(  !obj->is_moving()  ) {
							obj->set_flag( obj_t::highlight );
						}
					}
					else {
						obj->clear_flag( obj_t::highlight );
					}
				}
				gr->set_flag( grund_t::dirty );
				// here on water
				if(  gr->is_water()  ||  gr->ist_natur()  ) {
					if(  marking  ) {
						gr->set_flag( grund_t::marked );
					}
					else {
						gr->clear_flag( grund_t::marked );
					}
				}

			}
		}
		// always remove
		if(  grund_t *old_gr = welt->lookup(current_stop_mark->get_pos())  ) {
			current_stop_mark->mark_image_dirty( current_stop_mark->get_image(), 0 );
			old_gr->obj_remove( current_stop_mark );
			old_gr->set_flag( grund_t::dirty );
			current_stop_mark->set_pos( koord3d::invalid );
		}
		// add if required
		if(  marking  &&  schedule->get_current_stop() < schedule->get_count() ) {
			current_stop_mark->set_pos( schedule->entries[schedule->get_current_stop()].pos );
			if(  grund_t *gr = welt->lookup(current_stop_mark->get_pos())  ) {
				gr->obj_add( current_stop_mark );
				current_stop_mark->set_flag( obj_t::dirty );
				gr->set_flag( grund_t::dirty );
			}
		}
		current_stop_mark->clear_flag( obj_t::highlight );
	}

	void update_schedule()
	{
		// compare schedules
		bool ok = (last_schedule != NULL)  &&  last_schedule->entries.get_count() == schedule->entries.get_count();
		for(uint i=0; ok  &&  i<last_schedule->entries.get_count(); i++) {
			ok = last_schedule->entries[i] == schedule->entries[i];
		}
		if (ok) {
			if (!last_schedule->empty()) {
				entries[ last_schedule->get_current_stop() ]->set_active(false);
				entries[ schedule->get_current_stop() ]->set_active(true);
				last_schedule->set_current_stop( schedule->get_current_stop() );
			}
		}
		else {
			remove_all();
			entries.clear();
			buf.clear();
			buf.append(translator::translate("Please click on the map to add\nwaypoints or stops to this\nschedule."));
			if (schedule->empty()) {
				new_component<gui_textarea_t>(&buf);
			}
			else {
				for(uint i=0; i<schedule->entries.get_count(); i++) {
					entries.append( new_component<gui_schedule_entry_t>(player, schedule->entries[i], i));
					entries.back()->add_listener( this );
				}
				entries[ schedule->get_current_stop() ]->set_active(true);
			}
			if (last_schedule) {
				last_schedule->copy_from(schedule);
			}
			else {
				last_schedule = schedule->copy();
			}
			set_size(get_min_size());
		}
		highlight_schedule(true);
	}

	void draw(scr_coord offset) OVERRIDE
	{
		if( schedule ) {
			update_schedule();
		}
		gui_aligned_container_t::draw(offset);
	}

	bool action_triggered(gui_action_creator_t *, value_t v) OVERRIDE
	{
		// has to be one of the entries
		call_listeners(v);
		return true;
	}
};

/**
 * Entries in the waiting-time selection.
 */
class gui_waiting_time_item_t : public gui_scrolled_list_t::const_text_scrollitem_t
{
private:
	cbuffer_t buf;
	sint8 wait;

public:
	gui_waiting_time_item_t(sint8 w) : gui_scrolled_list_t::const_text_scrollitem_t(NULL, SYSCOL_TEXT)
	{
		wait = w;
		if (wait == 0) {
			buf.append(translator::translate("off"));
		}
		else {
			buf.printf("1/%d",  1<<(16 - wait) );
		}
	}

	char const* get_text() const OVERRIDE { return buf; }

	sint8 get_wait_shift() const { return wait; }
};

cbuffer_t schedule_gui_stats_t::buf;

void gui_schedule_t::highlight_schedule( bool hl )
{
	stats->highlight_schedule(hl);
	update_tool(hl);
}

gui_schedule_t::gui_schedule_t(schedule_t* schedule_, player_t* player_, convoihandle_t cnv_, linehandle_t lin_) :
	lb_waitlevel(SYSCOL_TEXT_HIGHLIGHT, gui_label_t::right),
	lb_wait("month wait time"),
	lb_load("Full load"),
	stats(new schedule_gui_stats_t() ),
	scrolly(stats)
{
	scrolly.set_maximize( true );
	old_schedule = schedule = NULL;
	player   = NULL;
	mode = adding;

	set_table_layout(3,0);

	new_component<gui_margin_t>();
	// loading level and waiting time
	add_table(2,2);
	{
		add_component(&lb_load);

		numimp_load.set_width( 60 );
		numimp_load.set_value( 0 );
		numimp_load.set_limits( 0, 100 );
		numimp_load.set_increment_mode( gui_numberinput_t::PROGRESS );
		numimp_load.add_listener(this);
		add_component(&numimp_load);

		add_component(&lb_wait);

		add_component(&wait_load);
		wait_load.add_listener(this);

		wait_load.new_component<gui_waiting_time_item_t>(0);
		for(sint8 w = 7; w<=16; w++) {
			wait_load.new_component<gui_waiting_time_item_t>(w);
		}
		wait_load.set_rigid(true);
	}
	end_table();
	new_component<gui_margin_t>();

	// action button row
	new_component<gui_margin_t>();
	add_table( 4, 1 )->set_force_equal_columns( true );
	{
		bt_apply.init(button_t::roundbox | button_t::flexible, "Apply schedule");
		bt_apply.set_tooltip("Apply this schedule");
		bt_apply.add_listener(this);
		bt_apply.pressed = false;
		add_component(&bt_apply);

		bt_revert.init(button_t::roundbox | button_t::flexible, "Revert schedule");
		bt_revert.set_tooltip("Revert to original schedule");
		bt_revert.add_listener(this);
		bt_revert.pressed = false;
		add_component(&bt_revert);

		bt_return.init(button_t::roundbox | button_t::flexible, "Revert schedule");
		bt_return.add_listener(this);
		add_component(&bt_return);

		bt_mode.init(button_t::roundbox | button_t::flexible, bt_mode_text[mode]);
		bt_mode.set_tooltip(bt_mode_tooltip[mode]);
		bt_mode.add_listener(this);
		add_component(&bt_mode);
	}
	end_table();
	new_component<gui_margin_t>();

	scrolly.set_show_scroll_x(true);
	scrolly.set_scroll_amount_y(LINESPACE+1);
	add_component(&scrolly,3);
	stats->add_listener(this);

	mode = adding;
	current_schedule_rotation = welt->get_settings().get_rotation();

	if (schedule_) {
		init(schedule_, player_, cnv_, lin_);
	}

	scrolly.set_maximize(true);
	
	set_size( gui_aligned_container_t::get_min_size() );
}

gui_schedule_t::~gui_schedule_t()
{
	delete schedule;
	delete stats;
}

void gui_schedule_t::init(schedule_t* schedule_, player_t* player, convoihandle_t cnv, linehandle_t lin)
{
	if( old_schedule != schedule_ ) {

		if( old_schedule ) {
			stats->highlight_schedule( false );
			update_tool( false );
			schedule->cleanup();
			schedule->finish_editing();
		}

		// initialization
		this->old_schedule = schedule_;
		this->convoi_mode = cnv;
		this->line_mode = lin;
		this->player = player;
		current_schedule_rotation = welt->get_settings().get_rotation();
		
		// prepare editing
		schedule = old_schedule->copy();

		make_return = (schedule->get_waytype() == road_wt || schedule->get_waytype() == air_wt || schedule->get_waytype() == water_wt);
		if(  make_return  ) {
			// return tickets
			bt_return.set_text("return ticket");
			bt_return.set_tooltip("Add stops for backward travel");
		}
		else {
			bt_return.set_text("Invert stops");
			bt_return.set_tooltip("Mirrors order of stops");
		}

		stats->player = player;
		stats->schedule = schedule;

		numimp_load.set_value( schedule->get_current_entry().minimum_loading );

		mode = adding;
		update_selection();
	}
	set_size(gui_aligned_container_t::get_min_size());
}


void gui_schedule_t::update_tool(bool set)
{
	if(  !set  ||  mode==removing  ) {
		// reset tools, if still selected ...
		if(welt->get_tool(player->get_player_nr())==tool_t::general_tool[TOOL_SCHEDULE_ADD]) {
			if(tool_t::general_tool[TOOL_SCHEDULE_ADD]->get_default_param()==(const char *)schedule) {
				welt->set_tool( tool_t::general_tool[TOOL_QUERY], player );
			}
		}
		else if(welt->get_tool(player->get_player_nr())==tool_t::general_tool[TOOL_SCHEDULE_INS]) {
			if(tool_t::general_tool[TOOL_SCHEDULE_INS]->get_default_param()==(const char *)schedule) {
				welt->set_tool( tool_t::general_tool[TOOL_QUERY], player );
			}
		}
	}
	else {
		//  .. or set them again
		if(mode==adding) {
			tool_t::general_tool[TOOL_SCHEDULE_ADD]->set_default_param((const char *)schedule);
			welt->set_tool( tool_t::general_tool[TOOL_SCHEDULE_ADD], player );
		}
		else if(mode==inserting) {
			tool_t::general_tool[TOOL_SCHEDULE_INS]->set_default_param((const char *)schedule);
			welt->set_tool( tool_t::general_tool[TOOL_SCHEDULE_INS], player );
		}
	}
}


void gui_schedule_t::update_selection()
{
	lb_wait.set_color( SYSCOL_BUTTON_TEXT_DISABLED );
	wait_load.disable();

	if(  !schedule->empty()  ) {
		schedule->set_current_stop( min(schedule->get_count()-1,schedule->get_current_stop()) );
		const uint8 current_stop = schedule->get_current_stop();
		if(  haltestelle_t::get_halt(schedule->entries[current_stop].pos, player).is_bound()  ) {
			lb_load.set_color( SYSCOL_TEXT );
			numimp_load.enable();
			numimp_load.set_value( schedule->entries[current_stop].minimum_loading );

			sint8 wait = 0;
			if(  schedule->entries[current_stop].minimum_loading>0  ) {
				lb_wait.set_color( SYSCOL_TEXT );
				wait_load.enable();

				wait = schedule->entries[current_stop].waiting_time_shift;
			}

			for(int i=0; i<wait_load.count_elements(); i++) {
				if (gui_waiting_time_item_t *item = dynamic_cast<gui_waiting_time_item_t*>( wait_load.get_element(i) ) ) {
					if (item->get_wait_shift() == wait) {
						wait_load.set_selection(i);
						break;
					}
				}
			}

		}
		else {
			lb_load.set_color( SYSCOL_BUTTON_TEXT_DISABLED );
			numimp_load.disable();
			numimp_load.set_value( 0 );
		}
	}
}


bool gui_schedule_t::action_triggered( gui_action_creator_t *comp, value_t p)
{
	if (comp == &bt_apply) {
		// tell to apply to listener
		value_t v;
		v.p = schedule;
		call_listeners(v);
	}
	else if (comp == &bt_revert) {
		// revert changes and tell listener
		if (schedule) {
			stats->highlight_schedule(false);
			delete schedule;
			schedule = NULL;
		}
		schedule = old_schedule->copy();
		stats->schedule = schedule;
		update_selection();
		value_t v;
		v.p = NULL;
		call_listeners(v);
	}
	else if (comp == &bt_mode) {
		mode = (mode_t)((int)(mode+1) % MAX_MODE);
		bt_mode.set_text( bt_mode_text[mode] );
		bt_mode.set_tooltip( bt_mode_tooltip[mode] );
		update_tool(true);
	}
	else if(comp == &numimp_load) {
		if (!schedule->empty()) {
			schedule->entries[schedule->get_current_stop()].minimum_loading = (uint8)p.i;
			update_selection();
		}
	}
	else if(comp == &wait_load) {
		if(!schedule->empty()) {
			if (gui_waiting_time_item_t *item = dynamic_cast<gui_waiting_time_item_t*>( wait_load.get_selected_item())) {
				schedule->entries[schedule->get_current_stop()].waiting_time_shift = item->get_wait_shift();
				update_selection();
			}
		}
	}
	else if(comp == &bt_return) {
		schedule->add_return_way(make_return);
	}
	else if (comp == stats) {
		// click on one of the schedule entries
		const int line = p.i;

		if(  line >= 0 && line < schedule->get_count()  ) {
			schedule->set_current_stop( line );
			if(  mode == removing  ) {
				stats->highlight_schedule( false );
				schedule->remove();
				action_triggered( &bt_mode, value_t() );
			}
			update_selection();
		}
	}
	return true;
}


void gui_schedule_t::draw(scr_coord pos)
{
	if( schedule ) {
		// check if we missed a map rotation
		const uint8 world_rotation = world()->get_settings().get_rotation();
		while(  current_schedule_rotation != world_rotation  ) {
			current_schedule_rotation = (current_schedule_rotation + 1) % 4;
			schedule->rotate90( (4+current_schedule_rotation - world_rotation) & 1 ? world()->get_size().x : world()->get_size().y );
		}
		bt_apply.enable(player == world()->get_active_player());

		if(  convoi_mode.is_bound()  ) {
			// change current entry while convois drives on
			schedule_t *cnv_scd = convoi_mode->get_schedule();
			koord3d current = cnv_scd->get_current_entry().pos;
			int idx = 0;
			FOR( minivec_tpl<schedule_entry_t>, ent, schedule->entries ) {
				if( ent.pos == current ) {
					schedule->set_current_stop( idx );
					break;
				}
				idx++;
			}
			
		}
	}
	// always dirty, to cater for shortening of halt names and change of selections
	gui_aligned_container_t::draw(pos);
}


/**
 * Set window size and adjust component sizes and/or positions accordingly
 */
void gui_schedule_t::set_size(scr_size size)
{
	gui_aligned_container_t::set_size(size);
	size = get_size();
	// manually enlarge size of wait_load combobox
	wait_load.set_size( scr_size(numimp_load.get_size().w, wait_load.get_size().h) );
	// make scrolly take all of space
	scrolly.set_size( scr_size(scrolly.get_size().w, get_size().h - scrolly.get_pos().y - D_MARGIN_BOTTOM));
}


void gui_schedule_t::rdwr(loadsave_t *file)
{
	// convoy data
	convoi_t::rdwr_convoihandle_t(file, convoi_mode);
	simline_t::rdwr_linehandle_t(file, line_mode);

	if(  file->is_loading()  ) {
		uint16 schedule_type, player_nr;
		schedule_t *load_schedule;

		file->rdwr_short(player_nr);
		player = welt->get_player(player_nr);

		file->rdwr_short(schedule_type);
		switch (schedule_type) {
			case schedule_t::truck_schedule:       load_schedule = new truck_schedule_t(); break;
			case schedule_t::train_schedule:       load_schedule = new train_schedule_t(); break;
			case schedule_t::ship_schedule:        load_schedule = new ship_schedule_t(); break;
			case schedule_t::airplane_schedule:    load_schedule = new airplane_schedule_t(); break;
			case schedule_t::monorail_schedule:    load_schedule = new monorail_schedule_t(); break;
			case schedule_t::tram_schedule:        load_schedule = new maglev_schedule_t(); break;
			case schedule_t::maglev_schedule:      load_schedule = new truck_schedule_t(); break;
			case schedule_t::narrowgauge_schedule: load_schedule = new narrowgauge_schedule_t(); break;
			default:
				load_schedule = new train_schedule_t();
				dbg->error("schedule_gui_t::rdwr", "Could not restore schedule window for generic schdule");
				load_schedule->rdwr(file);
				delete load_schedule;
				return;
		}

		load_schedule->rdwr( file );
		init(load_schedule, player, convoi_mode, line_mode);
		delete load_schedule;
		set_size(size);
	}
	else {
		uint16 schedule_type = schedule->get_type();
		uint16 player_nr = player->get_player_nr();
		file->rdwr_short(player_nr);
		file->rdwr_short(schedule_type);
		schedule->rdwr( file );
	}
}


