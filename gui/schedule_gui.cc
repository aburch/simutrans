/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simline.h"
#include "../simcolor.h"
#include "../simhalt.h"
#include "../simworld.h"
#include "../simmenu.h"
#include "../simconvoi.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "../boden/grund.h"

#include "../obj/zeiger.h"

#include "../dataobj/schedule.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include "../player/simplay.h"

#include "../tpl/vector_tpl.h"

#include "depot_frame.h"
#include "schedule_gui.h"
#include "line_item.h"

#include "components/gui_button.h"
#include "components/gui_image.h"
#include "components/gui_textarea.h"
#include "minimap.h"

static karte_ptr_t welt;

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
		last_schedule = NULL;

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
		update_schedule();

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

	char const* get_text () const OVERRIDE { return buf; }

	sint8 get_wait_shift() const { return wait; }
};

cbuffer_t schedule_gui_stats_t::buf;

schedule_gui_t::schedule_gui_t(schedule_t* schedule_, player_t* player_, convoihandle_t cnv_) :
	gui_frame_t( translator::translate("Fahrplan"), NULL),
	line_selector(line_scrollitem_t::compare),
	lb_waitlevel(SYSCOL_TEXT_HIGHLIGHT, gui_label_t::right),
	lb_wait("month wait time"),
	lb_load("Full load"),
	stats(new schedule_gui_stats_t() ),
	scrolly(stats)
{
	schedule = NULL;
	player   = NULL;
	if (schedule_) {
		init(schedule_, player_, cnv_);
	}
}

schedule_gui_t::~schedule_gui_t()
{
	if(  player  ) {
		update_tool( false );
		// hide schedule on minimap (may not current, but for safe)
		minimap_t::get_instance()->set_selected_cnv( convoihandle_t() );
	}
	delete schedule;
	delete stats;
}

