/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 *
 *  BEWARE: non-standard node structure!
 *	0   Vordergrundbilder
 *	1   Hintergrundbilder
 *	2   Cursor/Icon (Hajo: 14-Feb-02: now also icon image)
 *	3   Vordergrundbilder - snow
 *	4   Hintergrundbilder - snow
 */

#ifndef __BRUECKE_BESCH_H
#define __BRUECKE_BESCH_H


#include "skin_besch.h"
#include "bildliste_besch.h"
#include "text_besch.h"
#include "../simtypes.h"
#include "../simimg.h"

#include "../dataobj/ribi.h"
#include "../dataobj/way_constraints.h"

class werkzeug_t;
class checksum_t;

class bruecke_besch_t : public obj_besch_std_name_t {
    friend class bridge_writer_t;
    friend class bridge_reader_t;

private:
	sint32  topspeed;
	uint32  preis;

	/**
	* Maintenance cost per square/month
	* @author Hj. Malthaner
	*/
	uint32 maintenance;

	uint8  wegtyp;
	uint8 pillars_every;	// =0 off
	bool pillars_asymmetric;	// =0 off else leave one off for north/west slopes
	uint offset;	// flag, because old bridges had their name/copyright at the wrong position

	uint8 max_length;	// =0 off, else maximum length
	uint8 max_height;	// =0 off, else maximum length
	uint32 max_weight; //@author: jamespetts. Weight limit for vehicles.

	// allowed era
	uint16 intro_date;
	uint16 obsolete_date;

	/* number of seasons (0 = none, 1 = no snow/snow
	*/
	sint8 number_seasons;

	/*Way constraints for, e.g., loading gauges, types of electrification, etc.
	* @author: jamespetts*/
	//uint8 way_constraints_permissive;
	//uint8 way_constraints_prohibitive;
	way_constraints_of_way_t way_constraints;

	werkzeug_t *builder;


public:
	/*
	 * Nummerierung all der verschiedenen Schienstücke
	 */
	enum img_t {
		NS_Segment, OW_Segment, N_Start, S_Start, O_Start, W_Start, N_Rampe, S_Rampe, O_Rampe, W_Rampe, NS_Pillar, OW_Pillar
	};

	/*
	 * Name und Copyright sind beim Cursor gespeichert!
	 */
	const char *get_name() const { return get_cursor()->get_name(); }
	const char *get_copyright() const { return get_cursor()->get_copyright(); }

	skin_besch_t const* get_cursor() const { return get_child<skin_besch_t>(2 + offset); }

	image_id get_hintergrund(img_t img, uint8 season) const 	{
		const bild_besch_t *bild = NULL;
		if(season && number_seasons == 1) {
			bild = get_child<bildliste_besch_t>(3 + offset)->get_bild(img);
		}
		if(bild == NULL) {
			bild = get_child<bildliste_besch_t>(0 + offset)->get_bild(img);
		}
		return bild != NULL ? bild->get_nummer() : IMG_LEER;
	}

	image_id get_vordergrund(img_t img, uint8 season) const {
		const bild_besch_t *bild = NULL;
		if(season && number_seasons == 1) {
			bild = get_child<bildliste_besch_t>(4 + offset)->get_bild(img);
		}
		if(bild == NULL) {
			bild = get_child<bildliste_besch_t>(1 + offset)->get_bild(img);
		}
		return bild != NULL ? bild->get_nummer() : IMG_LEER;
	}

	static img_t get_simple(ribi_t::ribi ribi);
	static img_t get_start(ribi_t::ribi ribi);
	static img_t get_rampe(ribi_t::ribi ribi);
	static img_t get_pillar(ribi_t::ribi ribi);

	waytype_t get_waytype() const { return static_cast<waytype_t>(wegtyp); }

	sint32 get_preis() const { return preis; }

	sint32 get_wartung() const { return maintenance; }

	/**
	 * Determines max speed in km/h allowed on this bridge
	 * @author Hj. Malthaner
	 */
	sint32  get_topspeed() const { return topspeed; }

	/**
	 * Determines max weight in tonnes for vehicles allowed on this bridge
	 * @author jamespetts
	 */
	uint32 get_max_weight() const { return max_weight; }

	/**
	 * Distance of pillars (=0 for no pillars)
	 * @author prissi
	 */
	int  get_pillar() const { return pillars_every; }

	/**
	 * skips lowest pillar on south/west slopes?
	 * @author prissi
	 */
	bool  has_pillar_asymmetric() const { return pillars_asymmetric; }

	/**
	 * maximum bridge span (=0 for infinite)
	 * @author prissi
	 */
	int  get_max_length() const { return max_length; }

	/**
	 * maximum bridge height (=0 for infinite)
	 * @author prissi
	 */
	int  get_max_height() const { return max_height; }

	/**
	 * @return introduction month
	 * @author Hj. Malthaner
	 */
	int get_intro_year_month() const { return intro_date; }

	/**
	 * @return time when obsolete
	 * @author prissi
	 */
	int get_retire_year_month() const { return obsolete_date; }


	/* Way constraints: determines whether vehicles
	 * can travel on this way. This method decodes
	 * the byte into bool values. See here for
	 * information on bitwise operations: 
	 * http://www.cprogramming.com/tutorial/bitwise_operators.html
	 * @author: jamespetts
	 * */
	//const bool permissive_way_constraint_set(uint8 i)
	//{
	//	return ((way_constraints_permissive & 1)<<i != 0);
	//}

	//const bool prohibitive_way_constraint_set(uint8 i)
	//{
	//	return ((way_constraints_prohibitive & 1)<<i != 0);
	//}

	//uint8 get_way_constraints_permissive() const { return way_constraints_permissive; }
	//uint8 get_way_constraints_prohibitive() const { return way_constraints_prohibitive; }
	const way_constraints_of_way_t& get_way_constraints() const { return way_constraints; }
	void set_way_constraints(const way_constraints_of_way_t& way_constraints) { this->way_constraints = way_constraints; }

	// default tool for building
	werkzeug_t *get_builder() const {
		return builder;
	}
	void set_builder( werkzeug_t *w )  {
		builder = w;
	}

	void calc_checksum(checksum_t *chk) const;
};

#endif
