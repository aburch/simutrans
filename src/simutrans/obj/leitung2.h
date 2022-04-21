/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef OBJ_LEITUNG2_H
#define OBJ_LEITUNG2_H


#include "../obj/sync_steppable.h"
#include "../dataobj/koord3d.h"
#include "../dataobj/ribi.h"
#include "simobj.h"
#include "../tpl/slist_tpl.h"

// bitshift for converting internal power values to MW for display
extern const uint32 POWER_TO_MW;

#define CREDIT_PER_MWS 2

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

	bool is_transformer:1;

	// direction of the next pylon
	ribi_t::ribi ribi:4;

	/**
	* We are part of this network
	*/
	powernet_t * net;

	const way_desc_t *desc;

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

public:
	// number of fractional bits for network load values
	static const uint8 FRACTION_PRECISION;

	powernet_t* get_net() const { return net; }
	/**
	 * Changes the currently registered power net.
	 * Can be overwritten to modify the power net on change.
	 */
	virtual void set_net(powernet_t* p) { net = p; }

	const way_desc_t * get_desc() { return desc; }
	void set_desc(const way_desc_t *new_desc) { desc = new_desc; }

	int gimme_neighbours(leitung_t **conn);
	static fabrik_t * suche_fab_4(koord pos);

	leitung_t(loadsave_t *file);
	leitung_t(koord3d pos, player_t *player);
	virtual ~leitung_t();

	// just book the costs for destruction
	void cleanup(player_t *) OVERRIDE;

	// for map rotation
	void rotate90() OVERRIDE;

	typ get_typ() const OVERRIDE { return leitung; }

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
	const char *is_deletable(const player_t *player) OVERRIDE;

	/**
	 * @return maintenance of this object (powerline or transformer)
	 */
	sint64 get_maintenance() const;
};



class pumpe_t : public leitung_t
{
public:
	static void new_world();
	static void step_all(uint32 delta_t);

private:
	static slist_tpl<pumpe_t *> pumpe_list;

	fabrik_t *fab;

	// The power supplied through the transformer.
	uint32 power_supply;

	void step(uint32 delta_t);

public:
	pumpe_t(loadsave_t *file);
	pumpe_t(koord3d pos, player_t *player);
	~pumpe_t();

	void set_net(powernet_t* p) OVERRIDE;

	/**
	 * Set the power supply of the transformer.
	 */
	void set_power_supply(uint32 newsupply);

	/**
	 * Get the power supply of the transformer.
	 */
	uint32 get_power_supply() const {return power_supply;}

	/**
	 * Get the normalized satisfaction value of the power consumed, updated every tick.
	 * Return value is fixed point with FRACTION_PRECISION fractional bits.
	 */
	sint32 get_power_consumption() const;

	typ get_typ() const OVERRIDE { return pumpe; }

	const char *get_name() const OVERRIDE {return "Aufspanntransformator";}

	void info(cbuffer_t & buf) const OVERRIDE;

	void rdwr(loadsave_t *file) OVERRIDE;
	void finish_rd() OVERRIDE;

	void calc_image() OVERRIDE {} // empty; otherwise it will change to leitung

	const fabrik_t* get_factory() const { return fab; }
};


/*
 * Distribution transformers act as an interface between power networks and
 * and power consuming factories.
 */
class senke_t : public leitung_t, public sync_steppable
{
public:
	static void new_world();
	static void step_all(uint32 delta_t);

	/**
	 * Read and write static state.
	 * This is used to make payments occur at the same time after load as after saving.
	 */
	static void static_rdwr(loadsave_t *file);

private:
	// List of all distribution transformers.
	static slist_tpl<senke_t *> senke_list;

	// Timer for global power payment.
	static uint32 payment_timer;

	fabrik_t *fab;

	// Pwm timer for duty cycling image.
	uint32 delta_t_sum;

	// Timer for recalculating image.
	uint32 next_t;

	// The power requested through the transformer.
	uint32 power_demand;

	// Energy accumulator (how much energy has been metered).
	uint64 energy_acc;

	void step(uint32 delta_t);

	// Pay out revenue for the energy metered.
	void pay_revenue();

public:
	senke_t(loadsave_t *file);
	senke_t(koord3d pos, player_t *player);
	~senke_t();

	void set_net(powernet_t* p) OVERRIDE;

	typ get_typ() const OVERRIDE { return senke; }

	/**
	 * Set the power demand of the transformer.
	 */
	void set_power_demand(uint32 newdemand);

	/**
	 * Get the power demand of the transformer.
	 */
	uint32 get_power_demand() const {return power_demand;}

	/**
	 * Get the normalized satisfaction value of the power demand, updated every tick.
	 * Return value is fixed point with FRACTION_PRECISION fractional bits.
	 */
	sint32 get_power_satisfaction() const;

	/**
	 * Used to alternate between displaying power on and power off images.
	 * Frequency determined by the percentage of power supplied.
	 * Gives players a visual indication of a power network with insufficient generation.
	 */
	sync_result sync_step(uint32 delta_t) OVERRIDE;

	const char *get_name() const OVERRIDE {return "Abspanntransformator";}

	void info(cbuffer_t & buf) const OVERRIDE;

	void rdwr(loadsave_t *file) OVERRIDE;
	void finish_rd() OVERRIDE;

	void calc_image() OVERRIDE {} // empty; otherwise it will change to leitung

	const fabrik_t* get_factory() const { return fab; }
};


#endif
