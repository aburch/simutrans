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
	halthandle_t gib_ziel() const { return ziel; }
	void setze_ziel(halthandle_t ziel) { this->ziel = ziel; }

	halthandle_t gib_zwischenziel() const { return zwischenziel; }
	void setze_zwischenziel(halthandle_t zwischenziel) { this->zwischenziel = zwischenziel; }

	koord gib_zielpos() const { return zielpos; }
	void setze_zielpos(const koord zielpos) { this->zielpos = zielpos; }

	ware_t();
	ware_t(const ware_besch_t *typ);
	ware_t(karte_t *welt,loadsave_t *file);

	/**
	 * gibt den nicht-uebersetzten warennamen zurück
	 * @author Hj. Malthaner
	 */
	const char *gib_name() const { return gib_besch()->gib_name(); }
	const char *gib_mass() const { return gib_besch()->gib_mass(); }
	uint16 gib_preis() const { return gib_besch()->gib_preis(); }
	uint8 gib_catg() const { return gib_besch()->gib_catg(); }
	uint8 gib_index() const { return index; }

	const ware_besch_t* gib_besch() const { return index_to_besch[index]; }
	void setze_besch(const ware_besch_t* type);

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
		return index==w.gib_index()  &&  ziel==w.gib_ziel()  &&  (index<2  ||  zielpos==w.gib_zielpos());
	}
};

#endif
