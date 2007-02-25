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

#include "simconst.h"
#include "simtypes.h"

#include "convoihandle_t.h"
#include "halthandle_t.h"

#include "tpl/weighted_vector_tpl.h"
#include "tpl/minivec_tpl.h"
#include "tpl/vector_tpl.h"
#include "tpl/slist_tpl.h"

#include "dataobj/marker.h"
#include "dataobj/einstellungen.h"

#include "simplan.h"

#include "simdebug.h"

struct event_t;
struct sound_info;
class stadt_t;
class ding_t;
class fabrik_t;
class gebaeude_t;
class zeiger_t;
class grund_t;
class planquadrat_t;
class karte_ansicht_t;
class sync_steppable;
class cstring_t;


/**
 * Die Karte ist der zentrale Bestandteil der Simulation. Sie
 * speichert alle Daten und Objekte.
 *
 * @author Hj. Malthaner
 */
class karte_t
{
public:
	/**
	* Hoehe eines Punktes der Karte mit "perlin noise"
	*
	* @param frequency in 0..1.0 roughness, the higher the rougher
	* @param amplitude in 0..160.0 top height of mountains, may not exceed 160.0!!!
	* @author Hj. Malthaner
	*/
	static int perlin_hoehe(const int x, const int y, const double frequency, const double amplitude);

private:
	// all cursor interaction goes via this function
	// it will call save_mouse_funk first with init, then with the position and with exit, when another tool is selected without click
	// see simwerkz.cc for practical examples of such functions
	struct save_mouse_func {
		int (*save_mouse_funk)(spieler_t *, karte_t *, koord pos, value_t param);
		value_t mouse_funk_param;
		int mouse_funk_ok_sound;
		int mouse_funk_ko_sound;
		int zeiger_versatz;
		int zeiger_bild;
	};

	// die Einstellungen
	einstellungen_t *einstellungen;

	// aus performancegruenden werden einige Einstellungen local gecached
	sint16 cached_groesse_gitter_x;
	sint16 cached_groesse_gitter_y;
	// diese Werte sind um ein kleiner als die Werte für das Gitter
	sint16 cached_groesse_karte_x;
	sint16 cached_groesse_karte_y;
	// maximum size for waitng bars etc.
	int cached_groesse_max;

	// die mausfunktion
	int (* mouse_funk)(spieler_t *, karte_t *, koord pos, value_t param);

	value_t mouse_funk_param;
	int mouse_funk_ok_sound;
	int mouse_funk_ko_sound;

	/**
	 * redraw whole map
	 */
	bool dirty;

	/**
	 * fuer softes scrolling
	 */
	int x_off, y_off;

	/* current position */
	koord ij_off;

	/* this is the current offset for getting from tile to screen */
	koord ansicht_ij_off;

	/**
	 * Mauszeigerposition, intern
	 * @author Hj. Malthaner
	 */
	int mi, mj;

	/**
	 * If this is true, the map will not be scrolled
	 * on right-drag
	 * @author Hj. Malthaner
	 */
	bool scroll_lock;

	/*
	* the current convoi to follow
	* @author prissi
	*/
	convoihandle_t follow_convoi;

	/**
	 * water level height
	 * @author Hj. Malthaner
	 */
	sint16 grundwasser;

	/**
	 * current snow height (might change during the game)
	 * @author prissi
	 */
	sint16 snowline;

	// changes the snowline height (for the seasons)
	// @author prissi
	void recalc_snowline();

	/**
	 * table for fast conversion from height to climate
	 * @author prissi
	 */
	uint8 height_to_climate[32];

	zeiger_t *zeiger;

	slist_tpl<sync_steppable *> sync_add_list;	// these objects are move to the sync_list (but before next sync step, so they do not interfere!)
	slist_tpl<sync_steppable *> sync_list;

	vector_tpl<convoihandle_t> convoi_array;

	slist_tpl<fabrik_t *> fab_list;

	weighted_vector_tpl<gebaeude_t *> ausflugsziele;

	slist_tpl<koord> labels;

