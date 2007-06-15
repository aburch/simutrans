/*
 * dialog zur Eingabe eines Fahrplanes
 *
 * Hj. Malthaner
 *
 * Juli 2000
 */

#ifndef gui_fahrplan_gui_h
#define gui_fahrplan_gui_h

#include "../utils/cbuffer_t.h"
#include "components/gui_label.h"
#include "gui_frame.h"
#include "components/gui_combobox.h"
#include "components/gui_button.h"
#include "components/action_listener.h"

#include "components/gui_textarea.h"
#include "components/gui_scrollpane.h"

#include "../convoihandle_t.h"
#include "../linehandle_t.h"


class fahrplan_t;
class spieler_t;


/**
 * GUI fuer Fahrplaene
 *
 * @author Hj. Malthaner
 */
class fahrplan_gui_t :	public gui_frame_t,
						public action_listener_t
{
 public:

  /**
   * Fuellt buf mit Beschreibung des i-ten Eintrages des Fahrplanes
   *
   * @author Hj. Malthaner
   */
  static void gimme_stop_name(cbuffer_t & buf,
			      karte_t *welt,
			      const fahrplan_t *fpl,
			      int i,
			      int max_chars);

	/**
	 * Fuellt buf mit Beschreibung des i-ten Eintrages des Fahrplanes
	 * short version, without loading level and position ...
	 * @author Hj. Malthaner
	 */
	static void gimme_short_stop_name(cbuffer_t & buf,
				     karte_t *welt,
				     const fahrplan_t *fpl,
				     int i,
				     int max_chars);

private:
	static char no_line[128];

    enum mode_t {adding, inserting, removing, none};

	vector_tpl<linehandle_t> lines;

    mode_t mode;

    button_t bt_add;
    button_t bt_insert;
    button_t bt_remove;
    button_t bt_prev;
    button_t bt_next;
    button_t bt_promote_to_line;
    button_t bt_return;

    gui_scrollpane_t scrolly;
    gui_textarea_t fpl_text;
    gui_combobox_t line_selector;
    gui_label_t lb_line;
    gui_label_t lb_load;

		fahrplan_t* fpl;
    spieler_t *sp;
    convoihandle_t cnv;

    cbuffer_t buf;

    linehandle_t new_line;

    void init_line_selector();

	/**
	 * initialize layout, etc.
	 * @author hsiegeln
	 */
	void init();

public:
    fahrplan_gui_t(fahrplan_t* fpl, spieler_t* sp);
    fahrplan_gui_t(convoihandle_t cnv, spieler_t* sp);

    /**
     * Mausklicks werden hiermit an die GUI-Komponenten
     * gemeldet
     */
    void infowin_event(const event_t *ev);

    const char * gib_hilfe_datei() const {return "schedule.txt";}

    /**
     * Zeichnet das Frame
     * @author Hansjörg Malthaner
     */
    void zeichnen(koord pos, koord gr);

    /**
     * show or hide the line selector combobox and its associated label
     * @author hsiegeln
     */
    void show_line_selector(bool yesno) {
    	line_selector.set_visible(yesno);
    	lb_line.set_visible(yesno);
    }

	void get_fpl_text(cbuffer_t & buf);

    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     *
     * Returns true, if action is done and no more
     * components should be triggered.
     * V.Meyer
     */
    bool action_triggered(gui_komponente_t *komp, value_t extra);
};

#endif
