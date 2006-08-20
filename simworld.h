/*
 * simworld.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/* simworld.h
 *
 * zentrale Datenstruktur von Simutrans
 * von Hj. Malthaner 1998
 */

#ifndef simworld_h
#define simworld_h

#ifndef ifc_karte_modell_h
#include "ifc/karte_modell.h"
#endif

#ifndef convoihandle_t_h
#include "convoihandle_t.h"
#endif

#ifndef halthandle_t_h
#include "halthandle_t.h"
#endif

#ifndef simsound_h
#include "simsound.h"
#endif

#ifndef tpl_slist_tpl_h
#include "tpl/slist_tpl.h"
#endif

#ifndef tpl_array_tpl_h
#include "tpl/array_tpl.h"
#endif

#ifndef __MARKER_H
#include "dataobj/marker.h"
#endif

#ifndef simplan_h
#include "simplan.h"
#endif

#include "simgraph.h"
#include "simdebug.h"

struct event_t;
class simlinemgmt_t;
class stadt_t;
class ding_t;
class fabrik_t;
class convoi_t;
class gebaeude_t;
class zeiger_t;
class grund_t;
class planquadrat_t;
class haltestelle_t;

class einstellungen_t;

class sync_steppable;

class cstring_t;

template <class T> class vector_tpl;

/**
 * Die Karte ist der zentrale Bestandteil der Simulation. Sie
 * speichert alle Daten und Objekte.
 *
 * @author Hj. Malthaner
 */
class karte_t : public karte_modell_t
{
public:
    /**
     * anzahl ticks pro tag in bits
     * @see ticks_per_tag
     * @author Hj. Malthaner
     */
    enum { ticks_bits_per_tag = 18 };


    /**
     * anzahl ticks pro tag
     * @author Hj. Malthaner
     */
    enum { ticks_per_tag = (1 << ticks_bits_per_tag) };

    /**
     * Hoehe eines Punktes der Karte mit "perlin noise"
     *
     * @param frequency in 0..1.0 roughness, the higher the rougher
     * @param amplitude in 0..160.0 top height of mountains, may not exceed 160.0!!!
     * @author Hj. Malthaner
     */
    static int perlin_hoehe(const int x, const int y, const double frequency, const double amplitude);

    // @author hsiegeln
    simlinemgmt_t * simlinemgmt;

private:

    // die Einstellungen
    einstellungen_t *einstellungen;

    // aus performancegruenden werden einige Einstellungen local gecached
    unsigned int cached_groesse;

    // die mausfunktion
    int (* mouse_funk)(spieler_t *, karte_t *, koord pos, value_t param);

    value_t mouse_funk_param;
    int mouse_funk_ok_sound;
    int mouse_funk_ko_sound;

    /**
     * Tomas: forest related values - no of trees could be on
     * other places as well
     */
    unsigned char max_no_of_trees_on_square;

    /**
     * Max. number of players
     * @author Hj. Malthaner
     */
    static const int anz_spieler;


    /**
     * fuer softes scrolling
     */
    int x_off, y_off;


    /**
     * Mauszeigerposition, intern
     * @author Hj. Malthaner
     */
    int i,j;


    /**
     * If this is true, the map will not be scrolled
     * on right-drag
     * @author Hj. Malthaner
     */
    bool scroll_lock;


    /**
     * Die Hoehe des (Grund-) Wasserspiegels.
     * @author Hj. Malthaner
     */
    int grundwasser;


    zeiger_t *zeiger;


    slist_tpl<sync_steppable *> sync_list;

    slist_tpl<convoihandle_t> convoi_list;

    slist_tpl<fabrik_t *> fab_list;

    slist_tpl<gebaeude_t *> ausflugsziele;
    int ausflugsziel_max_pax;
    array_tpl<int> *ausflugsziele_accumulated_level;
    int all_ausflugsziele_top_pax;

    slist_tpl<koord> labels;

    vector_tpl<stadt_t *> *stadt;


    /**
     * Fraktale, rekursive Landschaftserzeugung
     * @author Hj. Malthaner
     */
    void calc_hoehe(int x1, int y1, int x2, int y2);