	weighted_vector_tpl<stadt_t*> stadt;

	minivec_tpl<save_mouse_func *> quick_shortcuts;

	karte_ansicht_t *view;

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
	void raise_clean(sint16 x, sint16 y, sint16 h);

	bool can_raise_to(sint16 x, sint16 y, sint16 h) const;
	int  raise_to(sint16 x, sint16 y, sint16 h,bool set_slopes);

	bool can_lower_to(sint16 x, sint16 y, sint16 h) const;
	int  lower_to(sint16 x, sint16 y, sint16 h,bool set_slopes);

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

	planquadrat_t *plan;

	sint8 *grid_hgts;

	marker_t marker;

	/**
	 * Die Spieler
	 * @author Hj. Malthaner
	 */
	spieler_t *spieler[MAX_PLAYER_COUNT];                   // Mensch ist spieler Nr. 0
	spieler_t	*active_player;
	uint8 active_player_nr;

	/*
	 * counter for schedules
	 * if a new schedule is active, this counter will increment
	 * stations check this counter and will reroute their goods if changed
	 * @author prissi
	 */
	uint8 schedule_counter;

	/**
	 * Die Zeit in ms
	 * @author Hj. Malthaner
	 */
	uint32 ticks;		      // Anzahl ms seit Erzeugung
	uint32 next_month_ticks;	// from now on is next month

	// default time stretching factor
	uint32 time_multiplier;

	// true, if fast forward
	bool fast_forward;

	uint32 last_frame_ms[256];
	uint8 last_frame_idx;
	uint32 last_step_time;

	sint32 current_month;	// monat+12*jahr
	sint32 letzter_monat;  // Absoluter Monat 0..12
	sint32 letztes_jahr;   // Absolutes Jahr
	sint32 basis_jahr;   // Jahr ab dem die ticks zählen

	uint8 season;	// current season

	long steps;          // Anzahl steps seit Erzeugung
	long steps_bis_jetzt;
	bool is_sound;	// flag, that now no sound will play
	bool doit;          // flag fuer simulationsabbruch (false == abbruch)
	bool m_quit_simutrans;// true = unload simutrans      //02-Nov-2001   Markus Weber    Added
	int sleep_time;     // sleep time fuer simulationsschleife

	// may change due to timeline
	const weg_besch_t *city_road;

	int average_speed[4];
	// recalculated speed boni for different vehicles
	void recalc_average_speed();

	/**
	 * fuer performancevergleiche
	 * @author Hj. Malthaner
	 */
	uint32 realFPS;

	int last_simloops;

	void do_pause();         // Spiel pausieren
	void neuer_monat();      // Monatliche Aktionen
	void neues_jahr();       // Jaehrliche Aktionen

	/**
	 * internal saving method
	 * @author Hj. Malthaner
	 */
	void speichern(loadsave_t *file,bool silent);


	/**
	 * internal loading method
	 * @author Hj. Malthaner
	 */
	void laden(loadsave_t *file);

public:
	/**
	 * Absoluter Monat
	 * @author prissi
	 */
	inline uint32 get_last_month() const { return letzter_monat; }

	// @author hsiegeln
	inline sint32 get_last_year() { return letztes_jahr; }

	// @author hsiegeln
	inline sint32 get_base_year() { return basis_jahr; }

	/**
	 * dirty: redraw whole screen
	 * @author Hj. Malthaner
	 */
	void setze_dirty() {dirty=true;}
	void setze_dirty_zurueck() {dirty=false;}
	bool ist_dirty() const {return dirty;}

	karte_ansicht_t *gib_ansicht() const { return view; }
	void setze_ansicht(karte_ansicht_t *v) { view = v; }

	/**
	 * viewpoint in tile koordinates
	 * @author Hj. Malthaner
	 */
	koord gib_ij_off() const {return ij_off;}

	/**
	 * set viewport (tile koordinates)
	 * @author Hj. Malthaner
	 */
	void setze_ij_off(koord3d ij);

