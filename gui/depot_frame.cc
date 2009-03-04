/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>
#include <math.h>

#include "../simworld.h"
#include "../vehicle/simvehikel.h"
#include "../simconvoi.h"
#include "../simdepot.h"
#include "../simwin.h"
#include "../simcolor.h"
#include "../simdebug.h"
#include "../simgraph.h"
#include "../simline.h"
#include "../simlinemgmt.h"

#include "../tpl/slist_tpl.h"

#include "fahrplan_gui.h"
#include "line_management_gui.h"
#include "line_item.h"
#include "components/gui_image_list.h"
#include "messagebox.h"
#include "depot_frame.h"

#include "../besch/ware_besch.h"
#include "../besch/intro_dates.h"
#include "../bauer/vehikelbauer.h"
#include "../dataobj/fahrplan.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../utils/simstring.h"

#include "../bauer/warenbauer.h"

#include "../boden/wege/weg.h"


char depot_frame_t::no_line_text[128];	// contains the current translation of "<no line>"


depot_frame_t::depot_frame_t(depot_t* depot) :
	gui_frame_t(txt_title, depot->get_besitzer()),
	depot(depot),
	icnv(depot->convoi_count()-1),
	lb_convois(NULL, COL_BLACK, gui_label_t::left),
	lb_convoi_value(NULL, COL_BLACK, gui_label_t::right),
	lb_convoi_line(NULL, COL_BLACK, gui_label_t::left)
{
DBG_DEBUG("depot_frame_t::depot_frame_t()","get_max_convoi_length()=%i",depot->get_max_convoi_length());
	selected_line = depot->get_selected_line(); //linehandle_t();
	strcpy(no_line_text, translator::translate("<no line>"));

	sprintf(txt_title, "(%d,%d) %s", depot->get_pos().x, depot->get_pos().y, translator::translate(depot->get_name()));

	/*
	 * [CONVOY ASSEMBLER]
	 */
	const waytype_t wt = depot->get_wegtyp();
	const weg_t *w = get_welt()->lookup(depot->get_pos())->get_weg(wt!=tram_wt ? wt : track_wt);
	const bool weg_electrified = w ? w->is_electrified() : false;
	convoy_assembler = new gui_convoy_assembler_t(get_welt(), wt, weg_electrified, depot->get_player_nr() );
	convoy_assembler->set_depot_frame(this);
	convoy_assembler->add_listener(this);
	update_convoy();

	/*
	 * [SELECT]:
	 */
	add_komponente(&lb_convois);

	bt_prev.set_typ(button_t::arrowleft);
	bt_prev.add_listener(this);
	add_komponente(&bt_prev);

	add_komponente(&inp_name);

	bt_next.set_typ(button_t::arrowright);
	bt_next.add_listener(this);
	add_komponente(&bt_next);

	/*
	* [SELECT ROUTE]:
	*/
	line_selector.set_highlight_color(PLAYER_FLAG | (depot->get_besitzer()->get_player_color1()+1));
	line_selector.add_listener(this);
	add_komponente(&line_selector);
	depot->get_besitzer()->simlinemgmt.sort_lines();

	add_komponente(&lb_convoi_value);
	add_komponente(&lb_convoi_line);

	/*
	* [ACTIONS]
	*/
	bt_start.set_typ(button_t::roundbox);
	bt_start.add_listener(this);
	bt_start.set_tooltip("Start the selected vehicle(s)");
	add_komponente(&bt_start);

	bt_schedule.set_typ(button_t::roundbox);
	bt_schedule.add_listener(this);
	bt_schedule.set_tooltip("Give the selected vehicle(s) an individual schedule");
	add_komponente(&bt_schedule);

	bt_destroy.set_typ(button_t::roundbox);
	bt_destroy.add_listener(this);
	bt_destroy.set_tooltip("Move the selected vehicle(s) back to the depot");
	add_komponente(&bt_destroy);

	bt_sell.set_typ(button_t::roundbox);
	bt_sell.add_listener(this);
	bt_sell.set_tooltip("Sell the selected vehicle(s)");
	add_komponente(&bt_sell);

	/*
	* new route management buttons
	*/
	bt_new_line.set_typ(button_t::roundbox);
	bt_new_line.add_listener(this);
	bt_new_line.set_tooltip("Lines are used to manage groups of vehicles");
	add_komponente(&bt_new_line);

	bt_apply_line.set_typ(button_t::roundbox);
	bt_apply_line.add_listener(this);
	bt_apply_line.set_tooltip("Add the selected vehicle(s) to the selected line");
	add_komponente(&bt_apply_line);

	bt_change_line.set_typ(button_t::roundbox);
	bt_change_line.add_listener(this);
	bt_change_line.set_tooltip("Modify the selected line");
	add_komponente(&bt_change_line);

	bt_copy_convoi.set_typ(button_t::roundbox);
	bt_copy_convoi.add_listener(this);
	bt_copy_convoi.set_tooltip("Copy the selected convoi and its schedule or line");
	add_komponente(&bt_copy_convoi);

	koord gr = koord(0,0);
	layout(&gr);
	update_data();
	gui_frame_t::set_fenstergroesse(gr);

	// text will be translated by ourselves (after update data)!
	lb_convois.set_text_pointer(txt_convois);
	lb_convoi_value.set_text_pointer(txt_convoi_value);
	lb_convoi_line.set_text_pointer(txt_convoi_line);

	add_komponente(convoy_assembler);

	// Hajo: Trigger layouting
	set_resizemode(diagonal_resize);
}

