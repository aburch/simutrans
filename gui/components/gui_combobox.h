/*
 * gui_textinput.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_components_gui_combobox_h
#define gui_components_gui_combobox_h

#include "../../ifc/gui_komponente.h"
#include "../../tpl/slist_tpl.h"
#include "../ifc/action_listener.h"
#include "../scrolled_list.h"
#include "gui_textinput.h"

struct event_t;

/**
 * Ein einfaches Texteingabefeld. Es hat keinen eigenen Textpuffer,
 * nur einen Zeiger auf den Textpuffer, der von jemand anderem Bereitgestellt
 * werden muss.
 *
 * @date 19-Apr-01
 * @author Hj. Malthaner
 */

class gui_combobox_t : public gui_textinput_t
{
private:

    /**
     * the drop box list
     * @author hsiegeln
     */
    scrolled_list_gui_t * droplist;

    /**
     * the max size this component can have
     * @author hsiegeln
     */
    koord max_size;

    /**
     * the selection in the droplist
     * @author hsiegeln
     */
    int selection;

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
     * Zeichnet die Komponente
     * @author Hj. Malthaner
     */
    void zeichnen(koord offset) const;

	/**
	 * called when the focus should be released
	 * does some cleanup before releasing
	 * overrides function in base class
	 * @author hsiegeln
	 */
	bool release_focus();

	/**
	 * add element to droplist
	 * @author hsiegeln
	 */
	void append_element(char * text) { droplist->append_element( text ); };

	/**
	 * remove all elements from droplist
	 * @author hsiegeln
	 */
	void clear_elements() { droplist->clear_elements(); };

	/**
	 * sets the highlight color for the droplist
	 * @author hsiegeln
	 */
	void set_highlight_color(int color) { droplist->setze_highlight_color(color); };

	/**
	 * set maximum size for control
	 * @author hsiegeln
	 */
	 void set_max_size(koord max) { max_size = max; };

	 /**
	  * returns the selection id
	  * @author hsiegeln
	  */
	  int get_selection() { return droplist->gib_selection(); };

	  /**
	   * sets the selection
	   * @author hsiegeln
	   */
	  void set_selection(int s)
	  {
		selection = s;
		droplist->setze_selection( selection );
	  };

};

#endif
