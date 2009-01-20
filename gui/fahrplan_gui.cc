/*
 * dialog zur Eingabe eines Fahrplanes
 *
 * Hj. Malthaner
 *
 * Juli 2000
 */

#include "../simconvoi.h"
#include "../simline.h"
#include "../simlinemgmt.h"
#include "../simcolor.h"
#include "../simwin.h"
#include "../simhalt.h"
#include "../simskin.h"
#include "../simworld.h"
#include "../simwerkz.h"
#include "../simgraph.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "../boden/grund.h"

#include "../dataobj/fahrplan.h"
#include "../dataobj/translator.h"

#include "fahrplan_gui.h"
#include "line_item.h"
#include "components/list_button.h"
#include "components/gui_button.h"
#include "karte.h"

char fahrplan_gui_t::no_line[128];	// contains the current translation of "<no line>"

// here are the default loading values
#define MAX_LADEGRADE (12)

static char ladegrade[MAX_LADEGRADE]=
{
	0, 1, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100
};


/**
 * Fuellt buf mit Beschreibung des i-ten Eintrages des Fahrplanes
 *
 * @author Hj. Malthaner
 */
void fahrplan_gui_t::gimme_stop_name(cbuffer_t & buf, karte_t *welt, const schedule_t *fpl, int i, int max_chars)
{
	if(i<0  ||  fpl==NULL  ||  i>=fpl->get_count()) {
		dbg->warning("void fahrplan_gui_t::gimme_stop_name()","tried to recieved unused entry %i in schedule %p.",i,fpl);
		return;
	}
	const linieneintrag_t& entry = fpl->eintrag[i];
	const grund_t* gr = welt->lookup(entry.pos);
	char tmp [256];

	if(gr==NULL) {
		sprintf( tmp, "%s (%i,%i,%i)", translator::translate("Invalid coordinate"), entry.pos.x, entry.pos.y, entry.pos.z );
	}
	else {
		halthandle_t halt = gr->ist_wasser() ? haltestelle_t::get_halt(welt, entry.pos) : gr->get_halt();

		if(halt.is_bound()) {
			if (entry.ladegrad != 0) {
				sprintf(tmp, "%d%% %s (%d,%d)",
					entry.ladegrad,
					halt->get_name(),
					entry.pos.x, entry.pos.y);
			}
			else {
				sprintf(tmp, "%s (%d,%d)",
					halt->get_name(),
					entry.pos.x, entry.pos.y);
			}
		}
		else {
			if(gr->get_depot() != NULL) {
				sprintf(tmp, "%s (%d,%d)", translator::translate("Depot"), entry.pos.x, entry.pos.y);
			}
			else {
				sprintf(tmp, "%s (%d,%d)", translator::translate("Wegpunkt"), entry.pos.x, entry.pos.y);
			}
		}
	}
	sprintf(tmp+max_chars-4, "...");
	buf.append(tmp);
}


/**
 * Fuellt buf mit Beschreibung des i-ten Eintrages des Fahrplanes
 * short version, without loading level and position ...
 * @author Hj. Malthaner
 */
void fahrplan_gui_t::gimme_short_stop_name(cbuffer_t &buf, karte_t *welt, const schedule_t *fpl, int i, int max_chars)
{
	if(i<0  ||  fpl==NULL  ||  i>=fpl->get_count()) {
		dbg->warning("void fahrplan_gui_t::gimme_stop_name()","tried to recieved unused entry %i in schedule %p.",i,fpl);
		return;
	}
	const linieneintrag_t& entry = fpl->eintrag[i];
	const grund_t* gr = welt->lookup(entry.pos);
	const char *p;
	if(gr==NULL) {
		p = translator::translate("Invalid coordinate");
	}
	else {
		halthandle_t halt = haltestelle_t::get_halt(welt, entry.pos);

		if(halt.is_bound()) {
			p = halt->get_name();
		}
		else {
			if(gr->get_depot() != NULL) {
				p = translator::translate("Depot");
			}
			else {
				p = translator::translate("Wegpunkt");
			}
		}
	}
	// finally append
	if(strlen(p)>(unsigned)max_chars) {
		ALLOCA(char, tmp, max_chars + 1);
		strncpy( tmp, p, max_chars-3 );
		strcpy( tmp+max_chars-3, "..." );
		buf.append(tmp);
	}
	else {
		buf.append(p);
	}
}