depot_frame_t::~depot_frame_t()
{
	delete convoy_assembler;
}


void depot_frame_t::layout(koord *gr)
{
	koord fgr = (gr!=NULL)? *gr : get_fenstergroesse();

	/*
	*	Dialog format:
	*
	*	Main structure are these parts from top to bottom:
	*
	*	    [SELECT]		convoi-selector
	*	    [CONVOI]		current convoi (*)
	*	    [ACTIONS]		convoi action buttons
	*	    [PANEL]		vehicle panel (*)
	*	    [VINFO]		vehicle infotext (*)
	*
	*	(*) In CONVOI ASSEMBLER
	*
	*
	*	Structure of [SELECT] is:
	*
	*	    [Info]
	*	    [PREV][LABEL][NEXT]
	*
	*  PREV and NEXT are small buttons - Label is adjusted to total width.
	*/
	int SELECT_HEIGHT = 14;

	/*
	*	Structure of [ACTIONS] is a row of buttons:
	*
	*	    [Start][Schedule][Destroy][Sell]
	*      [new Route][change Route][delete Route]
	*/
	int ABUTTON_WIDTH = 128;
	int ABUTTON_HEIGHT = 14;
	int ACTIONS_WIDTH = 4 * ABUTTON_WIDTH;
	int ACTIONS_HEIGHT = ABUTTON_HEIGHT + ABUTTON_HEIGHT; // @author hsiegeln: added "+ ABUTTON_HEIGHT"
	convoy_assembler->set_convoy_tabs_skip(ACTIONS_HEIGHT);

	/*
	* Total width is the max from [CONVOI] and [ACTIONS] width.
	*/
	int MIN_TOTAL_WIDTH = max(convoy_assembler->get_convoy_image_width(), ACTIONS_WIDTH);
	int TOTAL_WIDTH = max(fgr.x,max(convoy_assembler->get_convoy_image_width(), ACTIONS_WIDTH));

	/*
	*	Now we can do the first vertical adjustement:
	*/
	int SELECT_VSTART = 16;
	int ASSEMBLER_VSTART = SELECT_VSTART + SELECT_HEIGHT + LINESPACE;
	int ACTIONS_VSTART = ASSEMBLER_VSTART + convoy_assembler->get_convoy_height();

	/*
	* Now we determine the row/col layout for the panel and the total panel
	* size.
	* build_vehicle_lists() fills loks_vec and waggon_vec.
	* Total width will be expanded to match completo columns in panel.
	*/
	convoy_assembler->set_panel_rows(gr  &&  gr->y==0?-1:fgr.y-ASSEMBLER_VSTART);

	/*
	 *	Now we can do the complete vertical adjustement:
	 */
	int TOTAL_HEIGHT = ASSEMBLER_VSTART + convoy_assembler->get_height();
	int MIN_TOTAL_HEIGHT = ASSEMBLER_VSTART + convoy_assembler->get_min_height();

	/*
	* DONE with layout planning - now build everything.
	*/
	set_min_windowsize(koord(MIN_TOTAL_WIDTH, MIN_TOTAL_HEIGHT));
	if(fgr.x<TOTAL_WIDTH) {
		gui_frame_t::set_fenstergroesse(koord(MIN_TOTAL_WIDTH, max(fgr.y,MIN_TOTAL_HEIGHT) ));
	}
	if(gr  &&  gr->x==0) {
		gr->x = TOTAL_WIDTH;
	}
	if(gr  &&  gr->y==0) {
		gr->y = TOTAL_HEIGHT;
	}

	/*
	 * [SELECT]:
	 */
	lb_convois.set_pos(koord(4, SELECT_VSTART - 10));

	bt_prev.set_pos(koord(5, SELECT_VSTART + 2));

	inp_name.set_pos(koord(5+12, SELECT_VSTART));
	inp_name.set_groesse(koord(TOTAL_WIDTH - 26-8, 14));

	bt_next.set_pos(koord(TOTAL_WIDTH - 15, SELECT_VSTART + 2));

	/*
	 * [SELECT ROUTE]:
	 * @author hsiegeln
	 */
	line_selector.set_pos(koord(5, SELECT_VSTART + 14));
	line_selector.set_groesse(koord(TOTAL_WIDTH - 8, 14));
	line_selector.set_max_size(koord(TOTAL_WIDTH - 8, LINESPACE*13+2+16));
	line_selector.set_highlight_color(1);

	/*
	 * [CONVOI]
	 */
	convoy_assembler->set_pos(koord(0,ASSEMBLER_VSTART));
	convoy_assembler->set_groesse(koord(TOTAL_WIDTH,convoy_assembler->get_height()));
	convoy_assembler->layout();

	lb_convoi_value.set_pos(koord(TOTAL_WIDTH-10, ASSEMBLER_VSTART + convoy_assembler->get_convoy_image_height()));
	lb_convoi_line.set_pos(koord(4, ASSEMBLER_VSTART + convoy_assembler->get_convoy_image_height() + LINESPACE * 2));
 

	/*
	 * [ACTIONS]
	 */
	bt_start.set_pos(koord(0, ACTIONS_VSTART));
	bt_start.set_groesse(koord(TOTAL_WIDTH/4, ABUTTON_HEIGHT));
	bt_start.set_text("Start");

	bt_schedule.set_pos(koord(TOTAL_WIDTH/4, ACTIONS_VSTART));
	bt_schedule.set_groesse(koord(TOTAL_WIDTH*2/4-TOTAL_WIDTH/4, ABUTTON_HEIGHT));
	bt_schedule.set_text("Fahrplan");

	bt_destroy.set_pos(koord(TOTAL_WIDTH*2/4, ACTIONS_VSTART));
	bt_destroy.set_groesse(koord(TOTAL_WIDTH*3/4-TOTAL_WIDTH*2/4, ABUTTON_HEIGHT));
	bt_destroy.set_text("Aufloesen");

	bt_sell.set_pos(koord(TOTAL_WIDTH*3/4, ACTIONS_VSTART));
	bt_sell.set_groesse(koord(TOTAL_WIDTH-TOTAL_WIDTH*3/4, ABUTTON_HEIGHT));
	bt_sell.set_text("Verkauf");

	/*
	 * ACTIONS for new route management buttons
	 * @author hsiegeln
	 */
	bt_new_line.set_pos(koord(0, ACTIONS_VSTART+ABUTTON_HEIGHT));
	bt_new_line.set_groesse(koord(TOTAL_WIDTH/4, ABUTTON_HEIGHT));
	bt_new_line.set_text("New Line");

	bt_apply_line.set_pos(koord(TOTAL_WIDTH/4, ACTIONS_VSTART+ABUTTON_HEIGHT));
	bt_apply_line.set_groesse(koord(TOTAL_WIDTH*2/4-TOTAL_WIDTH/4, ABUTTON_HEIGHT));
	bt_apply_line.set_text("Apply Line");

	bt_change_line.set_pos(koord(TOTAL_WIDTH*2/4, ACTIONS_VSTART+ABUTTON_HEIGHT));
	bt_change_line.set_groesse(koord(TOTAL_WIDTH*3/4-TOTAL_WIDTH*2/4, ABUTTON_HEIGHT));
	bt_change_line.set_text("Update Line");

	bt_copy_convoi.set_pos(koord(TOTAL_WIDTH*3/4, ACTIONS_VSTART+ABUTTON_HEIGHT));
	bt_copy_convoi.set_groesse(koord(TOTAL_WIDTH-TOTAL_WIDTH*3/4, ABUTTON_HEIGHT));
	bt_copy_convoi.set_text("Copy Convoi");
}




