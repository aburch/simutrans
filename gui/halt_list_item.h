/*
 * gui_convoiinfo.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 * Written (w) 2001 Markus Weber
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef halt_list_item_h
#define halt_list_item_h

#include "../dataobj/koord.h"
#include "../ifc/gui_komponente.h"
#include "../simhalt.h"
#include "../halthandle_t.h"


struct event_t;


/**
 * Komponenten von Fenstern sollten von dieser Klassse abgeleitet werden.
 *
 * @autor Hj. Malthaner
 */
class halt_list_item_t : public gui_komponente_t
{
private:
    /**
     * Handle des anzuzeigenden Convois.
     * @author Hj. Malthaner
     */
    halthandle_t halt;


    /**
     * Nummer des anzuzeigenden Convois.
     * @author Hj. Malthaner
     */
    int nummer;

public:

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
    };


    /**
     * Konstruktor.
     * @param cnv das Handle für den anzuzeigenden Convoi.
     * @author Hj. Malthaner
     */
    halt_list_item_t::halt_list_item_t(halthandle_t halt, int n) {
	this->halt = halt;
	nummer = n;
    };


    /**
     * Virtueller Destruktor, damit Klassen sauber abgeleitet werden können
     * @author Hj. Malthaner
     */
    virtual ~halt_list_item_t() {};


    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author Hj. Malthaner
     */
    virtual void infowin_event(const event_t *);


    /**
     * Zeichnet die Komponente
     * @author Hj. Malthaner
     */
    void zeichnen(koord offset) const;
};

#endif