	// fine offset within the viewprt tile
	int gib_x_off() const {return x_off;}
	int gib_y_off() const {return y_off;}

	// the koordinates between the screen and a tile may have several offset
	// this routine caches them
	void setze_ansicht_ij_offset( koord k ) { ansicht_ij_off=k; }
	koord gib_ansicht_ij_offset() const { return ansicht_ij_off; }

	/**
	 * If this is true, the map will not be scrolled
	 * on right-drag
	 * @author Hj. Malthaner
	 */
	void set_scroll_lock(bool yesno);

	/* functions for following a convoi on the map
	* give an unboud handle to unset
	*/
	void set_follow_convoi(convoihandle_t cnv);
	convoihandle_t get_follow_convoi() const { return follow_convoi; }

	einstellungen_t* gib_einstellungen() const { return einstellungen; }

	// returns current speed bonus
	int get_average_speed(waytype_t typ) const;

	bool is_fast_forward();

	/**
	 * sollte einen const zeiger_t * zurueckgeben, aber wegen der Tests
	 * braucht man mehr Zugriff, deshalb ohne const
	 *
	 * @author Hj. Malthaner
	 */
	zeiger_t * gib_zeiger() const;

	/**
	 * marks an area using the grund_t mark flag
	 * @author prissi
	 */
	void mark_area( const sint16 lx, const sint16 uy, const uint8 radius, const bool mark );

	spieler_t * gib_spieler(int n) const {assert(((uint8)n)<MAX_PLAYER_COUNT); return spieler[n]; }
	spieler_t* get_active_player() const { return active_player; }
	uint8 get_active_player_nr() const { return active_player_nr; }
	void switch_active_player(uint8 nr);

	// if a schedule is changed, it will increment the schedule counter
	// every step the haltstelle will check and reroute the goods if needed
	uint8 get_schedule_counter() const { return schedule_counter; }
	void set_schedule_counter() { schedule_counter++; }

	// often used, therefore found here
	bool use_timeline() const { return einstellungen->gib_use_timeline(); }

	void reset_timer();
	void step_year();

	// returns either 0 or the current year*16 + month
	uint16 get_timeline_year_month() const { return einstellungen->gib_use_timeline() ? current_month : 0; }

	/**
	* anzahl ticks pro tag in bits
	* @see ticks_per_tag
	* @author Hj. Malthaner
	*/
	uint32 ticks_bits_per_tag;

	/**
	* anzahl ticks pro tag
	* @author Hj. Malthaner
	*/
	uint32 ticks_per_tag;

	void setze_ticks_bits_per_tag(uint32 bits) {ticks_bits_per_tag = bits; ticks_per_tag = (1 << ticks_bits_per_tag); }

	sint32 get_time_multiplier() const { return time_multiplier; }

	/**
	 * 0=winter, 1=spring, 2=summer, 3=autumn
	 * @author prissi
	 */
	uint8 gib_jahreszeit() const { return season; }

	/**
	 * Zeit seit Kartenerzeugung/dem letzen laden in ms
	 * @author Hj. Malthaner
	 */
	uint32 gib_zeit_ms() const { return ticks; }

	/**
	 * Absoluter Monat
	 * @author prissi
	 */
	uint32 get_current_month() const { return current_month; }

	// prissi: current city road
	// may change due to timeline
	const weg_besch_t* get_city_road() const { return city_road; }

	/**
	 * Anzahl steps seit Kartenerzeugung
	 * @author Hj. Malthaner
	 */
	long gib_steps() const { return steps; }

	/**
	 * Idle time. Nur zur Anzeige verwenden!
	 * @author Hj. Malthaner
	 */
	int gib_schlaf_zeit() const { return sleep_time; }

	/**
	 * Anzahl frames in der letzten Sekunde Realzeit
	 * @author prissi
	 */
	int gib_realFPS() const { return realFPS; }

	/**
	 * Anzahl Simulationsloops in der letzten Sekunde. Kann sehr ungenau sein!
	 * @author Hj. Malthaner
	 */
	int gib_simloops() const { return last_simloops; }