    /**
     * Read a heightfield from file
     * @param filename name of heightfield file
     * @author Hj. Malthaner
     */
    void calc_hoehe_mit_heightfield(const cstring_t & filename);


    /**
     * Landschafterzeugung mit "perlin noise"
     * @author Hj. Malthaner
     */
    void calc_hoehe_mit_perlin();


    /**
     * Helferroutine fuer cleanup_karte()
     * @see karte_t::cleanup_karte
     * @author Hj. Malthaner
     */
    void raise_clean(int x,int y, int h);


    /**
     * Die fraktale Erzugung der Karte ist nicht perfekt.
     * cleanup_karte() beseitigt etwaige Fehler.
     * @author Hj. Malthaner
     */
    void cleanup_karte();


    /**
     * entfernt alle objecte, loescht alle datenstrukturen
     * gibt allen erreichbaren speicher frei
     * @author Hj. Malthaner
     */
    void destroy();



    void blick_aendern(event_t *ev);
    void bewege_zeiger(const event_t *ev);
    void interactive_event(event_t &ev);


    /**
     * Calculates slope for grid at pos.
     * @author Hj. Malthaner
     */
    void calc_slope(koord pos);


    planquadrat_t *plan;

    sint8 *grid_hgts;


    /**
     * Slopes of the grids. ATM only lower 4 bits used.
     * @author Hj. Malthaner
     */
    uint8 *slopes;


    marker_t marker;


    /**
     * Die Spieler
     * @author Hj. Malthaner
     */
    spieler_t  *spieler[8];                   // Mensch ist spieler Nr. 0


    /**
     * Die Zeit in ms
     * @author Hj. Malthaner
     */
    unsigned long ticks;		      // Anzahl ms seit Erzeugung

    unsigned long last_step_time;

    unsigned int letzter_monat;  // Anzahl Spielmonate seit Erzeugung
    unsigned int letztes_jahr;   // Anzahl Spieljahre seit Erzeugung

    int season;	// current season

    int steps;          // Anzahl steps seit Erzeugung
    int steps_bis_jetzt;
    bool doit;          // flag fuer simulationsabbruch (false == abbruch)
    bool m_quit_simutrans;// true = unload simutrans      //02-Nov-2001   Markus Weber    Added
    int sleep_time;     // sleep time fuer simulationsschleife


    /**
     * fuer performancevergleiche
     * @author Hj. Malthaner
     */
    int thisFPS, lastFPS, realFPS;

    int last_simloops;

    void verteile_baeume(int dichte);

    void do_pause();         // Spiel pausieren
    void neuer_monat();      // Monatliche Aktionen
    void neues_jahr();       // Jaehrliche Aktionen


    /**
     * internal saving method
     * @author Hj. Malthaner
     */
    void speichern(loadsave_t *file);


    /**
     * internal loading method
     * @author Hj. Malthaner
     */
    void laden(loadsave_t *file);

public:

    // @author hsiegeln
    unsigned int get_last_year() { return letztes_jahr; };

    int gib_x_off() const {return x_off;};
    int gib_y_off() const {return y_off;};


    /**
     * If this is true, the map will not be scrolled
     * on right-drag
     * @author Hj. Malthaner
     */
    void set_scroll_lock(bool yesno);


    //tree number related routines
    unsigned char gib_max_no_of_trees_on_square () const {
      return max_no_of_trees_on_square;
    };

    void setze_max_no_of_trees_on_square (unsigned char number) {
      max_no_of_trees_on_square = number;
    };


    void zentriere_auf(koord k);


    /**
     * offsets für zeigerposition
     * @author Hj. Malthaner
     */
    static const int Z_PLAN;
    static const int Z_GRID;

    // das ist eher ein flag als ein offset
    static const int Z_LINES;


    inline int gib_groesse() const {return cached_groesse;};

    einstellungen_t * gib_einstellungen() const {return einstellungen;};