void depot_frame_t::set_fenstergroesse( koord gr )
{
	koord g=gr;
	layout(&g);
	update_data();
	gui_frame_t::set_fenstergroesse(gr);
}

static void get_line_list(const depot_t* depot, vector_tpl<linehandle_t>* lines)
{
	depot->get_besitzer()->simlinemgmt.get_lines(depot->get_line_type(), lines);
}


void depot_frame_t::update_data()
{
	switch(depot->convoi_count()) {
		case 0:
			tstrncpy(txt_convois, translator::translate("no convois"), lengthof(txt_convois));
			break;
		case 1:
			if(icnv == -1) {
				sprintf(txt_convois, translator::translate("1 convoi"));
			} else {
				sprintf(txt_convois, translator::translate("convoi %d of %d"), icnv + 1, depot->convoi_count());
			}
			break;
		default:
			if(icnv == -1) {
				sprintf(txt_convois, translator::translate("%d convois"), depot->convoi_count());
			} else {
				sprintf(txt_convois, translator::translate("convoi %d of %d"), icnv + 1, depot->convoi_count());
			}
			break;
	}

	/*
	* Reset counts and check for valid vehicles
	*/
	convoihandle_t cnv = depot->get_convoi(icnv);
	if(cnv.is_bound() && cnv->get_vehikel_anzahl() > 0) {
		tstrncpy(txt_cnv_name, cnv->get_internal_name(), lengthof(txt_cnv_name));
		inp_name.set_text(txt_cnv_name, lengthof(txt_cnv_name));
	}

	// update the line selector
	line_selector.clear_elements();
	line_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( no_line_text, COL_BLACK ) );
	if(!selected_line.is_bound()) {
		line_selector.set_selection( 0 );
	}

	// check all matching lines
	vector_tpl<linehandle_t> lines;
	get_line_list(depot, &lines);
	for (vector_tpl<linehandle_t>::const_iterator i = lines.begin(), end = lines.end(); i != end; i++) {
		linehandle_t line = *i;
		line_selector.append_element( new line_scrollitem_t(line) );
		if(line==selected_line) {
			line_selector.set_selection( line_selector.count_elements()-1 );
		}
	}
	convoy_assembler->update_data();
}


