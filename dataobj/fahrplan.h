#ifndef fahrplan_h
#define fahrplan_h

#include "linieneintrag.h"

#include "../halthandle_t.h"

#include "../tpl/minivec_tpl.h"


class cbuffer_t;
class grund_t;
class player_t;
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
	schedule_t() : abgeschlossen(false), bidirectional(false), mirrored(false), same_spacing_shift(true), aktuell(0), spacing(0) {}

public:
	minivec_tpl<linieneintrag_t> eintrag;

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

	virtual waytype_t get_waytype() const = 0;

	/**
	* get current stop of the schedule (fahrplan)
	* @author hsiegeln
	*/
	uint8 get_aktuell() const { return aktuell; }

	// always returns a valid entry to the current stop
	linieneintrag_t const& get_current_eintrag() const { return aktuell >= eintrag.get_count() ? dummy_eintrag : eintrag[aktuell]; }

private:
	/**
	 * Fix up aktuell value, which we may have made out of range
	 * @author neroden
	 */
	void make_aktuell_valid() {
		uint8 count = eintrag.get_count();
		if(  count == 0  ) {
			aktuell = 0;
		}
		else if(  aktuell >= count  ) {
			aktuell = count-1;
		}
	}

public:
	/**
	 * set the current stop of the schedule (fahrplan)
	 * if new value is bigger than stops available, the max stop will be used
	 * @author hsiegeln
	 */
	void set_aktuell(uint8 new_aktuell) {
		aktuell = new_aktuell;
		make_aktuell_valid();
	}

	// advance entry by one ...
	void advance() {
		if(  !eintrag.empty()  ) {
			aktuell = (aktuell+1)%eintrag.get_count();
		}
	}
	// decrement entry by one
	void advance_reverse() {
		if(  !eintrag.empty()  ) {
			aktuell = aktuell ? aktuell-1 : eintrag.get_count()-1;
		}
	}

	/**
	 * Sets the current entry to a reversing type
	 */
	void set_reverse(sint8 reverse = 1, sint16 index = -1)
	{
		uint8 inx = index == -1 ? aktuell : (uint8)index;
 		if(!eintrag.empty())
		{
			eintrag[inx].reverse = reverse;
		}
	}

	/**
	 * Increment or decrement the given index according to the given direction.
	 * Also switches the direction if necessary.
	 * @author yobbobandana
	 */
	void increment_index(uint8 *index, bool *reversed) const;

	/***
	 * "Completed"
	 */
	inline bool ist_abgeschlossen() const { return abgeschlossen; }
	void eingabe_abschliessen() { abgeschlossen = true; } // "Input completed"
	void eingabe_beginnen() { abgeschlossen = false; }
	inline int get_spacing() const { return spacing; }
	inline void set_spacing( int s ) { spacing = s; }

	virtual ~schedule_t() {}

	schedule_t(loadsave_t*);

	/**
	 * returns a halthandle for the next halt in the schedule (or unbound)
	 */
	halthandle_t get_next_halt( player_t *player, halthandle_t halt ) const;

	/**
	 * returns a halthandle for the previous halt in the schedule (or unbound)
	 */
	halthandle_t get_prev_halt( player_t *player ) const;

	/**
	 * fügt eine koordinate an stelle aktuell in den Fahrplan ein
	 * alle folgenden Koordinaten verschieben sich dadurch
	 */
	bool insert(const grund_t* gr, uint16 ladegrad = 0, uint8 waiting_time_shift = 0, sint16 spacing_shift = 0, bool wait_for_time = false, bool show_failure = false);
	/**
	 * hängt eine koordinate an den fahrplan an
	 */
	bool append(const grund_t* gr, uint16 ladegrad = 0, uint8 waiting_time_shift = 0, sint16 spacing_shift = 0, bool wait_for_time = false);

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

	inline bool is_bidirectional() const { return bidirectional; }
	inline bool is_mirrored() const { return mirrored; }
	inline bool is_same_spacing_shift() const { return same_spacing_shift; }
	void set_bidirectional(bool bidirec = true ) { bidirectional = bidirec; }
	void set_mirrored(bool mir = true ) { mirrored = mir; }
	void set_same_spacing_shift(bool s = true) { same_spacing_shift = s; }

	/*
	 * compare this fahrplan with another, ignoring order and exact positions and waypoints
	 * @author prissi
	 */
	bool similar( const schedule_t *fpl, const player_t *player );

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

	/** Checks whetehr the given stop is contained in the schedule
	 * @author: jamespetts, September 2011
	 */
	bool is_contained (koord3d pos);