    /**
     * sollte einen const zeiger_t * zurueckgeben, aber wegen der Tests
     * braucht man mehr Zugriff, deshalb ohne const
     *
     * @author Hj. Malthaner
     */
    ding_t * gib_zeiger() const;

    spieler_t * gib_spieler(int n);



    /**
     * 0=winter, 1=spring, 2=summer, 3=autumn
     * @author prissi
     */
    inline unsigned long gib_jahreszeit() const { return season; };


    /**
     * Zeit seit Kartenerzeugung in ms
     * @author Hj. Malthaner
     */
    inline unsigned long gib_zeit_ms() const { return ticks; };


    /**
     * Anzahl steps seit Kartenerzeugung
     * @author Hj. Malthaner
     */
    inline int gib_steps() const { return steps; };


    /**
     * Idle time. Nur zur Anzeige verwenden!
     * @author Hj. Malthaner
     */
    inline int gib_schlaf_zeit() const { return sleep_time; };


    /**
     * Anzahl frames in der letzten Sekunde Spielzeit. Kann sehr ungenau sein!
     * @author Hj. Malthaner
     */
    inline int gib_FPS() const { return lastFPS; };


    /**
     * Anzahl frames in der letzten Sekunde Realzeit
     * @author prissi
     */
    inline int gib_realFPS() const { return realFPS; };


    /**
     * Anzahl Simulationsloops in der letzten Sekunde. Kann sehr ungenau sein!
     * @author Hj. Malthaner
     */
    inline int gib_simloops() const { return last_simloops; };


    /**
     * Zugriff auf das Städte Array.
     * @author Hj. Malthaner
     */
    inline const vector_tpl<stadt_t *> * gib_staedte() const { return stadt; };


    /**
     * Zugriff auf das Städte Array.
     * @author Hj. Malthaner
     */
    void add_stadt(stadt_t *);


    /**
     * Holt den Grundwasserlevel der Karte
     *
     * @author Hj. Malthaner
     */
    int gib_grundwasser() const { return grundwasser; };


    /**
     * Bindet einen Funktionsaufruf an einen Mausklick. Spielt zusaätzlich
     * einen Sound, je nachdem, ob der Aufruf erfolgreich war oder nicht.
     *
     * @param ok_sound sound für Erfolgsfall
     * @param ko_sound sound für Fehlerfall
     *
     * @author Hj. Malthaner
     */
    void setze_maus_funktion(int (* mouse_funk)(spieler_t *, karte_t *, koord pos),
                             int zeiger_bild,
			     int zeiger_versatz,
			     int ok_sound,
			     int ko_sound);

    /**
     * Spezialvarainte mit einem Parameter, der immer übergeben wird
     * Hajo: changed parameter type from long to value_t because some
     *       parts of the code pass pointers
     * @author V. Meyer, Hj. Malthaner
     */
    void setze_maus_funktion(int (* mouse_funk)(spieler_t *,
						karte_t *,
						koord pos,
						value_t param),
                             int zeiger_bild,
			     int zeiger_versatz,
			     value_t param,
			     int ok_sound,
			     int ko_sound);

    void setze_scroll_multi(int n);


    inline bool ist_in_kartengrenzen(koord k) const {
	return (k.x>=0 && k.y>=0 &&
		k.x<(short)cached_groesse && k.y<(short)cached_groesse);
    }


    inline bool ist_in_kartengrenzen(unsigned int i, unsigned int j) const {
	return (i<cached_groesse && j<cached_groesse);
    }


    inline bool ist_in_gittergrenzen(koord k) const {
	return (k.x>=0 && k.y>=0 &&
		k.x<=(short)cached_groesse && k.y<=(short)cached_groesse);
    }

    inline bool ist_in_gittergrenzen(unsigned int i, unsigned int j) const {
	return (i<=cached_groesse && j<=cached_groesse);
    }


    /**
     * Inline because called very frequently!
     * @return Planquadrat an koordinate pos
     * @author Hj. Malthaner
     */
    inline const planquadrat_t * lookup(const koord k) const
    {
      return ist_in_kartengrenzen(k.x, k.y) ? &plan[k.x+k.y*cached_groesse] : 0;
    };


