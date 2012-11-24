/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef dings_leitung_t
#define dings_leitung_t


#include "../ifc/sync_steppable.h"
#include "../dataobj/koord3d.h"
#include "../dataobj/ribi.h"
#include "../simdings.h"
#include "../tpl/slist_tpl.h"

#define POWER_TO_MW (12)  // bitshift for converting internal power values to MW for display

class powernet_t;
class spieler_t;
class fabrik_t;
class weg_besch_t;

class leitung_t : public ding_t
{
protected:
	image_id bild;

	// powerline over ways
	bool is_crossing:1;

	// direction of the next pylon
	ribi_t::ribi ribi:4;

	/**
	* We are part of this network
	* @author Hj. Malthaner
	*/
	powernet_t * net;

	const weg_besch_t *besch;

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
	void calc_bild();

public:
	powernet_t* get_net() const { return net; }
	void set_net(powernet_t* p) { net = p; }

	const weg_besch_t * get_besch() { return besch; }
	void set_besch(const weg_besch_t *new_besch) { besch = new_besch; }

	int gimme_neighbours(leitung_t **conn);
	static fabrik_t * suche_fab_4(koord pos);

	leitung_t(karte_t *welt, loadsave_t *file);
	leitung_t(karte_t *welt, koord3d pos, spieler_t *sp);
	virtual ~leitung_t();

	// just book the costs for destruction
	void entferne(spieler_t *);

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

	ribi_t::ribi get_ribi(void) const { return ribi; }

	inline void set_bild( image_id b ) { bild = b; }
	image_id get_bild() const {return is_crossing ? IMG_LEER : bild;}
	image_id get_after_bild() const {return is_crossing ? bild : IMG_LEER;}

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
	virtual void laden_abschliessen();

	/**
	* Speichert den Zustand des Objekts.
	*
	* @param file Zeigt auf die Datei, in die das Objekt geschrieben werden
	* soll.
	* @author Hj. Malthaner
	*/
	virtual void rdwr(loadsave_t *file);
};



class pumpe_t : public leitung_t
{
public:
	static void neue_karte();
	static void step_all(long delta_t);

private:
	static slist_tpl<pumpe_t *> pumpe_list;

	fabrik_t *fab;
	uint32 supply;

	void step(long delta_t);

public:
	pumpe_t(karte_t *welt, loadsave_t *file);
	pumpe_t(karte_t *welt, koord3d pos, spieler_t *sp);
	~pumpe_t();

	typ get_typ() const { return pumpe; }

	const char *get_name() const {return "Aufspanntransformator";}

	void info(cbuffer_t & buf) const;

	void laden_abschliessen();

	void calc_bild() {}	// otherwise it will change to leitung

	const fabrik_t* get_factory() const { return fab; }
};



class senke_t : public leitung_t, public sync_steppable
{
public:
	static void neue_karte();
	static void step_all(long delta_t);

private:
	static slist_tpl<senke_t *> senke_list;

	sint32 einkommen;
	sint32 max_einkommen;
	fabrik_t *fab;
	sint32 delta_sum;
	sint32 next_t;
	uint32 last_power_demand;
	uint32 power_load;

	void step(long delta_t);

public:
	senke_t(karte_t *welt, loadsave_t *file);
	senke_t(karte_t *welt, koord3d pos, spieler_t *sp);
	~senke_t();

	typ get_typ() const { return senke; }

	// used to alternate between displaying power on and power off images at a frequency determined by the percentage of power supplied
	// gives players a visual indication of a power network with insufficient generation
	bool sync_step(long delta_t);

	const char *get_name() const {return "Abspanntransformator";}

	void info(cbuffer_t & buf) const;

	void laden_abschliessen();

	void calc_bild() {}	// otherwise it will change to leitung

	const fabrik_t* get_factory() const { return fab; }
};


#endif
