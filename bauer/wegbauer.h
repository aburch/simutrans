/*
 * simbau.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef simbau_h
#define simbau_h

#include "../boden/wege/weg.h"
#include "../tpl/array_tpl.h"
#include "../tpl/array2d_tpl.h"
#include "../tpl/slist_tpl.h"
#include "../simtypes.h"

class weg_besch_t;
class kreuzung_besch_t;
class karte_t;
class spieler_t;
class grund_t;

class werkzeug_parameter_waehler_t;


/**
 * Diese Klasse übernimmt Wegsuche und Bau von Strassen, Schienen etc.
 * @author Hj. Malthaner
 */
class wegbauer_t
{
    static slist_tpl<const kreuzung_besch_t *> kreuzungen;
public:
    static const weg_besch_t *leitung_besch;

    static const kreuzung_besch_t *gib_kreuzung(weg_t::typ ns, weg_t::typ ow);

    static bool register_besch(const weg_besch_t *besch);
    static bool register_besch(const kreuzung_besch_t *besch);
    static bool alle_wege_geladen();
    static bool alle_kreuzungen_geladen();


    /**
     * Finds a way with a given speed limit for a given waytype
     * @author prissi
     */
    static const weg_besch_t *  weg_search(weg_t::typ wtyp,int speed_limit);

    /**
     * Tries to look up description for way, described by way type,
     * system type and construction type
     * @author Hj. Malthaner
     */
    static const weg_besch_t * gib_besch(const char * way_name);


    /**
     * Fill menu with icons of given waytype
     * @author Hj. Malthaner
     */
    static void fill_menu(werkzeug_parameter_waehler_t *wzw,
        weg_t::typ wtyp,
        int (* wz1)(spieler_t *, karte_t *, koord, value_t),
        int sound_ok,
        int sound_ko);

    enum bautyp {
        strasse,
  schiene,
  leitung,
  schiene_bot,
  schiene_bot_bau,
  strasse_bot
    };

    bool kann_ribis_setzen(const grund_t *bd, const koord zv);
    static bool kann_mit_strasse_erreichen(const grund_t *bd, const koord zv);
    static bool ist_bruecke_tunnel_ok(grund_t *bd_von, koord zv, grund_t *bd_nach);

private:

    enum { unseen = 9999999 };
    enum { max_route_laenge = 1024 };


    class info_t
    {
    public:
  int val;
  koord k;
    };

    class dir_force_t
    {
    public:
       koord k;
       koord force_dir;
    };

    // an manchen stellen, z.B. Brücken und tunnelenden darf man nur in
    // einer richtung weitermachen. dir_forces verwaltet diese info

    slist_tpl<dir_force_t *> dir_forces;

    koord forces_lookup(koord k);
    void  forces_update(koord k, koord force_dir);

    array2d_tpl<info_t> *info;

    slist_tpl<info_t *> queue;

    spieler_t *sp;


    /**
     * Type of building operation
     * @author Hj. Malthaner
     */
    enum bautyp bautyp;


    /**
     * Type of way to build
     * @author Hj. Malthaner
     */
    const weg_besch_t * besch;


    /**
     * If a way is built on top of another way, should the type
     * of the former way be kept or replced (true == keep)
     * @author Hj. Malthaner
     */
    bool keep_existing_ways;
    bool keep_existing_faster_ways;


    karte_t *welt;
    int maximum;    // hoechste Suchtiefe

    array_tpl<koord> *route;


    int calc_cost(koord pos);


    koord remove();
    bool  update(koord k, int pri);

    koord calc_bruecke_ziel(const koord pos1, const koord pos2);
    bool check_brueckenbau(const koord k, const koord t,
                           const koord start, const koord ziel);


    koord calc_tunnel_ziel(const koord pos1, const koord pos2);
    bool check_tunnelbau(const koord k, const koord t,
                         const koord start, const koord ziel);

    bool check_step(const koord k, const koord t,
                    const koord start, const koord ziel, int cost,
        koord force_dir);

    void calc_route_init();
    void intern_calc_route(koord start, const koord ziel);

    bool pruefe_route();

    ribi_t::ribi calc_ribi(int step);

    static koord3d bruecke_finde_ende(karte_t *welt, koord3d pos, koord zv);
    static koord3d tunnel_finde_ende(karte_t *welt, koord3d pos, const koord zv);

    void baue_tunnel_und_bruecken();

    void optimiere_stelle(int index);

    void baue_strasse();
    void baue_schiene();
    void baue_leitung();


public:

    koord gib_route_bei(int i) const {return route->at(i);};

    int n,max_n;

    bool baubaer;


    /**
     * If a way is built on top of another way, should the type
     * of the former way be kept or replced (true == keep)
     * @author Hj. Malthaner
     */
    void set_keep_existing_ways(bool yesno);

    /* If a way is built on top of another way, should the type
     * of the former way be kept or replaced, if the current way is faster (true == keep)
     * @author Hj. Malthaner
     */
    void set_keep_existing_faster_ways(bool yesno);


    void route_fuer(enum bautyp wt, const weg_besch_t * besch);

    void set_maximum(int n);

    wegbauer_t(karte_t *welt, spieler_t *spl);
    ~wegbauer_t();


    void calc_route(koord start, const koord ziel);
  bool check_crossing(const koord zv, const grund_t *bd,weg_t::typ wtyp) const;
  bool check_for_leitung(const koord zv, const grund_t *bd) const;
    bool ist_grund_fuer_strasse(koord pos, const koord zv, koord start, koord ziel) const;

    void baue();
};

#endif
