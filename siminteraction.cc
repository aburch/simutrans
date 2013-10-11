/*
 * Copyright (c) 2013 Markohs
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#include "siminteraction.h"

#include "dataobj/environment.h"
#include "gui/gui_frame.h"
#include "gui/help_frame.h"
#include "network/network_cmd_ingame.h"
#include "dataobj/scenario.h"
#include "simevent.h"
#include "simtools.h"
#include "simmenu.h"
#include "player/simplay.h"
#include "simsound.h"
#include "simsys.h"
#include "simticker.h"
#include "gui/simwin.h"
#include "simworld.h"
#include "besch/sound_besch.h"
#include "obj/zeiger.h"


void interaction_t::move_view( const event_t &ev )
{
	if(!world->get_scroll_lock()) {
		koord new_ij = world->get_world_position();

		sint16 new_xoff = world->get_x_off() - (ev.mx - ev.cx) * env_t::scroll_multi;
		sint16 new_yoff = world->get_y_off() - (ev.my-ev.cy) * env_t::scroll_multi;

		// this sets the new position and mark screen dirty
		// => with next refresh we will be at a new location
		world->change_world_position( new_ij, new_xoff, new_yoff );

		// move the mouse pointer back to starting location => infinite mouse movement
		if ((ev.mx - ev.cx)!=0  ||  (ev.my-ev.cy)!=0) {
#ifdef __BEOS__
			change_drag_start(ev.mx - ev.cx, ev.my - ev.cy);
#else
			display_move_pointer(ev.cx, ev.cy);
#endif
		}
	}
}


void interaction_t::move_cursor( const event_t &ev )
{
	zeiger_t *zeiger = world->get_zeiger();

	if(! zeiger ) {
		// No cursor to move, exit
		return;
	}

	static int mb_alt=0;

	werkzeug_t *wkz = world->get_werkzeug(world->get_active_player_nr());

	const koord3d pos = world->get_new_cursor_position(koord(ev.mx,ev.my), wkz->is_grid_tool());

	if( pos == koord3d::invalid ) {
		zeiger->change_pos(pos);
		return;
	}


	// move cursor
	const koord3d prev_pos = zeiger->get_pos();
	if(  (prev_pos != pos ||  ev.button_state != mb_alt)  ) {

		mb_alt = ev.button_state;

		zeiger->change_pos(pos);

		if(  !env_t::networkmode  ||  wkz->is_move_network_save(world->get_active_player())) {
			wkz->flags = event_get_last_control_shift() | werkzeug_t::WFL_LOCAL;
			if(wkz->check_pos( world, world->get_active_player(), zeiger->get_pos() )==NULL) {
				if(  ev.button_state == 0  ) {
					is_dragging = false;
				}
				else if(ev.ev_class==EVENT_DRAG) {
					if(!is_dragging  &&  wkz->check_pos( world, world->get_active_player(), prev_pos )==NULL) {
						const char* err = world->get_scenario()->is_work_allowed_here(world->get_active_player(), wkz->get_id(), wkz->get_waytype(), prev_pos);
						if (err == NULL) {
							is_dragging = true;
						}
						else {
							is_dragging = false;
						}
					}
				}
				if (is_dragging) {
					const char* err = world->get_scenario()->is_work_allowed_here(world->get_active_player(), wkz->get_id(), wkz->get_waytype(), pos);
					if (err == NULL) {
						wkz->move( world, world->get_active_player(), is_dragging, pos );
					}
				}
			}
			wkz->flags = 0;
		}

		if(  (ev.button_state&7)==0  ) {
			// time, since mouse got here
			world->set_mouse_rest_time(dr_time());
			world->set_sound_wait_time(AMBIENT_SOUND_INTERVALL);	// 13s no movement: play sound
		}
	}
}


void interaction_t::interactive_event( const event_t &ev )
 {
	if(ev.ev_class == EVENT_KEYBOARD) {
		DBG_MESSAGE("interaction_t::interactive_event()","Keyboard event with code %d '%c'", ev.ev_code, (ev.ev_code>=32  &&  ev.ev_code<=126) ? ev.ev_code : '?' );

		switch(ev.ev_code) {

			// cursor movements
			case '9':
				world->change_world_position(world->get_world_position() + koord::nord);
				world->set_dirty();
				break;
			case '1':
				world->change_world_position(world->get_world_position() + koord::sued);
				world->set_dirty();
				break;
			case '7':
				world->change_world_position(world->get_world_position() + koord::west);
				world->set_dirty();
				break;
			case '3':
				world->change_world_position(world->get_world_position() + koord::ost);
				world->set_dirty();
				break;
			case '6':
			case SIM_KEY_RIGHT:
				world->change_world_position(world->get_world_position() + koord(+1,-1));
				world->set_dirty();
				break;
			case '2':
			case SIM_KEY_DOWN:
				world->change_world_position(world->get_world_position() + koord(+1,+1));
				world->set_dirty();
				break;
			case '8':
			case SIM_KEY_UP:
				world->change_world_position(world->get_world_position() + koord(-1,-1));
				world->set_dirty();
				break;
			case '4':
			case SIM_KEY_LEFT:
				world->change_world_position(world->get_world_position() + koord(-1,+1));
				world->set_dirty();
				break;

			// closing windows
			case 27:
			case 127:
				// close topmost win
				destroy_win( win_get_top() );
				break;

			case SIM_KEY_F1:
				if(  gui_frame_t *win = win_get_top()  ) {
					if(  const char *helpfile = win->get_hilfe_datei()  ) {
						help_frame_t::open_help_on( helpfile );
						break;
					}
				}
				world->set_werkzeug( werkzeug_t::dialog_tool[WKZ_HELP], world->get_active_player() );
				break;

			// just ignore the key
			case 0:
				break;

			// distinguish between backspace and ctrl-H (both keycode==8), and enter and ctrl-M (both keycode==13)
			case 8:
			case 13:
				if(  !IS_CONTROL_PRESSED(&ev)  ) {
					// Control is _not_ pressed => Backspace or Enter pressed.
					if(  ev.ev_code == 8  ) {
						// Backspace
						sound_play(SFX_SELECT);
						destroy_all_win(false);
					}
					// Ignore Enter and Backspace but not Ctrl-H and Ctrl-M
					break;
				}

			default:
				{
					bool ok=false;
					FOR(vector_tpl<werkzeug_t*>, const i, werkzeug_t::char_to_tool) {
						if (i->command_key == ev.ev_code) {
							world->set_werkzeug(i, world->get_active_player());
							ok = true;
							break;
						}
					}
					if(!ok) {
						help_frame_t::open_help_on( "keys.txt" );
					}
				}
				break;
		}
	}

	if(  IS_LEFTRELEASE(&ev)  &&  ev.my < display_get_height() - 32 + (16*ticker::empty())  ) {

		DBG_MESSAGE("interaction_t::interactive_event(event_t &ev)", "calling a tool");

		if(world->is_within_grid_limits(world->get_zeiger()->get_pos().get_2d())) {
			const char *err = NULL;
			bool result = true;
			werkzeug_t *wkz = world->get_werkzeug(world->get_active_player_nr());
			// first check for visibility etc
			err = wkz->check_pos( world, world->get_active_player(), world->get_zeiger()->get_pos() );
			if (err==NULL) {
				wkz->flags = event_get_last_control_shift();
				if (!env_t::networkmode  ||  wkz->is_work_network_save()  ||  wkz->is_work_here_network_save( world, world->get_active_player(), world->get_zeiger()->get_pos() ) ) {
					// do the work
					wkz->flags |= werkzeug_t::WFL_LOCAL;
					// check allowance by scenario
					koord3d const& pos = world->get_zeiger()->get_pos();
					if (world->get_scenario()->is_scripted()) {
						if (!world->get_scenario()->is_tool_allowed(world->get_active_player(), wkz->get_id(), wkz->get_waytype()) ) {
							err = "";
						}
						else {
							err = world->get_scenario()->is_work_allowed_here(world->get_active_player(), wkz->get_id(), wkz->get_waytype(), pos);
						}
					}
					if (err == NULL) {
						err = wkz->work(world, world->get_active_player(), world->get_zeiger()->get_pos());
						if( err == NULL ) {
							// Check if we need to update pointer(zeiger) position.
							if ( wkz->update_pos_after_use() ) {
								// Cursor might need movement (screen has changed, we get a new one under the mouse pointer)
								const koord3d pos_new = world->get_new_cursor_position(koord(ev.mx,ev.my),wkz->is_grid_tool());
								world->get_zeiger()->set_pos(pos_new);
							}
						}
					}
				}
				else {
					// queue tool for network
					nwc_tool_t *nwc = new nwc_tool_t(world->get_active_player(), wkz, world->get_zeiger()->get_pos(), world->get_steps(), world->get_map_counter(), false);
					network_send_server(nwc);
					result = false;
					// reset tool
					wkz->init(world, world->get_active_player());
				}
			}
			if (result) {
				// play sound / error message
				world->get_active_player()->tell_tool_result(wkz, world->get_zeiger()->get_pos(), err, true);
			}
			wkz->flags = 0;
		}
	}

	// mouse wheel scrolled -> rezoom
	if (ev.ev_class == EVENT_CLICK) {
		const sint16 org_raster_width = get_tile_raster_width();
		if(ev.ev_code==MOUSE_WHEELUP) {
			if(win_change_zoom_factor(true)) {
				world->set_dirty();
			}
		}
		else if(ev.ev_code==MOUSE_WHEELDOWN) {
			if(win_change_zoom_factor(false)) {
				world->set_dirty();
			}
		}
		const sint16 new_raster_width = get_tile_raster_width();
		if (org_raster_width != new_raster_width) {
			// scale the fine offsets for displaying
			world->set_x_off( (world->get_x_off() * new_raster_width) / org_raster_width );
			world->set_y_off( (world->get_y_off() * new_raster_width) / org_raster_width );
		}
	}
}


bool interaction_t::process_event( event_t &ev )
{
	if(ev.ev_class==EVENT_SYSTEM  &&  ev.ev_code==SYSTEM_QUIT) {
		// quit the program if this window is closed
		destroy_all_win(true);
		env_t::quit_simutrans = true;
		return true;
	}

	if(ev.ev_class==IGNORE_EVENT) {
		// ignore it
		return false;
	}

	DBG_DEBUG4("interaction_t::process_event", "calling check_pos_win");
	if(check_pos_win(&ev)){
		// The event is shallowed by the GUI, next.
		return false;
	}

	DBG_DEBUG4("interaction_t::process_event", "after check_pos_win");

	// Handle map drag with right-click

	bool cursor_hidden = false;

	if(IS_RIGHTCLICK(&ev)) {
		display_show_pointer(false);
		cursor_hidden = true;
	}
	else if(IS_RIGHTRELEASE(&ev)) {
		display_show_pointer(true);
		cursor_hidden = false;
	}
	else if(IS_RIGHTDRAG(&ev)) {
		// unset following
		world->set_follow_convoi( convoihandle_t() );
		move_view(ev);
	}
	else {
		if(cursor_hidden) {
			display_show_pointer(true);
			cursor_hidden = false;
		}
	}

	DBG_DEBUG4("interaction_t::process_event", "check if cursor needs movement");


	if( (ev.ev_class==EVENT_DRAG  &&  ev.ev_code==MOUSE_LEFTBUTTON)  ||  (ev.button_state==0  &&  ev.ev_class==EVENT_MOVE)  ||  ev.ev_class==EVENT_RELEASE) {
		move_cursor(ev);
	}

	DBG_DEBUG4("interaction_t::process_event", "calling interactive_event");

	interactive_event(ev);

	DBG_DEBUG4("interaction_t::process_event", "end of event handling");

	return false;
}


void interaction_t::check_events()
{
	event_t ev;

	if(  env_t::networkmode  ) {
		set_random_mode( INTERACTIVE_RANDOM );
	}

	win_poll_event(&ev);

	while (  ev.ev_class != EVENT_NONE ) {

		DBG_DEBUG4("interaction_t::check_events", "called win_poll_event");

		if (process_event(ev)) {
			// We have been asked to stop processing, exit.
			return;
		}

		win_poll_event(&ev);
	}

	if(  env_t::networkmode  ) {
		clear_random_mode( INTERACTIVE_RANDOM );
	}
}


interaction_t::interaction_t(karte_t *welt)
{
	world = welt;
	is_dragging = false;
}