bool depot_frame_t::action_triggered( gui_action_creator_t *komp,value_t p)
{
	convoihandle_t cnv = depot->get_convoi(icnv);
	if(cnv.is_bound()) {
		cnv->set_name(txt_cnv_name);
	}

	if(komp != NULL) {	// message from outside!
		if(komp == convoy_assembler) {
			const koord k=*static_cast<const koord *>(p.p);
			switch (k.x) {
				case gui_convoy_assembler_t::clear_convoy_action:
					if (cnv.is_bound()) {
						depot->disassemble_convoi(cnv, false);
						icnv--;
						update_convoy();
					}
					break;
				case gui_convoy_assembler_t::remove_vehicle_action:
					if (cnv.is_bound()) {
						depot->remove_vehicle(cnv, k.y);
					}
					break;
				default: // append/insert_in_front
					const vehikel_besch_t* vb;
					if (k.x==gui_convoy_assembler_t::insert_vehicle_in_front_action) {
						vb=(*convoy_assembler->get_vehicles())[0];
					} else {
						vb=(*convoy_assembler->get_vehicles())[convoy_assembler->get_vehicles()->get_count()-1];
					}
					vehikel_t* veh = depot->find_oldest_newest(vb, true);
					if (veh == NULL) {
						// nothing there => we buy it
						veh = depot->buy_vehicle(vb);
					}
					if(!cnv.is_bound()) 
					{
						// create a new convoi
						cnv = depot->add_convoi();
						icnv = depot->convoi_count() - 1;
						cnv->set_name((*convoy_assembler->get_vehicles())[0]->get_name());
					}
					depot->append_vehicle(cnv, veh, k.x == gui_convoy_assembler_t::insert_vehicle_in_front_action);
					break;
			}
		} else if(komp == &bt_start) {
			if (depot->start_convoi(cnv)) {
				icnv--;
				update_convoy();
			}
		} else if(komp == &bt_schedule) {
			fahrplaneingabe();
			return true;
		} else if(komp == &bt_destroy) {
			if (depot->disassemble_convoi(cnv, false)) {
				icnv--;
				update_convoy();
			}
		} else if(komp == &bt_sell) {
			if (depot->disassemble_convoi(cnv, true)) {
				icnv--;
				update_convoy();
			}
		} else if(komp == &bt_next) {
			if(++icnv == (int)depot->convoi_count()) {
				icnv = -1;
			}
			update_convoy();
		} else if(komp == &bt_prev) {
			if(icnv-- == -1) {
				icnv = depot->convoi_count() - 1;
			}
			update_convoy();
		} else if(komp == &bt_new_line) {
			new_line();
			return true;
		} else if(komp == &bt_change_line) {
			change_line();
			return true;
		} else if(komp == &bt_copy_convoi) {
			depot->copy_convoi(cnv);
			// automatically select newly created convoi
			icnv = depot->convoi_count()-1;
			update_convoy();
		} else if(komp == &bt_apply_line) {
			apply_line();
		} else if(komp == &line_selector) {
			int selection = p.i;
//DBG_MESSAGE("depot_frame_t::action_triggered()","line selection=%i",selection);
			if(  (unsigned)(selection-1)<line_selector.count_elements()  ) {
				vector_tpl<linehandle_t> lines;
				get_line_list(depot, &lines);
				selected_line = lines[selection - 1];
				depot->set_selected_line(selected_line);
			}
			else {
				// remove line
				selected_line = linehandle_t();
				line_selector.set_selection( 0 );
			}
		}
		else {
			return false;
		}
		convoy_assembler->build_vehicle_lists();
	}
	update_data();
	layout(NULL);
	return true;
}


