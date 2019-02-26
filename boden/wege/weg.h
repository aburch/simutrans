/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef boden_wege_weg_h
#define boden_wege_weg_h

#include "../../display/simimg.h"
#include "../../simtypes.h"
#include "../../simobj.h"
#include "../../descriptor/way_desc.h"
#include "../../dataobj/koord3d.h"
#include "../../simskin.h"


class karte_t;
class way_desc_t;
class cbuffer_t;
template <class T> class slist_tpl;


// maximum number of months to store information
#define MAX_WAY_STAT_MONTHS 2

// number of different statistics collected
#define MAX_WAY_STATISTICS 2

enum way_statistics {
	WAY_STAT_GOODS   = 0, ///< number of goods transported over this way
	WAY_STAT_CONVOIS = 1  ///< number of convois that passed this way
};

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
class weg_t : public obj_no_info_t
{

	friend class strasse_t;

public:
	/**
	* Get list of all ways
	* @author Hj. Malthaner
	*/
	static const slist_tpl <weg_t *> & get_alle_wege();

	enum {
		HAS_SIDEWALK   = 0x01,
		IS_ELECTRIFIED = 0x02,
		HAS_SIGN       = 0x04,
		HAS_SIGNAL     = 0x08,
		HAS_WAYOBJ     = 0x10,
		HAS_CROSSING   = 0x20,
		IS_DIAGONAL    = 0x40, // marker for diagonal image
		IS_SNOW = 0x80	// marker, if above snowline currently
	};

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
	const way_desc_t * desc;

	/**
	* Richtungsbits für den Weg. Norden ist oben rechts auf dem Monitor.
	* 1=Nord, 2=Ost, 4=Sued, 8=West
	* @author Hj. Malthaner
	*/
	uint8 ribi:4;

	/**
	* Mask for ribi (Richtungsbits => Direction Bits)
	* @author Hj. Malthaner
	*/
	uint8 ribi_maske:4;

	/**
	* flags like walkway, electrification, road sings
	* @author Hj. Malthaner
	*/
	uint8 flags;

	/**
	* max speed; could not be taken for desc, since other object may modify the speed
	* @author Hj. Malthaner
	*/
	uint16 max_speed;

	image_id image;
	image_id foreground_image;


	/**
	* Initializes all member variables
	* @author Hj. Malthaner
	*/
	void init();

	/**
	* initializes statistic array
	* @author hsiegeln
	*/
	virtual void init_statistics();

protected:

	enum image_type { image_flat, image_slope, image_diagonal, image_switch };

	/**
	 * initializes both front and back images
	 * switch images are set in schiene_t::reserve
	 */
	void set_images(image_type typ, uint8 ribi, bool snow, bool switch_nw=false);

public:
	weg_t(loadsave_t*) : obj_no_info_t() { init(); }
	weg_t() : obj_no_info_t() { init(); }

	virtual ~weg_t();

#ifdef MULTI_THREAD
	void lock_mutex();
	void unlock_mutex();
#endif

	/**
	 * Actual image recalculation
	 */
	void calc_image() OVERRIDE;

	/**
	 * Called whenever the season or snowline height changes
	 * return false and the obj_t will be deleted
	 */
	bool check_season(const bool calc_only_season_change) OVERRIDE;

	/**
	* Setzt die erlaubte Höchstgeschwindigkeit
	* @author Hj. Malthaner
	*/
	void set_max_speed(sint32 s) { max_speed = s; }

	/**
	* Ermittelt die erlaubte Höchstgeschwindigkeit
	* @author Hj. Malthaner
	*/
	sint32 get_max_speed() const { return max_speed; }

	/**
	* Setzt neue Description. Ersetzt alte Höchstgeschwindigkeit
	* mit wert aus Description.
	* @author Hj. Malthaner
	*/
	void set_desc(const way_desc_t *b);
	const way_desc_t *get_desc() const { return desc; }

	// returns a way with the matching type
	static weg_t *alloc(waytype_t wt);

	// returns a string with the "official name of the waytype"
	static const char *waytype_to_string(waytype_t wt);

	void rdwr(loadsave_t *file) OVERRIDE;

	/**
	* Info-text for this way
	* @author Hj. Malthaner
	*/
	void info(cbuffer_t & buf) const OVERRIDE;

	/**
	 * @return NULL if OK, otherwise an error message
	 * @author Hj. Malthaner
	 */
	const char *is_deletable(const player_t *player) OVERRIDE;

	/**
	* Wegtyp zurückliefern
	*/
	waytype_t get_waytype() const OVERRIDE = 0;

