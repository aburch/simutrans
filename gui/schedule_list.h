#ifndef gui_schedule_list_h
#define gui_schedule_list_h

#include "components/gui_resizer.h"                             // 28-Dec-2001  Markus Weber    Added
#include "components/gui_textinput.h"
#include "gui_frame.h"
#include "scrolled_list.h"
#include "gui_container.h"
#include "gui_scrollpane.h"
#include "gui_convoiinfo.h"
#include "../simline.h"
#include "tab_panel.h"

class spieler_t;
class karte_t;
class gui_chart_t;

/**
 * Window.
 * Will display list of schedules.
 * Contains buttons: edit new remove
 * Resizable.
 *
 * @author Niels Roest
 * @author hsiegeln: major redesign
 */
class schedule_list_gui_t : public gui_frame_t, public action_listener_t
{
private:

  karte_t *welt;
  spieler_t *sp;

  koord groesse;
  scrolled_list_gui_t *scl;
  button_t bt_new_line, bt_change_line, bt_delete_line;
  gui_container_t cont, cont_haltestellen;
  gui_scrollpane_t scrolly, scrolly_haltestellen;
  gui_speedbar_t filled_bar;
  gui_textinput_t inp_name;
  gui_chart_t *chart;
  button_t filterButtons[MAX_LINE_COST];
  tab_panel_t tabs;

  static const char cost_type[MAX_LINE_COST][64];
  static const int cost_type_color[MAX_LINE_COST];

  int selection, capacity, load, loadfactor;

  void display(koord pos);

  simline_t * line;

  slist_tpl <simline_t *> lines;

  int SCL_HEIGHT;

  void build_line_list(int filter);

public:

  schedule_list_gui_t(karte_t *welt,spieler_t *sp);
  ~schedule_list_gui_t();

  koord gib_fenstergroesse() const { return groesse; }

    /**
     * in top-level fenstern wird der Name in der Titelzeile dargestellt
     * @return den nicht uebersetzten Namen der Komponente
     * @author Hj. Malthaner
     */
    const char *gib_name() const {return "Line Management";};

  /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    virtual const char * gib_hilfe_datei() const {return "linemanagement.txt";};

    /**
     * gibt den Besitzer zurück
     *
     * @author Hj. Malthaner
     */
	virtual spieler_t* gib_besitzer() { return sp; }

    /**
     * @return Einen Beschreibungsstext für das Objekt, der z.B. in einem
     * Beobachtungsfenster angezeigt wird, NULL wenn kein Fenster angezeigt
     * werden soll
     *
     * @author Hj. Malthaner
     * @see simwin
     */
    char * info(char *buf) const;


    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author Hj. Malthaner
     */
    void infowin_event(const event_t *ev);


    /**
     * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
     * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
     * in dem die Komponente dargestellt wird.
     *
     * @author Hj. Malthaner
     */
    void zeichnen(koord pos, koord gr);

    bool action_triggered(gui_komponente_t *komp);

    /**
     * resize window in response to a resize event
     * @author Hj. Malthaner
     */
    virtual void resize(const koord delta);
};

#endif