karte_t *fahrplan_gui_stats_t::welt = NULL;

void fahrplan_gui_stats_t::zeichnen(koord offset)
{
	if(fpl) {
		sint16 width = 16;
		cbuffer_t buf(512);
		image_id const arrow_right_normal = skinverwaltung_t::window_skin->get_bild(10)->get_nummer();

		for (int i = 0; i < fpl->get_count(); i++) {

			buf.clear();
			buf.printf( "%i) ", i+1 );
			fahrplan_gui_t::gimme_stop_name( buf, welt, fpl, i, 512 );
			width = max( width, display_calc_proportional_string_len_width(buf,buf.len()) );
			display_proportional_clip(offset.x + 4 + 10, offset.y + i * (LINESPACE + 1) + 2, buf, ALIGN_LEFT, COL_BLACK, true);

			if(i!=fpl->get_aktuell()) {
				// goto information
				display_color_img(arrow_right_normal, offset.x + 2, offset.y + i * (LINESPACE + 1), 0, false, true);
			}
			else {
				// select goto button
				display_color_img(skinverwaltung_t::window_skin->get_bild(11)->get_nummer(),
					offset.x + 2, offset.y + i * (LINESPACE + 1), 0, false, true);
			}
		}
		set_groesse( koord(width+11, fpl->get_count() * (LINESPACE + 1) ) );
	}
}



fahrplan_gui_t::~fahrplan_gui_t()
{
	update_werkzeug( false );
	// hide schedule on minimap (may not current, but for safe)
	reliefkarte_t::get_karte()->set_current_fpl(NULL, 0); // (*fpl,player_nr)
}



