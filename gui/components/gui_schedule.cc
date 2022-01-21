/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../../simline.h"
#include "../../simcolor.h"
#include "../../simintr.h"
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

#include "gui_button.h"
#include "gui_image.h"
#include "gui_textarea.h"
#include "gui_timeinput.h"
#include "gui_component.h"

#include "gui_schedule.h"

static karte_ptr_t welt;

#define DELETE_FLAG (0x8000)
#define UP_FLAG (0x4000)
#define DOWN_FLAG (0x2000)

/**
 * One entry in the list of schedule entries.
 */
class gui_schedule_entry_t :
	public gui_aligned_container_t,
	public gui_action_creator_t,
	public action_listener_t
{
	schedule_entry_t entry;
	bool is_current;
	uint number;
	player_t* player;
	gui_label_buf_t stop;
	button_t arrow, up, down, del;
	gui_label_buf_t stop_extra;
	bool valid;

public:
	gui_schedule_entry_t(player_t* pl, schedule_entry_t e, uint n)
	{
		player = pl;
		entry  = e;
		number = n;
		is_current = false;
		valid = true;
		set_table_layout(6,1);

		arrow.init( button_t::posbutton_automatic, "" );

		up.init( button_t::arrowup, "" );
		up.add_listener( this );

		down.init( button_t::arrowdown, "" );
		down.add_listener( this );

		add_component(&arrow);
		add_component(&stop);
		add_component(&up);
		add_component(&down);

		del.init( button_t::imagebox, NULL );
		del.set_image( skinverwaltung_t::gadget->get_image_id(SKIN_GADGET_CLOSE) );
		del.set_size( gui_theme_t::gui_arrow_left_size );
		del.add_listener( this );
		del.set_tooltip( "Delete the current stop" );
		add_component( &del );

		add_component(&stop_extra);

		update_label();
	}

	void update_label()
	{
		arrow.set_targetpos3d(entry.pos);
		stop.buf().printf("%i) ", number+1);
		schedule_t::gimme_stop_name(stop.buf(), welt, player, entry, -1);
		stop.update();
		if(  haltestelle_t::get_halt( entry.pos, player ).is_bound()  ) {
			if(  !entry.get_absolute_departures()  ) {
				if( entry.waiting_time > 0 ) {
					// relative waiting time
					stop_extra.buf().printf( "(%d%% %s%s)", (int)entry.minimum_loading, translator::translate( "in " ), difftick_to_string( entry.get_waiting_ticks(), false ) );
				}
				else {
					stop_extra.buf().printf( "(%i%%)", entry.minimum_loading );
				}
			}
			else {
				// absolute departure time
				uint16 i = 0;
				uint32 starttick = entry.get_waiting_ticks();
				uint32 delta = welt->ticks_per_world_month/entry.get_absolute_departures();
				stop_extra.buf().append( tick_to_string( starttick, true ) );
				while( ++i < entry.get_absolute_departures() ) {
					stop_extra.buf().append( ", " );
					stop_extra.buf().append( tick_to_string( starttick+i*delta, true ) );
				}
			}
		}
		stop_extra.update();
		list_dirty = false; // or the first mouseclick will be swallowed!
	}

	void draw(scr_coord offset) OVERRIDE
	{
		update_label();
		if (is_current  ||  !valid) {
			display_fillbox_wh_clip_rgb(pos.x + offset.x, pos.y + offset.y, size.w, size.h, valid ? SYSCOL_LIST_BACKGROUND_SELECTED_F : MONEY_MINUS, false);
		}
		gui_aligned_container_t::draw(offset);
	}

	void set_active(bool yesno)
	{
		is_current = yesno;
		stop.set_color(yesno ? SYSCOL_TEXT_HIGHLIGHT : SYSCOL_TEXT);
		stop_extra.set_color(yesno ? SYSCOL_TEXT_HIGHLIGHT : SYSCOL_TEXT);
	}

	bool action_triggered( gui_action_creator_t *c, value_t ) OVERRIDE
	{
		if( c == &up ) {
			call_listeners( UP_FLAG | number );
			return true;
		}
		if( c == &down ) {
			call_listeners( DOWN_FLAG | number );
			return true;
		}
		if( c == &del ) {
			call_listeners( DELETE_FLAG | number );
			return true;
		}
		return false;
	}

	bool infowin_event(const event_t *ev) OVERRIDE
	{
		if(  IS_RIGHTRELEASE(ev)  ) {
			// just center on it
			welt->get_viewport()->change_world_position( entry.pos );
			return true;
		}
		else if(ev->button_state==1  ){
			set_focus( this );
			if( !gui_aligned_container_t::infowin_event( ev ) && stop.getroffen( ev->cx, ev->cy ) ) {
				// not handled, so we make i aktive
				call_listeners( number );
			}
			return true;
		}
		return gui_aligned_container_t::infowin_event(ev);
	}

	void mark_valid(bool valid)
	{
		this->valid = valid;
	}
};

