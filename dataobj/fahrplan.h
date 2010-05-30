#ifndef fahrplan_h
#define fahrplan_h

#include "linieneintrag.h"

#include "../tpl/minivec_tpl.h"


class cbuffer_t;
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

protected:
	schedule_t(waytype_t const waytype) :
		abgeschlossen(false),
		aktuell(0),
		my_waytype(waytype)
	{}

public:
	minivec_tpl<struct linieneintrag_t> eintrag;

	/**
	* sollte eine Fehlermeldung ausgeben, wenn halt nicht erlaubt ist
	* @author Hj. Malthaner
	*/
	virtual char const* fehlermeldung() const = 0;

	/**
	* der allgemeine Fahrplan erlaubt haltestellen überall.
	* diese Methode sollte in den unterklassen redefiniert werden.
	* @author Hj. Malthaner
	*/
	virtual bool ist_halt_erlaubt(const grund_t *gr) const;

	bool empty() const { return eintrag.empty(); }

	uint8 get_count() const { return eintrag.get_count(); }

	virtual schedule_type get_type() const = 0;

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

	schedule_t(waytype_t, loadsave_t*);

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

	// fills the given buffer with a schedule
	void sprintf_schedule( cbuffer_t &buf ) const;

	// converts this string into a schedule
	bool sscanf_schedule( const char * );

private:
	bool  abgeschlossen;
	uint8 aktuell;

	waytype_t const my_waytype;

	static struct linieneintrag_t dummy_eintrag;
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
	zugfahrplan_t() : schedule_t(track_wt) {}
	zugfahrplan_t(loadsave_t* const file) : schedule_t(track_wt, file) {}
	schedule_t* copy() { schedule_t *s = new zugfahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "Zughalt muss auf\nSchiene liegen!\n"; }

	schedule_type get_type() const { return zugfahrplan; }

protected:
	zugfahrplan_t(waytype_t const way) : schedule_t(way) {}
	zugfahrplan_t(waytype_t const way, loadsave_t* const file) : schedule_t(way, file) {}
};

/* the schedule for monorail ...
 * @author Hj. Malthaner
 */
class tramfahrplan_t : public zugfahrplan_t
{
public:
	tramfahrplan_t() : zugfahrplan_t(tram_wt) {}
	tramfahrplan_t(loadsave_t* const file) : zugfahrplan_t(tram_wt, file) {}
	schedule_t* copy() { schedule_t *s = new tramfahrplan_t(); s->copy_from(this); return s; }

	schedule_type get_type() const { return tramfahrplan; }
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
	autofahrplan_t() : schedule_t(road_wt) {}
	autofahrplan_t(loadsave_t* const file) : schedule_t(road_wt, file) {}
	schedule_t* copy() { schedule_t *s = new autofahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "Autohalt muss auf\nStrasse liegen!\n"; }

	schedule_type get_type() const { return autofahrplan; }
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
	schifffahrplan_t() : schedule_t(water_wt) {}
	schifffahrplan_t(loadsave_t* const file) : schedule_t(water_wt, file) {}
	schedule_t* copy() { schedule_t *s = new schifffahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "Schiffhalt muss im\nWasser liegen!\n"; }

	schedule_type get_type() const { return schifffahrplan; }
};


/* the schedule for air ...
 * @author Hj. Malthaner
 */
class airfahrplan_t : public schedule_t
{
public:
	airfahrplan_t() : schedule_t(air_wt) {}
	airfahrplan_t(loadsave_t* const file) : schedule_t(air_wt, file) {}
	schedule_t* copy() { schedule_t *s = new airfahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "Flugzeughalt muss auf\nRunway liegen!\n"; }

	schedule_type get_type() const { return airfahrplan; }
};

/* the schedule for monorail ...
 * @author Hj. Malthaner
 */
class monorailfahrplan_t : public schedule_t
{
public:
	monorailfahrplan_t() : schedule_t(monorail_wt) {}
	monorailfahrplan_t(loadsave_t* const file) : schedule_t(monorail_wt, file) {}
	schedule_t* copy() { schedule_t *s = new monorailfahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "Monorailhalt muss auf\nMonorail liegen!\n"; }

	schedule_type get_type() const { return monorailfahrplan; }
};

/* the schedule for maglev ...
 * @author Hj. Malthaner
 */
class maglevfahrplan_t : public schedule_t
{
public:
	maglevfahrplan_t() : schedule_t(maglev_wt) {}
	maglevfahrplan_t(loadsave_t* const file) : schedule_t(maglev_wt, file) {}
	schedule_t* copy() { schedule_t *s = new maglevfahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "Maglevhalt muss auf\nMaglevschiene liegen!\n"; }

	schedule_type get_type() const { return maglevfahrplan; }
};

/* and narrow guage ...
 * @author Hj. Malthaner
 */
class narrowgaugefahrplan_t : public schedule_t
{
public:
	narrowgaugefahrplan_t() : schedule_t(narrowgauge_wt) {}
	narrowgaugefahrplan_t(loadsave_t* const file) : schedule_t(narrowgauge_wt, file) {}
	schedule_t* copy() { schedule_t *s = new narrowgaugefahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "On narrowgauge track only!\n"; }

	schedule_type get_type() const { return narrowgaugefahrplan; }
};


#endif
