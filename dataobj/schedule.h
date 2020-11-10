/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_SCHEDULE_H
#define DATAOBJ_SCHEDULE_H


#include "schedule_entry.h"

#include "../halthandle_t.h"

#include "../tpl/minivec_tpl.h"
#include "../tpl/koordhashtable_tpl.h"

#include "../simskin.h"
#include "../display/simimg.h"
#include "../descriptor/skin_desc.h"

#define TIMES_HISTORY_SIZE 3

static const char* const schedule_type_text[] =
{
	"All", "Truck", "Train", "Ship", "Air", "Monorail", "Tram", "Maglev", "Narrowgauge"
};

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
		schedule = 0, truck_schedule = 1, train_schedule = 2, ship_schedule = 3, airplane_schedule = 4, monorail_schedule = 5, tram_schedule = 6, maglev_schedule = 7, narrowgauge_schedule = 8,
	};

protected:
	schedule_t() : editing_finished(false), bidirectional(false), mirrored(false), same_spacing_shift(true), current_stop(0), spacing(0) {}

public:
	minivec_tpl<schedule_entry_t> entries;

	/**
	* sollte eine Fehlermeldung ausgeben, wenn halt nicht erlaubt ist
	* @author Hj. Malthaner
	*/
	virtual char const* get_error_msg() const = 0;

	/**
	* der allgemeine Fahrplan erlaubt haltestellen überall.
	* diese Methode sollte in den unterklassen redefiniert werden.
	* @author Hj. Malthaner
	*/
	virtual bool is_stop_allowed(const grund_t *gr) const;

	bool empty() const { return entries.empty(); }

	uint8 get_count() const { return entries.get_count(); }

	virtual schedule_type get_type() const = 0;

	virtual waytype_t get_waytype() const = 0;

	/**
	* get current stop of the schedule (schedule)
	* @author hsiegeln
	*/
	uint8 get_current_stop() const { return current_stop; }

	// always returns a valid entry to the current stop
	schedule_entry_t const& get_current_entry() const { return current_stop >= entries.get_count() ? dummy_entry : entries[current_stop]; }

private:
	/**
	 * Fix up current_stop value, which we may have made out of range
	 * @author neroden
	 */
	void make_current_stop_valid() {
		uint8 count = entries.get_count();
		if(  count == 0  ) {
			current_stop = 0;
		}
		else if(  current_stop >= count  ) {
			current_stop = count-1;
		}
	}

