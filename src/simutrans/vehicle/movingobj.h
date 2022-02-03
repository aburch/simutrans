/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef VEHICLE_MOVINGOBJ_H
#define VEHICLE_MOVINGOBJ_H


#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/vector_tpl.h"
#include "../descriptor/groundobj_desc.h"
#include "../simcolor.h"
#include "../obj/sync_steppable.h"

#include "vehicle_base.h"


/**
 * moving stuff like sheep or birds
 */
class movingobj_t : public vehicle_base_t, public sync_steppable
{
private:
	/// distance to move
	uint32 weg_next;

	/// direction will change after this time
	uint16 timetochange;

	/// type of object, index into movingobj_typen
	uint8 movingobjtype;

	koord3d pos_next_next;

	/// static table to find desc by name
	static stringhashtable_tpl<groundobj_desc_t *> desc_table;

	/// static vector for fast lookup of desc
	static vector_tpl<const groundobj_desc_t *> movingobj_typen;

protected:
	void rdwr(loadsave_t *file) OVERRIDE;

	void calc_image() OVERRIDE;

public:
	static bool register_desc(groundobj_desc_t *desc);
	static bool successfully_loaded();

	static const groundobj_desc_t *random_movingobj_for_climate(climate cl);

	movingobj_t(loadsave_t *file);
	movingobj_t(koord3d pos, const groundobj_desc_t *);
	~movingobj_t();

	sync_result sync_step(uint32 delta_t) OVERRIDE;

	// always free
	virtual bool check_next_tile(const grund_t *) const;
	grund_t* hop_check() OVERRIDE;
	void hop(grund_t* gr) OVERRIDE;
	waytype_t get_waytype() const OVERRIDE { return get_desc()->get_waytype(); }

	const char *get_name() const OVERRIDE {return "Movingobj";}
	typ get_typ() const OVERRIDE { return movingobj; }

	/**
	 * Called whenever the season or snowline height changes
	 * return false and the obj_t will be deleted
	 */
	bool check_season(const bool) OVERRIDE;

	void show_info() OVERRIDE;

	void info(cbuffer_t & buf) const OVERRIDE;

	void cleanup(player_t *player) OVERRIDE;

	const groundobj_desc_t* get_desc() const { return movingobj_typen[movingobjtype]; }

	bool is_flying() const OVERRIDE { return get_desc()->get_waytype()==air_wt; }

	void * operator new(size_t s);
	void operator delete(void *p);
};

#endif