	/**
	* Holt den Grundwasserlevel der Karte
	* @author Hj. Malthaner
	*/
	sint16 gib_grundwasser() const { return grundwasser; }

	/**
	* returns the current snowline height
	* @author prissi
	*/
	sint16 get_snowline() const { return snowline; }

	/**
	* returns the current climate for a given height
	* uses as private lookup table for speed
	* @author prissi
	*/
	climate get_climate(sint16 height) const
	{
		const sint16 h=(height-grundwasser)/Z_TILE_STEP;
		if(h<0) {
			return water_climate;
		} else if(h>=32) {
			return arctic_climate;
		}
		return (climate)height_to_climate[h];
	}

	/**
	 * offsets für zeigerposition
	 * @author Hj. Malthaner
	 */
	static const int Z_PLAN;
	static const int Z_GRID;

	/**
	 * Bindet einen Funktionsaufruf an einen Mausklick. Spielt zusaätzlich
	 * einen Sound, je nachdem, ob der Aufruf erfolgreich war oder nicht.
	 *
	 * @param ok_sound sound für Erfolgsfall
	 * @param ko_sound sound für Fehlerfall
	 *
	 * @author Hj. Malthaner
	 */
	void setze_maus_funktion(int (* mouse_funk)(spieler_t *,karte_t *, koord pos),
		int zeiger_bild, int zeiger_versatz,
		int ok_sound, int ko_sound);

	/**
	 * Spezialvarainte mit einem Parameter, der immer übergeben wird
	 * Hajo: changed parameter type from long to value_t because some
	 *       parts of the code pass pointers
	 * @author V. Meyer, Hj. Malthaner
	 */
	void setze_maus_funktion(int (* funktion)(spieler_t *, karte_t *, koord, value_t param),
		int zeiger_bild, int zeiger_versatz,
		value_t param,
		int ok_sound, int ko_sound);

	void setze_scroll_multi(int n);

	// all stuf concerning map size
	inline int gib_groesse_x() const { return cached_groesse_gitter_x; }
	inline int gib_groesse_y() const { return cached_groesse_gitter_y; }
	inline int gib_groesse_max() const { return cached_groesse_max; }

	inline bool ist_in_kartengrenzen(koord k) const {
		// prissi: since negative values will make the whole result negative, we can use bitwise or
		// faster, since pentiums and other long pipeline processors do not like jumps
		return (k.x|k.y|(cached_groesse_karte_x-k.x)|(cached_groesse_karte_y-k.y))>=0;
		// this is omly 67% of the above speed
		//return k.x>=0 &&  k.y>=0  &&  cached_groesse_karte_x>=k.x  &&  cached_groesse_karte_y>=k.y;
	}

	inline bool ist_in_kartengrenzen(sint16 x, sint16 y) const {
	// prissi: since negative values will make the whole result negative, we can use bitwise or
	// faster, since pentiums and other long pipeline processors do not like jumps
		return (x|y|(cached_groesse_karte_x-x)|(cached_groesse_karte_y-y))>=0;
//		return x>=0 &&  y>=0  &&  cached_groesse_karte_x>=x  &&  cached_groesse_karte_y>=y;
	}

	inline bool ist_in_gittergrenzen(koord k) const {
	// prissi: since negative values will make the whole result negative, we can use bitwise or
	// faster, since pentiums and other long pipeline processors do not like jumps
		return (k.x|k.y|(cached_groesse_gitter_x-k.x)|(cached_groesse_gitter_y-k.y))>=0;
//		return k.x>=0 &&  k.y>=0  &&  cached_groesse_gitter_x>=k.x  &&  cached_groesse_gitter_y>=k.y;
	}

	inline bool ist_in_gittergrenzen(sint16 x, sint16 y) const {
	// prissi: since negative values will make the whole result negative, we can use bitwise or
	// faster, since pentiums and other long pipeline processors do not like jumps
		return (x|y|(cached_groesse_gitter_x-x)|(cached_groesse_gitter_y-y))>=0;
//		return x>=0 &&  y>=0  &&  cached_groesse_gitter_x>=x  &&  cached_groesse_gitter_y>=y;
	}