private:
	bool abgeschlossen;
	bool bidirectional;
	bool mirrored;
	bool same_spacing_shift;
	uint8 aktuell;
	sint16 spacing;

	static linieneintrag_t dummy_eintrag;
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
	zugfahrplan_t() {}
	zugfahrplan_t(loadsave_t* const file) : schedule_t(file) {}
	schedule_t* copy() { schedule_t *s = new zugfahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "Zughalt muss auf\nSchiene liegen!\n"; }

	schedule_type get_type() const { return zugfahrplan; }

	waytype_t get_waytype() const { return track_wt; }
};

/* the schedule for monorail ...
 * @author Hj. Malthaner
 */
class tramfahrplan_t : public zugfahrplan_t
{
public:
	tramfahrplan_t() {}
	tramfahrplan_t(loadsave_t* const file) : zugfahrplan_t(file) {}
	schedule_t* copy() { schedule_t *s = new tramfahrplan_t(); s->copy_from(this); return s; }

	schedule_type get_type() const { return tramfahrplan; }

	waytype_t get_waytype() const { return tram_wt; }
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
	autofahrplan_t() {}
	autofahrplan_t(loadsave_t* const file) : schedule_t(file) {}
	schedule_t* copy() { schedule_t *s = new autofahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "Autohalt muss auf\nStrasse liegen!\n"; }

	schedule_type get_type() const { return autofahrplan; }

	waytype_t get_waytype() const { return road_wt; }
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
	schifffahrplan_t() {}
	schifffahrplan_t(loadsave_t* const file) : schedule_t(file) {}
	schedule_t* copy() { schedule_t *s = new schifffahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "Schiffhalt muss im\nWasser liegen!\n"; }

	schedule_type get_type() const { return schifffahrplan; }

	waytype_t get_waytype() const { return water_wt; }
};


/* the schedule for air ...
 * @author Hj. Malthaner
 */
class airfahrplan_t : public schedule_t
{
public:
	airfahrplan_t() {}
	airfahrplan_t(loadsave_t* const file) : schedule_t(file) {}
	schedule_t* copy() { schedule_t *s = new airfahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "Flugzeughalt muss auf\nRunway liegen!\n"; }

	schedule_type get_type() const { return airfahrplan; }

	waytype_t get_waytype() const { return air_wt; }
};

/* the schedule for monorail ...
 * @author Hj. Malthaner
 */
class monorailfahrplan_t : public schedule_t
{
public:
	monorailfahrplan_t() {}
	monorailfahrplan_t(loadsave_t* const file) : schedule_t(file) {}
	schedule_t* copy() { schedule_t *s = new monorailfahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "Monorailhalt muss auf\nMonorail liegen!\n"; }

	schedule_type get_type() const { return monorailfahrplan; }

	waytype_t get_waytype() const { return monorail_wt; }
};

/* the schedule for maglev ...
 * @author Hj. Malthaner
 */
class maglevfahrplan_t : public schedule_t
{
public:
	maglevfahrplan_t() {}
	maglevfahrplan_t(loadsave_t* const file) : schedule_t(file) {}
	schedule_t* copy() { schedule_t *s = new maglevfahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "Maglevhalt muss auf\nMaglevschiene liegen!\n"; }

	schedule_type get_type() const { return maglevfahrplan; }

	waytype_t get_waytype() const { return maglev_wt; }
};

/* and narrow guage ...
 * @author Hj. Malthaner
 */
class narrowgaugefahrplan_t : public schedule_t
{
public:
	narrowgaugefahrplan_t() {}
	narrowgaugefahrplan_t(loadsave_t* const file) : schedule_t(file) {}
	schedule_t* copy() { schedule_t *s = new narrowgaugefahrplan_t(); s->copy_from(this); return s; }
	const char *fehlermeldung() const { return "On narrowgauge track only!\n"; }

	schedule_type get_type() const { return narrowgaugefahrplan; }

	waytype_t get_waytype() const { return narrowgauge_wt; }
};

#endif