public:
	/**
	 * set the current stop of the schedule (schedule)
	 * if new value is bigger than stops available, the max stop will be used
	 * @author hsiegeln
	 */
	void set_current_stop(uint8 new_current_stop) {
		current_stop = new_current_stop;
		make_current_stop_valid();
	}

	// advance entry by one ...
	void advance() {
		if(  !entries.empty()  ) {
			current_stop = (current_stop+1)%entries.get_count();
		}
	}
	// decrement entry by one
	void advance_reverse() {
		if(  !entries.empty()  ) {
			current_stop = current_stop ? current_stop-1 : entries.get_count()-1;
		}
	}

	/**
	 * Sets the current entry to a reversing type
	 */
	void set_reverse(sint8 reverse = 1, sint16 index = -1)
	{
		uint8 inx = index == -1 ? current_stop : (uint8)index;
 		if(!entries.empty())
		{
			entries[inx].reverse = reverse;
		}
	}

	/**
	 * Increment or decrement the given index according to the given direction.
	 * Also switches the direction if necessary.
	 * @author yobbobandana
	 */
	void increment_index(uint8 *index, bool *reversed) const;

	/**
	 * Same as increment_index(), but skips waypoints.
	 * @author suitougreentea
	 */
	void increment_index_until_next_halt(player_t* player, uint8 *index, bool *reversed) const;

	/***
	 * "Completed"
	 */
	inline bool is_editing_finished() const { return editing_finished; }
	void finish_editing() { editing_finished = true; }
	void start_editing() { editing_finished = false; }
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
	 * fügt eine koordinate an stelle current_stop in den Fahrplan ein
	 * all folgenden Koordinaten verschieben sich dadurch
	 */
	bool insert(const grund_t* gr, uint16 minimum_loading = 0, uint8 waiting_time_shift = 0, sint16 spacing_shift = 0, bool wait_for_time = false, bool show_failure = false);
	/**
	 * hängt eine koordinate an den schedule an
	 */
	bool append(const grund_t* gr, uint16 minimum_loading = 0, uint8 waiting_time_shift = 0, sint16 spacing_shift = 0, bool wait_for_time = false);

	// cleanup a schedule, removes double entries
	void cleanup();

	/**
	 * entfern entries[current_stop] aus dem schedule
	 * all folgenden Koordinaten verschieben sich dadurch
	 */
	bool remove();

	void rdwr(loadsave_t *file);

	void rotate90( sint16 y_size );

	/**
	 * if the passed in schedule matches "this", then return true
	 * @author hsiegeln
	 */
	bool matches(karte_t *welt, const schedule_t *schedule);

	inline bool is_bidirectional() const { return bidirectional; }
	inline bool is_mirrored() const { return mirrored; }
	inline bool is_same_spacing_shift() const { return same_spacing_shift; }
	void set_bidirectional(bool bidirec = true ) { bidirectional = bidirec; }
	void set_mirrored(bool mir = true ) { mirrored = mir; }
	void set_same_spacing_shift(bool s = true) { same_spacing_shift = s; }

	/*
	 * compare this schedule with another, ignoring order and exact positions and waypoints
	 * @author prissi
	 */
	bool similar( const schedule_t *schedule, const player_t *player );

	/**
	 * calculates a return way for this schedule
	 * will add elements 1 to maxi-1 in reverse order to schedule
	 * @author hsiegeln
	 */
	void add_return_way();

	virtual schedule_t* copy() = 0;//{ return new schedule_t(this); }

	// copy all entries from schedule src to this and adjusts current_stop
	void copy_from(const schedule_t *src);

	// fills the given buffer with a schedule
	void sprintf_schedule( cbuffer_t &buf ) const;

	// converts this string into a schedule
	bool sscanf_schedule( const char * );

	/** Checks whetehr the given stop is contained in the schedule
	 * @author: jamespetts, September 2011
	 */
	bool is_contained(koord3d pos);

	image_id get_schedule_type_symbol() const
	{
		switch (get_type())
		{
			case schedule_t::truck_schedule:
				return skinverwaltung_t::autohaltsymbol->get_image_id(0);
			case schedule_t::train_schedule:
				return skinverwaltung_t::zughaltsymbol->get_image_id(0);
			case schedule_t::ship_schedule:
				return skinverwaltung_t::schiffshaltsymbol->get_image_id(0);
			case schedule_t::airplane_schedule:
				return skinverwaltung_t::airhaltsymbol->get_image_id(0);
			case schedule_t::monorail_schedule:
				return skinverwaltung_t::monorailhaltsymbol->get_image_id(0);
			case schedule_t::tram_schedule:
				return skinverwaltung_t::tramhaltsymbol->get_image_id(0);
			case schedule_t::maglev_schedule:
				return skinverwaltung_t::maglevhaltsymbol->get_image_id(0);
			case schedule_t::narrowgauge_schedule:
				return skinverwaltung_t::narrowgaugehaltsymbol->get_image_id(0);
			case schedule_t::schedule:
			default:
				return IMG_EMPTY;
				break;
		}
		return IMG_EMPTY;
	}

	inline char const *get_schedule_type_name() const
	{
		return schedule_type_text[get_type()];
	};

private:
	bool editing_finished;
	bool bidirectional;
	bool mirrored;
	bool same_spacing_shift;
	uint8 current_stop;
	sint16 spacing;

	static schedule_entry_t dummy_entry;
};

/**
* Schedules with stops on tracks.
*
* @author Hj. Malthaner
*/
class train_schedule_t : public schedule_t
{
public:
	train_schedule_t() {}
	train_schedule_t(loadsave_t* const file) : schedule_t(file) {}
	schedule_t* copy() { schedule_t *s = new train_schedule_t(); s->copy_from(this); return s; }
	const char *get_error_msg() const { return "Zughalt muss auf\nSchiene liegen!\n"; }

	schedule_type get_type() const { return train_schedule; }

	waytype_t get_waytype() const { return track_wt; }
};

/* the schedule for monorail ...
 * @author Hj. Malthaner
 */
class tram_schedule_t : public train_schedule_t
{
public:
	tram_schedule_t() {}
	tram_schedule_t(loadsave_t* const file) : train_schedule_t(file) {}
	schedule_t* copy() { schedule_t *s = new tram_schedule_t(); s->copy_from(this); return s; }

	schedule_type get_type() const { return tram_schedule; }

	waytype_t get_waytype() const { return tram_wt; }
};


/**
* Schedules with stops on roads.
*
* @author Hj. Malthaner
*/
class truck_schedule_t : public schedule_t
{
public:
	truck_schedule_t() {}
	truck_schedule_t(loadsave_t* const file) : schedule_t(file) {}
	schedule_t* copy() { schedule_t *s = new truck_schedule_t(); s->copy_from(this); return s; }
	const char *get_error_msg() const { return "Autohalt muss auf\nStrasse liegen!\n"; }

