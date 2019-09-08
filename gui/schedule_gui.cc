/*
 * Dialog window for defining a schedule
 *
 * Hj. Malthaner
 *
 * Juli 2000
 */

#include "../simline.h"
#include "../simcolor.h"
#include "../simdepot.h"
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
#include "../vehicle/simvehicle.h"

#include "../tpl/vector_tpl.h"

#include "depot_frame.h"
#include "schedule_gui.h"
#include "line_item.h"

#include "components/gui_button.h"
#include "karte.h"



// shows/deletes highlighting of tiles
void schedule_gui_stats_t::highlight_schedule( schedule_t *markschedule, bool marking )
{
	marking &= env_t::visualize_schedule;
	FOR(minivec_tpl<schedule_entry_t>, const& i, markschedule->entries) {
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
	if(  marking  &&  markschedule->get_current_stop() < markschedule->get_count() ) {
		current_stop_mark->set_pos( markschedule->entries[markschedule->get_current_stop()].pos );
		if(  grund_t *gr = welt->lookup(current_stop_mark->get_pos())  ) {
			gr->obj_add( current_stop_mark );
			current_stop_mark->set_flag( obj_t::dirty );
			gr->set_flag( grund_t::dirty );
		}
	}
	current_stop_mark->clear_flag( obj_t::highlight );
}


/**
 * Append description of entry to buf.
 */
void schedule_gui_t::gimme_stop_name(cbuffer_t & buf, const player_t *player_, const schedule_entry_t &entry, bool no_control_tower )
{
	halthandle_t halt = haltestelle_t::get_halt(entry.pos, player_);
	if(halt.is_bound()) 
	{
		char modified_name[320];
		if(no_control_tower)
		{
			sprintf(modified_name, "%s [%s]", halt->get_name(), translator::translate("NO CONTROL TOWER"));
		}
		else
		{
			sprintf(modified_name, "%s", halt->get_name());
		}

		if(entry.wait_for_time)
		{
			buf.printf("[*] ");
		}

		if (entry.minimum_loading != 0)
		{
			buf.printf("%d%% ", entry.minimum_loading);
		}
		buf.printf("%s (%s)", modified_name, entry.pos.get_str() );
	}
	else {
		const grund_t* gr = welt->lookup(entry.pos);
		if(  gr==NULL  ) {
			buf.printf("%s (%s)", translator::translate("Invalid coordinate"), entry.pos.get_str() );
		}
		else if(  gr->get_depot() != NULL  ) {
			buf.printf("%s (%s)", translator::translate("Depot"), entry.pos.get_str() );
		}
		else if(  const char *label_text = gr->get_text()  ){
			buf.printf("%s %s (%s)", translator::translate("Wegpunkt"), label_text, entry.pos.get_str() );
		}
		else {
			buf.printf("%s (%s)", translator::translate("Wegpunkt"), entry.pos.get_str() );
		}
	}

	if(entry.reverse == 1)
	{
		buf.printf(" [<<]");
	}
}


void schedule_gui_t::gimme_short_stop_name(cbuffer_t& buf, player_t const* const player_, const schedule_t *schedule, int i, int max_chars)
{
	if (i < 0 || schedule == NULL || i >= schedule->get_count()) {
		dbg->warning("void schedule_gui_t::gimme_short_stop_name()", "tried to receive unused entry %i in schedule %p.", i, schedule);
		return;
	}
	const schedule_entry_t& entry = schedule->entries[i];
	const char* p;
	halthandle_t halt = haltestelle_t::get_halt(entry.pos, player_);
	if (halt.is_bound()) {
		p = halt->get_name();
	}
	else {
		const grund_t* gr = welt->lookup(entry.pos);
		if (gr == NULL) {
			p = translator::translate("Invalid coordinate");
		}
		else if (gr->get_depot() != NULL) {
			p = translator::translate("Depot");
		}
		else {
			p = translator::translate("Wegpunkt");
		}
	}

	// Finally start to append the entry. Start with the most complicated...
	if (entry.wait_for_time && entry.reverse == 1)
	{
		if (strlen(p) > (unsigned)max_chars - 8)
		{
			buf.printf("[*] %.*s... [<<]", max_chars - 12, p);
		}
		else
		{
			buf.append("[*] ");
			buf.append(p);
			buf.append(" [<<]");
		}
	}
	else if (entry.wait_for_time)
	{
		if (strlen(p) > (unsigned)max_chars - 4)
		{
			buf.printf("[*] %.*s...", max_chars - 8, p);
		}
		else
		{
			buf.append("[*] ");
			buf.append(p);
		}
	}
	else if (entry.reverse == 1)
	{
		if (strlen(p) > (unsigned)max_chars - 4)
		{
			buf.printf("%.*s... [<<]", max_chars - 8, p);
		}
		else
		{
			buf.append(p);
			buf.append(" [<<]");
		}
	}
	else if (strlen(p) > (unsigned)max_chars)
	{
		buf.printf("%.*s...", max_chars - 3, p);
	}
	else
	{
		buf.append(p);
	}
}



zeiger_t *schedule_gui_stats_t::current_stop_mark = NULL;
cbuffer_t schedule_gui_stats_t::buf;

void schedule_gui_stats_t::draw(scr_coord offset)
{
	if(  !schedule  ) {
		return;
	}

	if(  schedule->empty()  ) {
		buf.clear();
		buf.append(translator::translate("Please click on the map to add\nwaypoints or stops to this\nschedule."));
		sint16 const width = display_multiline_text(offset.x + 4, offset.y, buf, SYSCOL_TEXT_HIGHLIGHT );
		set_size(scr_size(width + 4 + 16, 3 * LINESPACE));
	}
	else {
		int    i     = 0;
		size_t sel   = schedule->get_current_stop();
		sint16 width = get_size().w - D_MARGIN_LEFT - D_MARGIN_RIGHT;
		offset.x += D_MARGIN_LEFT;
		offset.y += D_V_SPACE;
		koord last_stop_pos = schedule->entries[0].pos.get_2d();
		double distance;
		FOR(minivec_tpl<schedule_entry_t>, const& e, schedule->entries) 
		{
			if (sel == 0) 
			{
				// highlight current entry (width is just wide enough, scrolly will do clipping)
				display_fillbox_wh_clip(offset.x, offset.y - 1, 2048, LINESPACE + 1, player->get_player_color1() + 1, false);
			}

			if(last_stop_pos != e.pos.get_2d())
			{
				distance = (double)(shortest_distance(last_stop_pos, e.pos.get_2d()) * welt->get_settings().get_meters_per_tile()) / 1000.0;
				buf.printf(" %.1f%s", distance, "km");
				
				PLAYER_COLOR_VAL const c = sel == 0 ? SYSCOL_TEXT_HIGHLIGHT : SYSCOL_TEXT;
				sint16           const w = display_proportional_clip(offset.x + 4 + 10, offset.y, buf, ALIGN_LEFT, c, true);
				if (width < w) 
				{
					width = w;
				}

				// the goto button (right arrow)
				display_img_aligned(gui_theme_t::pos_button_img[ sel == 0 ], scr_rect( offset.x, offset.y, 14, LINESPACE ), ALIGN_CENTER_V | ALIGN_CENTER_H, true);
				last_stop_pos = e.pos.get_2d();
				--sel;
				offset.y += LINESPACE + 1;
			}

			buf.clear();
			buf.printf("%i) ", ++i);
			bool no_control_tower = schedule->get_waytype() == air_wt;
			if (no_control_tower)
			{
				halthandle_t halt = welt->lookup(e.pos)->get_halt();
				no_control_tower = halt.is_bound() && halt->has_no_control_tower();
			}
			
			schedule_gui_t::gimme_stop_name(buf, player, e, no_control_tower);	
		}

		if (sel == 0) 
		{
			// highlight current entry (width is just wide enough, scrolly will do clipping)
			display_fillbox_wh_clip(offset.x, offset.y - 1, 2048, LINESPACE + 1, player->get_player_color1() + 1, false);
		}
		distance = (double)(shortest_distance(last_stop_pos, schedule->entries[0].pos.get_2d()) * welt->get_settings().get_meters_per_tile()) / 1000;
		buf.printf(" %.1f%s", distance, "km");
		PLAYER_COLOR_VAL c = sel == 0 ? SYSCOL_TEXT_HIGHLIGHT : SYSCOL_TEXT;
		sint16           w = display_proportional_clip(offset.x + 4 + 10, offset.y, buf, ALIGN_LEFT, c, true);
		if (width < w) 
		{
			width = w;
		}

		// the goto button (right arrow)
		display_img_aligned(gui_theme_t::pos_button_img[ sel == 0 ], scr_rect( offset.x, offset.y, 14, LINESPACE ), ALIGN_CENTER_V | ALIGN_CENTER_H, true);

		set_size(scr_size(width + 16, schedule->get_count() * (LINESPACE + 1)));
		highlight_schedule(schedule, true);
	}
}



schedule_gui_stats_t::schedule_gui_stats_t(player_t *player_)
{
	schedule = NULL;
	player = player_;
	if(  current_stop_mark==NULL  ) {
		current_stop_mark = new zeiger_t(koord3d::invalid, NULL );
		current_stop_mark->set_image( tool_t::general_tool[TOOL_SCHEDULE_ADD]->cursor );
	}
}



schedule_gui_stats_t::~schedule_gui_stats_t()
{
	if(  grund_t *gr = welt->lookup(current_stop_mark->get_pos())  ) {
		current_stop_mark->mark_image_dirty( current_stop_mark->get_image(), 0 );
		gr->obj_remove(current_stop_mark);
	}
	current_stop_mark->set_pos( koord3d::invalid );
}



schedule_gui_t::~schedule_gui_t()
{
	if(  player  ) {
		update_tool( false );
		// hide schedule on minimap (may not current, but for safe)
		reliefkarte_t::get_karte()->set_current_cnv( convoihandle_t() );
	}
}



schedule_gui_t::schedule_gui_t(schedule_t* sch_, player_t* player_, convoihandle_t cnv_) :
	gui_frame_t( translator::translate("Fahrplan"), player_),
	lb_line("Serves Line:"),
	lb_wait("month wait time"),
	lb_waitlevel_as_clock(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::right),
	lb_load("Full load"),
	lb_spacing("Spacing cnv/month, shift"),
	lb_spacing_as_clock(NULL, SYSCOL_TEXT, gui_label_t::right),
	lb_spacing_shift_as_clock(NULL, SYSCOL_TEXT, gui_label_t::right),
	stats(player_),
	scrolly(&stats),
	old_schedule(sch_),
	player(player_),
	cnv(cnv_)
{
	old_schedule->start_editing();
	schedule = old_schedule->copy();
	stats.set_schedule(schedule);
	if(  !cnv.is_bound()  ) {
		old_line = new_line = linehandle_t();
		show_line_selector(false);
	}
	else {
		// set this schedule as current to show on minimap if possible
		reliefkarte_t::get_karte()->set_current_cnv( cnv );
		old_line = new_line = cnv_->get_line();
	}
	old_line_count = 0;

	scr_coord_val ypos = D_MARGIN_TOP;
	if(  cnv.is_bound()  ) {
		// things, only relevant to convois, like creating/selecting lines
		bt_promote_to_line.init( button_t::roundbox, "promote to line", scr_coord( BUTTON3_X, ypos ) );
		bt_promote_to_line.set_tooltip("Create a new line based on this schedule");
		bt_promote_to_line.add_listener(this);
		add_component(&bt_promote_to_line);

		lb_line.align_to( &bt_promote_to_line, ALIGN_CENTER_V, scr_coord( D_MARGIN_LEFT, 0 ) );
		add_component( &lb_line );

		ypos += D_BUTTON_HEIGHT+1;

		line_selector.set_pos(scr_coord(D_MARGIN_LEFT, ypos));
		line_selector.set_size(scr_size(BUTTON4_X-D_MARGIN_LEFT, D_BUTTON_HEIGHT));
		line_selector.set_max_size(scr_size(BUTTON4_X-D_MARGIN_LEFT, 13*LINESPACE+D_TITLEBAR_HEIGHT-1));
		line_selector.set_highlight_color(player->get_player_color1() + 1);
		line_selector.clear_elements();

		init_line_selector();
		line_selector.add_listener(this);
		add_component(&line_selector);

		ypos += D_BUTTON_HEIGHT + D_V_SPACE;
	}

	// loading level and return tickets
	const scr_coord_val label_width = min( (D_BUTTON_WIDTH<<1) + D_H_SPACE, max( lb_load.get_size().w, lb_wait.get_size().w ) );

	numimp_load.set_pos( scr_coord( D_MARGIN_LEFT + label_width + D_H_SPACE, ypos ) );
	numimp_load.set_width( 60 );
	numimp_load.set_value( schedule->get_current_entry().minimum_loading );
	numimp_load.set_limits( 0, 100 );
	numimp_load.set_increment_mode(10);
	numimp_load.add_listener(this);
	add_component(&numimp_load);

	bt_bidirectional.init(button_t::square_automatic, "Alternate directions", scr_coord( BUTTON3_X, ypos ), scr_size(D_BUTTON_WIDTH*2,D_BUTTON_HEIGHT) );
	bt_bidirectional.set_tooltip("When adding convoys to the line, every second convoy will follow it in the reverse direction.");
	bt_bidirectional.pressed = schedule->is_bidirectional();
	bt_bidirectional.add_listener(this);
	add_component(&bt_bidirectional);

	lb_load.set_width( label_width );
	lb_load.align_to( &numimp_load, ALIGN_CENTER_V, scr_coord( D_MARGIN_LEFT, 0 ) );
	add_component( &lb_load );

	ypos += D_BUTTON_HEIGHT;

	// Maximum waiting time
	lb_wait.set_pos( scr_coord( D_MARGIN_LEFT, ypos+2 ) );
	add_component(&lb_wait);

	if(  schedule->get_current_entry().waiting_time_shift==0  ) {
		strcpy( str_parts_month, translator::translate("off") );
		strcpy( str_parts_month_as_clock, translator::translate("off") );

	}
	else {
		sprintf( str_parts_month, "1/%d",  1<<(16-schedule->get_current_entry().waiting_time_shift) );
		sint64 ticks_waiting = welt->ticks_per_world_month >> (16-schedule->get_current_entry().waiting_time_shift);
		welt->sprintf_ticks(str_parts_month_as_clock, sizeof(str_parts_month_as_clock), ticks_waiting + 1);
	}

	lb_waitlevel_as_clock.set_text_pointer(str_parts_month_as_clock, false);
	lb_waitlevel_as_clock.set_size( numimp_load.get_size() - scr_size( D_ARROW_LEFT_WIDTH + D_ARROW_RIGHT_WIDTH, 0 ) );
	lb_waitlevel_as_clock.set_pos( scr_coord(bt_wait_prev.get_pos().x, ypos + 2));
	lb_waitlevel_as_clock.align_to( &numimp_load, ALIGN_EXTERIOR_V | ALIGN_TOP | ALIGN_LEFT, scr_coord( gui_theme_t::gui_arrow_left_size.w, 0 ) );
	add_component(&lb_waitlevel_as_clock);

	bt_wait_prev.set_typ( button_t::arrowleft );
	bt_wait_prev.align_to( &lb_waitlevel_as_clock, ALIGN_EXTERIOR_H | ALIGN_RIGHT | ALIGN_CENTER_V );
	bt_wait_prev.add_listener(this);
	add_component( &bt_wait_prev );

	bt_wait_next.set_typ( button_t::arrowright );
	bt_wait_next.align_to( &lb_waitlevel_as_clock, ALIGN_EXTERIOR_H | ALIGN_LEFT | ALIGN_CENTER_V );
	bt_wait_next.add_listener(this);
	lb_waitlevel_as_clock.set_width( bt_wait_next.get_pos().x-bt_wait_prev.get_pos().x-bt_wait_prev.get_size().w );
	add_component( &bt_wait_next );

	lb_wait.set_width( label_width );
	lb_wait.align_to( &lb_waitlevel_as_clock, ALIGN_CENTER_V, scr_coord( D_MARGIN_LEFT, 0 ) );
	add_component( &lb_wait );

	if(!cnv.is_bound())
	{
		// Wait for time
		bt_wait_for_time.init(button_t::square_automatic, "Wait for time", scr_coord( BUTTON1_X, ypos+12 ), scr_size(D_BUTTON_WIDTH*2,D_BUTTON_HEIGHT) );
		bt_wait_for_time.set_tooltip("If this is set, convoys will wait until one of the specified times before departing, the specified times being fractions of a month.");
		bt_wait_for_time.pressed = schedule->get_current_entry().wait_for_time;
		bt_wait_for_time.add_listener(this);
		add_component(&bt_wait_for_time);
	}

	// Mirror schedule/alternate directions
	bt_mirror.init(button_t::square_automatic, "return ticket", scr_coord( BUTTON3_X, ypos ), scr_size(D_BUTTON_WIDTH*2,D_BUTTON_HEIGHT) );
	bt_mirror.set_tooltip("Vehicles make a round trip between the schedule endpoints, visiting all stops in reverse after reaching the end.");
	bt_mirror.pressed = schedule->is_mirrored();
	bt_mirror.add_listener(this);
	add_component(&bt_mirror);

	ypos += LINESPACE;

	ypos += D_BUTTON_HEIGHT;

	// Spacing
	if ( !cnv.is_bound() ) {
		lb_spacing.set_pos( scr_coord( 10, ypos+2 ) );
		add_component(&lb_spacing);
		numimp_spacing.set_pos( scr_coord( BUTTON3_X, ypos+2 ) );
		//numimp_spacing.set_width( 60 );
		numimp_spacing.set_width_by_len(3);
		numimp_spacing.set_value( schedule->get_spacing() );
		numimp_spacing.set_limits( 0, 999 );
		numimp_spacing.set_increment_mode( 1 );
		numimp_spacing.add_listener(this);
		add_component(&numimp_spacing);

		// Spacing shift
		int spacing_shift_mode = welt->get_settings().get_spacing_shift_mode();
		if ( spacing_shift_mode > settings_t::SPACING_SHIFT_DISABLED) {
			numimp_spacing_shift.set_pos( scr_coord( numimp_spacing.get_pos().x + numimp_spacing.get_size().w + D_H_SPACE, ypos+2 ) );
			//numimp_spacing_shift.set_width( 60 );
			numimp_spacing_shift.set_width_by_len(3);
			numimp_spacing_shift.set_value( schedule->get_current_entry().spacing_shift  );
			numimp_spacing_shift.set_limits( 0,welt->get_settings().get_spacing_shift_divisor() );
			numimp_spacing_shift.set_increment_mode( 1 ); 
			numimp_spacing_shift.add_listener(this);
			add_component(&numimp_spacing_shift);
		}

		ypos += D_BUTTON_HEIGHT;

		if (spacing_shift_mode > settings_t::SPACING_SHIFT_PER_LINE) {
			//Same spacing button
			bt_same_spacing_shift.init(button_t::square_automatic, "Use same shift for all stops.", scr_coord( BUTTON1_X , ypos+2 ), scr_size(D_BUTTON_WIDTH*3,D_BUTTON_HEIGHT) );
			bt_same_spacing_shift.set_tooltip("Use one spacing shift value for all stops in schedule.");
			bt_same_spacing_shift.pressed = schedule->is_same_spacing_shift();
			bt_same_spacing_shift.add_listener(this);
			add_component(&bt_same_spacing_shift);
		}

		lb_spacing_as_clock.set_pos(scr_coord( numimp_spacing.get_pos().x, ypos+2 ) );
		lb_spacing_as_clock.set_width(50);
		lb_spacing_as_clock.set_text_pointer(str_spacing_as_clock, false);
		add_component(&lb_spacing_as_clock);

		if (spacing_shift_mode > settings_t::SPACING_SHIFT_PER_LINE) {
			lb_spacing_shift_as_clock.set_pos(scr_coord( numimp_spacing_shift.get_pos().x, ypos+2 ) );
			lb_spacing_shift_as_clock.set_width(50);
			lb_spacing_shift_as_clock.set_text_pointer(str_spacing_shift_as_clock, false);
			add_component(&lb_spacing_shift_as_clock);
		}

		ypos += D_BUTTON_HEIGHT;
	}

	ypos += D_V_SPACE;

	bt_add.init(button_t::roundbox_state, "Add Stop", scr_coord(BUTTON1_X, ypos ), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT) );
	bt_add.set_tooltip("Appends stops at the end of the schedule");
	bt_add.add_listener(this);
	bt_add.pressed = true;
	add_component(&bt_add);

	bt_insert.init(button_t::roundbox_state, "Ins Stop", scr_coord(BUTTON2_X, ypos ), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT) );
	bt_insert.set_tooltip("Insert stop before the current stop");
	bt_insert.add_listener(this);
	bt_insert.pressed = false;
	add_component(&bt_insert);

	bt_remove.init(button_t::roundbox_state, "Del Stop", scr_coord(BUTTON3_X, ypos ), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT) );
	bt_remove.set_tooltip("Delete the current stop");
	bt_remove.add_listener(this);
	bt_remove.pressed = false;
	add_component(&bt_remove);

	ypos += D_BUTTON_HEIGHT;

	scrolly.set_pos( scr_coord( 0, ypos ) );
	scrolly.set_show_scroll_x(true);
	scrolly.set_scroll_amount_y(LINESPACE+1);
	add_component(&scrolly);

	mode = adding;
	update_selection();

	set_windowsize( scr_size(BUTTON4_X + 35, ypos+D_BUTTON_HEIGHT+(schedule->get_count()>0 ? min(15,schedule->get_count()) : 15)*(LINESPACE+1)+D_TITLEBAR_HEIGHT) );
	set_min_windowsize( scr_size(BUTTON4_X + 35, ypos+D_BUTTON_HEIGHT+3*(LINESPACE+1)+D_TITLEBAR_HEIGHT) );

	set_resizemode(diagonal_resize);
	resize( scr_coord(0,0) );
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
	lb_load.set_color( COL_GREY3 );
	numimp_load.disable();
	numimp_load.set_value( 0 );
	bt_wait_prev.disable();
	lb_wait.set_color( COL_GREY3 );
	lb_spacing.set_color( COL_GREY3 );
	lb_spacing_as_clock.set_color( COL_GREY3 );
	numimp_spacing.disable();
	numimp_spacing_shift.disable();
	sprintf(str_spacing_as_clock, "%s", translator::translate("off") );
	lb_spacing_shift.set_color( COL_GREY3 );
	lb_spacing_shift_as_clock.set_color( COL_GREY3 );
	sprintf(str_spacing_shift_as_clock, "%s", translator::translate("off") );

	strcpy( str_parts_month, translator::translate("off") );
	lb_waitlevel_as_clock.set_color( COL_GREY3 );
	bt_wait_next.disable();

	if(  !schedule->empty()  ) {
		schedule->set_current_stop( min(schedule->get_count()-1,schedule->get_current_stop()) );
		const uint8 current_stop = schedule->get_current_stop();
		bt_wait_for_time.pressed = schedule->get_current_entry().wait_for_time;
		if(  haltestelle_t::get_halt(schedule->entries[current_stop].pos, player).is_bound()  ) {
			if(!schedule->get_current_entry().wait_for_time)
			{
				lb_load.set_color( SYSCOL_TEXT );
				numimp_load.enable();
				numimp_load.set_value( schedule->entries[current_stop].minimum_loading );
			}
			else if(!schedule->get_spacing())
			{
				// Cannot have wait for time without some spacing. 
				schedule->set_spacing(1);
				numimp_spacing.set_value(1);
			}
			
			if(  schedule->entries[current_stop].minimum_loading>0 || schedule->entries[current_stop].wait_for_time ) {
				bt_wait_prev.enable();
				lb_wait.set_color( SYSCOL_TEXT );
				lb_spacing.set_color( SYSCOL_TEXT );
				numimp_spacing.enable();
				numimp_spacing_shift.enable();
				numimp_spacing_shift.set_value(schedule->get_current_entry().spacing_shift);
				if (schedule->get_spacing() ) {
					lb_spacing_shift.set_color( SYSCOL_TEXT );
					lb_spacing_as_clock.set_color( SYSCOL_TEXT );
					lb_spacing_shift_as_clock.set_color( SYSCOL_TEXT );
					welt->sprintf_ticks(str_spacing_as_clock, sizeof(str_spacing_as_clock), welt->ticks_per_world_month/schedule->get_spacing());
					welt->sprintf_ticks(str_spacing_shift_as_clock, sizeof(str_spacing_as_clock),
							schedule->entries[current_stop].spacing_shift * welt->ticks_per_world_month/welt->get_settings().get_spacing_shift_divisor()+1
							);
				}
				lb_waitlevel_as_clock.set_color( SYSCOL_TEXT_HIGHLIGHT );
				bt_wait_next.enable();
			}
			if(  (schedule->entries[current_stop].minimum_loading>0 || schedule->entries[current_stop].wait_for_time) &&  schedule->entries[current_stop].waiting_time_shift>0  ) {
				sprintf( str_parts_month, "1/%d",  1<<(16-schedule->entries[current_stop].waiting_time_shift) );
				sint64 ticks_waiting = welt->ticks_per_world_month >> (16-schedule->get_current_entry().waiting_time_shift);
				welt->sprintf_ticks(str_parts_month_as_clock, sizeof(str_parts_month_as_clock), ticks_waiting + 1);
			}
			else {
				strcpy( str_parts_month, translator::translate("off") );
				strcpy( str_parts_month_as_clock, translator::translate("off") );
			}
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

		if(  ev->my>=scrolly.get_pos().y+16  ) {
			// we are now in the multiline region ...
			const int line = ( ev->my - scrolly.get_pos().y + scrolly.get_scroll_y() - 16)/(LINESPACE+1);

			if(  line >= 0 && line < schedule->get_count()  ) {
				if(  IS_RIGHTCLICK(ev)  ||  ev->mx<16  ) {
					// just center on it
					welt->get_viewport()->change_world_position( schedule->entries[line].pos );
				}
				else if(  ev->mx<scrolly.get_size().w-11  ) {
					schedule->set_current_stop( line );
					if(  mode == removing  ) {
						stats.highlight_schedule( schedule, false );
						schedule->remove();
						action_triggered( &bt_add, value_t() );
					}
					update_selection();
				}
			}
		}
	}
	else if(  ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE  &&  schedule!=NULL  ) {

		for(  int i=0;  i<schedule->get_count();  i++  ) {
			stats.highlight_schedule( schedule, false );
		}

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
						delete schedule;
					}
				}
				else {
					cbuffer_t buf;
					schedule->sprintf_schedule( buf );
					cnv->call_convoi_tool( 'g', buf );
					delete schedule;
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
			schedule->entries[schedule->get_current_stop()].minimum_loading = (uint16)p.i;
			update_selection();
		}
	}
	else if(comp == &bt_wait_prev) {
		if(!schedule->empty()) {
			sint8& wait = schedule->entries[schedule->get_current_stop()].waiting_time_shift;
			if(wait>7) {
				wait --;
			}
			else if(  wait>0  ) {
				wait = 0;
			}
			else {
				wait = 16;
			}
			update_selection();
		}
	}
	else if(comp == &bt_wait_next) {
		if(!schedule->empty()) {
			sint8& wait = schedule->entries[schedule->get_current_stop()].waiting_time_shift;
			if(wait==0) {
				wait = 7;
			}
			else if(wait<16) {
				wait ++;
			}
			else {
				wait = 0;
			}
			update_selection();
		}
	}
	else if (comp == &numimp_spacing) {
		schedule->set_spacing(p.i);
		update_selection();
	} 
	else if(comp == &numimp_spacing_shift) {
		if (!schedule->empty()) {
			if ( schedule->is_same_spacing_shift() ) {
			    for(  uint8 i=0;  i<schedule->entries.get_count();  i++  ) {
					schedule->entries[i].spacing_shift = p.i;
				}
			} else {
				schedule->entries[schedule->get_current_stop()].spacing_shift = p.i;
			}
			update_selection();
		}
	} 
	else if (comp == &bt_mirror) {
		schedule->set_mirrored(bt_mirror.pressed);
	} 
	else if (comp == &bt_bidirectional) {
		schedule->set_bidirectional(bt_bidirectional.pressed);
	} 
	else if (comp == &bt_same_spacing_shift) {
		schedule->set_same_spacing_shift(bt_same_spacing_shift.pressed);
		if ( schedule->is_same_spacing_shift() ) {
		    for(  uint8 i=0;  i<schedule->entries.get_count();  i++  ) {
				schedule->entries[i].spacing_shift = schedule->entries[schedule->get_current_stop()].spacing_shift;
			}
		}
	} 
	else if(comp == &bt_wait_for_time)
	{
		if(!schedule->empty())
		{
			schedule->entries[schedule->get_current_stop()].wait_for_time = bt_wait_for_time.pressed;
			update_selection();
		}
	}
	else if (comp == &line_selector) {
		uint32 selection = p.i;
//DBG_MESSAGE("schedule_gui_t::action_triggered()","line selection=%i",selection);
		if(  line_scrollitem_t *li = dynamic_cast<line_scrollitem_t*>(line_selector.get_element(selection))  ) {
			new_line = li->get_line();
			stats.highlight_schedule( schedule, false );
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
		buf.printf( "c,0,%i,%ld,", (int)schedule->get_type(), (long)(uint64)old_schedule );
		schedule->sprintf_schedule( buf );
		tool->set_default_param(buf);
		welt->set_tool( tool, player );
		// since init always returns false, it is safe to delete immediately
		delete tool;
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
		line_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("<no line>"), SYSCOL_TEXT ) );
	}

	FOR(  vector_tpl<linehandle_t>,  line,  lines  ) {
		line_selector.append_element( new line_scrollitem_t(line) );
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
	line_selector.sort( offset, NULL );
	old_line_count = player->simlinemgmt.get_line_count();
	last_schedule_count = schedule->get_count();
}



