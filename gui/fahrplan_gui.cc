/*
 * fahrplan_gui.cc
 *
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

char fahrplan_gui_t::no_line[128];	// contains the current translation of "<no line>"

// here are the default loading values
#define MAX_LADEGRADE (8)

static char ladegrade[MAX_LADEGRADE]=
{
	0, 1, 10, 25, 50, 75, 90, 100
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
	halthandle_t halt = haltestelle_t::gib_halt(welt,fpl->eintrag.get(i).pos);
	char tmp [256];

	if(halt.is_bound()) {
		if(fpl->eintrag.get(i).ladegrad!=0) {
			sprintf(tmp, "%d%% %s (%d,%d)",
				fpl->eintrag.get(i).ladegrad,
				halt->gib_name(),
				fpl->eintrag.get(i).pos.x, fpl->eintrag.get(i).pos.y);
		}
		else {
			sprintf(tmp, "%s (%d,%d)",
				halt->gib_name(),
				fpl->eintrag.get(i).pos.x, fpl->eintrag.get(i).pos.y);
		}
	}
	else {
		const grund_t *gr = welt->lookup(fpl->eintrag.get(i).pos);

		if(gr && gr->gib_depot() != NULL) {
			sprintf(tmp, "%s (%d,%d)", translator::translate("Depot"), fpl->eintrag.get(i).pos.x, fpl->eintrag.get(i).pos.y);
		}
		else {
			sprintf(tmp, "%s (%d,%d)", translator::translate("Wegpunkt"), fpl->eintrag.get(i).pos.x, fpl->eintrag.get(i).pos.y);
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
void fahrplan_gui_t::gimme_short_stop_name(cbuffer_t & buf,
				     karte_t *welt,
				     const fahrplan_t *fpl,
				     int i,
				     int max_chars)
{
	if(i<0  ||  fpl==NULL  ||  i>=fpl->maxi()) {
		dbg->warning("void fahrplan_gui_t::gimme_stop_name()","tried to recieved unused entry %i in schedule %p.",i,fpl);
		return;
	}
	halthandle_t halt = haltestelle_t::gib_halt(welt,fpl->eintrag.get(i).pos);
	const char *p;

	if(halt.is_bound()) {
		p = halt->gib_name();
	}
	else {
		const grund_t *gr = welt->lookup(fpl->eintrag.get(i).pos);

		if(gr && gr->gib_depot() != NULL) {
			p = translator::translate("Depot");
		}
		else {
			p = translator::translate("Wegpunkt");
		}
	}
	// finally append
	if(strlen(p)>(unsigned)max_chars) {
#ifdef _MSC_VER /* no var array on the stack supported */
		char *tmp = static_cast<char *>(alloca(max_chars+1));
#else
		char tmp[max_chars+1];
#endif
		strncpy( tmp, p, max_chars-3 );
		strcpy( tmp+max_chars-3, "..." );
		buf.append(tmp);
	}
	else {
		buf.append(p);
	}
}


fahrplan_gui_t::fahrplan_gui_t(karte_t *welt, convoihandle_t cnv, spieler_t *sp) :
	gui_frame_t("Fahrplan", sp),
	scrolly(&fpl_text),
	fpl_text("\n"),
	lb_line("Serves Line:"),
	lb_load("Full load"),
	buf(8192)
{
	this->welt = welt;
	this->sp = sp;
	this->fpl = cnv->gib_fahrplan();
	this->cnv = cnv;
	if (cnv->get_line().is_bound()) {
		new_line = cnv->get_line();
	}
	init();
}



fahrplan_gui_t::fahrplan_gui_t(karte_t *welt, fahrplan_t *fpl, spieler_t *sp) :
	gui_frame_t("Fahrplan", sp),
	scrolly(&fpl_text),
	fpl_text("\n"),
	lb_line("Serves Line:"),
	lb_load("Full load"),
	buf(8192)
{
	this->welt = welt;
	this->sp = sp;
	this->fpl = fpl;
DBG_DEBUG("fahrplan_gui_t::fahrplan_gui_t()","fahrplan %p",fpl);
	this->cnv = NULL;
	new_line = linehandle_t();
	show_line_selector(false);
	init();
}



