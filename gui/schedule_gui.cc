/*
 * schedule_gui.cc
 *
 * Window for editing a schedule
 * by Niels Roest
 *
 * Largely based on fahrplan_gui_t
 * by Hajo Malthaner
 *
 * December 2000
 */

#include "../simcolor.h"
#include "schedule_gui.h"
#include "scrolled_list.h"

#include "../simplay.h"
#include "../simevent.h"
#include "../simgraph.h"
#include "../simdisplay.h"

#include "../dataobj/fahrplan.h"
#include "../dataobj/translator.h"


static int gib_instance() { static int instances = 1; return instances++; }

schedule_gui_t::schedule_gui_t(karte_t *welt, spieler_t *sp)
: infowin_t(welt), buttons(6)
{
  button_t button_def;

  this->welt = welt;
  this->sp = sp;
  groesse = koord(280,260);

  fpl = new fahrplan_t();


  sprintf(fpl_name, "%s %u\n",
	  translator::translate("FPL_Route"), gib_instance());

  // init scrolled lists
  scl1 = new scrolled_list_gui_t(scrolled_list_gui_t::select);
  scl1->setze_groesse(koord(258,44));
  scl1->setze_pos(koord(11,94));
  scl1->setze_highlight_color(sp->kennfarbe+1);

  scl2 = new scrolled_list_gui_t(scrolled_list_gui_t::select);
  scl2->setze_groesse(koord(258,44));
  scl2->setze_pos(koord(11,189));
  scl2->setze_highlight_color(sp->kennfarbe+1);


    // resize button
    button_def.setze_pos(groesse - koord(16,16));
    button_def.setze_typ(button_t::box);
    buttons.append(button_def);

    // normal buttons rename insert append remove view
    button_def.setze_groesse(koord(80,14));
    button_def.setze_pos(koord(11,52));
    button_def.text = translator::translate("bESG Rename");
    button_def.setze_typ(button_t::roundbox);
    buttons.append(button_def);

    button_def.setze_groesse(koord(72,14));
    button_def.setze_pos(koord(11,143));
    button_def.text = translator::translate("bESG Insert");
    buttons.append(button_def);

    button_def.setze_groesse(koord(72,14));
    button_def.setze_pos(koord(88,143));
    button_def.text = translator::translate("bESG Append");
    buttons.append(button_def);

    button_def.setze_groesse(koord(72,14));
    button_def.setze_pos(koord(165,143));
    button_def.text = translator::translate("bESG Remove");
    buttons.append(button_def);

    button_def.setze_groesse(koord(80,14));
    button_def.setze_pos(koord(11,237));
    button_def.text = translator::translate("bESG View");
    buttons.append(button_def);

    gib_fensterbuttons();
    request_vertical_resize(100);
}


void schedule_gui_t::button_append_pressed()
{
  scl1->append_element("Test test");
}


void schedule_gui_t::button_view_pressed()
{
  scl2->append_element("Test2 test2");
}


/*
 * little reminder of fahrplan interface:
 *
 * fpl->maxi       is highest index of stops
 * fpl->aktuell    current index being edited
 * fpl->eintrag[i] accesses index i
 */

int schedule_gui_t::request_vertical_resize(int delta)
{
  // algorithm: try to enlarge both with halve delta.
  // if one succeeds, and one fails, give delta to succeeding.
  const koord old1 = scl1->gib_groesse();
  const koord old2 = scl2->gib_groesse();
  int delta1, delta2;
  if (old2.x % 1) { delta1 = delta/2; delta2 = delta - delta1; }
  else            { delta2 = delta/2; delta1 = delta - delta2; }
  koord neu1 = old1 + koord(0,delta1);
  koord neu2 = old2 + koord(0,delta2);
  neu1 = scl1->request_groesse(neu1);
  neu2 = scl2->request_groesse(neu2);
  int delta1r = neu1.y - old1.y;
  int delta2r = neu2.y - old2.y;
#if 0
  int deltar = delta1r + delta2r;

  if (delta != deltar) { // delta not reached; maybe some slack in 1 or 2
    if (delta1r == delta1) { // maybe slack in 1
      neu1 += koord(0,deltar - delta);
      neu1 = scl1->setze_groesse(neu1);
      delta1r = neu2.y - old2.y;
    } else if (delta2r == delta2) { // maybe slack in 2
      neu2 += koord(0,deltar - delta);
      neu2 = scl2->setze_groesse(neu2);
      delta2r = neu2.y - old2.y;
    }
  }
#endif

  delta = delta1r + delta2r;

  // adapt all variables
  groesse += koord(0, delta);
  buttons.at(0).pos.y += delta; // resize
  buttons.at(2).pos.y += delta1r; // insert
  buttons.at(3).pos.y += delta1r; // append
  buttons.at(4).pos.y += delta1r;
  scl2->setze_pos(scl2->gib_pos() + koord(0,delta1r));
  buttons.at(5).pos.y += delta; // view

  return delta;
}