void depot_frame_t::infowin_event(const event_t *ev)
{
	gui_frame_t::infowin_event(ev);

	if(IS_WINDOW_CHOOSE_NEXT(ev)) {

		bool dir = (ev->ev_code==NEXT_WINDOW);
		depot_t *next_dep = depot_t::find_depot( depot->get_pos(), depot->get_typ(), depot->get_besitzer(), dir == NEXT_WINDOW );
		if(next_dep == NULL) {
			if(dir == NEXT_WINDOW) {
				// check the next from start of map
				next_dep = depot_t::find_depot( koord3d(-1,-1,0), depot->get_typ(), depot->get_besitzer(), true );
			}
			else {
				// respecive end of map
				next_dep = depot_t::find_depot( koord3d(8192,8192,127), depot->get_typ(), depot->get_besitzer(), false );
			}
		}

		if(next_dep  &&  next_dep!=this->depot) {
			/**
			 * Replace our depot_frame_t with a new at the same position.
			 * Volker Meyer
			 */
			int x = win_get_posx(this);
			int y = win_get_posy(this);
			destroy_win( this );

			next_dep->zeige_info();
			win_set_pos( win_get_magic((long)next_dep), x, y );
			get_welt()->change_world_position(next_dep->get_pos());
		}
	} else if(IS_WINDOW_REZOOM(ev)) {
		koord gr = get_fenstergroesse();
		set_fenstergroesse(gr);
	} else if(ev->ev_class == INFOWIN && ev->ev_code == WIN_OPEN) {
		convoy_assembler->build_vehicle_lists();
		update_data();
		layout(NULL);
	}
	else {
		if(IS_LEFTCLICK(ev) &&  !line_selector.getroffen(ev->cx, ev->cy-16)) {
			// close combo box; we must do it ourselves, since the box does not recieve outside events ...
			line_selector.close_box();
		}
	}
}