void schedule_gui_t::draw(scr_coord pos, scr_size size)
{
	if(  player->simlinemgmt.get_line_count()!=old_line_count  ||  last_schedule_count!=schedule->get_count()  ) {
		// lines added or deleted
		init_line_selector();
		last_schedule_count = schedule->get_count();
		update_selection();
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
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void schedule_gui_t::set_windowsize(scr_size size)
{
	gui_frame_t::set_windowsize(size);

	size = get_windowsize()-scr_size(0,16+1);
	scrolly.set_size(size-scr_size(0,scrolly.get_pos().y));

	line_selector.set_max_size(scr_size(BUTTON4_X-D_MARGIN_LEFT, size.h-line_selector.get_pos().y -16-1));
}


void schedule_gui_t::map_rotate90( sint16 y_size)
{
	schedule->rotate90(y_size);
}


schedule_gui_t::schedule_gui_t():
gui_frame_t( translator::translate("Fahrplan"), NULL),
	lb_line("Serves Line:"),
	lb_wait("month wait time"),
	lb_waitlevel_as_clock(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::right),
	lb_load("Full load"),
	stats(NULL),
	scrolly(&stats),
	schedule(NULL),
	old_schedule(NULL),
	player(NULL),
	cnv()
{
	// just a dummy
}


void schedule_gui_t::rdwr(loadsave_t *file)
{
	// this handles only schedules of bound convois
	// lines are handled by line_management_gui_t

	// window size
	scr_size size = get_windowsize();
	size.rdwr( file );

	// convoy data
	if (file->get_version() <=112002) {
		// dummy data
		uint8 player_nr = 0;
		koord3d cnv_pos( koord3d::invalid);
		char name[128];
		name[0] = 0;
		file->rdwr_byte( player_nr );
		file->rdwr_str( name, lengthof(name) );
		cnv_pos.rdwr( file );
	}
	else {
		// handle
		convoi_t::rdwr_convoihandle_t(file, cnv);
	}

	// schedules
	if(  file->is_loading()  ) {
		// dummy types
		old_schedule = new truck_schedule_t();
		schedule = new truck_schedule_t();
	}
	old_schedule->rdwr(file);
	schedule->rdwr(file);

	if(  file->is_loading()  ) {
		if(  cnv.is_bound() ) {
			// now we can open the window ...
			scr_coord const& pos = win_get_pos(this);
			schedule_gui_t *w = new schedule_gui_t( cnv->get_schedule(), cnv->get_owner(), cnv );
			create_win(pos.x, pos.y, w, w_info, (ptrdiff_t)cnv->get_schedule());
			w->set_windowsize( size );
			w->schedule->copy_from( schedule );
			cnv->get_schedule()->finish_editing();
			w->schedule->finish_editing();
		}
		else {
			dbg->error( "schedule_gui_t::rdwr", "Could not restore schedule window for (%d)", cnv.get_id() );
		}
		player = NULL;
		delete old_schedule;
		delete schedule;
		schedule = old_schedule = NULL;
		cnv = convoihandle_t();
		destroy_win( this );
	}
}