/**
 * @return gibt wunschgroesse für das beobachtungsfenster zurueck
 */
koord
schedule_gui_t::gib_fenstergroesse() const
{
    return groesse;
}

vector_tpl<button_t> *
schedule_gui_t::gib_fensterbuttons()
{
    buttons.at(0).kennfarbe = gib_besitzer()->kennfarbe;
    buttons.at(1).text = translator::translate("bESG Rename");
    buttons.at(2).text = translator::translate("bESG Insert");
    buttons.at(3).text = translator::translate("bESG Append");
    buttons.at(4).text = translator::translate("bESG Remove");
    buttons.at(5).text = translator::translate("bESG View");

    return &buttons;
}

int schedule_gui_t::gib_bild() const
{ return 243; }
koord schedule_gui_t::gib_bild_offset() const
{ return koord(-10,0); }

void schedule_gui_t::infowin_event(const event_t *ev)
{
  infowin_t::infowin_event(ev);

  int x = ev->cx;
  int y = ev->cy;

  // 0 = resize. 1 = rename. 2 = insert. 3 = append. 4 = remove. 5 = view.
  int i;
  if(IS_LEFTCLICK(ev)) {
    for (i=0;i<6;i++) {
      if (buttons.at(i).getroffen(x, y)) {
	buttons.at(i).pressed = true;
	if (i==1) { /* button__rename_pressed(); */ }
	if (i==2) { /* button__insert_pressed(); */}
	if (i==3) { button_append_pressed(); }
	if (i==4) { /* button_remove_pressed(); */ }
	if (i==5) { button_view_pressed(); }
	// etc
      }
    }

  } else if(IS_LEFTRELEASE(ev)) {
    for (i=0;i<6;i++) {
      if(buttons.at(i).getroffen(x, y)) {
	buttons.at(i).pressed = false;
      }
    }
  } else if(IS_LEFTDRAG(ev)) {
    if(buttons.at(0).getroffen(x, y)) { // resize
      int delta = ev->my - y;

      delta = request_vertical_resize(delta);
      change_drag_start(0, delta);
    }
  }

    if (scl1->getroffen(x, y)) {
	event_t ev2 = *ev;
	translate_event(&ev2, -scl1->gib_pos().x, -scl1->gib_pos().y);
	scl1->infowin_event(&ev2);
    } else if (scl2->getroffen(x, y)) {
	event_t ev2 = *ev;
	translate_event(&ev2, -scl2->gib_pos().x, -scl2->gib_pos().y);
	scl2->infowin_event(&ev2);
    }
}

void schedule_gui_t::zeichnen(koord pos, koord gr)
{
  const int x = pos.x;
        int y = pos.y;

  infowin_t::zeichnen(pos, gr);

  display_proportional(x+11, y+25, translator::translate("tESG Schedule:"),
		       ALIGN_LEFT, BLACK, false);
  display_proportional(x+11, y+37, gib_fpl_name(),
		       ALIGN_LEFT, WHITE, false);
  display_divider(x+11,y+74, 256);
  // (rename button)
  display_proportional(x+11, y+81, translator::translate("tESG Schedule:"),
		       ALIGN_LEFT, BLACK, false);
  scl1->zeichnen(pos);

  y += scl1->gib_groesse().y;

  // (insert, append, remove button)
  display_divider(x+11,y+129,256);
  display_proportional(x+11, y+139, translator::translate("tESG Vehicles:"),
		       ALIGN_LEFT, BLACK, false);
  scl2->zeichnen(pos);
  // (view button)

}
