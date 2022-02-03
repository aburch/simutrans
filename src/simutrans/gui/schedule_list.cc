/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "messagebox.h"
#include "schedule_list.h"
#include "line_management_gui.h"
#include "components/gui_convoiinfo.h"
#include "line_item.h"
#include "simwin.h"

#include "../simcolor.h"
#include "../obj/depot.h"
#include "../simhalt.h"
#include "../world/simworld.h"
#include "../simevent.h"
#include "../display/simgraph.h"
#include "../simskin.h"
#include "../simconvoi.h"
#include "../vehicle/vehicle.h"
#include "../simlinemgmt.h"
#include "../tool/simmenu.h"
#include "../utils/simstring.h"
#include "../player/simplay.h"

#include "../builder/vehikelbauer.h"

#include "../dataobj/schedule.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include "../obj/way/kanal.h"
#include "../obj/way/maglev.h"
#include "../obj/way/monorail.h"
#include "../obj/way/narrowgauge.h"
#include "../obj/way/runway.h"
#include "../obj/way/schiene.h"
#include "../obj/way/strasse.h"

#include "../utils/unicode.h"


#include "minimap.h"

#define MAX_SORT_IDX (5)
static uint8 idx_to_sort_mode[MAX_SORT_IDX] = { line_scrollitem_t::SORT_BY_NAME, line_scrollitem_t::SORT_BY_PROFIT, line_scrollitem_t::SORT_BY_REVENUE, line_scrollitem_t::SORT_BY_TRANSPORTED, line_scrollitem_t::SORT_BY_CONVOIS };
static const char *idx_to_sort_text[MAX_SORT_IDX] = { "Name", "Profit", "Revenue", "Transported", "Number of convois" };

/// selected tab per player
static uint8 selected_tab[MAX_PLAYER_COUNT] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/// selected line per tab (static)
linehandle_t schedule_list_gui_t::selected_line[MAX_PLAYER_COUNT][simline_t::MAX_LINE_TYPE];

