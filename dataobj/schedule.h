#ifndef schedule_h
#define schedule_h

#include "schedule_entry.h"

#include "../halthandle_t.h"

#include "../tpl/minivec_tpl.h"


class cbuffer_t;
class grund_t;
class player_t;
class karte_t;


/**
 * Class to hold schedule of vehicles in Simutrans.
 *
 * @author Hj. Malthaner
 */
class schedule_t
{
	bool  editing_finished;
	uint8 current_stop;

	static schedule_entry_t dummy_entry;

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

protected:
	schedule_t() : editing_finished(false), current_stop(0) {}

public:
	enum schedule_type {
		schedule = 0, truck_schedule = 1, train_schedule = 2, ship_schedule = 3, airplane_schedule = 4, monorail_schedule = 5, tram_schedule = 6, maglev_schedule = 7, narrowgauge_schedule = 8,
	};

	minivec_tpl<schedule_entry_t> entries;

	/**
	 * Returns error message if stops are not allowed
	 * @author Hj. Malthaner
	 */
	virtual char const* get_error_msg() const = 0;

	/**
	 * Returns true if this schedule allows stop at the
	 * given tile.
	 * @author Hj. Malthaner
	 */
	bool is_stop_allowed(const grund_t *gr) const;

	bool empty() const { return entries.empty(); }

	uint8 get_count() const { return entries.get_count(); }

	virtual schedule_type get_type() const = 0;

	virtual waytype_t get_waytype() const = 0;

	/**
	 * Get current stop of the schedule.
	 * @author hsiegeln
	 */
	uint8 get_current_stop() const { return current_stop; }

	/// returns the current stop, always a valid entry
	schedule_entry_t const& get_current_entry() const { return current_stop >= entries.get_count() ? dummy_entry : entries[current_stop]; }

	/**
	 * Set the current stop of the schedule .
	 * If new value is bigger than stops available, the max stop will be used.
	 * @author hsiegeln
	 */
	void set_current_stop(uint8 new_current_stop) {
		current_stop = new_current_stop;
		make_current_stop_valid();
	}

	/// advance current_stop by one
	void advance() {
		if(  !entries.empty()  ) {
			current_stop = (current_stop+1)%entries.get_count();
		}
	}

	inline bool is_editing_finished() const { return editing_finished; }
	void finish_editing() { editing_finished = true; }
	void start_editing() { editing_finished = false; }

	virtual ~schedule_t() {}

	/**
	 * returns a halthandle for the next halt in the schedule (or unbound)
	 */
	halthandle_t get_next_halt( player_t *player, halthandle_t halt ) const;

	/**
	 * returns a halthandle for the previous halt in the schedule (or unbound)
	 */
	halthandle_t get_prev_halt( player_t *player ) const;

	/**
	 * Inserts a coordinate at current_stop into the schedule.
	 */
	bool insert(const grund_t* gr, uint8 minimum_loading = 0, uint8 waiting_time_shift = 0);

	/**
	 * Appends a coordinate to the schedule.
	 */
	bool append(const grund_t* gr, uint8 minimum_loading = 0, uint8 waiting_time_shift = 0);

	/**
	 * Cleanup a schedule, removes double entries.
	 */
	void cleanup();

	/**
	 * Remove current_stop entry from the schedule.
	 */
	bool remove();

	void rdwr(loadsave_t *file);

	void rotate90( sint16 y_size );

	/**
	 * if the passed in schedule matches "this", then return true
	 * @author hsiegeln
	 */
	bool matches(karte_t *welt, const schedule_t *schedule);

	/**
	 * Compare this schedule with another, ignoring order and exact positions and waypoints.
	 * @author prissi
	 */
	bool similar( const schedule_t *schedule, const player_t *player );

	/**
	 * Calculates a return way for this schedule.
	 * Will add elements 1 to end in reverse order to schedule.
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

	/**
	 * Append description of entry to buf.
	 * If @p max_chars > 0 then append short version, without loading level and position.
	 */
	static void gimme_stop_name(cbuffer_t& buf, karte_t* welt, player_t const* player_, schedule_entry_t const& entry, int max_chars);
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
	schedule_t* copy() { schedule_t *s = new train_schedule_t(); s->copy_from(this); return s; }
	const char *get_error_msg() const { return "Zughalt muss auf\nSchiene liegen!\n"; }

	schedule_type get_type() const { return train_schedule; }

	waytype_t get_waytype() const { return track_wt; }
};

/**
 * Schedules with stops on tram tracks.
 * @author Hj. Malthaner
 */
class tram_schedule_t : public train_schedule_t
{
public:
	tram_schedule_t() {}
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
class airplane_schedule_t : public schedule_t
{
public:
	airplane_schedule_t() {}
	schedule_t* copy() { schedule_t *s = new airplane_schedule_t(); s->copy_from(this); return s; }
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
	schedule_t* copy() { schedule_t *s = new narrowgauge_schedule_t(); s->copy_from(this); return s; }
	const char *get_error_msg() const { return "On narrowgauge track only!\n"; }

	schedule_type get_type() const { return narrowgauge_schedule; }

	waytype_t get_waytype() const { return narrowgauge_wt; }
};


#endif
