/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef obj_leitung_t
#define obj_leitung_t


#include "../ifc/sync_steppable.h"
#include "../dataobj/koord3d.h"
#include "../dataobj/ribi.h"
#include "../simobj.h"
#include "../tpl/slist_tpl.h"

#define POWER_TO_MW (12)  // bitshift for converting internal power values to MW for display

class powernet_t;
class player_t;
class fabrik_t;
class weg_besch_t;

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
	* @author Hj. Malthaner
	*/
	powernet_t * net;

	const weg_besch_t *desc;

	/**
	* Connect this piece of powerline to its neighbours
	* -> this can merge power networks
	* @author Hj. Malthaner
	*/
	void verbinde();

	void replace(powernet_t* neu);

	void add_ribi(ribi_t::ribi r) { ribi |= r; }

	/**
	* Dient zur Neuberechnung des Bildes
	* @author Hj. Malthaner
	*/
	void calc_image();

public:
	powernet_t* get_net() const { return net; }
	void set_net(powernet_t* p) { net = p; }

	const weg_besch_t * get_desc() { return desc; }
	void set_desc(const weg_besch_t *new_desc) { desc = new_desc; }

	int gimme_neighbours(leitung_t **conn);
	static fabrik_t * suche_fab_4(koord pos);

	leitung_t(loadsave_t *file);
	leitung_t(koord3d pos, player_t *player);
	virtual ~leitung_t();

	// just book the costs for destruction
	void cleanup(player_t *);

	// for map rotation
	void rotate90();

	typ get_typ() const { return leitung; }

	const char *get_name() const {return "Leitung"; }

	/**
	 * waytype associated with this object
	 */
	waytype_t get_waytype() const { return powerline_wt; }

	/**
	* @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
	* Beobachtungsfenster angezeigt wird.
	* @author Hj. Malthaner
	*/
	void info(cbuffer_t & buf) const;

	ribi_t::ribi get_ribi() const { return ribi; }

	inline void set_image( image_id b ) { image = b; }
	image_id get_image() const {return is_crossing ? IMG_EMPTY : image;}
	image_id get_front_image() const {return is_crossing ? image : IMG_EMPTY;}

	/**
	* Recalculates the images of all neighbouring
	* powerlines and the powerline itself
	*
	* @author Hj. Malthaner
	*/
	void calc_neighbourhood();

	/**
	* Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
	* um das Aussehen des Dings an Boden und Umgebung anzupassen
	*
	* @author Hj. Malthaner
	*/
	virtual void finish_rd();

	/**
	* Speichert den Zustand des Objekts.
	*
	* @param file Zeigt auf die Datei, in die das Objekt geschrieben werden
	* soll.
	* @author Hj. Malthaner
	*/
	virtual void rdwr(loadsave_t *file);

	/**
	 * @return NULL if OK, otherwise an error message
	 * @author Hj. Malthaner
	 */
	virtual const char *is_deletable(const player_t *player);
};



class pumpe_t : public leitung_t
{
public:
	static void new_world();
	static void step_all(uint32 delta_t);

private:
	static slist_tpl<pumpe_t *> pumpe_list;

	fabrik_t *fab;
	uint32 supply;

	void step(uint32 delta_t);

public:
	pumpe_t(loadsave_t *file);
	pumpe_t(koord3d pos, player_t *player);
	~pumpe_t();

	typ get_typ() const { return pumpe; }

	const char *get_name() const {return "Aufspanntransformator";}

	void info(cbuffer_t & buf) const;

	void finish_rd();

	void calc_image() {}	// otherwise it will change to leitung

	const fabrik_t* get_factory() const { return fab; }
};


/*
 * Distribution transformers act as an interface between power networks and
 * and power consuming factories. They work in a pipelined way by taking the
 * energy demand of a factory, solving it between ticks and feeding the results
 * back the next tick.
 *
 * This buffering means that the factory will get the results for a demand
 * after 2 ticks, with the first tick getting the previous result. Any unfulfilled
 * demand from a result will be refunded to the factory so that the factory can
 * calculate how well demand is being fulfilled in general.
 */
class senke_t : public leitung_t, public sync_steppable
{
public:
	static void new_world();
	static void step_all(uint32 delta_t);

private:
	static slist_tpl<senke_t *> senke_list;

	sint32 einkommen;
	sint32 max_einkommen;
	fabrik_t *fab;
	sint32 delta_sum;
	sint32 next_t;

	// the power demand to be solved next tick
	uint32 next_power_demand;

	// the last power demand solved
	uint32 last_power_demand;

	// the last power satisfaction computed
	uint32 power_load;

	void step(uint32 delta_t);

public:
	senke_t(loadsave_t *file);
	senke_t(koord3d pos, player_t *player);
	~senke_t();

	typ get_typ() const { return senke; }

	// used to alternate between displaying power on and power off images at a frequency determined by the percentage of power supplied
	// gives players a visual indication of a power network with insufficient generation
	sync_result sync_step(uint32 delta_t);

	const char *get_name() const {return "Abspanntransformator";}

	void info(cbuffer_t & buf) const;

	void finish_rd();

	void calc_image() {}	// otherwise it will change to leitung

	const fabrik_t* get_factory() const { return fab; }
};


#endif
