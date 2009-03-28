#ifndef simware_h
#define simware_h

#include "halthandle_t.h"
#include "dataobj/koord.h"
#include "bauer/warenbauer.h"
#include "besch/ware_besch.h"

class ware_besch_t;
class karte_t;

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

	uint32 menge : 24;

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

	//@author: jamespetts
	//A handle to the ultimate origin.
	halthandle_t origin;

	/**
	 * die engültige Zielposition,
	 * das ist i.a. nicht die Zielhaltestellenposition
	 * @author Hj. Malthaner
	 */
	// "the final target position, which is on behalf not the goal stop position"
	koord zielpos;

	//@author: jamespetts
	//The number of remaining steps on this packet's journey.
	uint8 journey_steps;

	// @author: jamespetts
	// The distance travelled so far this leg of the journey.
	uint32 accumulated_distance;

public:
	const halthandle_t &get_ziel() const { return ziel; }
	void set_ziel(const halthandle_t &ziel) { this->ziel = ziel; }

	const halthandle_t &get_zwischenziel() const { return zwischenziel; }
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
	uint16 get_preis() const { return get_besch()->get_preis(); }
	uint8 get_catg() const { return get_besch()->get_catg(); }
	uint8 get_index() const { return index; }

	//@author: jamespetts
	halthandle_t get_origin() const { return origin; }
	void set_origin(halthandle_t value) { origin = value; }

	//@author: jamespetts
	uint32 get_accumulated_distance() const { return accumulated_distance; }
	//void add_distance(uint32 distance) { accumulated_distance += distance; }
	void add_distance(uint32 distance);
	void reset_accumulated_distance() { accumulated_distance = 0; }

	const ware_besch_t* get_besch() const { return index_to_besch[index]; }
	void set_besch(const ware_besch_t* type);

	void rdwr(karte_t *welt,loadsave_t *file);

	void laden_abschliessen(karte_t *welt);

	// find out the category ...
	bool is_passenger() const {  return index==0; }
	bool is_mail() const {  return index==1; }
	bool is_freight() const {  return index>2; }

	int operator==(const ware_t &w) {
		return	menge == w.menge &&
			zwischenziel == w.zwischenziel &&
			// No need to repeat the position checking if
			// this will be done in can_merge_with.
			(index < 3 || zielpos == w.zielpos) &&
			can_merge_with(w);
	}

	// Lighter version of operator == that only checks equality
	// of metrics needed for merging.
	inline bool can_merge_with (const ware_t &w) const
	{
		return index  == w.index  &&
			ziel  == w.ziel  &&
			// Only merge the destination *position* if the load is not freight
			(index > 2 || zielpos == w.zielpos) &&
			origin == w.origin;
	}


	uint8 get_journey_steps() { return journey_steps; }
	void set_journey_steps(uint8 value) { journey_steps = value; }

	int operator!=(const ware_t &w) { return !(*this == w); 	}

	// mail and passengers just care about target station
	// freight needs to obey coordinates (since more than one factory might by connected!)
	inline bool same_destination(const ware_t &w) const {
		return index==w.get_index()  &&  ziel==w.get_ziel()  &&  (index<2  ||  zielpos==w.get_zielpos());
	}
};

#endif
