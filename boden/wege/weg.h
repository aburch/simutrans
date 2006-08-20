/*
 * weg.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef boden_wege_weg_h
#define boden_wege_weg_h

#ifndef koord3d_h
#include "../../dataobj/koord3d.h"
#endif

class vehikel_basis_t;
class karte_t;
class weg_besch_t;
class cbuffer_t;

template <class T> class slist_tpl;


// maximum number of months to store information
#define MAX_WAY_STAT_MONTHS 2

// number of different statistics collected
#define MAX_WAY_STATISTICS 2

// number of goods transported over this weg
#define WAY_STAT_GOODS 0

// number of convois that passed this weg
#define WAY_STAT_CONVOIS 1



/**
 * <p>Der Weg ist die Basisklasse fuer alle Verkehrswege in Simutrans.
 * Wege "gehören" immer zu einem Grund. Sie besitzen Richtungsbits sowie
 * eine Maske fuer Richtungsbits.</p>
 *
 * <p>Ein Weg gehört immer zu genau einer Wegsorte</p>
 *
 * <p>Kreuzungen werden dadurch unterstützt, daß ein Grund zwei Wege
 * enthalten kann (prinzipiell auch mehrere möglich.</p>
 *
 * <p>Wegtyp -1 ist reserviert und kann nicht für Wege benutzt werden<p>
 *
 * @author Hj. Malthaner
 */
class weg_t
{
public:
    /**
     * Vordefinierte Wegtypen.
     * @author Hj. Malthaner
     */
    enum typ {invalid=-1, strasse=1, schiene=2, wasser=3, tramschiene=4, powerline=128};


    /**
     * Get list of all ways
     * @author Hj. Malthaner
     */
    static const slist_tpl <weg_t *> & gib_alle_wege();


private:

    /**
     * array for statistical values
     * MAX_WAY_STAT_MONTHS: [0] = actual value; [1] = last month value
     * MAX_WAY_STATISTICS: see #define at top of file
     * @author hsiegeln
     */
    unsigned int statistics[MAX_WAY_STAT_MONTHS][MAX_WAY_STATISTICS];


    /**
     * Way type description
     * @author Hj. Malthaner
     */
    const weg_besch_t * besch;


    /**
     * Position on map
     * @author Hj. Malthaner
     */
    koord3d pos;


    /**
     * Richtungsbits für den Weg. Norden ist oben rechts auf dem Monitor.
     * 1=Nord, 2=Ost, 4=Sued, 8=West
     * @author Hj. Malthaner
     */
    ribi_t::ribi ribi;


    /**
     * Maske für Richtungsbits
     * @author Hj. Malthaner
     */
    ribi_t::ribi ribi_maske;


    /**
     * Erlaubte Höchstgeschwindigkeit
     * @author Hj. Malthaner
     */
    unsigned short max_speed;


    /**
     * Initializes all member variables
     * @author Hj. Malthaner
     */
    void init(karte_t *);


    /**
     * initializes statistic array
     * @author hsiegeln
     */
    void init_statistics();


protected:

    /**
     * Pointer to the world of this way. Static to conserve space.
     * Change to instance variable once more than one world is available.
     * @author Hj. Malthaner
     */
    static karte_t *welt;


    int calc_bild(koord3d pos, const weg_besch_t *besch) const;
public:


    /**
     * Get position on map
     * @author Hj. Malthaner
     */
    koord3d gib_pos() const {return pos;};


    /**
     * Set position on map
     * @author Hj. Malthaner
     */
    void setze_pos(koord3d p) {pos = p;};


    /**
     * Setzt die erlaubte Höchstgeschwindigkeit
     * @author Hj. Malthaner
     */
    void setze_max_speed(unsigned int s);


    /**
     * Ermittelt die erlaubte Höchstgeschwindigkeit
     * @author Hj. Malthaner
     */
    int gib_max_speed() const {return max_speed;};


