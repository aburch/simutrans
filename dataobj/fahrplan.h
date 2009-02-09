#ifndef fahrplan_h
#define fahrplan_h

#include "linieneintrag.h"

#include "../tpl/minivec_tpl.h"


class grund_t;
class karte_t;


/**
 * Eine Klasse zur Speicherung von Fahrplänen in Simutrans.
 *
 * @author Hj. Malthaner
 */
class schedule_t
{
public:
	enum schedule_type {
		fahrplan = 0, autofahrplan = 1, zugfahrplan = 2, schifffahrplan = 3, airfahrplan = 4, monorailfahrplan = 5, tramfahrplan = 6, maglevfahrplan = 7, narrowgaugefahrplan = 8,
	};

private:
	bool abgeschlossen;

	static struct linieneintrag_t dummy_eintrag;

protected:
	schedule_t() {}

	uint8 aktuell;

	waytype_t my_waytype;
	schedule_type type;

	/*
	 * initializes the fahrplan object
	 * is shared by fahrplan_t constructors
	 * @author hsiegeln
	 */
	void init();

public:
	minivec_tpl<struct linieneintrag_t> eintrag;

	/**
	* sollte eine Fehlermeldung ausgeben, wenn halt nicht erlaubt ist
	* @author Hj. Malthaner
	*/
	virtual const char *fehlermeldung() const { return ""; }

	/**
	* der allgemeine Fahrplan erlaubt haltestellen überall.
	* diese Methode sollte in den unterklassen redefiniert werden.
	* @author Hj. Malthaner
	*/
	virtual bool ist_halt_erlaubt(const grund_t *gr) const;

	uint8 get_count() const { return eintrag.get_count(); }

	schedule_type get_type() const {return type;}

	waytype_t get_waytype() const {return my_waytype;}

	/**
	* get current stop of fahrplan
	* @author hsiegeln
	*/
	uint8 get_aktuell() const { return aktuell; }

	// always returns a valid entry to the current stop
	const struct linieneintrag_t &get_current_eintrag() const { return aktuell>=eintrag.get_count() ? dummy_eintrag : eintrag[aktuell]; }

	/**
	 * set the current stop of the fahrplan
	 * if new value is bigger than stops available, the max stop will be used
	 * @author hsiegeln
	 */
	void set_aktuell(uint8 new_aktuell) {
		if(  new_aktuell>=eintrag.get_count()  ) {
			new_aktuell = max(1,eintrag.get_count())-1;
		}
		aktuell = new_aktuell;
	}

	// advance entry by one ...
	void advance() {
		if(  !eintrag.empty()  ) {
			aktuell = (aktuell+1)%eintrag.get_count();
		}
	}

	inline bool ist_abgeschlossen() const { return abgeschlossen; }
	void eingabe_abschliessen() { abgeschlossen = true; }
	void eingabe_beginnen() { abgeschlossen = false; }

	virtual ~schedule_t() {}

	schedule_t(loadsave_t *file);

	/**
	 * fügt eine koordinate an stelle aktuell in den Fahrplan ein
	 * alle folgenden Koordinaten verschieben sich dadurch
	 */
	bool insert(const grund_t* gr, uint8 ladegrad = 0, uint8 waiting_time_shift = 0);

	/**
	 * hängt eine koordinate an den fahrplan an
	 */
	bool append(const grund_t* gr, uint8 ladegrad = 0, uint8 waiting_time_shift = 0);

	// cleanup a schedule, removes double entries
	void cleanup();

	/**
	 * entfern eintrag[aktuell] aus dem fahrplan
	 * alle folgenden Koordinaten verschieben sich dadurch
	 */
	bool remove();

	void rdwr(loadsave_t *file);

	void rotate90( sint16 y_size );

	/**
	 * if the passed in fahrplan matches "this", then return true
	 * @author hsiegeln
	 */
	bool matches(karte_t *welt, const schedule_t *fpl);

	/**
	 * calculates a return way for this schedule
	 * will add elements 1 to maxi-1 in reverse order to schedule
	 * @author hsiegeln
	 */
	void add_return_way();

	virtual schedule_t* copy() = 0;//{ return new schedule_t(this); }

	// copy all entries from schedule src to this and adjusts aktuell
	void copy_from(const schedule_t *src);
};


