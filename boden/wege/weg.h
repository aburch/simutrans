/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef BODEN_WEGE_WEG_H
#define BODEN_WEGE_WEG_H


#include "../../display/simimg.h"
#include "../../simtypes.h"
#include "../../obj/simobj.h"
#include "../../descriptor/way_desc.h"
#include "../../dataobj/koord3d.h"


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
	WAY_STAT_CONVOIS = 1, ///< number of convois that passed this way
	WAY_STAT_MAX
};


/**
 * Ways is the base class for all traffic routes. (roads, track, runway etc.)
 * Ways always "belong" to a ground. They have direction bits (ribis) as well as
 * a mask for ribis.
 *
 * A way always is of a single type (waytype_t).
 * Crossings are supported by the fact that a ground can have more than one way.
 */
class weg_t : public obj_no_info_t
{
public:
	/**
	* Get list of all ways
	*/
	static const slist_tpl <weg_t *> & get_alle_wege();

	enum {
		HAS_SIDEWALK   = 1 << 0,
		IS_ELECTRIFIED = 1 << 1,
		HAS_SIGN       = 1 << 2,
		HAS_SIGNAL     = 1 << 3,
		HAS_WAYOBJ     = 1 << 4,
		HAS_CROSSING   = 1 << 5,
		IS_DIAGONAL    = 1 << 6, // marker for diagonal image
		IS_SNOW        = 1 << 7  // marker, if above snowline currently
	};

private:
	/**
	* array for statistical values
	* MAX_WAY_STAT_MONTHS: [0] = actual value; [1] = last month value
	* MAX_WAY_STATISTICS: see #define at top of file
	*/
	sint16 statistics[MAX_WAY_STAT_MONTHS][MAX_WAY_STATISTICS];

	/**
	* Way type description
	*/
	const way_desc_t * desc;

	/**
	* Direction bits (ribis) for the way. North is in the upper right corner of the monitor.
	* 1=North, 2=East, 4=South, 8=West
	*/
	uint8 ribi:4;

	/**
	* Mask for ribi (Richtungsbits => Direction Bits)
	*/
	uint8 ribi_maske:4;

	/**
	* flags like walkway, electrification, road sings
	*/
	uint8 flags;

	/**
	* max speed; could not be taken for desc, since other object may modify the speed
	*/
	uint16 max_speed;

	image_id image;
	image_id foreground_image;

	/**
	* Initializes all member variables
	*/
	void init();

	/**
	* initializes statistic array
	*/
	void init_statistics();

protected:

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

	enum image_type {
		image_flat,
		image_slope,
		image_diagonal,
		image_switch
	};

	/**
	 * initializes both front and back images
	 * switch images are set in schiene_t::reserve
	 * needed by tunnel mouths
	 */
	void set_images(image_type typ, uint8 ribi, bool snow, bool switch_nw = false);

	/**
	 * Called whenever the season or snowline height changes
	 * return false and the obj_t will be deleted
	 */
	bool check_season(const bool calc_only_season_change) OVERRIDE;

	void set_max_speed(sint32 s) { max_speed = s; }
	sint32 get_max_speed() const { return max_speed; }

	/// @note Replaces max speed of the way by the max speed property of the descriptor.
	void set_desc(const way_desc_t *b);
	const way_desc_t *get_desc() const { return desc; }

	// returns a way with the matching type
	static weg_t *alloc(waytype_t wt);

	// returns a string with the "official name of the waytype"
	static const char *waytype_to_string(waytype_t wt);

	void rdwr(loadsave_t *file) OVERRIDE;

	/**
	* Info-text for this way
	*/
	void info(cbuffer_t & buf) const OVERRIDE;

	/**
	 * @return NULL if OK, otherwise an error message
	 */
	const char *is_deletable(const player_t *player) OVERRIDE;

	waytype_t get_waytype() const OVERRIDE = 0;

	/// @copydoc obj_t::get_typ
	typ get_typ() const OVERRIDE { return obj_t::way; }

	/**
	* Die Bezeichnung des Wegs
	*/
	const char *get_name() const OVERRIDE { return desc->get_name(); }

	/**
	* Add direction bits (ribi) for a way.
	*
	* @note After changing of ribi the image of the way is wrong. To correct this,
	* grund_t::calc_image needs to be called. This is not done here (Too expensive).
	*/
	void ribi_add(ribi_t::ribi ribi) { this->ribi |= (uint8)ribi;}

	/**
	* Remove direction bits (ribi) for a way.
	*
	* @note After changing of ribi the image of the way is wrong. To correct this,
	* grund_t::calc_image needs to be called. This is not done here (Too expensive).
	*/
	void ribi_rem(ribi_t::ribi ribi) { this->ribi &= (uint8)~ribi;}

	/**
	* Set direction bits (ribi) for the way.
	*
	* @note After changing of ribi the image of the way is wrong. To correct this,
	* grund_t::calc_image needs to be called. This is not done here (Too expensive).
	*/
	void set_ribi(ribi_t::ribi ribi) { this->ribi = (uint8)ribi;}

	/**
	* Get the unmasked direction bits (ribi) for the way (without signals or other ribi changer).
	*/
	ribi_t::ribi get_ribi_unmasked() const { return (ribi_t::ribi)ribi; }

	/**
	* Get the masked direction bits (ribi) for the way (with signals or other ribi changer).
	*/
	ribi_t::ribi get_ribi() const { return (ribi_t::ribi)(ribi & ~ribi_maske); }

	/**
	* For signals it is necessary to mask out certain ribi to prevent vehicles
	* from driving the wrong way (e.g. oneway roads)
	*/
	void set_ribi_maske(ribi_t::ribi ribi) { ribi_maske = (uint8)ribi; }
	ribi_t::ribi get_ribi_maske() const { return (ribi_t::ribi)ribi_maske; }

	/**
	 * called during map rotation
	 */
	void rotate90() OVERRIDE;

	/**
	* book statistics - is called very often and therefore inline
	*/
	void book(int amount, way_statistics type) { statistics[0][type] += amount; }

	/**
	* return statistics value
	* always returns last month's value
	*/
	int get_statistics(int type) const { return statistics[1][type]; }

	sint64 get_stat(int month, int stat_type) const { assert(stat_type<WAY_STAT_MAX  &&  0<=month  &&  month<MAX_WAY_STAT_MONTHS); return statistics[month][stat_type]; }
	/**
	* new month
	*/
	void new_month();

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
	image_id get_front_image() const OVERRIDE {return foreground_image;}


	// correct maintenance
	void finish_rd() OVERRIDE;
} GCC_PACKED;

#endif