void fahrplan_gui_t::init()
{
	setze_opaque( true );

	fpl->eingabe_beginnen();
	strcpy(no_line, translator::translate("<no line>"));

	bt_add.pos = koord( 0, 280-BUTTON_HEIGHT-16 );
	bt_add.groesse = koord(BUTTON_WIDTH,BUTTON_HEIGHT);
	bt_add.setze_text("Add Stop");
	bt_add.setze_typ(button_t::roundbox_state);
	bt_add.add_listener(this);
	bt_add.pressed = true;
	add_komponente(&bt_add);

	bt_insert.pos = koord( BUTTON_WIDTH, 280-BUTTON_HEIGHT-16 );
	bt_insert.groesse = koord(BUTTON_WIDTH,BUTTON_HEIGHT);
	bt_insert.setze_text("Ins Stop");
	bt_insert.setze_typ(button_t::roundbox_state);
	bt_insert.add_listener(this);
	bt_insert.pressed = false;
	add_komponente(&bt_insert);

	bt_remove.pos = koord( BUTTON_WIDTH*2, 280-BUTTON_HEIGHT-16 );
	bt_remove.groesse = koord(BUTTON_WIDTH,BUTTON_HEIGHT);
	bt_remove.setze_text("Del Stop");
	bt_remove.setze_typ(button_t::roundbox_state);
	bt_remove.add_listener(this);
	bt_remove.pressed = false;
	add_komponente(&bt_remove);

	bt_prev.pos.x = 85;
	bt_prev.pos.y = 23;
	bt_prev.setze_typ(button_t::arrowleft);
	bt_prev.add_listener(this);
	add_komponente(&bt_prev);

	bt_next.pos.x = 140;
	bt_next.pos.y = 23;
	bt_next.setze_typ(button_t::arrowright);
	bt_next.add_listener(this);
	add_komponente(&bt_next);

	if(cnv!=NULL) {
		bt_promote_to_line.pos = koord( 0, 280-BUTTON_HEIGHT-BUTTON_HEIGHT-16 );
		bt_promote_to_line.groesse = koord((BUTTON_WIDTH*3)/2,BUTTON_HEIGHT);
		bt_promote_to_line.pos.x = bt_add.pos.x;
		bt_promote_to_line.setze_text("promote to line");
		bt_promote_to_line.set_tooltip("Create a new line based on this schedule");
		bt_promote_to_line.setze_typ(button_t::roundbox);
		bt_promote_to_line.add_listener(this);
		add_komponente(&bt_promote_to_line);
	}

	bt_return.pos = koord((BUTTON_WIDTH*3)/2, 280-BUTTON_HEIGHT-BUTTON_HEIGHT-16 );
	bt_return.groesse = koord((BUTTON_WIDTH*3)/2,BUTTON_HEIGHT);
	bt_return.setze_text("return ticket");
	bt_return.set_tooltip("Add stops for backward travel");
	bt_return.setze_typ(button_t::roundbox);
	bt_return.add_listener(this);
	add_komponente(&bt_return);

	line_selector.setze_pos(koord(BUTTON_WIDTH, 5));
	line_selector.setze_groesse(koord(BUTTON_WIDTH*2, 14));
	line_selector.set_max_size(koord(BUTTON_WIDTH*2, 13*LINESPACE+2+16));
	line_selector.set_highlight_color(welt->get_active_player()->get_player_color()+1);
	line_selector.clear_elements();

	sp->simlinemgmt.sort_lines();
	init_line_selector();
	line_selector.add_listener(this);
	add_komponente(&line_selector);

	lb_line.setze_pos(koord(11, 7));
	add_komponente(&lb_line);

	lb_load.setze_pos(koord(11, 23));
	add_komponente(&lb_load);

	if(fpl->maxi() > 0) {
		mode = none;
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN, NO_SOUND, NO_SOUND);
	}
	else {
		mode = adding;
		welt->setze_maus_funktion(wkz_fahrplan_add, skinverwaltung_t::fahrplanzeiger->gib_bild_nr(0), welt->Z_PLAN, (value_t)fpl, NO_SOUND, NO_SOUND);
	}

	// fill buffer with halt detail
	buf.clear();
	fpl_text.setze_text(buf);
	fpl_text.setze_pos(koord(10,10));

	// add scrollbar
	scrolly.setze_pos(koord(0, 39));
	scrolly.setze_groesse( koord(BUTTON_WIDTH*3, 280-83) );
	// scrolly.set_show_scroll_x(false);
	add_komponente(&scrolly);

	setze_fenstergroesse( koord(BUTTON_WIDTH*3, 280) );
}




