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

class powernet_t;
class spieler_t;
class fabrik_t;

class leitung_t : public ding_t
{
protected:
	image_id bild;

	// direction of the next pylon
	ribi_t::ribi ribi;

	/**
	* We are part of this network
	* @author Hj. Malthaner
	*/
	powernet_t * net;

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
	void recalc_bild();

public:
	powernet_t* get_net() const { return net; }
	void set_net(powernet_t* p) { net = p; }

	int gimme_neighbours(leitung_t **conn);
	static fabrik_t * suche_fab_4(koord pos);

	leitung_t(karte_t *welt, loadsave_t *file);
	leitung_t(karte_t *welt, koord3d pos, spieler_t *sp);
	virtual ~leitung_t();

	// just book the costs for destruction
	void entferne(spieler_t *);

	// for map rotation
	void rotate90();

	enum ding_t::typ get_typ() const {return leitung;}

	const char *get_name() const {return "Leitung"; }

	/**
	* @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
	* Beobachtungsfenster angezeigt wird.
	* @author Hj. Malthaner
	*/
	void info(cbuffer_t & buf) const;

	ribi_t::ribi get_ribi(void) const { return ribi; }

	inline void set_bild( image_id b ) { bild = b; }
	image_id get_bild() const {return bild;}

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



class pumpe_t : public leitung_t, public sync_steppable
{
private:
	fabrik_t *fab;


public:
	pumpe_t(karte_t *welt, loadsave_t *file);
	pumpe_t(karte_t *welt, koord3d pos, spieler_t *sp);
	~pumpe_t();

	enum ding_t::typ get_typ() const {return pumpe;}

	bool sync_step(long delta_t);

	const char *name() const {return "Pumpe";}

	void laden_abschliessen();

	void calc_bild() {}	// otherwise it will change to leitung
};



class senke_t : public leitung_t, public sync_steppable
{
private:
	sint32 einkommen;
	sint32 max_einkommen;
	fabrik_t *fab;

public:
	senke_t(karte_t *welt, loadsave_t *file);
	senke_t(karte_t *welt, koord3d pos, spieler_t *sp);
	~senke_t();

	enum ding_t::typ get_typ() const {return senke;}

	bool sync_step(long delta_t);

	const char *name() const {return "Senke";}

	void info(cbuffer_t & buf) const;

	void laden_abschliessen();

	void calc_bild() {}	// otherwise it will change to leitung
};


#endif