fahrplan_gui_t::fahrplan_gui_t(schedule_t* fpl_, spieler_t* sp_, convoihandle_t cnv_) :
	gui_frame_t("Fahrplan", sp_),
	lb_line("Serves Line:"),
	lb_wait("month wait time"),
	lb_waitlevel(NULL, COL_WHITE, gui_label_t::right),
	lb_load("Full load"),
	stats(sp_->get_welt()),
	scrolly(&stats)
{
	this->sp = sp_;
	this->fpl = fpl_;
	this->cnv = cnv_;
	stats.set_fahrplan(fpl);
	if(!cnv.is_bound()) {
		new_line = linehandle_t();
		show_line_selector(false);
	}
	else {
		new_line = cnv_->get_line();
	}
	this->fpl = fpl;
	fpl->eingabe_beginnen();
	strcpy(no_line, translator::translate("<no line>"));

	sint16 ypos = 0;
	if (cnv.is_bound()) {
		// things, only relevant to convois, like creating/selecting lines
		lb_line.set_pos(koord(10, ypos+2));
		add_komponente(&lb_line);

		bt_promote_to_line.init(button_t::roundbox, "promote to line", koord( BUTTON_WIDTH*2, ypos ), koord(BUTTON_WIDTH,BUTTON_HEIGHT) );
		bt_promote_to_line.set_tooltip("Create a new line based on this schedule");
		bt_promote_to_line.add_listener(this);
		add_komponente(&bt_promote_to_line);

		ypos += BUTTON_HEIGHT+1;

		line_selector.set_pos(koord(0, ypos));
		line_selector.set_groesse(koord(BUTTON_WIDTH*3, BUTTON_HEIGHT));
		line_selector.set_max_size(koord(BUTTON_WIDTH*3, 13*LINESPACE+2+16));
		line_selector.set_highlight_color(sp->get_player_color1() + 1);
		line_selector.clear_elements();

		sp->simlinemgmt.sort_lines();
		init_line_selector();
		line_selector.add_listener(this);
		add_komponente(&line_selector);

		ypos += BUTTON_HEIGHT+4;
	}

	// loading level and return tickets
	lb_load.set_pos( koord( 10, ypos+2 ) );
	add_komponente(&lb_load);

	numimp_load.set_pos( koord( BUTTON_WIDTH*2-65, ypos+2 ) );
	numimp_load.set_groesse( koord( 60, BUTTON_HEIGHT ) );
	numimp_load.set_value( fpl->get_current_eintrag().ladegrad );
	numimp_load.set_limits( 0, 100 );
	numimp_load.set_increment_mode( gui_numberinput_t::PROGRESS );
	numimp_load.add_listener(this);
	add_komponente(&numimp_load);
	ypos += BUTTON_HEIGHT;

	// waiting in parts per month
	lb_wait.set_pos( koord( 10, ypos+2 ) );
	add_komponente(&lb_wait);

	bt_wait_prev.set_pos( koord( BUTTON_WIDTH*2-65, ypos+3 ) );
	bt_wait_prev.set_typ(button_t::arrowleft);
	bt_wait_prev.add_listener(this);
	add_komponente(&bt_wait_prev);

	if(  fpl->get_current_eintrag().waiting_time_shift==0  ) {
		strcpy( str_parts_month, translator::translate("off") );
	}
	else {
		sprintf( str_parts_month, "1/%d",  1<<(16-fpl->get_current_eintrag().waiting_time_shift) );
	}
	lb_waitlevel.set_text_pointer( str_parts_month );
	lb_waitlevel.set_pos( koord( BUTTON_WIDTH*2-20, ypos+3 ) );
	add_komponente(&lb_waitlevel);

	bt_wait_next.set_pos( koord( BUTTON_WIDTH*2-15, ypos+2 ) );
	bt_wait_next.set_typ(button_t::arrowright);
	bt_wait_next.add_listener(this);
	add_komponente(&bt_wait_next);

	bt_return.init(button_t::roundbox, "return ticket", koord( BUTTON_WIDTH*2, ypos ), koord(BUTTON_WIDTH,BUTTON_HEIGHT) );
	bt_return.set_tooltip("Add stops for backward travel");
	bt_return.add_listener(this);
	add_komponente(&bt_return);

	ypos += BUTTON_HEIGHT;

	bt_add.init(button_t::roundbox_state, "Add Stop", koord( 0, ypos ), koord(BUTTON_WIDTH,BUTTON_HEIGHT) );
	bt_add.set_tooltip("Appends stops at the end of the schedule");
	bt_add.add_listener(this);
	bt_add.pressed = true;
	add_komponente(&bt_add);

	bt_insert.init(button_t::roundbox_state, "Ins Stop", koord( BUTTON_WIDTH, ypos ), koord(BUTTON_WIDTH,BUTTON_HEIGHT) );
	bt_insert.set_tooltip("Insert stop before the current stop");
	bt_insert.add_listener(this);
	bt_insert.pressed = false;
	add_komponente(&bt_insert);

	bt_remove.init(button_t::roundbox_state, "Del Stop", koord( BUTTON_WIDTH*2, ypos ), koord(BUTTON_WIDTH,BUTTON_HEIGHT) );
	bt_remove.set_tooltip("Delete the current stop");
	bt_remove.add_listener(this);
	bt_remove.pressed = false;
	add_komponente(&bt_remove);

	ypos += BUTTON_HEIGHT;
	scrolly.set_pos( koord( 0, ypos ) );
	// scrolly.set_show_scroll_x(false);
	add_komponente(&scrolly);

	mode = adding;
	set_min_windowsize( koord(BUTTON_WIDTH*3, ypos+BUTTON_HEIGHT+3*(LINESPACE + 1)+16) );
	resize( koord(0,0) );
	resize( koord(0,(LINESPACE + 1)*min(15,fpl->get_count())) );

	// set this schedule as current to show on minimap if possible
	reliefkarte_t::get_karte()->set_current_fpl(fpl, sp->get_player_nr()); // (*fpl,player_nr)
	set_resizemode(diagonal_resize);
}