/**
 * Mausklicks werden hiermit an die GUI-Komponenten
 * gemeldet
 */
void
fahrplan_gui_t::infowin_event(const event_t *ev)
{
	if (IS_LEFTCLICK(ev) &&  !line_selector.getroffen(ev->cx, ev->cy-16)  &&  scrolly.getroffen(ev->cx+12, ev->cy)) {

		// close combo box; we must do it ourselves, since the box does not recieve outside events ...
		line_selector.close_box();

		if(ev->my>=40+16) {
			// we are now in the multiline region ...
			const int line = ((ev->my - (39 - scrolly.get_scroll_y()))/LINESPACE)-3;

			if(line >= 0 && line < fpl->maxi()) {
				fpl->aktuell = line;
				if(mode == removing) {
					fpl->remove();
				}
			}
		}
	}
	else if(ev->ev_class == INFOWIN && ev->ev_code == WIN_CLOSE) {

		fpl->cleanup();
		fpl->eingabe_abschliessen();
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN, NO_SOUND, NO_SOUND);
		if (cnv.is_bound()) {
			// if a line is selected
			if (new_line.is_bound()) {
				// if the selected line is different to the convoi's line, apply it
				if(new_line!=cnv->get_line()) {
					cnv->set_line(new_line);
				}
			}
			else {
				// no line is selected, so unset the line
				cnv->unset_line();
			}
		}
	}
	gui_frame_t::infowin_event(ev);
}