    /**
     * Inline because called very frequently!
     * @return grund an pos/hoehe
     * @author Hj. Malthaner
     */
    inline grund_t * lookup(const koord3d pos) const
    {
	const planquadrat_t *plan = lookup(pos.gib_2d());
	return plan ? plan->gib_boden_in_hoehe(pos.z) : NULL;
    };


    /**
     * Get slope at position pos. Outside the map everything is flat.
     * @author Hj. Malthaner
     */
    uint8 get_slope(const koord k) const
    {
      return ist_in_kartengrenzen(k.x, k.y) ? slopes[k.x+k.y*cached_groesse] : 0;
    }


    /**
     * Creates an artificial slope at position k
     * @author Hj. Malthaner
     */
    void set_slope(koord k, uint8 slope);


    /**
     * bewegliche objecte müssen beim verlassen das verlassene feld
     * als dirty markieren
     * @author Hj. Malthaner
     */
    inline void markiere_dirty(koord3d k) {
      grund_t * gr = lookup(k);
      if(gr) gr->set_flag(grund_t::world_spot_dirty);
    }


    /**
     * der redraw löscht das dirty flag dann wieder
     * @author Hj. Malthaner
     */
    inline void markiere_clean(koord3d k) {
      grund_t * gr = lookup(k);
      if(gr) gr->clear_flag(grund_t::world_spot_dirty);
    }


    /**
     * abfrage ob dirty
     * @author Hj. Malthaner
     */
    inline bool ist_dirty(const koord3d k) const {
      const grund_t * gr = lookup(k);
      return gr && gr->get_flag(grund_t::world_spot_dirty);
    }


    /**
     * Wird vom Strassenbauer als Orientierungshilfe benutzt.
     * @author Hj. Malthaner
     */
    inline void markiere(koord3d k) { marker.markiere(lookup(k)); };


    /**
     * Wird vom Strassenbauer zum Entfernen der Orientierungshilfen benutzt.
     * @author Hj. Malthaner
     */
    inline void unmarkiere(koord3d k) { marker.unmarkiere(lookup(k)); };


    /**
     * Entfernt alle Markierungen.
     * @author Hj. Malthaner
     */
    inline void unmarkiere_alle() { marker.unmarkiere_alle(); }


    /**
     * Testet ob der Grund markiert ist.
     * @return Gibt true zurueck wenn der Untergrund markiert ist sonst false.
     * @author Hj. Malthaner
     */
    inline bool ist_markiert(koord3d k) const { return marker.ist_markiert(lookup(k)); };


     /**
     * Initialize map.
     * @param sets game settings
     * @param preselected_players defines which players the user has selected before he started the game
     * @author Hj. Malthaner
     */
    void init(einstellungen_t *sets);

    void init_felder();

    /**
     * Standard Konstruktor
     * @author Hj. Malthaner
     */
    karte_t();

    /**
     * Virtueller Destruktor damit der Compiler glücklich ist
     * @author Hj. Malthaner
     */
    virtual ~karte_t();


    /**
     * Prueft, ob das Planquadrat an Koordinate (x,y)
     * erniedrigt werden kann
     * @author V. Meyer
     */
    bool can_lower_plan_to(int x, int y, int h) const;


    /**
     * Prueft, ob das Planquadrat an Koordinate (x,y)
     * erhöht werden kann
     * @author V. Meyer
     */
    bool can_raise_plan_to(int x, int y, int h) const;


    /**
     * Prueft, ob das Planquadrat an Koordinate (x,y)
     * geaendert werden darf. (z.B. kann Wasser nicht geaendert werden)
     * @author Hj. Malthaner
     */
    bool is_plan_height_changeable(int x, int y) const;


    bool can_raise_to(int x,int y, int h) const;
    int  raise_to(int x,int y, int h);

    bool can_lower_to(int x,int y, int h) const;
    int  lower_to(int x,int y, int h);


    /**
     * Prueft, ob die Hoehe an Gitterkoordinate (x,y)
     * erhoeht werden kann.
     * @param x x-Gitterkoordinate
     * @param y y-Gitterkoordinate
     * @author Hj. Malthaner
     */
    bool can_raise(int x,int y) const;


