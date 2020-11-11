/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../dataobj/translator.h"

#include "simwin.h"
#include "gui_frame.h"
#include "line_item.h"
#include "minimap.h"
#include "depot_frame.h"

#include "../boden/grund.h"

#include "../dataobj/schedule.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"

#include "../player/simplay.h"

#include "../simconvoi.h"
#include "../simdepot.h"
#include "../simmenu.h"
#include "../simworld.h"

#include "schedule_gui.h"

schedule_gui_t::schedule_gui_t(schedule_t* schedule_, player_t* player_, convoihandle_t cnv_) :
	gui_frame_t( translator::translate( "Fahrplan" ), player_ ),
	line_selector(line_scrollitem_t::compare),
	scd( schedule_, player_, cnv_)
{
	cnv = cnv_;
	player = player_;


	// set this schedule as current to show on minimap if possible
	minimap_t::get_instance()->set_selected_cnv( cnv );

	set_table_layout( 1, 0 );

	line_selector.clear_elements();
	init_line_selector();
	line_selector.add_listener(this);
	add_component(&line_selector);

	add_component( &scd );

	set_resizemode(diagonal_resize);
	reset_min_windowsize();
	set_windowsize(get_windowsize());
}


void schedule_gui_t::rdwr(loadsave_t *file)
{
	// window size
	scr_size size = get_windowsize();
	size.rdwr( file );

	convoi_t::rdwr_convoihandle_t(file, cnv);

	simline_t::rdwr_linehandle_t(file, line);
	simline_t::rdwr_linehandle_t(file, old_line);

	scd.rdwr( file );
	if(  scd.get_schedule() == NULL  ) {
		destroy_win(this);
		cnv = convoihandle_t();
	}
	if(file->is_loading()) {
		player = cnv->get_owner();
		init_line_selector();
		reset_min_windowsize();
		set_windowsize(size);
	}
}


void schedule_gui_t::init_line_selector()
{
	if( cnv.is_bound() ) {
		line_selector.clear_elements();
		int selection = 0;
		vector_tpl<linehandle_t> lines;

		player->simlinemgmt.get_lines( cnv->get_schedule()->get_type(), &lines );

		// keep assignment with identical schedules
		if(  line.is_bound()  &&  !cnv->get_schedule()->matches( world(), line->get_schedule()  ) ) {
			if(  old_line.is_bound()  &&  cnv->get_schedule()->matches( world(), old_line->get_schedule()  ) ) {
				line = old_line;
			}
			else {
				line = linehandle_t();
			}
		}
		int offset = 0;
		if( !line.is_bound() ) {
			selection = 0;
			offset = 2;
			line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate( "<no line>" ), SYSCOL_TEXT );
			line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate( "<promote to line>" ), SYSCOL_TEXT );
		}

		FOR( vector_tpl<linehandle_t>, other_line, lines ) {
			line_selector.new_component<line_scrollitem_t>( other_line );
			if( !line.is_bound() ) {
				if( cnv->get_schedule()->matches( world(), other_line->get_schedule() ) ) {
					selection = line_selector.count_elements() - 1;
					line = other_line;
				}
			}
			else if( line == other_line ) {
				selection = line_selector.count_elements() - 1;
			}
		}

		line_selector.set_selection( selection );
		line_scrollitem_t::sort_mode = line_scrollitem_t::SORT_BY_NAME;
		line_selector.sort( offset );
		old_line_count = player->simlinemgmt.get_line_count();
	}
}

// check for line changes
void schedule_gui_t::draw( scr_coord pos, scr_size size )
{
	// recheck lines
	if(  cnv.is_bound()  ) {
		// unequal to line => remove from line ...
		if(  line.is_bound()  &&  !scd.get_schedule()->matches(world(),line->get_schedule())  ) {
			line = linehandle_t();
			line_selector.set_selection(0);
		}
		// only assign old line, when new_line is not equal
		if(  !line.is_bound()  &&  old_line.is_bound()  &&   scd.get_schedule()->matches(world(),old_line->get_schedule())  ) {
			line = old_line;
			init_line_selector();
		}
	}

	// recheck lines
	if(  cnv.is_bound()  ) {
		// unequal to line => remove from line ...
		if(  line.is_bound()  &&  !scd.get_schedule()->matches(world(),line->get_schedule())  ) {
			line = linehandle_t();
			line_selector.set_selection(0);
		}
		// only assign old line, when new_line is not equal
		if(  !line.is_bound()  &&  old_line.is_bound()  &&   scd.get_schedule()->matches(world(),old_line->get_schedule())  ) {
			line = old_line;
			init_line_selector();
		}
	}

	if(  player!=NULL  &&  player->simlinemgmt.get_line_count()!=old_line_count  ) {
		// lines added or deleted
		init_line_selector();
	}

	gui_frame_t::draw( pos, size );
}


bool schedule_gui_t::action_triggered( gui_action_creator_t *comp, value_t p )
{
	if(comp == &line_selector) {
		uint32 selection = p.i;
		if(  line_scrollitem_t *li = dynamic_cast<line_scrollitem_t*>(line_selector.get_element(selection))  ) {
			line = li->get_line();
			scd.init( line->get_schedule(), player, cnv );
		}
		else if(selection==0) {
			// remove line
			line = linehandle_t();
			line_selector.set_selection( 0 );
		}
		else if(selection==1) {
			// promote to line via tool!
			tool_t *tool = create_tool( TOOL_CHANGE_LINE | SIMPLE_TOOL );
			cbuffer_t buf;
			buf.printf( "c,0,%i,%ld,", (int)scd.get_schedule()->get_type(), (long)(intptr_t)cnv->get_schedule() );
			scd.get_schedule()->sprintf_schedule( buf );
			tool->set_default_param(buf);
			welt->set_tool( tool, player );
			// since init always returns false, it is safe to delete immediately
			delete tool;
			}
	}
	else if( comp == &scd ) {
		if( p.p == NULL ) {
			// revert
			init_line_selector();
		}
		else {
			// apply schedule
			if(cnv.is_bound()) {
				// do not send changes if the convoi is about to be deleted
				if (cnv->get_state() != convoi_t::SELF_DESTRUCT) {
					if (cnv->in_depot()) {
						const grund_t* const ground = welt->lookup(cnv->get_home_depot());
						if (ground) {
							const depot_t* const depot = ground->get_depot();
							if (depot) {
								depot_frame_t* const frame = dynamic_cast<depot_frame_t*>(win_get_magic((ptrdiff_t)depot));
								if (frame) {
									frame->update_data();
								}
							}
						}
					}
					// if a line is selected
					if (line.is_bound()) {
						// if the selected line is different to the convoi's line, apply it
						if (line != cnv->get_line()) {
							char id[16];
							sprintf(id, "%i,%i", line.get_id(), scd.get_schedule()->get_current_stop());
							cnv->call_convoi_tool('l', id);
						}
						else {
							cbuffer_t buf;
							scd.get_schedule()->sprintf_schedule(buf);
							cnv->call_convoi_tool('g', buf);
						}
					}
					else {
						cbuffer_t buf;
						scd.get_schedule()->sprintf_schedule(buf);
						cnv->call_convoi_tool('g', buf);
					}
				}
			}
		}
	}
	return true;
}
