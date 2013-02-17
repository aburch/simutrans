/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef dings_groundobj_h
#define dings_groundobj_h

#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/vector_tpl.h"
#include "../besch/groundobj_besch.h"
#include "../dataobj/umgebung.h"

/**
 * Decorative objects, like rocks, ponds etc.
 */
class groundobj_t : public ding_t
{
private:
	/// type of object, index into groundobj_typen
	uint16 groundobjtype;

	/// the image, cached
	image_id bild;

	/// table to lookup object based on name
	static stringhashtable_tpl<groundobj_besch_t *> besch_names;

	/// all such objects
	static vector_tpl<const groundobj_besch_t *> groundobj_typen;

public:
	static bool register_besch(groundobj_besch_t *besch);
	static bool alles_geladen();

	static const groundobj_besch_t *random_groundobj_for_climate(climate cl, hang_t::typ slope );

	groundobj_t(karte_t *welt, loadsave_t *file);
	groundobj_t(karte_t *welt, koord3d pos, const groundobj_besch_t *);

	void rdwr(loadsave_t *file);

	image_id get_bild() const { return bild; }

	/// recalculates image depending on season and slope of ground
	void calc_bild();

	const char *get_name() const {return "Groundobj";}
	typ get_typ() const { return groundobj; }

	bool check_season(const long delta_t);

	void zeige_info();

	void info(cbuffer_t & buf) const;

	void entferne(spieler_t *sp);

	const groundobj_besch_t* get_besch() const { return groundobj_typen[groundobjtype]; }

	void * operator new(size_t s);
	void operator delete(void *p);
};

#endif
