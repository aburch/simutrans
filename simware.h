#ifndef simware_h
#define simware_h

#include "halthandle_t.h"
#include "dataobj/koord.h"
#include "descriptor/goods_desc.h"

class goods_manager_t;
class karte_t;
class player_t;

/** Class to handle goods packets (and their destinations) */
class ware_t
{
	friend class goods_manager_t;

private:
	/// private lookup table to speedup
	static const goods_desc_t *index_to_desc[256];

public:
	/// amount of goods
	uint32 menge;

	/// type of good, used as index into index_to_desc
	uint32 index: 8;

	// Necessary to determine whether to book 
	// jobs taken on arrival.
	bool is_commuting_trip : 1;

	/// The class of mail/passengers. Not used for goods.
	uint8 g_class;

	/// The percentage of the maximum tolerable journey time for 
	/// any given level of comfort that this packet of passengers 
	/// (if passengers) will travel in a lower class of accommodation
	/// than available on a convoy.
	uint16 comfort_preference_percentage;

private:
	/**
	 * Handle of target station.
	 * @author Hj. Malthaner
	 */
	halthandle_t ziel;

	/**
	 * Handle of station, where the packet has to leave convoy.
	 * @author Hj. Malthaner
	 */
	halthandle_t zwischenziel;

	/**
	 * A handle to the ultimate origin.
	 * @author: jamespetts
	 */
	halthandle_t origin;

	/**
	 * A handle to the previous transfer
	 * for the purposes of distance calculation
	 * @author: jamespetts, November 2011
	 */
	halthandle_t last_transfer;

	/**
	 * Target position (factory, etc)
	 * 
	 * "the final target position, which is on behalf 
	 * not the goal stop position"
	 *
	 * @author Hj. Malthaner
	 */
	koord zielpos;

	/**
	 * Update target (zielpos) for factory-going goods (after loading or rotating)
	 */
	void update_factory_target();

public:
	inline const halthandle_t &get_ziel() const { return ziel; }
	void set_ziel(const halthandle_t &ziel) { this->ziel = ziel; }

	inline const halthandle_t &get_zwischenziel() const { return zwischenziel; }
	inline halthandle_t &access_zwischenziel() { return zwischenziel; }
	void set_zwischenziel(const halthandle_t &zwischenziel) { this->zwischenziel = zwischenziel; }

	koord get_zielpos() const { return zielpos; }
	void set_zielpos(const koord zielpos) { this->zielpos = zielpos; }

	void reset() { menge = 0; ziel = zwischenziel = origin = last_transfer = halthandle_t(); zielpos = koord::invalid; }

	ware_t();
	ware_t(const goods_desc_t *typ);
	ware_t(const goods_desc_t *typ, halthandle_t o);
	ware_t(loadsave_t *file);

	/**
	 * gibt den nicht-uebersetzten warennamen zurück
	 * @author Hj. Malthaner
	 * "There the non-translated names were back"
	 */
	inline const char *get_name() const { return get_desc()->get_name(); }
	inline const char *get_mass() const { return get_desc()->get_mass(); }
	inline uint8 get_catg() const { return get_desc()->get_catg(); }
	inline uint8 get_index() const { return index; }
	inline uint8 get_class() const { return g_class; }
	inline void set_class(uint8 value) { g_class = value; }

	//@author: jamespetts
	inline halthandle_t get_origin() const { return origin; }
	void set_origin(halthandle_t value) { origin = value; }
	inline halthandle_t get_last_transfer() const { return last_transfer; }
	void set_last_transfer(halthandle_t value) { last_transfer = value; }

	inline const goods_desc_t* get_desc() const { return index_to_desc[index]; }
	void set_desc(const goods_desc_t* type);

	void rdwr(loadsave_t *file);

	void finish_rd(karte_t *welt);

	// find out the category ...
	inline bool is_passenger() const { return index == 0; }
	inline bool is_mail() const { return index == 1; }
	inline bool is_freight() const { return index > 2; }

	// The time at which this packet arrived at the current station
	// @author: jamespetts
	sint64 arrival_time;

	int operator==(const ware_t &w) {
		return	menge == w.menge &&
			zwischenziel == w.zwischenziel &&
			arrival_time == w.arrival_time &&
			index == w.index  &&
			ziel == w.ziel  &&
			zielpos == w.zielpos &&
			origin == w.origin &&
			last_transfer == w.last_transfer &&
			g_class == w.g_class;
	}

	bool can_merge_with(const ware_t &w)
	{
		return zwischenziel == w.zwischenziel &&
			index == w.index  &&
			ziel == w.ziel  &&
			zielpos == w.zielpos &&
			origin == w.origin &&
			last_transfer == w.last_transfer &&
			g_class == w.g_class;
	}

	bool operator <= (const ware_t &w)
	{
		// Used only for the binary heap
		return arrival_time <= w.arrival_time;
	}

	int operator!=(const ware_t &w) { return !(*this == w); }

	/**
	 * Adjust target coordinates.
	 * Must be called after factories have been rotated!
	 */
	void rotate90( sint16 y_size );
};

#endif
