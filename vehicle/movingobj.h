/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef vehicle_movingobj_h
#define vehicle_movingobj_h

#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/vector_tpl.h"
#include "../besch/groundobj_besch.h"
#include "../simcolor.h"
#include "../dataobj/umgebung.h"

#include "simvehikel.h"


/**
 * moving stuff like sheeps or birds
 * @author prissi
 */
class movingobj_t : public vehikel_basis_t, public sync_steppable
{
private:
	// type of tree
	uint16 groundobjtype:12;
	uint16 season:4;

	uint32 timetolife;

	/**
	 * Entfernungszaehler
	 * @author Hj. Malthaner
	 */
	uint32 weg_next;

	// static for administration
	static stringhashtable_tpl<uint32> besch_names;
	static vector_tpl<const groundobj_besch_t *> movingobj_typen;

protected:
	void rdwr(loadsave_t *file);

	void calc_bild();

public:
	static bool register_besch(groundobj_besch_t *besch);
	static bool alles_geladen();

	static const groundobj_besch_t *random_movingobj_for_climate(climate cl);

	// only the load save constructor should be called outside
	// otherwise I suggest use the plant tree function (see below)
	movingobj_t(karte_t *welt, loadsave_t *file);
	movingobj_t(karte_t *welt, koord3d pos, const groundobj_besch_t *);

	bool sync_step(long delta_t);

	// prissi: always free
	virtual bool ist_weg_frei() { return 1; }
	virtual bool hop_check() { return 1; }
	virtual void hop();
	virtual waytype_t gib_waytype() const { return gib_besch()->get_waytype(); }

	const char *gib_name() const {return "Movingobj";}
	enum ding_t::typ gib_typ() const {return movingobj;}

	bool check_season(const long delta_t);

	void zeige_info();

	void info(cbuffer_t & buf) const;

	void entferne(spieler_t *sp);

	const groundobj_besch_t* gib_besch() const { return movingobj_typen[groundobjtype]; }

	void * operator new(size_t s);
	void operator delete(void *p);
};

#endif
