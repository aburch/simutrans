/*
 * simfab.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef simfab_h
#define simfab_h

#include "simware.h"
#include "dataobj/koord3d.h"
#include "dataobj/translator.h"

#include "tpl/slist_tpl.h"
#include "tpl/vector_tpl.h"
#include "tpl/array_tpl.h"
#include "besch/fabrik_besch.h"
#include "halthandle_t.h"

// Fabrik

class haltestelle_t;
class karte_t;
class spieler_t;
class stadt_t;
class stringhashtable_t;


// production happens in every second
#define PRODUCTION_DELTA_T (1024)
// error of shifting
#define BASEPRODSHIFT (8)
// base production=1 is 16 => shift 4
#define MAX_PRODBASE_SHIFT (4)


// up to this distance, factories will be connected to their towns ...
#define CONNECT_TO_TOWN_SQUARE_DISTANCE 5000

/**
 * Eine Klasse für Fabriken in Simutrans. Fabriken produzieren und
 * verbrauchen Waren und beliefern nahe Haltestellen.
 *
 * Die Abfragefunktionen liefern -1 wenn eine Ware niemals
 * hergestellt oder verbraucht wird, 0 wenn gerade nichts
 * hergestellt oder verbraucht wird und > 0 sonst
 * (entspricht Vorrat/Verbrauch).
 *
 * @date 1998
 * @see haltestelle_t
 * @author Hj. Malthaner
 * @version $Revision: 1.21 $
 */
class fabrik_t
{
public:

    /**
     * Konstanten
     * @author Hj. Malthaner
     */
    enum { precision_bits = 6, old_precision_bits = 10 };

private:

    /**
     * Die möglichen Lieferziele
     * @author Hj. Malthaner
     */
    vector_tpl <koord> lieferziele;
    uint32 last_lieferziel_start;

    /**
     * suppliers to this factry
     * @author hsiegeln
     */
    vector_tpl <koord> suppliers;


    /**
     * Die erzeugten waren auf die Haltestellen verteilen
     * @author Hj. Malthaner
     */
    void verteile_waren(const uint32 produkt);

    /* still needed for the info dialog; otherwise useless
     */
    slist_tpl<stadt_t *> arbeiterziele;

    spieler_t *besitzer_p;
    karte_t *welt;

    const fabrik_besch_t *besch;

    /**
     * Bauposition gedreht?
     * @author V.Meyer
     */
    unsigned char rotate;

    /**
     * produktionsgrundmenge
     * @author Hj. Malthaner
     */
    int prodbase;


    /**
     * multiplikator für die Produktionsgrundmenge
     * @author Hj. Malthaner
     */
    int prodfaktor;


    /**
     * pointer auf das einganslagerfeld
     * @author Hj. Malthaner
     */
    vector_tpl<ware_t> *eingang;


    /**
     * pointer auf das ausgangslagerfeld
     * @author Hj. Malthaner
     */
    vector_tpl<ware_t> *ausgang;


    /**
     * bisherige abgabe in diesem monat pro ware
     * @author Hj. Malthaner
     */
    array_tpl<int> * abgabe_sum;


    /**
     * abgabe im letzten monat pro ware
     * @author Hj. Malthaner
     */
    array_tpl<int> * abgabe_letzt;


    /**
     * Zeitakkumulator für Produktion
     * @author Hj. Malthaner
     */
    long delta_sum;

   unsigned long total_input, total_output;
   unsigned status;

   void recalc_factory_status();
public:
  static fabrik_t * gib_fab(const karte_t *welt, const koord pos);

    /**
     * @return vehicle description object
     * @author Hj. Malthaner
     */
    const fabrik_besch_t *gib_besch() const {return besch; }

  /**
   * @author hsiegeln
   */
   void laden_abschliessen();

    /* check, if the coordinate belongs to this factory
     * @author prissi
     */
    bool is_fabrik( koord check );

    /* check, if the coordinate belongs to this factory
     * @author prissi
     */
    bool is_fabrik( koord check, koord extent );

    /**
     * Die Koordinate (Position) der fabrik
     * @author Hj. Malthaner
     */
    koord3d pos;


    void link_halt(halthandle_t halt);
    void unlink_halt(halthandle_t halt);


    const vector_tpl <koord> & gib_lieferziele() const {return lieferziele;};
    const vector_tpl <koord> & get_suppliers() const {return suppliers;};