schedule_list_gui_t::schedule_list_gui_t(player_t *player_) :
	gui_frame_t( translator::translate("Line Management"), player_),
	player(player_),
	scl(gui_scrolled_list_t::listskin, line_scrollitem_t::compare)
{
	schedule_filter[0] = 0;
	old_schedule_filter[0] = 0;
	current_sort_mode = 0;
	line_scrollitem_t::sort_reverse = false;

	// add components
	// first column: scrolled list of all lines

	set_table_layout(1,0);

	add_table(4,0);
	{
		// below line list: line filter
		new_component<gui_label_t>("Filter:");
		inp_filter.set_text( schedule_filter, lengthof( schedule_filter ) );
		inp_filter.add_listener( this );
		add_component( &inp_filter, 2 );
		new_component<gui_fill_t>();

		// freight type filter
		new_component<gui_empty_t>();
		viewable_freight_types.append(NULL);
		freight_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("All"), SYSCOL_TEXT);
		viewable_freight_types.append(goods_manager_t::passengers);
		freight_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Passagiere"), SYSCOL_TEXT);
		viewable_freight_types.append(goods_manager_t::mail);
		freight_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Post"), SYSCOL_TEXT);
		viewable_freight_types.append(goods_manager_t::none); // for all freight ...
		freight_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Fracht"), SYSCOL_TEXT);
		for (int i = 0; i < goods_manager_t::get_max_catg_index(); i++) {
			const goods_desc_t* freight_type = goods_manager_t::get_info_catg(i);
			const int index = freight_type->get_catg_index();
			if (index == goods_manager_t::INDEX_NONE || freight_type->get_catg() == 0) {
				continue;
			}
			freight_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(freight_type->get_catg_name()), SYSCOL_TEXT);
			viewable_freight_types.append(freight_type);
		}
		for (int i = 0; i < goods_manager_t::get_count(); i++) {
			const goods_desc_t* ware = goods_manager_t::get_info(i);
			if (ware->get_catg() == 0 && ware->get_index() > 2) {
				// Special freight: Each good is special
				viewable_freight_types.append(ware);
				freight_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(ware->get_name()), SYSCOL_TEXT);
			}
		}
		freight_type_c.set_selection(0);
		freight_type_c.set_focusable(true);
		freight_type_c.add_listener(this);
		add_component(&freight_type_c, 2);
		new_component<gui_fill_t>();


		// sort by what
		new_component<gui_label_t>("hl_txt_sort");
		for( int i=0; i<MAX_SORT_IDX;  i++ ) {
			sort_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate(idx_to_sort_text[i]), SYSCOL_TEXT) ;
		}
		sort_type_c.set_selection(current_sort_mode);
		sort_type_c.set_focusable( true );
		sort_type_c.add_listener( this );
		add_component(&sort_type_c);

		sorteddir.init(button_t::sortarrow_state, NULL);
		sorteddir.add_listener(this);
		sorteddir.pressed = line_scrollitem_t::sort_reverse;
		add_component(&sorteddir);

		new_component<gui_fill_t>();

		// line control buttons
		bt_new_line.init(button_t::roundbox, "New Line");
		bt_new_line.add_listener(this);
		add_component(&bt_new_line);

		bt_delete_line.init(button_t::roundbox, "Delete Line");
		bt_delete_line.set_tooltip("Delete the selected line (if without associated convois).");
		bt_delete_line.add_listener(this);
		add_component( &bt_delete_line );

		bt_single_line.init( button_t::square_automatic, "Single GUI" );
		bt_single_line.set_tooltip( "Closes topmost line window when new line selected." );
		bt_single_line.add_listener( this );
		bt_single_line.pressed = env_t::single_line_gui;
		add_component( &bt_single_line );

		new_component<gui_fill_t>();

	}
	end_table();

	// init scrolled list
	scl.add_listener(this);

	// tab panel
	tabs.init_tabs(&scl);
	tabs.add_listener(this);
	add_component(&tabs);

	// recover last selected line
	tabs.set_active_tab_waytype(simline_t::linetype_to_waytype((simline_t::linetype)selected_tab[player->get_player_nr()]));

	build_line_list((simline_t::linetype)selected_tab[player->get_player_nr()]);

	set_resizemode(diagonal_resize);
	reset_min_windowsize();
}