	inline bool ist_in_gittergrenzen(uint16 x, uint16 y) const {
		return (x<=(unsigned)cached_groesse_gitter_x && y<=(unsigned)cached_groesse_gitter_y);
	}

	/**
	* Inline because called very frequently!
	* @return Planquadrat an koordinate pos
	* @author Hj. Malthaner
	*/
	inline const planquadrat_t * lookup(const koord k) const
	{
		return ist_in_kartengrenzen(k.x, k.y) ? &plan[k.x+k.y*cached_groesse_gitter_x] : 0;
	}

	/**
	 * Inline because called very frequently!
	 * @return grund an pos/hoehe
	 * @author Hj. Malthaner
	 */
	inline grund_t * lookup(const koord3d pos) const
	{
		const planquadrat_t *plan = lookup(pos.gib_2d());
		return plan ? plan->gib_boden_in_hoehe(pos.z) : NULL;
	}

	/**
	 * Inline because called very frequently!
	 * @return grund at the bottom (where house will be built)
	 * @author Hj. Malthaner
	 */
	inline grund_t * lookup_kartenboden(const koord pos) const
	{
		const planquadrat_t *plan = lookup(pos);
		return plan ? plan->gib_kartenboden() : NULL;
	}

	/**
	 * returns the natural slope a a position
	 * @author prissi
	 */
	uint8	calc_natural_slope( const koord pos ) const;

	/**
	 * Wird vom Strassenbauer als Orientierungshilfe benutzt.
	 * @author Hj. Malthaner
	 */
	inline void markiere(koord3d k) { marker.markiere(lookup(k)); }
	inline void markiere(const grund_t* gr) { marker.markiere(gr); }

	/**
	 * Wird vom Strassenbauer zum Entfernen der Orientierungshilfen benutzt.
	 * @author Hj. Malthaner
	 */
	inline void unmarkiere(koord3d k) { marker.unmarkiere(lookup(k)); }
	inline void unmarkiere(const grund_t* gr) { marker.unmarkiere(gr); }

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
	inline bool ist_markiert(koord3d k) const { return marker.ist_markiert(lookup(k)); }
	inline bool ist_markiert(const grund_t* gr) const { return marker.ist_markiert(gr); }

	 /**
	 * Initialize map.
	 * @param sets game settings
	 * @param preselected_players defines which players the user has selected before he started the game
	 * @author Hj. Malthaner
	 */
	void init(einstellungen_t *sets);

	void init_felder();

	karte_t();

	~karte_t();

	// return an index to a halt (or creates a new one)
	// only used during loading
	halthandle_t get_halt_koord_index(koord k);

	/**
	 * Prueft, ob das Planquadrat an Koordinate (x,y)
	 * erniedrigt werden kann
	 * @author V. Meyer
	 */
	bool can_lower_plan_to(sint16 x, sint16 y, sint16 h) const;

	/**
	 * Prueft, ob das Planquadrat an Koordinate (x,y)
	 * erhöht werden kann
	 * @author V. Meyer
	 */
	bool can_raise_plan_to(sint16 x, sint16 y, sint16 h) const;

	/**
	 * Prueft, ob das Planquadrat an Koordinate (x,y)
	 * geaendert werden darf. (z.B. kann Wasser nicht geaendert werden)
	 * @author Hj. Malthaner
	 */
	bool is_plan_height_changeable(sint16 x, sint16 y) const;

	/**
	 * Prueft, ob die Hoehe an Gitterkoordinate (x,y)
	 * erhoeht werden kann.
	 * @param x x-Gitterkoordinate
	 * @param y y-Gitterkoordinate
	 * @author Hj. Malthaner
	 */
	bool can_raise(sint16 x,sint16 y) const;

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
	bool can_lower(sint16 x,sint16 y) const;

	/**
	 * Erniedrigt die Hoehe an Gitterkoordinate (x,y) um eins.
	 * @param pos Gitterkoordinate
	 * @author Hj. Malthaner
	 */
	int lower(koord pos);

