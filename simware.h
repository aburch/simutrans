#ifndef simware_h
#define simware_h

#include "halthandle_t.h"
#include "dataobj/koord.h"
#include "besch/ware_besch.h"

class warenbauer_t;
class karte_t;
class spieler_t;

/** Eine Klasse zur Verwaltung von Informationen ueber Fracht und Waren */
// "A class for the management of information on cargo and goods" (Google translations)
class ware_t
{
	friend class warenbauer_t;

private:
	// private lookup table to sppedup
	static const ware_besch_t *index_to_besch[256];

public:
	uint32 index: 8;

	uint32 menge : 23;

	/**
	 * To indicate that the ware's destination is a factory/consumer store
	 * @author Knightly
	 */
	uint32 to_factory : 1;

private:
	/**
	 * Koordinate der Zielhaltestelle ("Coordinate of the goal stop" - Babelfish). 
	 * @author Hj. Malthaner
	 */
	halthandle_t ziel;

	/**
	 * Koordinte des nächsten Zwischenstops ("Co-ordinate of the next stop")
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
	 * die engültige Zielposition,
	 * das ist i.a. nicht die Zielhaltestellenposition
	 * 
	 * "the final target position, which is on behalf 
	 * not the goal stop position"
	 * @author Hj. Malthaner
	 */
	koord zielpos;

public:
	const halthandle_t &get_ziel() const { return ziel; }
	void set_ziel(const halthandle_t &ziel) { this->ziel = ziel; }

	const halthandle_t &get_zwischenziel() const { return zwischenziel; }
	halthandle_t &access_zwischenziel() { return zwischenziel; }
	void set_zwischenziel(const halthandle_t &zwischenziel) { this->zwischenziel = zwischenziel; }

	koord get_zielpos() const { return zielpos; }
	void set_zielpos(const koord zielpos) { this->zielpos = zielpos; }

	ware_t();
	ware_t(const ware_besch_t *typ);
	ware_t(const ware_besch_t *typ, halthandle_t o);
	ware_t(karte_t *welt,loadsave_t *file);

	/**
	 * gibt den nicht-uebersetzten warennamen zurück
	 * @author Hj. Malthaner
	 * "There the non-translated names were back"
	 */
	const char *get_name() const { return get_besch()->get_name(); }
	const char *get_mass() const { return get_besch()->get_mass(); }
	uint16 get_fare(uint32 distance, uint32 starting_distance = 0) const { return (uint16) get_besch()->get_fare(distance, starting_distance); }
	uint8 get_catg() const { return get_besch()->get_catg(); }
	uint8 get_index() const { return index; }

	//@author: jamespetts
	halthandle_t get_origin() const { return origin; }
	void set_origin(halthandle_t value) { origin = value; }
	halthandle_t get_last_transfer() const { return last_transfer; }
	void set_last_transfer(halthandle_t value) { last_transfer = value; }

	const ware_besch_t* get_besch() const { return index_to_besch[index]; }
	void set_besch(const ware_besch_t* type);

	void rdwr(karte_t *welt,loadsave_t *file);

	void laden_abschliessen(karte_t *welt,spieler_t *sp);

	// find out the category ...
	bool is_passenger() const { return index == 0; }
	bool is_mail() const { return index == 1; }
	bool is_freight() const { return index > 2; }

	// The time at which this packet arrived at the current station
	// @author: jamespetts
	sint64 arrival_time;

	int operator==(const ware_t &w) {
		return	menge == w.menge &&
			zwischenziel == w.zwischenziel &&
			arrival_time == w.arrival_time &&
			index  == w.index  &&
			ziel  == w.ziel  &&
			zielpos == w.zielpos &&
			origin == w.origin && 
			last_transfer == w.last_transfer;
	}

	// Lighter version of operator == that only checks equality
	// of metrics needed for merging.
	inline bool can_merge_with (const ware_t &w) const
	{
		return index  == w.index  &&
			to_factory == w.to_factory &&
			ziel  == w.ziel  &&
			// Only merge the destination *position* if the load is freight (since more than one factory might by connected!)
			(!to_factory  ||  zielpos==w.get_zielpos()) &&
			origin == w.origin &&
			last_transfer == w.last_transfer;
	}

	int operator!=(const ware_t &w) { return !(*this == w); 	}
};

#endif