void
depot_frame_t::zeichnen(koord pos, koord groesse)
{
	if (get_welt()->get_active_player() != depot->get_besitzer()) {
		destroy_win(this);
		return;
	}

	const convoihandle_t cnv = depot->get_convoi(icnv);
	if(cnv.is_bound()) {
		if(cnv->get_vehikel_anzahl() > 0) {
			sprintf(txt_convoi_value, "%s %d$", translator::translate("Restwert:"), cnv->calc_restwert()/100);
			// just recheck if schedules match
			if(  cnv->get_line().is_bound()  &&  cnv->get_line()->get_schedule()->ist_abgeschlossen()  ) {
				cnv->check_pending_updates();
				if(  !cnv->get_line()->get_schedule()->matches( get_welt(), cnv->get_schedule() )  ) {
					cnv->unset_line();
				}
			}
			if(  cnv->get_line().is_bound()  ) {
				sprintf(txt_convoi_line, "%s %s", translator::translate("Serves Line:"), cnv->get_line()->get_name());

			}
			else {
				sprintf(txt_convoi_line, "%s %s", translator::translate("Serves Line:"), no_line_text);
			}
		}
		else {
			*txt_convoi_value = '\0';
		}
	}
	else {
		static char empty[2] = "\0";
		inp_name.set_text( empty, 0);
		*txt_convoi_value = '\0';
		*txt_convoi_line = '\0';
	}

	gui_frame_t::zeichnen(pos, groesse);

	if(!cnv.is_bound()) {
		display_proportional_clip(pos.x+inp_name.get_pos().x+2, pos.y+inp_name.get_pos().y+18, translator::translate("new convoi"), ALIGN_LEFT, COL_GREY1, true);
	}
}


void depot_frame_t::new_line()
{
	selected_line = depot->create_line();
DBG_MESSAGE("depot_frame_t::new_line()","id=%d",selected_line.get_id() );
	layout(NULL);
	update_data();
	create_win(new line_management_gui_t(selected_line, depot->get_besitzer()), w_info, (long)selected_line.get_rep() );
DBG_MESSAGE("depot_frame_t::new_line()","id=%d",selected_line.get_id() );
}


void depot_frame_t::apply_line()
{
	if(icnv > -1) {
		convoihandle_t cnv = depot->get_convoi(icnv);
		// if no convoi is selected, do nothing
		if (!cnv.is_bound()) {
			return;
		}

		if(selected_line.is_bound()) {
			// set new route only, a valid route is selected:
			cnv->set_line(selected_line);
		}
		else {
			// sometimes the user might wish to remove convoy from line
			// this happens here
			cnv->unset_line();
		}
	}
}


void depot_frame_t::change_line()
{
	if(selected_line.is_bound()) {
		create_win(new line_management_gui_t(selected_line, depot->get_besitzer()), w_info, (long)selected_line.get_rep() );
	}
}


void depot_frame_t::fahrplaneingabe()
{
	convoihandle_t cnv = depot->get_convoi(icnv);
	if(cnv.is_bound()  &&  cnv->get_vehikel_anzahl() > 0) {

		schedule_t *fpl = cnv->create_schedule();
		if(fpl!=NULL) {
			if(fpl->ist_abgeschlossen()) {

				// Fahrplandialog oeffnen
				create_win(new fahrplan_gui_t(fpl, cnv->get_besitzer(), convoihandle_t() ), w_info, (long)fpl);

				// evtl. hat ein callback cnv gelöscht, so erneut testen
				if(cnv.is_bound()  &&  fpl != NULL) {
					cnv->set_schedule(fpl);
				}
			}
			else {
				gui_fenster_t *fplwin = win_get_magic((long)fpl);
				top_win( fplwin );
//				create_win( new news_img("Es wird bereits\nein Fahrplan\neingegeben\n"), w_time_delete, magic_none);
			}
		}
		else {
			dbg->error( "depot_frame_t::fahrplaneingabe()", "cannot create schedule!" );
		}
	}
	else {
		create_win( new news_img("Please choose vehicles first\n"), w_time_delete, magic_none);
	}
}