	bool ebne_planquadrat(koord pos, sint16 hgt);

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
			convoi_array.append_unique(cnv,4);
	}
	void rem_convoi(convoihandle_t& cnv) { convoi_array.remove(cnv); }
	const vector_tpl<convoihandle_t> &get_convoi_array() const {return convoi_array;}
	const unsigned get_convoi_count() const {return convoi_array.get_count();}

	/**
	 * Zugriff auf das Städte Array.
	 * @author Hj. Malthaner
	 */
	const weighted_vector_tpl<stadt_t*>& gib_staedte() const { return stadt; }
	const stadt_t *get_random_stadt() const;
	void add_stadt(stadt_t *s);
	bool rem_stadt(stadt_t *s);

	/* tourist attraction list */
	void add_ausflugsziel(gebaeude_t *gb);
	void remove_ausflugsziel(gebaeude_t *gb);
	const gebaeude_t *gib_random_ausflugsziel() const;
	const weighted_vector_tpl<gebaeude_t*> &gib_ausflugsziele() const {return ausflugsziele; }

	void add_label(koord pos) { if (!labels.contains(pos)) labels.append(pos); }
	void remove_label(koord pos) { labels.remove(pos); }
	const slist_tpl<koord>& gib_label_list() const { return labels; }

	bool add_fab(fabrik_t *fab);
	bool rem_fab(fabrik_t *fab);
	int gib_fab_index(fabrik_t* fab) { return fab_list.index_of(fab); }
	fabrik_t* gib_fab(unsigned index) { return index < fab_list.count() ? fab_list.at(index) : NULL; }
	const slist_tpl<fabrik_t*>& gib_fab_list() const { return fab_list; }

	/* sucht zufaellig eine Fabrik aus der Fabrikliste
	 * @author Hj. Malthaner
	 */
	fabrik_t *get_random_fab() const;

	/* return all factories in this area
	* @author prissi
	*/
	vector_tpl<fabrik_t *> &find_all_factories( koord pos, koord extent );

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

	bool sync_add(sync_steppable *obj);
	bool sync_remove(sync_steppable *obj);

	void sync_step(long delta_t);

	void step(long delta_t);	// Nicht-Echtzeit

	inline planquadrat_t *access(int i, int j) {
		return ist_in_kartengrenzen(i, j) ? &plan[i + j*cached_groesse_gitter_x] : NULL;
	}

	inline planquadrat_t *access(koord k) {
		return ist_in_kartengrenzen(k) ? &plan[k.x + k.y*cached_groesse_gitter_x] : NULL;
	}

	/**
	 * @return Hoehe am Gitterpunkt i,j
	 * @author Hj. Malthaner
	 */
	inline int lookup_hgt(koord k) const {
		return ist_in_gittergrenzen(k.x, k.y) ? grid_hgts[k.x + k.y*(cached_groesse_gitter_x+1)]*Z_TILE_STEP : grundwasser;
	}

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
	bool ist_platz_frei(koord pos, sint16 w, sint16 h, int *last_y, climate_bits cl) const;

	/**
	 * @return eine Liste aller bebaubaren Plaetze mit Groesse w,h
	 * only used for town creation at the moment
	 * @author Hj. Malthaner
	 */
	slist_tpl<koord> * finde_plaetze(sint16 w, sint16 h, climate_bits cl) const;

	/**
	 * Spielt den Sound, wenn die Position im sichtbaren Bereich liegt.
	 * Spielt weiter entfernte Sounds leiser ab.
	 * @param pos Position an der das Ereignis stattfand
	 * @author Hj. Malthaner
	 */
	bool play_sound_area_clipped(koord pos, sound_info info);

	void mute_sound( bool state ) { is_sound = !state; }

	bool setze_hoehe(int x,int y,int h,int &n);

	/**
	 * Saves the map to a file
	 * @param filename name of the file to write
	 * @author Hansjörg Malthaner
	 */
	void speichern(const char *filename,bool silent);

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

#endif