/**
 * List of displayed schedule entries.
 */
class schedule_gui_stats_t :
	public gui_aligned_container_t,
	public action_listener_t,
	public gui_action_creator_t
{
	static cbuffer_t buf;

	vector_tpl<gui_schedule_entry_t*> entries;
	schedule_t *last_schedule; ///< last displayed schedule
	zeiger_t *current_stop_mark; ///< mark current stop on map

	bool invalid_entries;
public:
	schedule_t *schedule;      ///< schedule under editing
	player_t*  player;

	schedule_gui_stats_t()
	{
		set_table_layout(1,0);
		last_schedule = schedule = NULL;
		invalid_entries = false;

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

	void update_schedule(bool highlight)
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
			invalid_entries = false;
			buf.clear();
			buf.append(translator::translate("Please click on the map to add\nwaypoints or stops to this\nschedule."));
			if (schedule->empty()) {
				new_component<gui_textarea_t>(&buf);
			}
			else {
				for(uint i=0; i<schedule->entries.get_count(); i++) {

					gui_schedule_entry_t* entry = new_component<gui_schedule_entry_t>(player, schedule->entries[i], i);
					// mark double entries as invalid
					bool valid =     (schedule->entries[i].pos != schedule->entries[ (i+1)                               % schedule->entries.get_count() ].pos)
					             &&  (schedule->entries[i].pos != schedule->entries[ (i-1+schedule->entries.get_count()) % schedule->entries.get_count() ].pos);
					entry->mark_valid(valid);
					entry->add_listener( this );
					entries.append(entry);

					invalid_entries |= schedule->entries.get_count() > 1 && !valid;
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

			call_listeners( schedule->get_current_stop() );
		}
		if (highlight) {
			highlight_schedule(true);
		}
	}

	void draw(scr_coord offset) OVERRIDE
	{
		if( schedule ) {
			update_schedule(true);
		}
		gui_aligned_container_t::draw(offset);
	}

	bool action_triggered(gui_action_creator_t *, value_t v) OVERRIDE
	{
		// has to be one of the entries
		if( v.i & DELETE_FLAG ) {
			uint8 delete_stop = v.i & 0x00FF;
			highlight_schedule( false );
			schedule->remove_entry( delete_stop );
			highlight_schedule( true );
			call_listeners( schedule->get_current_stop() );
		}
		else if( v.i & UP_FLAG ) {
			uint8 up_stop = v.i & 0x00FF;
			schedule->move_entry_backward( up_stop );
			call_listeners( schedule->get_current_stop() );
		}
		else if( v.i & DOWN_FLAG ) {
			uint8 down_stop = v.i & 0x00FF;
			schedule->move_entry_forward( down_stop );
			call_listeners( schedule->get_current_stop() );
		}
		else {
			call_listeners(v);
		}
		return true;
	}

	bool has_invalid_entries() const
	{
		return invalid_entries;
	}
};

cbuffer_t schedule_gui_stats_t::buf;

schedule_t *gui_schedule_t::get_old_schedule() const
{
	if( convoi_mode.is_bound() ) {
		return convoi_mode->get_schedule();
	}
	else if( line_mode.is_bound() ) {
		return line_mode->get_schedule();
	}
	return  old_schedule;
}

void gui_schedule_t::highlight_schedule( bool hl )
{
	stats->highlight_schedule(hl);
	update_tool(hl);
}

gui_schedule_t::gui_schedule_t() :
	stats( new schedule_gui_stats_t() ),
	scrolly( stats ),
	departure( NULL )
{
	scrolly.set_maximize( true );
	old_schedule = schedule = NULL;
	player   = NULL;

	set_table_layout(1,0);

	// loading level and waiting time
	loading_details = add_table( 3, 3 );
	loading_details->set_margin( scr_size(D_MARGIN_LEFT,0), scr_size(D_MARGIN_RIGHT,0) );
	{
		add_component(&cb_wait,2);
		cb_wait.add_listener( this );
		cb_wait.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate( "Full load" ), SYSCOL_TEXT );
		cb_wait.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate( "Monthly departures" ), SYSCOL_TEXT );
		cb_wait.set_rigid(true);
		new_component<gui_fill_t>();

		add_component(&lb_load_str);
		lb_load_str.set_rigid(true);
		numimp_load.add_listener(this);
		numimp_load.set_rigid(true);
		add_component(&numimp_load);
		new_component<gui_fill_t>();

		add_component(&lb_departure_str);
		lb_departure_str.set_rigid(true);
		departure.set_rigid(true);
		departure.add_listener(this);
		add_component(&departure);
		new_component<gui_fill_t>();
	}
	end_table();

	// action button row
	button_row = add_table( 3, 1 );
	button_row->set_margin( scr_size(D_MARGIN_LEFT,0), scr_size(D_MARGIN_RIGHT,0) );
	{
		insert_mode.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Ins Stop"), SYSCOL_TEXT);
		insert_mode.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Add Stop"), SYSCOL_TEXT);
		insert_mode.set_selection( 1 );
		insert_mode.add_listener(this);
		add_component(&insert_mode);

		bt_revert.init(button_t::roundbox | button_t::flexible, "Revert schedule");
		bt_revert.set_tooltip("Revert to original schedule");
		bt_revert.add_listener(this);
		bt_revert.pressed = false;
		bt_revert.enable(false); // schedule was not changed yet
		add_component(&bt_revert);

		bt_return.init(button_t::roundbox | button_t::flexible, "return ticket");
		bt_return.add_listener(this);
		add_component(&bt_return);

		bt_remove_double.init(button_t::roundbox | button_t::flexible, "Cleanup schedule");
		bt_remove_double.set_tooltip("Remove double stops from schedule");
		bt_remove_double.add_listener(this);
		bt_remove_double.set_visible(false);
		add_component(&bt_remove_double);
	}
	end_table();

	scrolly.set_show_scroll_x(true);
	scrolly.set_scroll_amount_y(LINESPACE+1);
	add_component(&scrolly);
	stats->add_listener(this);

	current_schedule_rotation = welt->get_settings().get_rotation();

	scrolly.set_maximize(true);

	set_size( gui_aligned_container_t::get_min_size() );
}

