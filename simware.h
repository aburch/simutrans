#ifndef simware_h
#define simware_h

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
	koord ziel;

	/**
	 * Koordinte des nächsten Zwischenstops
	 * @author Hj. Malthaner
	 */
	koord zwischenziel;

	/**
	 * die engültige Zielposition,
	 * das ist i.a. nicht die Zielhaltestellenposition
	 * @author Hj. Malthaner
	 */
	koord zielpos;

public:
	koord gib_ziel() const { return ziel; }
	void setze_ziel(koord ziel) { this->ziel = ziel; }

	koord gib_zwischenziel() const { return zwischenziel; }
	void setze_zwischenziel(koord zwischenziel) { this->zwischenziel = zwischenziel; }

	koord gib_zielpos() const { return zielpos; }
	void setze_zielpos(koord zielpos) { this->zielpos = zielpos; }

	ware_t();
	ware_t(const ware_besch_t *typ);
	ware_t(karte_t *welt,loadsave_t *file);

	/**
	 * gibt den nicht-uebersetzten warennamen zurück
	 * @author Hj. Malthaner
	 */
	const char *gib_name() const { return gib_typ()->gib_name(); }
	const char *gib_mass() const { return gib_typ()->gib_mass(); }
	uint16 gib_preis() const { return gib_typ()->gib_preis(); }
	uint8 gib_catg() const { return gib_typ()->gib_catg(); }
	uint8 gib_index() const { return index; }

	const ware_besch_t* gib_typ() const { return index_to_besch[index]; }
	void setze_typ(const ware_besch_t* type);

	void rdwr(karte_t *welt,loadsave_t *file);

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
};

#endif