bool schedule_list_gui_t::action_triggered( gui_action_creator_t *comp, value_t v )
{
	if(  comp == &bt_new_line  ) {
		// create typed line
		assert(  tabs.get_active_tab_index() > 0  );
		// update line schedule via tool!
		tool_t *tmp_tool = create_tool( TOOL_CHANGE_LINE | SIMPLE_TOOL );
		cbuffer_t buf;
		int type = simline_t::waytype_to_linetype(tabs.get_active_tab_waytype());
		buf.printf( "c,0,%i,0,0|%i|", type, type );
		tmp_tool->set_default_param(buf);
		welt->set_tool( tmp_tool, player );
		// since init always returns false, it is safe to delete immediately
		delete tmp_tool;
		depot_t::update_all_win();
	}
	else if(  comp == &bt_delete_line  ) {
		vector_tpl<sint32> sel = scl.get_selections();
		FOR(vector_tpl<sint32>, i, sel) {
			linehandle_t line = ((line_scrollitem_t*)scl.get_element(i))->get_line();
			if (line->count_convoys()==0) {
				// delete this line via tool
				tool_t* tmp_tool = create_tool(TOOL_CHANGE_LINE | SIMPLE_TOOL);
				cbuffer_t buf;
				buf.printf("d,%i", line.get_id());
				tmp_tool->set_default_param(buf);
				welt->set_tool(tmp_tool, player);
				// since init always returns false, it is safe to delete immediately
				delete tmp_tool;
			}
		}
		depot_t::update_all_win();
	}
	else if(  comp == &bt_single_line  ) {
		env_t::single_line_gui ^= 1;
		bt_single_line.pressed = env_t::single_line_gui;
	}
	else if(  comp == &tabs  ) {
		int const tab = tabs.get_active_tab_index();
		uint8 old_selected_tab = selected_tab[player->get_player_nr()];
		selected_tab[player->get_player_nr()] = simline_t::waytype_to_linetype(tabs.get_active_tab_waytype());
		if(  old_selected_tab == simline_t::line  &&  selected_line[player->get_player_nr()][0].is_bound()  &&
			selected_line[player->get_player_nr()][0]->get_linetype() == selected_tab[player->get_player_nr()]  ) {
				// switching from general to same waytype tab while line is seletced => use current line instead
				selected_line[player->get_player_nr()][selected_tab[player->get_player_nr()]] = selected_line[player->get_player_nr()][0];
		}
		build_line_list((simline_t::linetype)selected_tab[player->get_player_nr()]);
		if(  tab>0  ) {
			bt_new_line.enable();
		}
		else {
			bt_new_line.disable();
		}
	}
	else if(  comp == &scl  ) {
		if(  line_scrollitem_t *li=(line_scrollitem_t *)scl.get_element(v.i)  ) {
			line = li->get_line();

			gui_frame_t *line_info = win_get_magic( (ptrdiff_t)line.get_rep() );
			// try to open to the right
			scr_coord sc = win_get_pos( this );
			scr_coord lc = sc;
			lc.x += get_windowsize().w;
			if( lc.x > display_get_width() ) {
				lc.x = max( 0, display_get_width() - 100 );
			}
			if(  line_info  ) {
				// close if open
				destroy_win( line_info );
			}
			else {
				if( bt_single_line.pressed  &&  win_get_open_count()>=2 ) {
					// close second topmost line window
					for( int i = win_get_open_count()-1; i>=0; i-- ) {
						if( gui_frame_t* gui = win_get_index( i ) ) {
							if( gui->get_rdwr_id()==magic_line_schedule_rdwr_dummy ) {
								destroy_win( gui );
								break;
							}
						}
					}
				}
				create_win( lc.x, lc.y, new line_management_gui_t(line, player, -1), w_info, (ptrdiff_t)line.get_rep(), true );
			}
		}
		scl.set_selection( -1 );
	}
	else if(  comp == &inp_filter  ) {
		if(  strcmp(old_schedule_filter,schedule_filter)  ) {
			build_line_list(simline_t::waytype_to_linetype(tabs.get_active_tab_waytype()));
			strcpy(old_schedule_filter,schedule_filter);
		}
	}
	else if(  comp == &freight_type_c  ) {
		build_line_list(simline_t::waytype_to_linetype(tabs.get_active_tab_waytype()));
	}
	else if (comp == &sorteddir) {
		line_scrollitem_t::sort_reverse = !line_scrollitem_t::sort_reverse;
		sorteddir.pressed = line_scrollitem_t::sort_reverse;
		build_line_list(simline_t::waytype_to_linetype(tabs.get_active_tab_waytype()));
	}
	else if(  comp == &sort_type_c  ) {
		build_line_list(simline_t::waytype_to_linetype(tabs.get_active_tab_waytype()));
	}

	return true;
}


void schedule_list_gui_t::draw(scr_coord pos, scr_size size)
{
	// deativate buttons, if not curretn player
	const bool activate = player == welt->get_active_player()  ||   welt->get_active_player()==welt->get_player( 1 );
	bt_new_line.enable( activate  &&  tabs.get_active_tab_index() > 0);
	bt_delete_line.enable(activate);

	// if search string changed, update line selection
	if(  old_line_count != player->simlinemgmt.get_line_count()  ||  strcmp( old_schedule_filter, schedule_filter )  ) {
		old_line_count = player->simlinemgmt.get_line_count();
		build_line_list(simline_t::waytype_to_linetype(tabs.get_active_tab_waytype()));
		strcpy( old_schedule_filter, schedule_filter );
	}

	vector_tpl<sint32> sel = scl.get_selections();
	bool can_delete = sel.get_count() > 0  &&  activate;
	if (can_delete) {
		FOR(vector_tpl<sint32>, i, sel) {
			linehandle_t line = ((line_scrollitem_t*)scl.get_element(i))->get_line();
			if (line->count_convoys() > 0) {
				can_delete = false;
				break;
			}
		}

	}
	bt_delete_line.enable(can_delete);

	gui_frame_t::draw(pos, size);
}


