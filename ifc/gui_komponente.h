/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef ifc_gui_komponente_h
#define ifc_gui_komponente_h

#include "../dataobj/koord.h"
#include "../simwin.h"

struct event_t;

/**
 * Komponenten von Fenstern sollten von dieser Klassse abgeleitet werden.
 *
 * @autor Hj. Malthaner
 */
class gui_komponente_t
{
private:
	/**
	* allow component to show/hide itself
	* @author hsiegeln
	*/
	bool visible;

	/**
	* Does this component have the input focus?
	* @author hsiegeln
	*/
	bool focus_gained;

	/**
	* some components might not be allowed to gain focus
	* for example: gui_textarea_t
	* this flag can be set to true to deny focus requesst for a gui_component always
	* @author hsiegeln
	*/
	bool read_only;

public:
	/**
	* Basic contructor, initialises member variables
	* @author Hj. Malthaner
	*/
	gui_komponente_t() {
		visible = true;
		focus_gained = false;
		read_only = true;
	}

	/**
	* Virtueller Destruktor, damit Klassen sauber abgeleitet werden können
	* @author Hj. Malthaner
	*/
	virtual ~gui_komponente_t() {}

	void set_read_only(bool yesno) {
		read_only = yesno;
	}

	bool get_allow_focus() { return !read_only; }

	/**
	* Sets component to be shown/hidden
	* @author Hj. Malthaner
	*/
	void set_visible(bool yesno) {
		visible = yesno;
	}


	/**
	* Checks if component should be displayed
	* @author Hj. Malthaner
	*/
	bool is_visible() const {return visible;}

	/**
	* Position der Komponente. Eintraege sind relativ zu links/oben der
	* umgebenden Komponente.
	* @author Hj. Malthaner
	*/
	koord pos;

	/**
	* Vorzugsweise sollte diese Methode zum Setzen der Position benutzt werden,
	* obwohl pos public ist.
	* @author Hj. Malthaner
	*/
	void setze_pos(koord pos) {
		this->pos = pos;
	}

	/**
	* Vorzugsweise sollte diese Methode zum Abfragen der Position benutzt werden,
	* obwohl pos public ist.
	* @author Hj. Malthaner
	*/
	koord gib_pos() const {
		return pos;
	}

	/**
	* Größe der Komponente.
	* @author Hj. Malthaner
	*/
	koord groesse;

	/**
	* Vorzugsweise sollte diese Methode zum Setzen der Größe benutzt werden,
	* obwohl groesse public ist.
	* @author Hj. Malthaner
	*/
	virtual void setze_groesse(koord groesse) {
		this->groesse = groesse;
	}

	/**
	* Vorzugsweise sollte diese Methode zum Abfragen der Größe benutzt werden,
	* obwohl groesse public ist.
	* @author Hj. Malthaner
	*/
	koord gib_groesse() const {
		return groesse;
	}

	/**
	* Prüft, ob eine Position innerhalb der Komponente liegt.
	* @author Hj. Malthaner
	*/
	virtual bool getroffen(int x, int y)
	{
		return (pos.x <= x && pos.y <= y && (pos.x+groesse.x) >= x && (pos.y+groesse.y) >= y);
	}

	/**
	* Events werden hiermit an die GUI-Komponenten
	* gemeldet
	* @author Hj. Malthaner
	* prissi: default -> do nothing
	*/
	virtual void infowin_event(const event_t *) { }

	/**
	* Zeichnet die Komponente
	* @author Hj. Malthaner
	*/
	virtual void zeichnen(koord offset) = 0;
};

#endif
