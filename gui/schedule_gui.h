/*
 * schedule_gui.h
 *
 * Window for editing a schedule
 * by Niels Roest
 *
 * Largely based on fahrplan_gui_t
 * by Hajo Malthaner
 *
 * December 2000
 */

#ifndef gui_schedule_gui_h
#define gui_schedule_gui_h

#include "infowin.h"
#include "scrolled_list.h"
#include "../tpl/vector_tpl.h"

class fahrplan_t;

class schedule_gui_t : public infowin_t
{
private:

  koord groesse;

  vector_tpl<button_t> buttons;

  karte_t *welt;
  fahrplan_t *fpl;
  spieler_t *sp;

  char fpl_name[60]; // 60 seems a nice number

  scrolled_list_gui_t *scl1, *scl2;

  void button_append_pressed();
  void button_view_pressed();

public:

  schedule_gui_t(karte_t *welt, spieler_t *sp);

  spieler_t* gib_besitzer() const { return sp; }
  const char * gib_name() const { return "nESG Edit Schedule"; }
  const char * gib_fpl_name() const { return fpl_name; }

  // compatibility function
  char *info(char *buf) const { buf[0]=' '; buf[1]=0; return buf; }

  // virtual int gib_bild() const; // could do with a nice image
  // virtual koord gib_bild_offset() const;

  koord gib_fenstergroesse() const;
  int request_vertical_resize(int delta);

  /**
   * @return einen Vector von button_t Objekten
   * @author Hj. Malthaner
   */
  virtual vector_tpl<button_t> *gib_fensterbuttons();

  int gib_bild() const;
  koord gib_bild_offset() const;

  virtual void infowin_event(const event_t *ev);
  void zeichnen(koord pos, koord gr);
};


#endif