void fahrplan_gui_t::update_werkzeug(bool set)
{
	karte_t *welt = sp->get_welt();
	if(!set  ||  mode==removing  ||  mode==undefined_mode) {
		// reset tools, if still selected ...
		if(welt->get_werkzeug()==werkzeug_t::general_tool[WKZ_FAHRPLAN_ADD]) {
			if(werkzeug_t::general_tool[WKZ_FAHRPLAN_ADD]->default_param==(const char *)fpl) {
				welt->set_werkzeug( werkzeug_t::general_tool[WKZ_ABFRAGE] );
			}
		}
		else if(welt->get_werkzeug()==werkzeug_t::general_tool[WKZ_FAHRPLAN_INS]) {
			if(werkzeug_t::general_tool[WKZ_FAHRPLAN_INS]->default_param==(const char *)fpl) {
				welt->set_werkzeug( werkzeug_t::general_tool[WKZ_ABFRAGE] );
			}
		}
	}
	else {
		//  .. or set them again
		if(mode==adding) {
			werkzeug_t::general_tool[WKZ_FAHRPLAN_ADD]->default_param = (const char *)fpl;
			sp->get_welt()->set_werkzeug( werkzeug_t::general_tool[WKZ_FAHRPLAN_ADD] );
		}
		else if(mode==inserting) {
			werkzeug_t::general_tool[WKZ_FAHRPLAN_INS]->default_param = (const char *)fpl;
			sp->get_welt()->set_werkzeug( werkzeug_t::general_tool[WKZ_FAHRPLAN_INS] );
		}
	}
}




void fahrplan_gui_t::update_selection()
{
	// update load
	lb_load.set_color( COL_GREY3 );
	lb_wait.set_color( COL_GREY3 );
	if(  fpl->get_count()>0  ) {
		fpl->set_aktuell( min(fpl->get_count()-1,fpl->get_aktuell()) );
		const uint8 aktuell = fpl->get_aktuell();
		if(haltestelle_t::get_halt(sp->get_welt(), fpl->eintrag[aktuell].pos).is_bound()) {
			lb_load.set_color( COL_BLACK );
			numimp_load.set_value( fpl->eintrag[aktuell].ladegrad );
			if(  fpl->eintrag[aktuell].ladegrad>0  ) {
				lb_wait.set_color( COL_BLACK );
			}
			if(  fpl->eintrag[aktuell].ladegrad>0  &&  fpl->eintrag[aktuell].waiting_time_shift>0  ) {
				sprintf( str_parts_month, "1/%d",  1<<(16-fpl->eintrag[aktuell].waiting_time_shift) );
			}
			else {
				strcpy( str_parts_month, translator::translate("off") );
			}
		}
		else {
			strcpy( str_ladegrad, "0%" );
			strcpy( str_parts_month, translator::translate("off") );
		}
	}
}



/**
 * Mausklicks werden hiermit an die GUI-Komponenten
 * gemeldet
 */
void
fahrplan_gui_t::infowin_event(const event_t *ev)
{
	if ( (ev)->ev_class == EVENT_CLICK  &&  !((ev)->ev_code==MOUSE_WHEELUP  ||  (ev)->ev_code==MOUSE_WHEELDOWN)  &&  !line_selector.getroffen(ev->cx, ev->cy-16))  {//  &&  !scrolly.getroffen(ev->cx, ev->cy+16)) {

		// close combo box; we must do it ourselves, since the box does not recieve outside events ...
		line_selector.close_box();

		if(ev->my>=scrolly.get_pos().y+16) {
			// we are now in the multiline region ...
			const int line = ( ev->my - scrolly.get_pos().y + scrolly.get_scroll_y() - 16)/(LINESPACE+1);

			if(line >= 0 && line < fpl->get_count()) {
				if(IS_RIGHTCLICK(ev)  ||  ev->mx<16) {
					// just center on it
					sp->get_welt()->change_world_position( fpl->eintrag[line].pos );
				}
				else if(ev->mx<scrolly.get_groesse().x-11) {
					fpl->set_aktuell( line );
					if(mode == removing) {
						fpl->remove();
						action_triggered( &bt_add, value_t() );
					}
					update_selection();
				}
			}
		}
	}
	else if(ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE  ) {

		update_werkzeug( false );
		fpl->cleanup();
		fpl->eingabe_abschliessen();
		if (cnv.is_bound()) {
			// if a line is selected
			if (new_line.is_bound()  &&  new_line->get_schedule()->matches( sp->get_welt(), fpl )) {
				// if the selected line is different to the convoi's line, apply it
				if(new_line!=cnv->get_line()) {
					uint8 akt = fpl->get_aktuell();
					cnv->set_line( new_line );
					cnv->get_schedule()->set_aktuell( akt );
				}
			}
			else {
				// no line is selected or line does not match => unset the line
				cnv->unset_line();
			}
		}
	}
	else if(ev->ev_class == INFOWIN  &&  (ev->ev_code == WIN_TOP  ||  ev->ev_code == WIN_OPEN)  ) {
		// just to be sure, renew the tools ...
		update_werkzeug( true );
	}
	gui_frame_t::infowin_event(ev);
}