    /**
     * Setzt neue Beschreibung. Ersetzt alte Höchstgeschwindigkeit
     * mit wert aus Beschreibung.
     * @author Hj. Malthaner
     */
    void setze_besch(const weg_besch_t *b);
    const weg_besch_t * gib_besch() const {return besch;};


    weg_t(karte_t *welt, loadsave_t *file);
    weg_t(karte_t *welt);

    virtual ~weg_t();

    virtual void rdwr(loadsave_t *file);


    /**
     * Info-text für diesen Weg
     * @author Hj. Malthaner
     */
    virtual void info(cbuffer_t & buf) const;


    /**
     * Wegtyp zurückliefern
     */
    virtual typ gib_typ() const = 0;


    /**
     * der typ_name (Bezeichnung) des Wegs
     */
    virtual const char *gib_typ_name() const {return "Weg"; }


    /**
     * Die Bezeichnung des Wegs
     * @author Hj. Malthaner
     */
    virtual const char *gib_name() const;


    /**
     */
    virtual bool ist_frei() const {return true;};


    /**
     * Liefert das benötgte Bild für den Weg.
     * Funktioniert nicht bei Wegtypkreuzungen!
     */
    virtual int calc_bild(koord3d /*pos*/) const { return -1; };


    /**
     * Setzt neue Richtungsbits für einen Weg.
     *
     * Nachdem die ribis geändert werden, ist das weg_bild des
     * zugehörigen Grundes falsch (Ein Aufruf von grund_t::calc_bild()
     * zur Reparatur muß folgen).
     * @param ribi Richtungsbits
     */
    void ribi_add(ribi_t::ribi ribi) { this->ribi |= ribi;};


    /**
     * Entfernt Richtungsbits von einem Weg.
     *
     * Nachdem die ribis geändert werden, ist das weg_bild des
     * zugehörigen Grundes falsch (Ein Aufruf von grund_t::calc_bild()
     * zur Reparatur muß folgen).
     * @param ribi Richtungsbits
     */
    void ribi_rem(ribi_t::ribi ribi) { this->ribi &= ~ribi;};


    /**
     * Setzt Richtungsbits für den Weg.
     *
     * Nachdem die ribis geändert werden, ist das weg_bild des
     * zugehörigen Grundes falsch (Ein Aufruf von grund_t::calc_bild()
     * zur Reparatur muß folgen).
     * @param ribi Richtungsbits
     */
    void setze_ribi(ribi_t::ribi ribi) { this->ribi = ribi;};


    /**
     * Ermittelt die unmaskierten Richtungsbits für den Weg.
     */
    ribi_t::ribi gib_ribi_unmasked() const { return ribi; };


    /**
     * Ermittelt die (maskierten) Richtungsbits für den Weg.
     */
    ribi_t::ribi gib_ribi() const { return ribi & ~ribi_maske; };


    /**
     * für Signale ist es notwendig, bestimmte Richtungsbits auszumaskieren
     * damit Fahrzeuge nicht "von hinten" über Ampeln fahren können.
     * @param ribi Richtungsbits
     */
    void setze_ribi_maske(ribi_t::ribi ribi) { ribi_maske = ribi; }


    /**
     * einige Wege können betreten werden. Manche Wege z.B.
     * schienen und strassen können aktionen ausführen z.b. ein signal
     * stellen wenn sie betreten werden.
     * @author Hj. Malthaner
     */
    virtual void betrete(vehikel_basis_t *) {};


    /**
     * Gegenstueck zu betrete()
     * @author Hj. Malthaner
     */
    virtual void verlasse(vehikel_basis_t *) {};


    /**
     * book statistics - is called very often and therefore inline
     * @author hsiegeln
     */
    void book(int amount, int type) {
      if (type < MAX_WAY_STATISTICS) {
	statistics[0][type] += amount;
      }
    }


    /**
     * return statistics value
     * always returns last month's value
     * @author hsiegeln
     */
    int get_statistics(int type) const { return statistics[1][type]; };


    /**
     * new month
     * @author hsiegeln
     */
    void neuer_monat();
};

#endif
