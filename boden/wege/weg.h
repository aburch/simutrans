/*
 * weg.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef boden_wege_weg_h
#define boden_wege_weg_h

#include "../../simimg.h"
#include "../../simtypes.h"
#include "../../simdings.h"
#include "../../besch/weg_besch.h"
#include "../../dataobj/koord3d.h"


class vehikel_basis_t;
class karte_t;
class weg_besch_t;
class cbuffer_t;

template <class T> class slist_tpl;


// maximum number of months to store information
#define MAX_WAY_STAT_MONTHS 2

// number of different statistics collected
#define MAX_WAY_STATISTICS 2

// number of goods transported over this weg
#define WAY_STAT_GOODS 0

// number of convois that passed this weg
#define WAY_STAT_CONVOIS 1



/**
 * <p>Der Weg ist die Basisklasse fuer alle Verkehrswege in Simutrans.
 * Wege "gehören" immer zu einem Grund. Sie besitzen Richtungsbits sowie
 * eine Maske fuer Richtungsbits.</p>
 *
 * <p>Ein Weg gehört immer zu genau einer Wegsorte</p>
 *
 * <p>Kreuzungen werden dadurch unterstützt, daß ein Grund zwei Wege
 * enthalten kann (prinzipiell auch mehrere möglich.</p>
 *
 * <p>Wegtyp -1 ist reserviert und kann nicht für Wege benutzt werden<p>
 *
 * @author Hj. Malthaner
 */
class weg_t : public ding_t
{
public:
	/**
	* Get list of all ways
	* @author Hj. Malthaner
	*/
	static const slist_tpl <weg_t *> & gib_alle_wege();

	enum { HAS_WALKWAY=1, IS_ELECTRIFIED=2, HAS_SIGN=4, HAS_WAYOBJ=8 };

	enum system_type { type_flat=0, type_elevated=1, type_tram=7, type_all=255 };

private:
	/**
	* array for statistical values
	* MAX_WAY_STAT_MONTHS: [0] = actual value; [1] = last month value
	* MAX_WAY_STATISTICS: see #define at top of file
	* @author hsiegeln
	*/
	sint16 statistics[MAX_WAY_STAT_MONTHS][MAX_WAY_STATISTICS];

	/**
	* Way type description
	* @author Hj. Malthaner
	*/
	const weg_besch_t * besch;

	/**
	* Richtungsbits für den Weg. Norden ist oben rechts auf dem Monitor.
	* 1=Nord, 2=Ost, 4=Sued, 8=West
	* @author Hj. Malthaner
	*/
	uint8 ribi:4;

	/**
	* Maske für Richtungsbits
	* @author Hj. Malthaner
	*/
	uint8 ribi_maske:4;

	/**
	* flags like walkway, electrification, road sings
	* @author Hj. Malthaner
	*/
	uint8 flags;

	/**
	* max speed; could not be taken for besch, since other object may modify the speed
	* @author Hj. Malthaner
	*/
	uint16 max_speed;

	image_id bild;

	/**
	* Initializes all member variables
	* @author Hj. Malthaner
	*/
	void init();

	/**
	* initializes statistic array
	* @author hsiegeln
	*/
	void init_statistics();


public:
	weg_t(karte_t* welt, loadsave_t*) : ding_t(welt) { init(); }
	weg_t(karte_t *welt) : ding_t(welt) { init(); }

	virtual ~weg_t();

	/* actual image recalculation */
	void calc_bild();

	/**
	* Setzt die erlaubte Höchstgeschwindigkeit
	* @author Hj. Malthaner
	*/
	void setze_max_speed(unsigned int s);

	/**
	* Ermittelt die erlaubte Höchstgeschwindigkeit
	* @author Hj. Malthaner
	*/
	uint16 gib_max_speed() const {return max_speed;}

	/**
	* Setzt neue Beschreibung. Ersetzt alte Höchstgeschwindigkeit
	* mit wert aus Beschreibung.
	* @author Hj. Malthaner
	*/
	void setze_besch(const weg_besch_t *b);
	const weg_besch_t * gib_besch() const {return besch;}

	// returns a way with the matching type
	static weg_t* alloc(waytype_t wt);