bool
fahrplan_gui_t::action_triggered(gui_komponente_t *komp,value_t /* */)
{
DBG_MESSAGE("fahrplan_gui_t::action_triggered()","komp=%p combo=%p",komp,&line_selector);

  if(komp == &bt_add) {
    if(mode != adding) {
      mode = adding;
	bt_add.pressed = true;
	bt_insert.pressed = false;
	bt_remove.pressed = false;
      welt->setze_maus_funktion(wkz_fahrplan_add,
				skinverwaltung_t::fahrplanzeiger->gib_bild_nr(0),
				welt->Z_PLAN,
				(value_t)fpl,
				NO_SOUND,
				NO_SOUND);
    }

  } else if(komp == &bt_insert) {
    if(mode != inserting) {
      mode = inserting;
		bt_add.pressed = false;
		bt_insert.pressed = true;
		bt_remove.pressed = false;
      welt->setze_maus_funktion(wkz_fahrplan_ins,
				skinverwaltung_t::fahrplanzeiger->gib_bild_nr(0),
				welt->Z_PLAN,
				(value_t)fpl,
				NO_SOUND,
				NO_SOUND);
    }

  } else if(komp == &bt_remove) {
    if(mode != removing) {
      mode = removing;
		bt_add.pressed = false;
		bt_insert.pressed = false;
		bt_remove.pressed = true;
      welt->setze_maus_funktion(wkz_abfrage,
				skinverwaltung_t::fragezeiger->gib_bild_nr(0),
				welt->Z_PLAN,
				NO_SOUND,
				NO_SOUND);
    } else {
      mode = none;
    }
  } else if(komp == &bt_prev) {
    if(fpl->maxi() > 0) {
      char & load = fpl->eintrag.at(fpl->aktuell).ladegrad;
      uint8 index=0;
      while(load>ladegrade[index]) {
      	index ++;
      }
      load = ladegrade[(index+MAX_LADEGRADE-1)%MAX_LADEGRADE];
    }
  } else if(komp == &bt_next) {
    if(fpl->maxi() > 0) {
      char & load = fpl->eintrag.at(fpl->aktuell).ladegrad;
      uint8 index=0;
      while(load>ladegrade[index]) {
      	index ++;
      }
      load = ladegrade[(index+1)%MAX_LADEGRADE];
    }
  } else if (komp == &bt_return) {
    fpl->add_return_way(welt);
	} else if (komp == &line_selector) {
		int selection = line_selector.get_selection();
DBG_MESSAGE("fahrplan_gui_t::action_triggered()","line selection=%i",selection);
		if (selection>0) {
			new_line = lines.at(selection-1);
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
//		create_win(-1, -1, 120, new nachrichtenfenster_t(welt, translator::translate("New line created!\nYou can assign the line now\nby selecting it from the\nline selector above.")), w_autodelete);
	}
	return true;
}


void
fahrplan_gui_t::zeichnen(koord pos, koord groesse)
{
	/*
	if (mode == adding) {
		bt_add.pressed = true;
		bt_insert.pressed = false;
		bt_remove.pressed = false;
	} else if (mode == inserting) {
		bt_add.pressed = false;
		bt_insert.pressed = true;
		bt_remove.pressed = false;
	} else if (mode == removing) {
		bt_add.pressed = false;
		bt_insert.pressed = false;
		bt_remove.pressed = true;
	}
*/
	get_fpl_text(buf);
	fpl_text.setze_text(buf);
//	scrolly.setze_groesse(scrolly.gib_groesse());

	gui_frame_t::zeichnen(pos, groesse);

	if(fpl) {
		char tmp[128];
		if(fpl->maxi() <= 0) {
			sprintf(tmp, "%3d%%\n", 0);
		}
		else {
			unsigned current = max( 0, min(fpl->aktuell,fpl->maxi()-1 ) );
			sprintf(tmp, "%3d%%\n", fpl->eintrag.get(current).ladegrad);
		}
		display_multiline_text(pos.x+105, pos.y+40, tmp, COL_BLACK);
	}


	if (line_selector.is_visible()) {
		// unequal to line => remove from line ...
		if(new_line.is_bound()  &&   !fpl->matches(new_line->get_fahrplan())) {
			new_line = linehandle_t();
			line_selector.setze_text(no_line, 128);
		}
		line_selector.zeichnen(pos+koord(0,16));
	}
}

void
fahrplan_gui_t::get_fpl_text(cbuffer_t & buf)
{
	if(fpl) {
		buf.clear();
		for(int i=0; i<fpl->maxi(); i++) {
			buf.append(i==fpl->aktuell ? "> " : "   ");
			buf.append(i+1);
			buf.append(".) ");
			gimme_stop_name(buf, welt, fpl, i, 240);
			buf.append("\n");
		}
		buf.append("\n\n");
	}
}

void fahrplan_gui_t::init_line_selector()
{
	line_selector.clear_elements();
	line_selector.append_element(no_line);
	int selection = -1;
	if (sp->simlinemgmt.count_lines() > 0) {
		sp->simlinemgmt.build_line_list(fpl->get_type(welt), &lines);
		slist_iterator_tpl<linehandle_t> iter( lines );
		while( iter.next() ) {
			linehandle_t line = iter.get_current();
			line_selector.append_element( line->get_name() );
			if(new_line==line) {
				selection = line_selector.count_elements()-1;
			}
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