	schedule_type get_type() const { return truck_schedule; }

	waytype_t get_waytype() const { return road_wt; }
};


/**
* Schedules with stops on water.
*
* @author Hj. Malthaner
*/
class ship_schedule_t : public schedule_t
{
public:
	ship_schedule_t() {}
	ship_schedule_t(loadsave_t* const file) : schedule_t(file) {}
	schedule_t* copy() { schedule_t *s = new ship_schedule_t(); s->copy_from(this); return s; }
	const char *get_error_msg() const { return "Schiffhalt muss im\nWasser liegen!\n"; }

	schedule_type get_type() const { return ship_schedule; }

	waytype_t get_waytype() const { return water_wt; }
};


/**
* Schedules for airplanes.
*
* @author Hj. Malthaner
*/
class airplane_schedule_ : public schedule_t
{
public:
	airplane_schedule_() {}
	airplane_schedule_(loadsave_t* const file) : schedule_t(file) {}
	schedule_t* copy() { schedule_t *s = new airplane_schedule_(); s->copy_from(this); return s; }
	const char *get_error_msg() const { return "Flugzeughalt muss auf\nRunway liegen!\n"; }

	schedule_type get_type() const { return airplane_schedule; }

	waytype_t get_waytype() const { return air_wt; }
};

/**
* Schedules with stops on mono-rails.
* @author Hj. Malthaner
*/
class monorail_schedule_t : public schedule_t
{
public:
	monorail_schedule_t() {}
	monorail_schedule_t(loadsave_t* const file) : schedule_t(file) {}
	schedule_t* copy() { schedule_t *s = new monorail_schedule_t(); s->copy_from(this); return s; }
	const char *get_error_msg() const { return "Monorailhalt muss auf\nMonorail liegen!\n"; }

	schedule_type get_type() const { return monorail_schedule; }

	waytype_t get_waytype() const { return monorail_wt; }
};

/**
* Schedules with stops on maglev tracks.
* @author Hj. Malthaner
*/
class maglev_schedule_t : public schedule_t
{
public:
	maglev_schedule_t() {}
	maglev_schedule_t(loadsave_t* const file) : schedule_t(file) {}
	schedule_t* copy() { schedule_t *s = new maglev_schedule_t(); s->copy_from(this); return s; }
	const char *get_error_msg() const { return "Maglevhalt muss auf\nMaglevschiene liegen!\n"; }

	schedule_type get_type() const { return maglev_schedule; }

	waytype_t get_waytype() const { return maglev_wt; }
};

/**
* Schedules with stops on narrowgauge tracks.
*
* @author Hj. Malthaner
*/
class narrowgauge_schedule_t : public schedule_t
{
public:
	narrowgauge_schedule_t() {}
	narrowgauge_schedule_t(loadsave_t* const file) : schedule_t(file) {}
	schedule_t* copy() { schedule_t *s = new narrowgauge_schedule_t(); s->copy_from(this); return s; }
	const char *get_error_msg() const { return "On narrowgauge track only!\n"; }

	schedule_type get_type() const { return narrowgauge_schedule; }

	waytype_t get_waytype() const { return narrowgauge_wt; }
};

class departure_point_t
{
public:
	union { sint16 entry;    sint16 x; };
	union { sint16 reversed; sint16 y; };

	departure_point_t(uint8 e, bool rev)
	{
		entry = e;
		reversed = rev;
	}

	departure_point_t()
	{
		entry = 0;
		reversed = 0;
	}
};

static inline bool operator == (const departure_point_t &a, const departure_point_t &b)
{
	// only this works with O3 optimisation!
	return (a.entry - b.entry) == 0 && a.reversed == b.reversed;
}

static inline bool operator != (const departure_point_t &a, const departure_point_t &b)
{
	// only this works with O3 optimisation!
	return (a.entry - b.entry) != 0 || a.reversed != b.reversed;
}

static inline bool operator == (const departure_point_t& a, int b)
{
	// For hashtable use.
	return b == 0 && a == departure_point_t(0, true);
}

class times_history_data_t {
private:
	uint32 history[TIMES_HISTORY_SIZE];
public:
	times_history_data_t();
	uint32 get_entry(uint16 index) const;
	void put(uint32 time);
	void set(uint16 index, uint32 time);
    uint32 get_average_seconds() const;
};

typedef koordhashtable_tpl<departure_point_t, times_history_data_t> times_history_map;

#endif
