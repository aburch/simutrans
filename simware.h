#ifndef simware_h
#define simware_h

#include "halthandle_t.h"
#include "dataobj/koord.h"
#include "bauer/warenbauer.h"
#include "besch/ware_besch.h"

class ware_besch_t;
class karte_t;

/** Eine Klasse zur Verwaltung von Informationen ueber Fracht und Waren */
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
	 * Koordinate der Zielhaltestelle
	 * @author Hj. Malthaner
	 */
	halthandle_t ziel;

	/**
	 * Koordinte des nächsten Zwischenstops
	 * @author Hj. Malthaner
	 */
	halthandle_t zwischenziel;

	/**
	 * die engültige Zielposition,
	 * das ist i.a. nicht die Zielhaltestellenposition
	 * @author Hj. Malthaner
	 */
	koord zielpos;

public:
	halthandle_t get_ziel() const { return ziel; }
	void set_ziel(halthandle_t ziel) { this->ziel = ziel; }

	halthandle_t get_zwischenziel() const { return zwischenziel; }
	void set_zwischenziel(halthandle_t zwischenziel) { this->zwischenziel = zwischenziel; }

	koord get_zielpos() const { return zielpos; }
	void set_zielpos(const koord zielpos) { this->zielpos = zielpos; }

	ware_t();
	ware_t(const ware_besch_t *typ);
	ware_t(karte_t *welt,loadsave_t *file);

	/**
	 * gibt den nicht-uebersetzten warennamen zurück
	 * @author Hj. Malthaner
	 */
	const char *get_name() const { return get_besch()->get_name(); }
	const char *get_mass() const { return get_besch()->get_mass(); }
	uint16 get_preis() const { return get_besch()->get_preis(); }
	uint8 get_catg() const { return get_besch()->get_catg(); }
	uint8 get_index() const { return index; }

	const ware_besch_t* get_besch() const { return index_to_besch[index]; }
	void set_besch(const ware_besch_t* type);

	void rdwr(karte_t *welt,loadsave_t *file);

	void laden_abschliessen(karte_t *welt);

	// find out the category ...
	bool is_passenger() const {  return index==0; }
	bool is_mail() const {  return index==1; }
	bool is_freight() const {  return index>2; }

	int operator==(const ware_t &w) {
		return index  == w.index  &&
			menge == w.menge &&
			ziel  == w.ziel  &&
			zwischenziel == w.zwischenziel &&
			zielpos == w.zielpos;
	}

	int operator!=(const ware_t &w) { return !(*this == w); 	}

	// mail and passengers just care about target station
	// freight needs to obey coordinates (since more than one factory might by connected!)
	inline bool same_destination(const ware_t &w) const {
		return index==w.get_index()  &&  ziel==w.get_ziel()  &&  (index<2  ||  zielpos==w.get_zielpos());
	}
};

#endif