    /**
     * Erhoeht die Hoehe an Gitterkoordinate (x,y) um eins.
     * @param pos Gitterkoordinate
     * @author Hj. Malthaner
     */
    int raise(koord pos);


    /**
     * Prueft, ob die Hoehe an Gitterkoordinate (x,y)
     * erniedrigt werden kann.
     * @param x x-Gitterkoordinate
     * @param y y-Gitterkoordinate
     * @author Hj. Malthaner
     */
    bool can_lower(int x,int y) const;


    /**
     * Erniedrigt die Hoehe an Gitterkoordinate (x,y) um eins.
     * @param pos Gitterkoordinate
     * @author Hj. Malthaner
     */
    int lower(koord pos);

    bool ebne_planquadrat(koord pos, int hgt);


    /**
     * Erzeugt einen Berg oder ein Tal
     * @param x x-Koordinate
     * @param y y-Koordinate
     * @param w Breite
     * @param h Hoehe
     * @param t Hoehe des Berges/Tiefe des Tales
     * @author Hj. Malthaner
     */
    void new_mountain(int x, int y, int w, int h, int t);


    void add_convoi(convoihandle_t &cnv) {
        assert(cnv.is_bound());
        convoi_list.insert( cnv );
    };
    bool rem_convoi(convoihandle_t &cnv) { return convoi_list.remove( cnv ); };
    const slist_tpl<convoihandle_t> &gib_convoi_list() const {return convoi_list;};


    void add_ausflugsziel(gebaeude_t *gb);
    void remove_ausflugsziel(gebaeude_t *gb);
    const slist_tpl<gebaeude_t *> & gib_ausflugsziele() const { return ausflugsziele; };
    const gebaeude_t *gib_random_ausflugsziel() const;
    int gib_ausflugsziele_max_pax() const { return ausflugsziel_max_pax; };


    void add_label(koord pos) { if(!labels.contains(pos)) { labels.append(pos); } };
    void remove_label(koord pos) {labels.remove(pos);};
    const slist_tpl<koord> &gib_label_list() const { return labels; };

    bool add_fab(fabrik_t *fab);
    bool rem_fab(fabrik_t *fab);
    int  gib_fab_index(fabrik_t *fab) { return fab_list.index_of(fab); };
    fabrik_t *  gib_fab(int index) { return (fabrik_t *)fab_list.at(index); };
    const slist_tpl<fabrik_t *> &gib_fab_list() const { return fab_list; };
    /* sucht zufaellig eine Fabrik aus der Fabrikliste
     * @author Hj. Malthaner
     */
    fabrik_t *get_random_fab() const;

    /**
     * sucht naechstgelegene Stadt an Position i,j
     * @author Hj. Malthaner
     */
    stadt_t * suche_naechste_stadt(koord pos) const;

    /**
     * Wenn uns die von "suche_naechste_stadt(koord pos)" Stadt nicht
     * gefällt, sucht diese Funktion die "naechste nach der letzten".
     * @author V. Meyer
     */
    stadt_t * suche_naechste_stadt(koord pos, const stadt_t *letzte) const;

#if 0
    /**
     * suche Haltestellen fuer einen Passagier/fracht
     * um den Punkt k herum - k selbst wird nicht geprüft!!!
     * liefert vector gefundener Haltestellen zurück
     * @author Hj. Malthaner
     */
    vector_tpl<halthandle_t> & suche_nahe_haltestellen(koord k, int radius, int mitte_wh, uint32 max_anzahl) const;
    // will not used, since 86 the ground knows himself about the next stops
#endif

    bool sync_add(sync_steppable *obj);
    bool sync_remove(sync_steppable *obj);

    void sync_prepare();	// Echtzeitfunktionen
    void sync_step(long delta_t);

    void step(long delta_t);	// Nicht-Echtzeit

    planquadrat_t *access(int i, int j);
    inline planquadrat_t *access(koord k) {return access(k.x, k.y);};