    /* workers origin only used for info dialog purposes; otherwise useless ...
     * @author Hj. Malthaner/prissi
     */
    void  add_arbeiterziel(stadt_t *stadt);
    void  rem_arbeiterziel(stadt_t *stadt);
    const slist_tpl <stadt_t *> & gib_arbeiterziele() const {return arbeiterziele;};

//    const slist_tpl<halthandle_t> gib_halt_list() { return halt_list; };

    /**
     * Fügt ein neues Lieferziel hinzu
     * @author Hj. Malthaner
     */
    void  add_lieferziel(koord ziel);
    void  rem_lieferziel(koord pos);

    /**
     * adds a supplier
     * @author Hj. Malthaner
     */
    void  add_supplier(koord pos);
    void  rem_supplier(koord pos);


    /**
     * Used while loading a saved game
     * @author Hj. Malthaner
     */
    fabrik_t(karte_t *welt, loadsave_t *file);


    fabrik_t(karte_t *welt, koord3d pos, spieler_t *sp, const fabrik_besch_t *fabesch);

    ~fabrik_t();

    /**
     * @return menge der ware typ
     *   -1 wenn typ nicht produziert wird
     *   sonst die gelagerte menge
     */
    int vorrat_an(const ware_besch_t *ware);        // Vorrat von Warentyp

    /**
     * @return 1 wenn verbrauch,
     * 0 wenn Produktionsstopp,
     * -1 wenn Ware nicht verarbeitet wird
     */
    int verbraucht(const ware_besch_t *);             // Nimmt fab das an ??
    int hole_ab(const ware_besch_t *, int menge );     // jemand will waren abholen
    int liefere_an(const ware_besch_t *, int menge);

    int gib_abgabe_letzt(int t) {return abgabe_letzt->at(t);};

    void step(long delta_t);                  // fabrik muss auch arbeiten
    void neuer_monat();

    const char *gib_name() const { return besch ? translator::translate(besch->gib_name()) : "unnamed"; }
    int gib_kennfarbe() const { return besch ? besch->gib_kennfarbe() : 0; }

    void info(cbuffer_t & buf);

    void rdwr(loadsave_t *file);

    inline koord3d gib_pos() const {return pos;};


   /**
     * Die Menge an Produzierter Ware je Zeitspanne PRODUCTION_DELTA_T
     *
     * @author Hj. Malthaner
     */
    uint32 produktion(uint32 produkt) const;


    /**
     * Die maximal produzierbare Menge je Monat
     *
     * @author Hj. Malthaner
     */
    int max_produktion() const;


    /**
     * gibt eine NULL-Terminierte Liste von Fabrikpointern zurück
     *
     * @author Hj. Malthaner
     */
    static vector_tpl<fabrik_t *> & sind_da_welche(karte_t *welt, koord min, koord max);


    /**
     * gibt true zurueck wenn sich ein fabrik im feld befindet
     *
     * @author Hj. Malthaner
     */
    static bool ist_da_eine(karte_t *welt, koord min, koord max);
    static bool ist_bauplatz(karte_t *welt, koord pos, koord groesse = koord(2, 2),bool water=false);


    // hier die methoden zum parametrisieren der Fabrik


    /**
     * Baut die Gebäude für die Fabrik
     *
     * @author Hj. Malthaner, V. Meyer
     */
    void baue(int rotate, bool clear);


    /**
     * setzt die Eingangswarentypen
     *
     * @author Hj. Malthaner
     */
    void set_eingang(vector_tpl<ware_t> * typen);
    const vector_tpl<ware_t> * gib_eingang() const {return eingang;};

    /**
     * setzt die Ausgangsswarentypen
     *
     * @author Hj. Malthaner
     */
    void set_ausgang(vector_tpl<ware_t> * typen);
    const vector_tpl<ware_t> * gib_ausgang() const {return ausgang;};


    /**
     * Produktionsgrundmenge
     * @author Hj. Malthaner
     */
    void set_prodbase(int i) {prodbase = i;};
  int get_prodbase(void) {return prodbase;};

    /**
     * Produktionsmultiplikator
     * @author Hj. Malthaner
     */
  void set_prodfaktor(int i) {prodfaktor= (i<16)?16:i;};
  int get_prodfaktor(void) const {return prodfaktor;};

   /* prissi: returns the status of the current factory, as well as output */
   enum { bad, medium, good, inactive, nothing };
   static unsigned status_to_color[5];
   unsigned calc_factory_status(unsigned long *input, unsigned long *output) const { if(input) {*input=total_input;} if(output) { *output=total_output; }; return status; };

    /**
     * Crossconnects all factories
     * @author Hj. Malthaner
     */
   void add_all_suppliers();
};

#endif