void schedule_gui_t::init(schedule_t* schedule_, player_t* player, convoihandle_t cnv)
{
	// initialization
	this->old_schedule = schedule_;
	this->cnv = cnv;
	this->player = player;
	set_owner(player);

	// prepare editing
	old_schedule->start_editing();
	schedule = old_schedule->copy();
	if(  !cnv.is_bound()  ) {
		old_line = new_line = linehandle_t();
	}
	else {
		// set this schedule as current to show on minimap if possible
		minimap_t::get_instance()->set_selected_cnv( cnv );
		old_line = new_line = cnv->get_line();
	}
	old_line_count = 0;

	stats->player = player;
	stats->schedule = schedule;
	stats->update_schedule();
	stats->add_listener(this);

	set_table_layout(1,0);

	if(  cnv.is_bound()  ) {
		add_table(3,1);
		// things, only relevant to convois, like creating/selecting lines
		new_component<gui_label_t>("Serves Line:");
		bt_promote_to_line.init( button_t::roundbox, "promote to line");
		bt_promote_to_line.set_tooltip("Create a new line based on this schedule");
		bt_promote_to_line.add_listener(this);
		new_component<gui_fill_t>();
		add_component(&bt_promote_to_line);
		end_table();

		line_selector.clear_elements();

		init_line_selector();
		line_selector.add_listener(this);
		add_component(&line_selector);
	}

	// loading level and waiting time
	add_table(2,2);
	{
		add_component(&lb_load);

		numimp_load.set_width( 60 );
		numimp_load.set_value( schedule->get_current_entry().minimum_loading );
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

	// return tickets
	if(  !env_t::hide_rail_return_ticket  ||  schedule->get_waytype()==road_wt  ||  schedule->get_waytype()==air_wt  ||  schedule->get_waytype()==water_wt  ) {
		//  hide the return ticket on rail stuff, where it causes much trouble
		bt_return.init(button_t::roundbox, "return ticket");
		bt_return.set_tooltip("Add stops for backward travel");
		bt_return.add_listener(this);
		add_component(&bt_return);
	}

	// action button row
	add_table(3,1)->set_force_equal_columns(true);
	bt_add.init(button_t::roundbox_state | button_t::flexible, "Add Stop");
	bt_add.set_tooltip("Appends stops at the end of the schedule");
	bt_add.add_listener(this);
	bt_add.pressed = true;
	add_component(&bt_add);

	bt_insert.init(button_t::roundbox_state | button_t::flexible, "Ins Stop");
	bt_insert.set_tooltip("Insert stop before the current stop");
	bt_insert.add_listener(this);
	bt_insert.pressed = false;
	add_component(&bt_insert);

	bt_remove.init(button_t::roundbox_state | button_t::flexible, "Del Stop");
	bt_remove.set_tooltip("Delete the current stop");
	bt_remove.add_listener(this);
	bt_remove.pressed = false;
	add_component(&bt_remove);
	end_table();

	scrolly.set_show_scroll_x(true);
	scrolly.set_scroll_amount_y(LINESPACE+1);
	add_component(&scrolly);

	mode = adding;
	update_selection();

	set_resizemode(diagonal_resize);

	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
}


void schedule_gui_t::update_tool(bool set)
{
	if(!set  ||  mode==removing  ||  mode==undefined_mode) {
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


void schedule_gui_t::update_selection()
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


/**
 * Mouse clicks are hereby reported to its GUI-Components
 */
bool schedule_gui_t::infowin_event(const event_t *ev)
{
	if( (ev)->ev_class == EVENT_CLICK  &&  !((ev)->ev_code==MOUSE_WHEELUP  ||  (ev)->ev_code==MOUSE_WHEELDOWN)  &&  !line_selector.getroffen(ev->cx, ev->cy-D_TITLEBAR_HEIGHT)  )  {

		// close combo box; we must do it ourselves, since the box does not receive outside events ...
		line_selector.close_box();
	}
	else if(  ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE  &&  schedule!=NULL  ) {

		stats->highlight_schedule( false );

		update_tool( false );
		schedule->cleanup();
		schedule->finish_editing();
		// now apply the changes
		if(  cnv.is_bound()  ) {
			// do not send changes if the convoi is about to be deleted
			if(  cnv->get_state() != convoi_t::SELF_DESTRUCT  ) {
				// if a line is selected
				if(  new_line.is_bound()  ) {
					// if the selected line is different to the convoi's line, apply it
					if(  new_line!=cnv->get_line()  ) {
						char id[16];
						sprintf( id, "%i,%i", new_line.get_id(), schedule->get_current_stop() );
						cnv->call_convoi_tool( 'l', id );
					}
					else {
						cbuffer_t buf;
						schedule->sprintf_schedule( buf );
						cnv->call_convoi_tool( 'g', buf );
					}
				}
				else {
					cbuffer_t buf;
					schedule->sprintf_schedule( buf );
					cnv->call_convoi_tool( 'g', buf );
				}

				if(  cnv->in_depot()  ) {
					const grund_t *const ground = welt->lookup( cnv->get_home_depot() );
					if(  ground  ) {
						const depot_t *const depot = ground->get_depot();
						if(  depot  ) {
							depot_frame_t *const frame = dynamic_cast<depot_frame_t *>( win_get_magic( (ptrdiff_t)depot ) );
							if(  frame  ) {
								frame->update_data();
							}
						}
					}
				}
			}
		}
	}
	else if(  ev->ev_class == INFOWIN  &&  (ev->ev_code == WIN_TOP  ||  ev->ev_code == WIN_OPEN)  &&  schedule!=NULL  ) {
		// just to be sure, renew the tools ...
		update_tool( true );
	}

	return gui_frame_t::infowin_event(ev);
}


bool schedule_gui_t::action_triggered( gui_action_creator_t *comp, value_t p)
{
DBG_MESSAGE("schedule_gui_t::action_triggered()","comp=%p combo=%p",comp,&line_selector);

	if(comp == &bt_add) {
		mode = adding;
		bt_add.pressed = true;
		bt_insert.pressed = false;
		bt_remove.pressed = false;
		update_tool( true );
	}
	else if(comp == &bt_insert) {
		mode = inserting;
		bt_add.pressed = false;
		bt_insert.pressed = true;
		bt_remove.pressed = false;
		update_tool( true );
	}
	else if(comp == &bt_remove) {
		mode = removing;
		bt_add.pressed = false;
		bt_insert.pressed = false;
		bt_remove.pressed = true;
		update_tool( false );
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
		schedule->add_return_way();
	}
	else if(comp == &line_selector) {
		uint32 selection = p.i;
		if(  line_scrollitem_t *li = dynamic_cast<line_scrollitem_t*>(line_selector.get_element(selection))  ) {
			new_line = li->get_line();
			stats->highlight_schedule( false );
			schedule->copy_from( new_line->get_schedule() );
			schedule->start_editing();
		}
		else {
			// remove line
			new_line = linehandle_t();
			line_selector.set_selection( 0 );
		}
	}
	else if(comp == &bt_promote_to_line) {
		// update line schedule via tool!
		tool_t *tool = create_tool( TOOL_CHANGE_LINE | SIMPLE_TOOL );
		cbuffer_t buf;
		buf.printf( "c,0,%i,%ld,", (int)schedule->get_type(), (long)(intptr_t)old_schedule );
		schedule->sprintf_schedule( buf );
		tool->set_default_param(buf);
		welt->set_tool( tool, player );
		// since init always returns false, it is safe to delete immediately
		delete tool;
	}
	else if (comp == stats) {
		// click on one of the schedule entries
		const int line = p.i;

		if(  line >= 0 && line < schedule->get_count()  ) {
			schedule->set_current_stop( line );
			if(  mode == removing  ) {
				stats->highlight_schedule( false );
				schedule->remove();
				action_triggered( &bt_add, value_t() );
			}
			update_selection();
		}
	}
	// recheck lines
	if(  cnv.is_bound()  ) {
		// unequal to line => remove from line ...
		if(  new_line.is_bound()  &&  !schedule->matches(welt,new_line->get_schedule())  ) {
			new_line = linehandle_t();
			line_selector.set_selection(0);
		}
		// only assign old line, when new_line is not equal
		if(  !new_line.is_bound()  &&  old_line.is_bound()  &&   schedule->matches(welt,old_line->get_schedule())  ) {
			new_line = old_line;
			init_line_selector();
		}
	}
	return true;
}


void schedule_gui_t::init_line_selector()
{
	line_selector.clear_elements();
	int selection = 0;
	vector_tpl<linehandle_t> lines;

	player->simlinemgmt.get_lines(schedule->get_type(), &lines);

	// keep assignment with identical schedules
	if(  new_line.is_bound()  &&  !schedule->matches( welt, new_line->get_schedule() )  ) {
		if(  old_line.is_bound()  &&  schedule->matches( welt, old_line->get_schedule() )  ) {
			new_line = old_line;
		}
		else {
			new_line = linehandle_t();
		}
	}
	int offset = 0;
	if(  !new_line.is_bound()  ) {
		selection = 0;
		offset = 1;
		line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate("<no line>"), SYSCOL_TEXT ) ;
	}

	FOR(  vector_tpl<linehandle_t>,  line,  lines  ) {
		line_selector.new_component<line_scrollitem_t>(line) ;
		if(  !new_line.is_bound()  ) {
			if(  schedule->matches( welt, line->get_schedule() )  ) {
				selection = line_selector.count_elements()-1;
				new_line = line;
			}
		}
		else if(  new_line == line  ) {
			selection = line_selector.count_elements()-1;
		}
	}

	line_selector.set_selection( selection );
	line_scrollitem_t::sort_mode = line_scrollitem_t::SORT_BY_NAME;
	line_selector.sort( offset );
	old_line_count = player->simlinemgmt.get_line_count();
	last_schedule_count = schedule->get_count();
}



void schedule_gui_t::draw(scr_coord pos, scr_size size)
{
	if(  player->simlinemgmt.get_line_count()!=old_line_count  ||  last_schedule_count!=schedule->get_count()  ) {
		// lines added or deleted
		init_line_selector();
		last_schedule_count = schedule->get_count();
	}

	// after loading in network games, the schedule might still being updated
	if(  cnv.is_bound()  &&  cnv->get_state()==convoi_t::EDIT_SCHEDULE  &&  schedule->is_editing_finished()  ) {
		assert( convoi_t::EDIT_SCHEDULE==1 ); // convoi_t::EDIT_SCHEDULE is 1
		schedule->start_editing();
		cnv->call_convoi_tool( 's', "1" );
	}

	// always dirty, to cater for shortening of halt names and change of selections
	set_dirty();
	gui_frame_t::draw(pos,size);
}


/**
 * Set window size and adjust component sizes and/or positions accordingly
 */
void schedule_gui_t::set_windowsize(scr_size size)
{
	gui_frame_t::set_windowsize(size);
	// manually enlarge size of wait_load combobox
	wait_load.set_size( scr_size(numimp_load.get_size().w, wait_load.get_size().h) );
	// make scrolly take all of space
	scrolly.set_size( scr_size(scrolly.get_size().w, get_client_windowsize().h - scrolly.get_pos().y - D_MARGIN_BOTTOM));

}


void schedule_gui_t::map_rotate90( sint16 y_size)
{
	schedule->rotate90(y_size);
}


void schedule_gui_t::rdwr(loadsave_t *file)
{
	// this handles only schedules of bound convois
	// lines are handled by line_management_gui_t

	// window size
	scr_size size = get_windowsize();
	size.rdwr( file );

	// convoy data
	convoi_t::rdwr_convoihandle_t(file, cnv);

	// save edited schedule
	if(  file->is_loading()  ) {
		// dummy types
		schedule = new truck_schedule_t();
	}
	schedule->rdwr(file);

	if(  file->is_loading()  ) {
		if(  cnv.is_bound() ) {

			schedule_t *save_schedule = schedule->copy();

			init(cnv->get_schedule(), cnv->get_owner(), cnv);
			// init replaced schedule, restore
			schedule->copy_from(save_schedule);
			delete save_schedule;

			set_windowsize(size);

			// draw will init editing phase again, has to be synchronized
			cnv->get_schedule()->finish_editing();
			schedule->finish_editing();

			win_set_magic(this, (ptrdiff_t)cnv->get_schedule());
		}
		else {
			player = NULL; // prevent destructor from updating
			destroy_win( this );
			dbg->error( "schedule_gui_t::rdwr", "Could not restore schedule window for (%d)", cnv.get_id() );
		}
	}
}