bool
fahrplan_gui_t::action_triggered( gui_action_creator_t *komp, value_t p)
{
DBG_MESSAGE("fahrplan_gui_t::action_triggered()","komp=%p combo=%p",komp,&line_selector);

	if(komp == &bt_add) {
		mode = adding;
		bt_add.pressed = true;
		bt_insert.pressed = false;
		bt_remove.pressed = false;
		update_werkzeug( true );

	} else if(komp == &bt_insert) {
		mode = inserting;
		bt_add.pressed = false;
		bt_insert.pressed = true;
		bt_remove.pressed = false;
		update_werkzeug( true );

	} else if(komp == &bt_remove) {
		mode = removing;
		bt_add.pressed = false;
		bt_insert.pressed = false;
		bt_remove.pressed = true;
		update_werkzeug( false );

	} else if(komp == &numimp_load) {
		if(fpl->get_count() > 0) {
			fpl->eintrag[fpl->get_aktuell()].ladegrad = p.i;
			update_selection();
		}
	} else if(komp == &bt_wait_prev) {
		if(fpl->get_count() > 0) {
			sint8& wait = fpl->eintrag[fpl->get_aktuell()].waiting_time_shift;
			if(wait>7) {
				wait --;
			}
			else {
				wait = 0;
			}
			update_selection();
		}
	} else if(komp == &bt_wait_next) {
		if(fpl->get_count() > 0) {
			sint8& wait = fpl->eintrag[fpl->get_aktuell()].waiting_time_shift;
			if(wait==0) {
				wait = 7;
			}
			else if(wait<16) {
				wait ++;
			}
			update_selection();
		}
	} else if (komp == &bt_return) {
		fpl->add_return_way();
	} else if (komp == &line_selector) {
		int selection = p.i;
//DBG_MESSAGE("fahrplan_gui_t::action_triggered()","line selection=%i",selection);
		if(  (unsigned)(selection-1)<line_selector.count_elements()  ) {
			new_line = lines[selection - 1];
			fpl->copy_from( new_line->get_schedule() );
			fpl->eingabe_beginnen();
		}
		else {
			// remove line
			new_line = linehandle_t();
			line_selector.set_selection( 0 );
		}
	} else if (komp == &bt_promote_to_line) {
		new_line = sp->simlinemgmt.create_line(fpl->get_type(), sp, this->fpl);
		init_line_selector();
	}
	// recheck lines
	if (cnv.is_bound()) {
		// unequal to line => remove from line ...
		if(new_line.is_bound()  &&   !fpl->matches(sp->get_welt(),new_line->get_schedule())) {
			new_line = linehandle_t();
			line_selector.set_selection(0);
		}
	}
	scrolly.set_groesse( scrolly.get_groesse() );
	return true;
}


void fahrplan_gui_t::init_line_selector()
{
	line_selector.clear_elements();
	line_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( no_line, COL_BLACK ) );
	int selection = -1;
	sp->simlinemgmt.get_lines(fpl->get_type(), &lines);
	for (vector_tpl<linehandle_t>::const_iterator i = lines.begin(), end = lines.end(); i != end; i++) {
		linehandle_t line = *i;
		line_selector.append_element( new line_scrollitem_t(line) );
		if (new_line == line) {
			selection = line_selector.count_elements() - 1;
		}
	}
	line_selector.set_selection( selection );
}



void fahrplan_gui_t::zeichnen(koord pos, koord gr)
{
	if(  new_line.is_bound()  &&  !new_line->get_schedule()->matches(sp->get_welt(),fpl)  ) {
		line_selector.set_selection(0);
	}
	gui_frame_t::zeichnen(pos,gr);
}



/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void fahrplan_gui_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);

	const koord groesse = get_fenstergroesse();
	scrolly.set_groesse( koord(groesse.x, groesse.y-scrolly.get_pos().y-16) );
}
