/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef vehicle_movingobj_h
#define vehicle_movingobj_h

#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/vector_tpl.h"
#include "../besch/groundobj_besch.h"
#include "../simcolor.h"

#include "simvehikel.h"


/**
 * moving stuff like sheeps or birds
 * @author prissi
 */
class movingobj_t : public vehikel_basis_t, public sync_steppable
{
private:
	/// distance to move
	uint32 weg_next;

	/// direction will change after this time
	uint16 timetochange;

	/// type of object, index into movingobj_typen
	uint8 movingobjtype;

	koord pos_next_next;

	/// static table to find besch by name
	static stringhashtable_tpl<groundobj_besch_t *> besch_names;

	/// static vector for fast lookup of besch
	static vector_tpl<const groundobj_besch_t *> movingobj_typen;

protected:
	void rdwr(loadsave_t *file);

	void calc_bild();

public:
	static bool register_besch(groundobj_besch_t *besch);
	static bool alles_geladen();

	static const groundobj_besch_t *random_movingobj_for_climate(climate cl);

	movingobj_t(loadsave_t *file);
	movingobj_t(koord3d pos, const groundobj_besch_t *);
	~movingobj_t();

	bool sync_step(uint32 delta_t);

	// prissi: always free
	virtual bool ist_befahrbar(const grund_t *) const;
	virtual bool ist_weg_frei() { return 1; }
	virtual bool hop_check();
	virtual grund_t* hop();
	virtual waytype_t get_waytype() const { return get_besch()->get_waytype(); }

	const char *get_name() const {return "Movingobj";}
	typ get_typ() const { return movingobj; }

	bool check_season(const long delta_t);

	void zeige_info();

	void info(cbuffer_t & buf) const;

	void entferne(spieler_t *sp);

	const groundobj_besch_t* get_besch() const { return movingobj_typen[movingobjtype]; }

	void * operator new(size_t s);
	void operator delete(void *p);
};

#endif
