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

protected:
	waytype_t my_waytype;
    schedule_type type;

	/*
	 * initializes the fahrplan object
	 * is shared by fahrplan_t constructors
	 * @author hsiegeln
	 */
	void init();

public:
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

	waytype_t get_waytype() const {return my_waytype;}

	minivec_tpl<struct linieneintrag_t> eintrag;
	short aktuell;

	int maxi() const { return eintrag.get_count(); }

	schedule_type get_type() const {return type;}

	/**
	* get current stop of fahrplan
	* @author hsiegeln
	*/
	int get_aktuell() const { return aktuell; }

	/**
	 * set the current stop of the fahrplan
	 * if new value is bigger than stops available, the max stop will be used
	 * @author hsiegeln
	 */
	void set_aktuell(int new_aktuell) { aktuell = (unsigned)new_aktuell >= eintrag.get_count() ? eintrag.get_count() - 1 : new_aktuell; }

	inline bool ist_abgeschlossen() const { return abgeschlossen; }
	void eingabe_abschliessen() { abgeschlossen = true; }
	void eingabe_beginnen() { abgeschlossen = false; }

	schedule_t();

	/**
	 * Copy constructor
	 * @author hsiegeln
	 */
	schedule_t(schedule_t *);

	virtual ~schedule_t();

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

	virtual schedule_t* copy() { return new schedule_t(this); }

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
protected:
	bool ist_halt_erlaubt(const grund_t *) const;

public:
	zugfahrplan_t() : schedule_t() { type = zugfahrplan; }
	zugfahrplan_t(loadsave_t* file) : schedule_t(file) { type = zugfahrplan; my_waytype=track_wt; }
	zugfahrplan_t(schedule_t* fpl) : schedule_t(fpl) { type = zugfahrplan; my_waytype=track_wt; }
	schedule_t* copy() { return new zugfahrplan_t(this); }
	const char *fehlermeldung() const { return "Zughalt muss auf\nSchiene liegen!\n"; }
};

/* the schedule for monorail ...
 * @author Hj. Malthaner
 */
class tramfahrplan_t : public zugfahrplan_t
{
protected:
	bool ist_halt_erlaubt(const grund_t *) const;

public:
	tramfahrplan_t() : zugfahrplan_t() { type = tramfahrplan; my_waytype=track_wt; }
	tramfahrplan_t(loadsave_t* file) : zugfahrplan_t(file) { type = tramfahrplan; my_waytype=track_wt; }
	tramfahrplan_t(schedule_t* fpl) : zugfahrplan_t(fpl) { type = tramfahrplan; my_waytype=track_wt; }
	schedule_t* copy() { return new tramfahrplan_t(this); }
};


/**
 * Eine Spezialisierung des Fahrplans die nur Stops auf Straßen
 * zuläßt.
 *
 * @author Hj. Malthaner
 */
class autofahrplan_t : public schedule_t
{
protected:
	bool ist_halt_erlaubt(const grund_t *) const;

public:
	autofahrplan_t() : schedule_t() { type = autofahrplan; my_waytype=road_wt; }
	autofahrplan_t(loadsave_t* file) : schedule_t(file) { type = autofahrplan; my_waytype=road_wt; }
	autofahrplan_t(schedule_t* fpl) : schedule_t(fpl) { type = autofahrplan; my_waytype=road_wt; }
	schedule_t* copy() { return new autofahrplan_t(this); }
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
protected:
    virtual bool ist_halt_erlaubt(const grund_t *) const;

public:
    schifffahrplan_t() : schedule_t() { type = schifffahrplan; my_waytype=water_wt; }
    schifffahrplan_t(loadsave_t* file) : schedule_t(file) { type = schifffahrplan; my_waytype=water_wt; }
    schifffahrplan_t(schedule_t* fpl) : schedule_t(fpl) { type = schifffahrplan; my_waytype=water_wt; }
    schedule_t* copy() { return new schifffahrplan_t(this); }
	const char *fehlermeldung() const { return "Schiffhalt muss im\nWasser liegen!\n"; }
};


/* the schedule for air ...
 * @author Hj. Malthaner
 */
class airfahrplan_t : public schedule_t
{
protected:
	bool ist_halt_erlaubt(const grund_t *) const;

public:
	airfahrplan_t() : schedule_t() { type = airfahrplan; my_waytype=air_wt; }
	airfahrplan_t(loadsave_t* file) : schedule_t(file) { type = airfahrplan; my_waytype=air_wt; }
	airfahrplan_t(schedule_t* fpl) : schedule_t(fpl) { type = airfahrplan; my_waytype=air_wt; }
	schedule_t* copy() { return new airfahrplan_t(this); }
	const char *fehlermeldung() const { return "Flugzeughalt muss auf\nRunway liegen!\n"; }
};

/* the schedule for monorail ...
 * @author Hj. Malthaner
 */
class monorailfahrplan_t : public schedule_t
{
public:
	monorailfahrplan_t() : schedule_t() { type = monorailfahrplan; my_waytype=monorail_wt; }
	monorailfahrplan_t(loadsave_t* file) : schedule_t(file) { type = monorailfahrplan; my_waytype=monorail_wt; }
	monorailfahrplan_t(schedule_t* fpl) : schedule_t(fpl) { type = monorailfahrplan; my_waytype=monorail_wt; }
	schedule_t* copy() { return new monorailfahrplan_t(this); }
	const char *fehlermeldung() const { return "Monorailhalt muss auf\nMonorail liegen!\n"; }
};

/* the schedule for maglev ...
 * @author Hj. Malthaner
 */
class maglevfahrplan_t : public schedule_t
{
public:
	maglevfahrplan_t() : schedule_t() { type = maglevfahrplan; my_waytype=maglev_wt; }
	maglevfahrplan_t(loadsave_t* file) : schedule_t(file) { type = maglevfahrplan; my_waytype=maglev_wt; }
	maglevfahrplan_t(schedule_t* fpl) : schedule_t(fpl) { type = maglevfahrplan; my_waytype=maglev_wt; }
	schedule_t* copy() { return new maglevfahrplan_t(this); }
	const char *fehlermeldung() const { return "Maglevhalt muss auf\nMaglevschiene liegen!\n"; }
};

/* and narrow guage ...
 * @author Hj. Malthaner
 */
class narrowgaugefahrplan_t : public schedule_t
{
public:
	narrowgaugefahrplan_t() : schedule_t() { type = narrowgaugefahrplan; my_waytype=narrowgauge_wt; }
	narrowgaugefahrplan_t(loadsave_t* file) : schedule_t(file) { type = narrowgaugefahrplan; my_waytype=narrowgauge_wt; }
	narrowgaugefahrplan_t(schedule_t* fpl) : schedule_t(fpl) { type = narrowgaugefahrplan; my_waytype=narrowgauge_wt; }
	schedule_t* copy() { return new narrowgaugefahrplan_t(this); }
	const char *fehlermeldung() const { return "On narrowgauge track only!\n"; }
};


#endif