void schedule_list_gui_t::build_line_list(simline_t::linetype filter)
{
	vector_tpl<linehandle_t>lines;

	sint32 sel = -1;
	scl.clear_elements();
	player->simlinemgmt.get_lines(filter, &lines);

	FOR(vector_tpl<linehandle_t>, const l, lines) {
		// search name
		if(  !*schedule_filter  ||  utf8caseutf8(l->get_name(), schedule_filter)  ) {
			// match good category
			if(  is_matching_freight_catg( l->get_goods_catg_index() )  ) {
				scl.new_component<line_scrollitem_t>(l, line_scrollitem_t::SELECT_WIN);
			}
			if(  line == l  ) {
				sel = scl.get_count() - 1;
			}
		}
	}
	scl.set_selection( sel );

	current_sort_mode = idx_to_sort_mode[ sort_type_c.get_selection() ];
	line_scrollitem_t::sort_mode = (line_scrollitem_t::sort_modes_t)current_sort_mode;
	scl.sort( 0 );
	scl.set_size(scl.get_size());

	old_line_count = player->simlinemgmt.get_line_count();
}


uint32 schedule_list_gui_t::get_rdwr_id()
{
	return magic_line_management_t+player->get_player_nr();
}


void schedule_list_gui_t::rdwr( loadsave_t *file )
{
	scr_size size;
	if(  file->is_saving()  ) {
		size = get_windowsize();
	}
	size.rdwr( file );
	tabs.rdwr( file );
	simline_t::rdwr_linehandle_t(file, line);
	scl.rdwr(file);
	sort_type_c.rdwr(file);
	freight_type_c.rdwr(file);
	file->rdwr_str(schedule_filter, lengthof(schedule_filter));
	file->rdwr_bool(line_scrollitem_t::sort_reverse);

	// open dialogue
	if(  file->is_loading()  ) {
		current_sort_mode = idx_to_sort_mode[sort_type_c.get_selection()];
		line_scrollitem_t::sort_mode = (line_scrollitem_t::sort_modes_t)current_sort_mode;
		build_line_list(simline_t::waytype_to_linetype(tabs.get_active_tab_waytype()));
		set_windowsize(size);
		// that would be the proper way to restore windows, would keep the simwin side cleaner) ...
//		win_set_magic(magic_line_management_t + player->get_player_nr())
	}
}


// borrowed code from minimap
bool schedule_list_gui_t::is_matching_freight_catg(const minivec_tpl<uint8> &goods_catg_index)
{
	const goods_desc_t *line_freight_type_group_index = viewable_freight_types[ freight_type_c.get_selection() ];
	// does this line/convoi has a matching freight
	if(  line_freight_type_group_index == goods_manager_t::passengers  ) {
		return goods_catg_index.is_contained(goods_manager_t::INDEX_PAS);
	}
	else if(  line_freight_type_group_index == goods_manager_t::mail  ) {
		return goods_catg_index.is_contained(goods_manager_t::INDEX_MAIL);
	}
	else if(  line_freight_type_group_index == goods_manager_t::none  ) {
		// all freights but not pax or mail
		for(  uint8 i = 0;  i < goods_catg_index.get_count();  i++  ) {
			if(  goods_catg_index[i] > goods_manager_t::INDEX_NONE  ) {
				return true;
			}
		}
		return false;
	}
	else if(  line_freight_type_group_index != NULL  ) {
		for(  uint8 i = 0;  i < goods_catg_index.get_count();  i++  ) {
			if(  goods_catg_index[i] == line_freight_type_group_index->get_catg_index()  ) {
				return true;
			}
		}
		return false;
	}
	// all true
	return true;
}
