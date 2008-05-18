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
#include "../simplay.h"
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
void fahrplan_gui_t::gimme_stop_name(cbuffer_t & buf,
				     karte_t *welt,
				     const fahrplan_t *fpl,
				     int i,
				     int max_chars)
{
	if(i<0  ||  fpl==NULL  ||  i>=fpl->maxi()) {
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
		halthandle_t halt = gr->ist_wasser() ? haltestelle_t::gib_halt(welt, entry.pos) : gr->gib_halt();

		if(halt.is_bound()) {
			if (entry.ladegrad != 0) {
				sprintf(tmp, "%d%% %s (%d,%d)",
					entry.ladegrad,
					halt->gib_name(),
					entry.pos.x, entry.pos.y);
			}
			else {
				sprintf(tmp, "%s (%d,%d)",
					halt->gib_name(),
					entry.pos.x, entry.pos.y);
			}
		}
		else {
			if(gr->gib_depot() != NULL) {
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
void fahrplan_gui_t::gimme_short_stop_name(cbuffer_t &buf, karte_t *welt, const fahrplan_t *fpl, int i, int max_chars)
{
	if(i<0  ||  fpl==NULL  ||  i>=fpl->maxi()) {
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
		halthandle_t halt = haltestelle_t::gib_halt(welt, entry.pos);

		if(halt.is_bound()) {
			p = halt->gib_name();
		}
		else {
			if(gr->gib_depot() != NULL) {
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
		image_id const arrow_right_normal = skinverwaltung_t::window_skin->gib_bild(10)->gib_nummer();

		for (uint32 i = 0; i < fpl->maxi(); i++) {

			buf.clear();
			buf.printf( "%i) ", i+1 );
			fahrplan_gui_t::gimme_stop_name( buf, welt, fpl, i, 512 );
			width = max( width, display_calc_proportional_string_len_width(buf,buf.len()) );
			display_proportional_clip(offset.x + 4 + 10, offset.y + i * (LINESPACE + 1) + 2, buf, ALIGN_LEFT, COL_BLACK, true);

			if(i!=fpl->aktuell) {
				// goto information
				display_color_img(arrow_right_normal, offset.x + 2, offset.y + i * (LINESPACE + 1), 0, false, true);
			}
			else {
				// select goto button
				display_color_img(skinverwaltung_t::window_skin->gib_bild(11)->gib_nummer(),
					offset.x + 2, offset.y + i * (LINESPACE + 1), 0, false, true);
			}
		}
		setze_groesse( koord(width, fpl->maxi() * (LINESPACE + 1) ) );
	}
}



fahrplan_gui_t::~fahrplan_gui_t()
{
	update_werkzeug( false );
	// hide schedule on minimap (may not current, but for safe)
	reliefkarte_t::gib_karte()->set_current_fpl(NULL, 0); // (*fpl,player_nr)
}



fahrplan_gui_t::fahrplan_gui_t(fahrplan_t* fpl_, spieler_t* sp_, convoihandle_t cnv_) :
	gui_frame_t("Fahrplan", sp_),
	lb_line("Serves Line:"),
	lb_wait("month wait time"),
	lb_waitlevel(NULL, COL_WHITE, gui_label_t::right),
	lb_load("Full load"),
	lb_loadlevel(NULL, COL_WHITE, gui_label_t::right),
	stats(sp_->get_welt()),
	scrolly(&stats)
{
	this->sp = sp_;
	this->fpl = fpl_;
	this->cnv = cnv_;
	stats.setze_fahrplan(fpl);
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
		lb_line.setze_pos(koord(10, ypos+2));
		add_komponente(&lb_line);

		bt_promote_to_line.init(button_t::roundbox, "promote to line", koord( BUTTON_WIDTH*2, ypos ), koord(BUTTON_WIDTH,BUTTON_HEIGHT) );
		bt_promote_to_line.set_tooltip("Create a new line based on this schedule");
		bt_promote_to_line.add_listener(this);
		add_komponente(&bt_promote_to_line);

		ypos += BUTTON_HEIGHT+1;

		line_selector.setze_pos(koord(0, ypos));
		line_selector.setze_groesse(koord(BUTTON_WIDTH*3, BUTTON_HEIGHT));
		line_selector.set_max_size(koord(BUTTON_WIDTH*3, 13*LINESPACE+2+16));
		line_selector.set_highlight_color(sp->get_player_color1() + 1);
		line_selector.clear_elements();

		sp->simlinemgmt.sort_lines();
		init_line_selector();
		line_selector.add_listener(this);
		add_komponente(&line_selector);

		ypos += BUTTON_HEIGHT+4;
	}

	// waiting in parts per month
	lb_wait.setze_pos( koord( 10, ypos+2 ) );
	add_komponente(&lb_wait);

	bt_wait_prev.pos = koord( BUTTON_WIDTH*2-65, ypos+3 );
	bt_wait_prev.setze_typ(button_t::arrowleft);
	bt_wait_prev.add_listener(this);
	add_komponente(&bt_wait_prev);

	if((unsigned)fpl->aktuell>=fpl->maxi()  ||  fpl->eintrag[fpl->aktuell].waiting_time_shift==0) {
		strcpy( str_parts_month, translator::translate("off") );
	}
	else {
		sprintf( str_parts_month, "1/%d",  1<<(16-fpl->eintrag[fpl->aktuell].waiting_time_shift) );
	}
	lb_waitlevel.set_text_pointer( str_parts_month );
	lb_waitlevel.pos = koord( BUTTON_WIDTH*2-20, ypos+3 );
	add_komponente(&lb_waitlevel);

	bt_wait_next.pos = koord( BUTTON_WIDTH*2-15, ypos+2 );
	bt_wait_next.setze_typ(button_t::arrowright);
	bt_wait_next.add_listener(this);
	add_komponente(&bt_wait_next);

	ypos += BUTTON_HEIGHT;

	// loading level and return tickets
	lb_load.setze_pos( koord( 10, ypos+2 ) );
	add_komponente(&lb_load);

	bt_prev.pos = koord( BUTTON_WIDTH*2-65, ypos+3 );
	bt_prev.setze_typ(button_t::arrowleft);
	bt_prev.add_listener(this);
	add_komponente(&bt_prev);

	sprintf( str_ladegrad, "%d%%", (unsigned)fpl->aktuell<fpl->maxi() ? fpl->eintrag[fpl->aktuell].ladegrad : 0 );
	lb_loadlevel.set_text_pointer( str_ladegrad );
	lb_loadlevel.pos = koord( BUTTON_WIDTH*2-20, ypos+3 );
	add_komponente(&lb_loadlevel);

	bt_next.pos = koord( BUTTON_WIDTH*2-15, ypos+2 );
	bt_next.setze_typ(button_t::arrowright);
	bt_next.add_listener(this);
	add_komponente(&bt_next);

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

	scrolly.setze_pos( koord( 0, ypos ) );
	scrolly.setze_groesse( koord(BUTTON_WIDTH*3, 280-ypos-16) );
	// scrolly.set_show_scroll_x(false);
	add_komponente(&scrolly);

	mode = adding;
	setze_fenstergroesse( koord(BUTTON_WIDTH*3, 280) );

	// set this schedule as current to show on minimap if possible
	reliefkarte_t::gib_karte()->set_current_fpl(fpl, sp->get_player_nr()); // (*fpl,player_nr)
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




/**
 * Mausklicks werden hiermit an die GUI-Komponenten
 * gemeldet
 */
void
fahrplan_gui_t::infowin_event(const event_t *ev)
{
	if ( (ev)->ev_class == EVENT_CLICK  && !line_selector.getroffen(ev->cx, ev->cy-16))  {//  &&  !scrolly.getroffen(ev->cx, ev->cy+16)) {

		// close combo box; we must do it ourselves, since the box does not recieve outside events ...
		line_selector.close_box();

		if(ev->my>=scrolly.pos.y+16) {
			// we are now in the multiline region ...
			const int line = ( ev->my - scrolly.pos.y + scrolly.get_scroll_y() - 16)/(LINESPACE+1);

			if(line >= 0 && line < fpl->maxi()) {
				if(IS_RIGHTCLICK(ev)  ||  ev->mx<16) {
					// just center on it
					sp->get_welt()->change_world_position( fpl->eintrag[line].pos );
				}
				else if(ev->mx<scrolly.gib_groesse().x-11) {
					fpl->aktuell = line;
					if(mode == removing) {
						fpl->remove();
						action_triggered( &bt_add, value_t() );
					}
					// update load
					if(fpl->maxi()>0) {
						sprintf( str_ladegrad, "%d%%", fpl->eintrag[fpl->aktuell].ladegrad );
						if(fpl->eintrag[fpl->aktuell].waiting_time_shift) {
							sprintf( str_parts_month, "1/%d",  1<<(16-fpl->eintrag[fpl->aktuell].waiting_time_shift) );
						}
						else {
							strcpy( str_parts_month, translator::translate("off") );
						}
					}
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
			if (new_line.is_bound()) {
				// if the selected line is different to the convoi's line, apply it
				if(new_line!=cnv->get_line()) {
					int akt=fpl->aktuell;
					cnv->set_line(new_line);
					cnv->gib_fahrplan()->aktuell = max( 0, min( akt, cnv->gib_fahrplan()->maxi()-1 ) );
				}
			}
			else {
				// no line is selected, so unset the line
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
fahrplan_gui_t::action_triggered(gui_komponente_t *komp,value_t /* */)
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

	} else if(komp == &bt_prev) {
		if(fpl->maxi() > 0) {
			uint8& load = fpl->eintrag[fpl->aktuell].ladegrad;
			uint8 index=0;
			while(load>ladegrade[index]) {
				index ++;
			}
			load = ladegrade[(index+MAX_LADEGRADE-1)%MAX_LADEGRADE];
			sprintf( str_ladegrad, "%d%%", load );
		}
	} else if(komp == &bt_next) {
		if(fpl->maxi() > 0) {
			uint8& load = fpl->eintrag[fpl->aktuell].ladegrad;
			uint8 index=0;
			while(load>ladegrade[index]) {
				index ++;
			}
			load = ladegrade[(index+1)%MAX_LADEGRADE];
			sprintf( str_ladegrad, "%d%%", load );
		}
	} else if(komp == &bt_wait_prev) {
		if(fpl->maxi() > 0) {
			sint8& wait = fpl->eintrag[fpl->aktuell].waiting_time_shift;
			if(wait>7) {
				wait --;
				sprintf( str_parts_month, "1/%d",  1<<(16-wait) );
			}
			else {
				wait = 0;
				strcpy( str_parts_month, translator::translate("off") );
			}
		}
	} else if(komp == &bt_wait_next) {
		if(fpl->maxi() > 0) {
			sint8& wait = fpl->eintrag[fpl->aktuell].waiting_time_shift;
			if(wait==0) {
				wait = 7;
			}
			else if(wait<16) {
				wait ++;
			}
			sprintf( str_parts_month, "1/%d",  1<<(16-wait) );
		}
	} else if (komp == &bt_return) {
		fpl->add_return_way();
	} else if (komp == &line_selector) {
		int selection = line_selector.get_selection();
DBG_MESSAGE("fahrplan_gui_t::action_triggered()","line selection=%i",selection);
		if (selection>0) {
			new_line = lines[selection - 1];
			line_selector.setze_text(new_line->get_name(), 128);
			fpl->copy_from( new_line->get_fahrplan() );
			fpl->eingabe_beginnen();
		}
		else {
			// remove line from convoy
			line_selector.setze_text(no_line, 128);
			new_line = linehandle_t();
		}
	} else if (komp == &bt_promote_to_line) {
		new_line = sp->simlinemgmt.create_line(fpl->get_type(), this->fpl);
		init_line_selector();
	}
	// recheck lines
	if (cnv.is_bound()) {
		// unequal to line => remove from line ...
		if(new_line.is_bound()  &&   !fpl->matches(new_line->get_fahrplan())) {
			new_line = linehandle_t();
			line_selector.setze_text(no_line, 128);
		}
	}
	scrolly.setze_groesse( scrolly.gib_groesse() );
	return true;
}


void fahrplan_gui_t::init_line_selector()
{
	line_selector.clear_elements();
	line_selector.append_element(no_line);
	int selection = -1;
	sp->simlinemgmt.get_lines(fpl->get_type(), &lines);
	for (vector_tpl<linehandle_t>::const_iterator i = lines.begin(), end = lines.end(); i != end; i++) {
		linehandle_t line = *i;
		line_selector.append_element(line->get_name(), line->get_state_color());
		if (new_line == line) {
			selection = line_selector.count_elements() - 1;
		}
	}

	line_selector.set_selection( selection );
	if(new_line.is_bound()) {
		line_selector.setze_text(new_line->get_name(), 128);
DBG_MESSAGE("fahrplan_gui_t::init_line_selector()","selection %i",selection);
	}
	else {
		line_selector.setze_text(no_line, 128);
	}
}