/**
 * Eine Spezialisierung des Fahrplans die nur Stops auf Schienen
 * zuläßt.
 *
 * @author Hj. Malthaner
 */
class zugfahrplan_t : public schedule_t
{
public:
	zugfahrplan_t() { init(); type = zugfahrplan; my_waytype=track_wt; }
	zugfahrplan_t(loadsave_t* file) : schedule_t(file) { type = zugfahrplan; my_waytype=track_wt; }
	schedule_t* copy() { schedule_t *s = new zugfahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "Zughalt muss auf\nSchiene liegen!\n"; }
};

/* the schedule for monorail ...
 * @author Hj. Malthaner
 */
class tramfahrplan_t : public zugfahrplan_t
{
public:
	tramfahrplan_t() { init(); type = tramfahrplan; my_waytype=tram_wt; }
	tramfahrplan_t(loadsave_t* file) : zugfahrplan_t(file) { type = tramfahrplan; my_waytype=track_wt; }
	schedule_t* copy() { schedule_t *s = new tramfahrplan_t(); s->copy_from(this); return s; }
};


/**
 * Eine Spezialisierung des Fahrplans die nur Stops auf Straßen
 * zuläßt.
 *
 * @author Hj. Malthaner
 */
class autofahrplan_t : public schedule_t
{
public:
	autofahrplan_t() { init(); type = autofahrplan; my_waytype=road_wt; }
	autofahrplan_t(loadsave_t* file) : schedule_t(file) { type = autofahrplan; my_waytype=road_wt; }
	schedule_t* copy() { schedule_t *s = new autofahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "Autohalt muss auf\nStrasse liegen!\n"; }
};


/**
 * Eine Spezialisierung des Fahrplans die nur Stops auf Wasser
 * zuläßt.
 *
 * @author Hj. Malthaner
 */
class schifffahrplan_t : public schedule_t
{
public:
	schifffahrplan_t() { init(); type = schifffahrplan; my_waytype=water_wt; }
	schifffahrplan_t(loadsave_t* file) : schedule_t(file) { type = schifffahrplan; my_waytype=water_wt; }
	schedule_t* copy() { schedule_t *s = new schifffahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "Schiffhalt muss im\nWasser liegen!\n"; }
};


/* the schedule for air ...
 * @author Hj. Malthaner
 */
class airfahrplan_t : public schedule_t
{
public:
	airfahrplan_t() { init(); type = airfahrplan; my_waytype=air_wt; }
	airfahrplan_t(loadsave_t* file) : schedule_t(file) { type = airfahrplan; my_waytype=air_wt; }
	schedule_t* copy() { schedule_t *s = new airfahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "Flugzeughalt muss auf\nRunway liegen!\n"; }
};

/* the schedule for monorail ...
 * @author Hj. Malthaner
 */
class monorailfahrplan_t : public schedule_t
{
public:
	monorailfahrplan_t() { init(); type = monorailfahrplan; my_waytype=monorail_wt; }
	monorailfahrplan_t(loadsave_t* file) : schedule_t(file) { type = monorailfahrplan; my_waytype=monorail_wt; }
	schedule_t* copy() { schedule_t *s = new monorailfahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "Monorailhalt muss auf\nMonorail liegen!\n"; }
};

/* the schedule for maglev ...
 * @author Hj. Malthaner
 */
class maglevfahrplan_t : public schedule_t
{
public:
	maglevfahrplan_t() { init(); type = maglevfahrplan; my_waytype=maglev_wt; }
	maglevfahrplan_t(loadsave_t* file) : schedule_t(file) { type = maglevfahrplan; my_waytype=maglev_wt; }
	schedule_t* copy() { schedule_t *s = new maglevfahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "Maglevhalt muss auf\nMaglevschiene liegen!\n"; }
};

/* and narrow guage ...
 * @author Hj. Malthaner
 */
class narrowgaugefahrplan_t : public schedule_t
{
public:
	narrowgaugefahrplan_t() { init(); type = narrowgaugefahrplan; my_waytype=narrowgauge_wt; }
	narrowgaugefahrplan_t(loadsave_t* file) : schedule_t(file) { type = narrowgaugefahrplan; my_waytype=narrowgauge_wt; }
	schedule_t* copy() { schedule_t *s = new narrowgaugefahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "On narrowgauge track only!\n"; }
};


#endif
