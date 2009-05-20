/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef dings_groundobj_h
#define dings_groundobj_h

#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/vector_tpl.h"
#include "../besch/groundobj_besch.h"
#include "../simcolor.h"
#include "../dataobj/umgebung.h"

/**
 * Bäume in Simutrans.
 * @author Hj. Malthaner
 */
class groundobj_t : public ding_t
{
private:
	// type of tree
	uint16 groundobjtype:12;
	uint16 season:4;

	image_id bild;
	uint16 age;	// in month

	// static for administration
	static stringhashtable_tpl<groundobj_besch_t *> besch_names;
	static vector_tpl<const groundobj_besch_t *> groundobj_typen;

public:
	static bool register_besch(groundobj_besch_t *besch);
	static bool alles_geladen();

	static const groundobj_besch_t *random_groundobj_for_climate(climate cl, hang_t::typ slope );

	// only the load save constructor should be called outside
	// otherwise I suggest use the plant tree function (see below)
	groundobj_t(karte_t *welt, loadsave_t *file);
	groundobj_t(karte_t *welt, koord3d pos, const groundobj_besch_t *);

	void rdwr(loadsave_t *file);

	// since the lookup of slopes is slow, image is cached
	image_id get_bild() const { return bild; }

	/**
	 * Berechnet Alter und Bild abhängig vom Alter
	 * @author Hj. Malthaner
	 */
	void calc_bild();

	const char *get_name() const {return "Groundobj";}
	enum ding_t::typ get_typ() const {return groundobj;}

	bool check_season(const long delta_t);

	void zeige_info();

	void info(cbuffer_t & buf) const;

	void entferne(spieler_t *sp);

	const groundobj_besch_t* get_besch() const { return groundobj_typen[groundobjtype]; }

	void * operator new(size_t s);
	void operator delete(void *p);
};

#endif