	/**
	* 'Jedes Ding braucht einen Typ.'
	* @return Gibt den typ des Objekts zurück.
	* @author Hj. Malthaner
	*/
	typ get_typ() const OVERRIDE { return obj_t::way; }

	/**
	* Die Bezeichnung des Wegs
	* @author Hj. Malthaner
	*/
	const char *get_name() const OVERRIDE { return desc->get_name(); }

	/**
	* Add direction bits (ribi) for a way.
	*
	* Nachdem die ribis geändert werden, ist das weg_image des
	* zugehörigen Grundes falsch (Ein Aufruf von grund_t::calc_image()
	* zur Reparatur muß folgen).
	* @param ribi Richtungsbits
	*/
	void ribi_add(ribi_t::ribi ribi) { this->ribi |= (uint8)ribi;}

	/**
	* Remove direction bits (ribi) on a way.
	*
	* Nachdem die ribis geändert werden, ist das weg_image des
	* zugehörigen Grundes falsch (Ein Aufruf von grund_t::calc_image()
	* zur Reparatur muß folgen).
	* @param ribi Richtungsbits
	*/
	void ribi_rem(ribi_t::ribi ribi) { this->ribi &= (uint8)~ribi;}

	/**
	* Set direction bits (ribi) for the way.
	*
	* Nachdem die ribis geändert werden, ist das weg_image des
	* zugehörigen Grundes falsch (Ein Aufruf von grund_t::calc_image()
	* zur Reparatur muß folgen).
	* @param ribi Richtungsbits
	*/
	void set_ribi(ribi_t::ribi ribi) { this->ribi = (uint8)ribi;}

	/**
	* Get the unmasked direction bits (ribi) for the way (without signals or other ribi changer).
	*/
	ribi_t::ribi get_ribi_unmasked() const { return (ribi_t::ribi)ribi; }

	/**
	* Get the masked direction bits (ribi) for the way (with signals or other ribi changer).
	*/
	virtual ribi_t::ribi get_ribi() const { return (ribi_t::ribi)(ribi & ~ribi_maske); };

	/**
	* für Signale ist es notwendig, bestimmte Richtungsbits auszumaskieren
	* damit Fahrzeuge nicht "von hinten" über Ampeln fahren können.
	* @param ribi Richtungsbits
	*/
	void set_ribi_maske(ribi_t::ribi ribi) { ribi_maske = (uint8)ribi; }
	ribi_t::ribi get_ribi_maske() const { return (ribi_t::ribi)ribi_maske; }

	/**
	 * called during map rotation
	 * @author priss
	 */
	virtual void rotate90() OVERRIDE;

	/**
	* book statistics - is called very often and therefore inline
	* @author hsiegeln
	*/
	void book(int amount, way_statistics type) { statistics[0][type] += amount; }

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
	virtual void new_month();

	void check_diagonal();

	void count_sign();

	/* flag query routines */
	void set_gehweg(const bool yesno) { flags = (yesno ? flags | HAS_SIDEWALK : flags & ~HAS_SIDEWALK); }
	inline bool hat_gehweg() const { return flags & HAS_SIDEWALK; }

	void set_electrify(bool janein) {janein ? flags |= IS_ELECTRIFIED : flags &= ~IS_ELECTRIFIED;}
	inline bool is_electrified() const {return flags&IS_ELECTRIFIED; }

	inline bool has_sign() const {return flags&HAS_SIGN; }
	inline bool has_signal() const {return flags&HAS_SIGNAL; }
	inline bool has_wayobj() const {return flags&HAS_WAYOBJ; }
	inline bool is_crossing() const {return flags&HAS_CROSSING; }
	inline bool is_diagonal() const {return flags&IS_DIAGONAL; }
	inline bool is_snow() const {return flags&IS_SNOW; }

	// this is needed during a change from crossing to tram track
	void clear_crossing() { flags &= ~HAS_CROSSING; }

	/**
	 * Clear the has-sign flag when roadsign or signal got deleted.
	 * As there is only one of signal or roadsign on the way we can safely clear both flags.
	 */
	void clear_sign_flag() { flags &= ~(HAS_SIGN | HAS_SIGNAL); }

	inline void set_image( image_id b ) { image = b; }
	image_id get_image() const OVERRIDE {return image;}

	inline void set_foreground_image( image_id b ) { foreground_image = b; }
	virtual image_id get_front_image() const OVERRIDE {return foreground_image;}


	// correct maintenance
	void finish_rd() OVERRIDE;
} GCC_PACKED;

#endif