gui_schedule_t::~gui_schedule_t()
{
	stats->highlight_schedule( false );
	update_tool(false);
	delete stats;
	delete schedule;
}

void gui_schedule_t::init(schedule_t* schedule_, player_t* player, convoihandle_t cnv, linehandle_t lin)
{
	if( old_schedule != schedule_ ) {
		if( old_schedule ) {
			stats->highlight_schedule( false );
			update_tool( false );
		}

		// initialization
		this->convoi_mode = cnv;
		this->line_mode = lin;
		this->player = player;
		current_schedule_rotation = welt->get_settings().get_rotation();

		// prepare editing
		if (schedule != schedule_) {
			delete schedule;
		}
		schedule = schedule_->copy();

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
		stats->update_schedule(false);

		numimp_load.set_value( schedule->get_current_entry().minimum_loading );

		// not allow to change entries beyond waiting time
		no_editing = (convoi_mode.is_bound() && line_mode.is_bound());
		bt_return.enable( !no_editing );

		update_selection();

		this->old_schedule = schedule_;
	}
	else {
		schedule->set_current_stop(schedule_->get_current_stop());
		update_selection();
	}
	set_size(gui_aligned_container_t::get_min_size());
}


void gui_schedule_t::update_tool(bool set)
{
	if (set) {
		uint16 toolnr = insert_mode.get_selection() == 0 ? TOOL_SCHEDULE_INS : TOOL_SCHEDULE_ADD;
		tool_t::general_tool[toolnr]->set_default_param((const char *)schedule);
		welt->set_tool(tool_t::general_tool[toolnr], player );
	}
	else {
		// we have to reset the tool (in particular, when window closes)
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
}


void gui_schedule_t::update_selection()
{
	// set all elements invisible first
	cb_wait.set_visible(false);
	numimp_load.set_visible(false);
	departure.set_visible(false);
	lb_load_str.set_visible(false);
	lb_departure_str.set_visible(false);

	if(  !schedule->empty()  ) {
		schedule->set_current_stop( min(schedule->get_count()-1,schedule->get_current_stop()) );

		const uint8 current_stop = schedule->get_current_stop();

		if(  haltestelle_t::get_halt(schedule->entries[current_stop].pos, player).is_bound()  ) {

			cb_wait.set_visible(true);
			if( schedule->entries[current_stop].get_absolute_departures() ) {
				cb_wait.set_selection( 1 );
				lb_load_str.set_visible(true);
				lb_load_str.set_text("Departures per month");
				numimp_load.set_visible( true );
				numimp_load.set_value( schedule->entries[current_stop].get_absolute_departures() );
				numimp_load.set_limits( 1, 154 );
				numimp_load.set_increment_mode( 1 );
				lb_departure_str.set_visible( true );
				lb_departure_str.set_text( "Departs at" );
				departure.set_visible( true );
				departure.set_ticks( schedule->entries[current_stop].waiting_time, true);
			}
			else {
				cb_wait.set_selection( 0 );
				lb_load_str.set_visible(true);
				lb_load_str.set_text("Minimum load");
				numimp_load.set_visible( true );
				numimp_load.set_value( schedule->entries[current_stop].minimum_loading );
				numimp_load.set_limits( 0, 100 );
				numimp_load.set_increment_mode( gui_numberinput_t::PROGRESS );
				lb_departure_str.set_text( "Max. waiting time" );
				lb_departure_str.set_visible( true );
				departure.set_visible( true );
				departure.set_ticks( schedule->entries[current_stop].waiting_time, false );
			}
		}
		else {
			// waypoint

		}
	}
	loading_details->set_size( loading_details->get_size() );
	set_size(size);
}


bool gui_schedule_t::action_triggered( gui_action_creator_t *comp, value_t p)
{
	if (comp == &bt_revert) {
		// revert changes and tell listener
		if (schedule) {
			stats->highlight_schedule(false);
			delete schedule;
			schedule = NULL;
		}
		schedule = get_old_schedule()->copy();
		stats->schedule = schedule;
		stats->update_schedule(true);
		update_selection();
		value_t v;
		v.p = NULL;
		call_listeners(v);
	}
	else if( comp == &bt_remove_double) {
		schedule->remove_double_entries();
		stats->update_schedule(true);
		update_selection();
	}
	else if( comp == &cb_wait) {
		if(  p.i==1  &&  schedule->entries[schedule->get_current_stop()].get_absolute_departures()==0  ) {
			// absolute departure mode
			schedule->entries[schedule->get_current_stop()].minimum_loading = 101;
		}
		if(  p.i==0  &&  schedule->entries[schedule->get_current_stop()].get_absolute_departures()  ) {
			schedule->entries[schedule->get_current_stop()].minimum_loading = 0;
		}
		update_selection();
	}
	else if( comp == &numimp_load ) {
		if (!schedule->empty()) {
			if( schedule->entries[schedule->get_current_stop()].minimum_loading <=100 ) {
				schedule->entries[schedule->get_current_stop()].minimum_loading = min( (uint8)p.i, 100 );
			}
			else {
				schedule->entries[schedule->get_current_stop()].minimum_loading = max( 101, min(255,(uint8)p.i+100) );
				// clip waiting time to 1/nth of the month
				if( schedule->entries[schedule->get_current_stop()].waiting_time>65535/(schedule->entries[schedule->get_current_stop()].minimum_loading-100) ) {
					schedule->entries[schedule->get_current_stop()].waiting_time=65535/(schedule->entries[schedule->get_current_stop()].minimum_loading-100);
				}
			}
			update_selection();
		}
	}
	else if(comp == &departure) {
		if(!schedule->empty()) {
			schedule->entries[schedule->get_current_stop()].waiting_time = (uint16)p.i;
			// clip waiting time to 1/nth of the month if absolute departure time
			uint16 wt = schedule->entries[schedule->get_current_stop()].minimum_loading;
			if( wt>100  &&  schedule->entries[schedule->get_current_stop()].waiting_time>65535/(wt-100) ) {
				schedule->entries[schedule->get_current_stop()].waiting_time = 65535/(wt-100);
			}
			update_selection();
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
			update_selection();
		}
		else if (schedule->empty()) {
			update_selection();
		}
	}
	// do not reset tool during initialisation
	if (old_schedule) {
		update_tool(true);
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
			schedule->rotate90( (4+current_schedule_rotation - world_rotation) & 1 ? world()->get_size().x-1 : world()->get_size().y-1 );
		}

		schedule_t *scd = get_old_schedule();
		// change current entry while convois drives on
		koord3d current = scd->get_current_entry().pos;
		int idx = 0;
		bool is_allowed = player==welt->get_active_player()  &&  !welt->get_active_player()->is_locked();
		bool is_all_same = scd->get_count()==schedule->get_count();
		is_all_same &= scd->get_current_stop() == schedule->get_current_stop();
		is_all_same &= !(convoi_mode.is_bound()  &&  line_mode.is_bound()  &&  line_mode != convoi_mode->get_line());
		FOR( minivec_tpl<schedule_entry_t>, ent, schedule->entries ) {
#if 0
			if( ent.pos == current ) {
				schedule->set_current_stop( idx );
			}
#else
			(void)current;
			(void)ent;
#endif
			if( is_all_same ) {
				is_all_same = scd->entries[idx] == schedule->entries[idx];
			}
			idx++;
		}
		bt_revert.enable( !is_all_same &&  is_allowed );

		cb_wait.enable( is_allowed );
		numimp_load.enable( is_allowed );
		departure.enable( is_allowed );
		bt_return.enable( is_allowed );

		bool invalid = stats->has_invalid_entries();
		bt_return.set_visible(!invalid);
		bt_remove_double.set_visible(invalid);
		button_row->set_size(button_row->get_size());
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

	// make scrolly take all of space
	scrolly.set_size( scr_size(scrolly.get_size().w, get_size().h - scrolly.get_pos().y));
}


void gui_schedule_t::rdwr(loadsave_t *file)
{
	// convoy data
	convoi_t::rdwr_convoihandle_t(file, convoi_mode);
	simline_t::rdwr_linehandle_t(file, line_mode);
	departure.rdwr( file );

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
		get_min_size();
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


