/*
 * gui_combobox.h
 *
 * with a connected edit field
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_components_gui_combobox_h
#define gui_components_gui_combobox_h

#include "../../simcolor.h"
#include "../../ifc/gui_action_creator.h"
#include "../../tpl/slist_tpl.h"
#include "gui_scrolled_list.h"
#include "gui_textinput.h"
#include "gui_button.h"

struct event_t;

class gui_combobox_t :
	public gui_komponente_action_creator_t,
	public action_listener_t
{
private:
	// buttons for setting selection manually
	gui_textinput_t textinp;
	button_t bt_prev;
	button_t bt_next;

    /**
     * the drop box list
     * @author hsiegeln
     */
    gui_scrolled_list_t * droplist;

	/*
	 * flag for first call
	 */
	bool first_call:1;

	/*
	 * flag for finish selection
	 */
	bool finish:1;

    /**
     * the max size this component can have
     * @author hsiegeln
     */
    koord max_size;

public:

    /**
     * Konstruktor
     *
     * @author Hj. Malthaner
     */
    gui_combobox_t();


    /**
     * Destruktor
     *
     * @author Hj. Malthaner
     */
    ~gui_combobox_t();

      /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author Hj. Malthaner
     */
    void infowin_event(const event_t *);

    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     */
    virtual bool action_triggered(gui_komponente_t *komp,value_t /* */);

    /**
     * Zeichnet die Komponente
     * @author Hj. Malthaner
     */
    void zeichnen(koord offset) const;

	/**
	 * add element to droplist
	 * @author hsiegeln
	 */
	void append_element(char * text,uint8 color=COL_BLACK) { droplist->append_element( text, color ); }

	/**
	 * remove all elements from droplist
	 * @author hsiegeln
	 */
	void clear_elements() { droplist->clear_elements(); }

	/**
	 * remove all elements from droplist
	 * @author hsiegeln
	 */
	int count_elements() { return droplist->get_count(); }

	/**
	 * sets the highlight color for the droplist
	 * @author hsiegeln
	 */
	void set_highlight_color(int color) { droplist->setze_highlight_color(color); }

	/**
	 * set maximum size for control
	 * @author hsiegeln
	 */
	 void set_max_size(koord max) { max_size = max; }

	 /**
	  * returns the selection id
	  * @author hsiegeln
	  */
	  int get_selection() { return droplist->gib_selection(); }

	  /**
	   * sets the selection
	   * @author hsiegeln
	   */
	  void set_selection(int s);

	/**
	* Setzt den Textpuffer
	*
	* @author Hj. Malthaner
	*/
	void setze_text(char *text, int max) {textinp.setze_text(text,max);}


	/**
	* Holt den Textpuffer
	*
	* @author Hj. Malthaner
	*/
	char *gib_text() const {return textinp.gib_text();}


    /**
     * Vorzugsweise sollte diese Methode zum Setzen der Größe benutzt werden,
     * obwohl groesse public ist.
     * @author Hj. Malthaner
     */
    void setze_groesse(koord groesse);

	/**
	 * called when the focus should be released
	 * does some cleanup before releasing
	 * @author hsiegeln
	 */
	void close_box();
};

#endif
