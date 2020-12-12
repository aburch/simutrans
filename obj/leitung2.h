/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef OBJ_LEITUNG2_H
#define OBJ_LEITUNG2_H


#include "../ifc/sync_steppable.h"
#include "../dataobj/koord3d.h"
#include "../dataobj/ribi.h"
#include "../simobj.h"
#include "../simcity.h"
#include "../tpl/slist_tpl.h"

#define POWER_TO_MW (12)  // bitshift for converting internal power values to mW for display. This is equivalent to dividing by 5,000
#define KW_DIVIDER (5) // Divider for converting internal power to kW for display. A bitshift will not suffice, so use a divider.

class powernet_t;
class player_t;
class fabrik_t;
class way_desc_t;

class leitung_t : public obj_t
{
protected:
	image_id image;

	// powerline over ways
	bool is_crossing:1;

	// direction of the next pylon
	ribi_t::ribi ribi:4;

	/**
	* We are part of this network
	*/
	powernet_t * net;

	const way_desc_t *desc;

	fabrik_t *fab;

	/**
	* Connect this piece of powerline to its neighbours
	* -> this can merge power networks
	*/
	void verbinde();

	void replace(powernet_t* neu);

	void add_ribi(ribi_t::ribi r) { ribi |= r; }

	/**
	* Dient zur Neuberechnung des Bildes
	*/
	void calc_image() OVERRIDE;

	/**
	* Use this value for scaling electricity consumption/demand
	* by both meters per tile and bits per month
	* @author: jamespetts
	*/
	sint32 modified_production_delta_t;

public:
	powernet_t* get_net() const { return net; }
	void set_net(powernet_t* p) { net = p; }

	const way_desc_t * get_desc() { return desc; }
	void set_desc(const way_desc_t *new_desc) { desc = new_desc; }

	int gimme_neighbours(leitung_t **conn);
	static fabrik_t * suche_fab_4(const koord pos);

	leitung_t(loadsave_t *file);
	leitung_t(koord3d pos, player_t *player);
	virtual ~leitung_t();

	// just book the costs for destruction
	void cleanup(player_t *) OVERRIDE;

	// for map rotation
	void rotate90() OVERRIDE;

#ifdef INLINE_OBJ_TYPE
protected:
	leitung_t(typ type, loadsave_t *file);
	leitung_t(typ type, koord3d pos, player_t *player);
public:
#else
	typ get_typ() const { return leitung; }
#endif

	const char *get_name() const OVERRIDE {return "Leitung"; }

	/**
	 * waytype associated with this object
	 */
	waytype_t get_waytype() const OVERRIDE { return powerline_wt; }

	/// @copydoc obj_t::info
	void info(cbuffer_t & buf) const OVERRIDE;

	ribi_t::ribi get_ribi() const { return ribi; }

	inline void set_image( image_id b ) { image = b; }
	image_id get_image() const OVERRIDE {return is_crossing ? IMG_EMPTY : image;}
	image_id get_front_image() const OVERRIDE {return is_crossing ? image : IMG_EMPTY;}

	/**
	* Recalculates the images of all neighbouring
	* powerlines and the powerline itself
	*/
	void calc_neighbourhood();

	void rdwr(loadsave_t *file) OVERRIDE;
	void finish_rd() OVERRIDE;

	/**
	 * @return NULL if OK, otherwise an error message
	 */
	virtual const char *is_deletable(const player_t *player) OVERRIDE;

	stadt_t *city;

	void clear_factory() { fab = NULL; }
};



class pumpe_t : public leitung_t
{
public:
	static void new_world();
	static void step_all(uint32 delta_t);

private:
	static slist_tpl<pumpe_t *> pumpe_list;

	uint32 supply;

	void step(uint32 delta_t);

public:
	pumpe_t(loadsave_t *file);
	pumpe_t(koord3d pos, player_t *player);
	virtual ~pumpe_t();

#ifdef INLINE_OBJ_TYPE
#else
	typ get_typ() const OVERRIDE { return pumpe; }
#endif

	const char *get_name() const OVERRIDE {return "Aufspanntransformator";}

	void info(cbuffer_t & buf) const OVERRIDE;

	void finish_rd() OVERRIDE;

	void calc_image() OVERRIDE {} // otherwise it will change to leitung

	const fabrik_t* get_factory() const { return fab; }
};



class senke_t : public leitung_t, public sync_steppable
{
public:
	static void new_world();
	static void step_all(uint32 delta_t);
	static slist_tpl<senke_t *> senke_list;

private:

	sint32 einkommen;
	sint32 max_einkommen;
	sint32 delta_sum;
	sint32 next_t;
	uint32 last_power_demand;
	uint32 power_load;

	void step(uint32 delta_t);

public:
	senke_t(loadsave_t *file);
	senke_t(koord3d pos, player_t *player, stadt_t *c);
	virtual ~senke_t();

#ifdef INLINE_OBJ_TYPE
#else
	typ get_typ() const { return senke; }
#endif

	// used to alternate between displaying power on and power off images at a frequency determined by the percentage of power supplied
	// gives players a visual indication of a power network with insufficient generation
	sync_result sync_step(uint32 delta_t) OVERRIDE;

	const char *get_name() const OVERRIDE {return "Abspanntransformator";}

	void info(cbuffer_t & buf) const OVERRIDE;

	void finish_rd() OVERRIDE;

	void calc_image() OVERRIDE {}	// otherwise it will change to leitung

	uint32 get_power_load() const;

	void set_city(stadt_t* c) { city = c; }

	void check_industry_connexion();

	const fabrik_t* get_factory() const { return fab; }

	uint32 get_last_power_demand() const { return last_power_demand; }
	void set_last_power_demand(uint32 value) { last_power_demand = value; }
};


#endif