	virtual void rdwr(loadsave_t *file);

	/**
	* Info-text für diesen Weg
	* @author Hj. Malthaner
	*/
	virtual void info(cbuffer_t & buf) const;

	void zeige_info() {} // show no info

	/**
	* Wegtyp zurückliefern
	*/
	virtual waytype_t gib_waytype() const = 0;

	/**
	* 'Jedes Ding braucht einen Typ.'
	* @return Gibt den typ des Objekts zurück.
	* @author Hj. Malthaner
	*/
	ding_t::typ gib_typ() const { return ding_t::way; }

	/**
	* Die Bezeichnung des Wegs
	* @author Hj. Malthaner
	*/
	const char *gib_name() const { return besch->gib_name(); }

	/**
	* Setzt neue Richtungsbits für einen Weg.
	*
	* Nachdem die ribis geändert werden, ist das weg_bild des
	* zugehörigen Grundes falsch (Ein Aufruf von grund_t::calc_bild()
	* zur Reparatur muß folgen).
	* @param ribi Richtungsbits
	*/
	void ribi_add(ribi_t::ribi ribi) { this->ribi |= (uint8)ribi;}

	/**
	* Entfernt Richtungsbits von einem Weg.
	*
	* Nachdem die ribis geändert werden, ist das weg_bild des
	* zugehörigen Grundes falsch (Ein Aufruf von grund_t::calc_bild()
	* zur Reparatur muß folgen).
	* @param ribi Richtungsbits
	*/
	void ribi_rem(ribi_t::ribi ribi) { this->ribi &= (uint8)~ribi;}

	/**
	* Setzt Richtungsbits für den Weg.
	*
	* Nachdem die ribis geändert werden, ist das weg_bild des
	* zugehörigen Grundes falsch (Ein Aufruf von grund_t::calc_bild()
	* zur Reparatur muß folgen).
	* @param ribi Richtungsbits
	*/
	void setze_ribi(ribi_t::ribi ribi) { this->ribi = (uint8)ribi;}

	/**
	* Ermittelt die unmaskierten Richtungsbits für den Weg.
	*/
	ribi_t::ribi gib_ribi_unmasked() const { return (ribi_t::ribi)ribi; }

	/**
	* Ermittelt die (maskierten) Richtungsbits für den Weg.
	*/
	ribi_t::ribi gib_ribi() const { return (ribi_t::ribi)(ribi & ~ribi_maske); }

	/**
	* für Signale ist es notwendig, bestimmte Richtungsbits auszumaskieren
	* damit Fahrzeuge nicht "von hinten" über Ampeln fahren können.
	* @param ribi Richtungsbits
	*/
	void setze_ribi_maske(ribi_t::ribi ribi) { ribi_maske = (uint8)ribi; }
	ribi_t::ribi gib_ribi_maske() const { return (ribi_t::ribi)ribi_maske; }

	/**
	* book statistics - is called very often and therefore inline
	* @author hsiegeln
	*/
	void book(int amount, int type) {
		if (type < MAX_WAY_STATISTICS) {
			statistics[0][type] += amount;
		}
	}

	/**
	* return statistics value
	* always returns last month's value
	* @author hsiegeln
	*/
	int get_statistics(int type) const { return statistics[1][type]; }

	/**
	* new month
	* @author hsiegeln
	*/
	void neuer_monat();

	/* flag query routines */
	void setze_gehweg(bool janein) {if(janein) { flags |= HAS_WALKWAY;} else {flags&=~HAS_WALKWAY;} }
	inline bool hat_gehweg() const {return flags&HAS_WALKWAY; }

	void set_electrify(bool janein) {janein ? flags |= IS_ELECTRIFIED : flags &= ~IS_ELECTRIFIED;}
	inline bool is_electrified() const {return flags&IS_ELECTRIFIED; }

	void count_sign();
	inline bool has_sign() const {return flags&HAS_SIGN; }
	inline bool has_wayobj() const {return flags&HAS_WAYOBJ; }

	void inline setze_bild( image_id b ) { bild = b; }
	image_id gib_bild() const {return bild;}

	// correct maitainace
	void laden_abschliessen();
} GCC_PACKED;

#endif