    /**
     * @return Hoehe am Gitterpunkt i,j
     * @author Hj. Malthaner
     */
    inline int lookup_hgt(koord k) const
    {
	return ist_in_gittergrenzen(k.x, k.y) ? grid_hgts[k.x + k.y*(cached_groesse+1)] << 4 : grundwasser;
    };


    /**
     * Sets grid height.
     * Never set grid_hgts manually, always use this method!
     * @author Hj. Malthaner
     */
    void setze_grid_hgt(koord k, int hgt);


    /**
     * @return Minimale Hoehe des Planquadrates i,j
     * @author Hj. Malthaner
     */
    int min_hgt(koord pos) const;


    /**
     * @return Maximale Hoehe des Planquadrates i,j
     * @author Hj. Malthaner
     */
    int max_hgt(koord pos) const;


    /**
     * @return true, wenn Platz an Stelle pos mit Groesse dim Wasser ist
     * @author V. Meyer
     */
    bool ist_wasser(koord pos, koord dim) const;

    /**
     * @return true, wenn Platz an Stelle i,j mit Groesse w,h bebaubar
     * @author Hj. Malthaner
     */
    bool ist_platz_frei(koord pos, int w, int h, int *last_y = NULL, bool check_slope=true ) const;

    /**
     * @return eine Liste aller bebaubaren Plaetze mit Groesse w,h
     * @author Hj. Malthaner
     */
    slist_tpl<koord> * finde_plaetze(int w, int h) const;



    /**
     * Spielt den Sound, wenn die Position im sichtbaren Bereich liegt.
     * Spielt weiter entfernte Sounds leiser ab.
     * @param pos Position an der das Ereignis stattfand
     * @author Hj. Malthaner
     */
    bool play_sound_area_clipped(koord pos, sound_info info);


    bool setze_hoehe_no_remap(int x,int y,int h, int &n);
    bool setze_hoehe(int x,int y,int h,int &n);


    /**
     * Saves the map to a file
     * @param filename name of the file to write
     * @author Hansjörg Malthaner
     */
    void speichern(const char *filename);


    /**
     * Loads a map from a file
     * @param filename name of the file to read
     * @author Hansjörg Malthaner
     */
    void laden(const char *filename);


    /**
     * Creates a map from a heightfield
     * @param sets game settings
     * @author Hansjörg Malthaner
     */
    void load_heightfield(einstellungen_t *sets);


    /**
     * Cancel the simulation or unload simutrans
     * @param quit_simutans True = Unload simutrans
     * @author Hj. Malthaner
     */
    void beenden(bool quit_simutans); // 02-Nov-2001    Markus Weber added parameter

    /**
     * Converts player point to an index of the player array
     *
     * @return player index (number)
     * @author Hansjörg Malthaner
     */
    int sp2num(spieler_t *sp);


    /**
     * Bearbeitet events. Sollte nur in extremen Ausnahmefällen und
     * nach sorgfältiger Abwägung aller Alternativen von ausserhalb
     * dieser Klasse gerufen werden.
     *
     * @author Hansjörg Malthaner
     */
    void interactive_update();


    /**
     * Main simulation loop.
     *
     * @return true if the main program should quit, false otherwise
     * @author Hansjörg Malthaner
     */
    bool interactive();
};


void warte_auf_taste(event_t *ev);

//#ifndef __cplusplus
//#define __cplusplus
//#endif
#include "simtools.h"

/**
 * Hoehe eines Punktes der Karte mit "perlin noise"
 *
 * @param frequency in 0..1.0 roughness, the higher the rougher
 * @param amplitude in 0..160.0 top height of mountains, may not exceed 160.0!!!
 * @author Hj. Malthaner
 */
inline int
karte_t::perlin_hoehe(const int x, const int y,
                      const double frequency, const double amplitude)
{
//    double perlin_noise_2D(double x, double y, double persistence);
//    return ((int)(perlin_noise_2D(x, y, 0.6)*160.0)) & 0xFFFFFFF0;
    return ((int)(perlin_noise_2D(x, y, frequency)*amplitude)) & 0xFFFFFFF0;
}


#endif
