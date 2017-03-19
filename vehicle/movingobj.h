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
#include "../descriptor/groundobj_desc.h"
#include "../simcolor.h"
#include "../ifc/sync_steppable.h"

#include "simvehicle.h"


/**
 * moving stuff like sheeps or birds
 * @author prissi
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
	static stringhashtable_tpl<groundobj_desc_t *> desc_names;

	/// static vector for fast lookup of desc
	static vector_tpl<const groundobj_desc_t *> movingobj_typen;

protected:
	void rdwr(loadsave_t *file);

	void calc_image();

public:
	static bool register_desc(groundobj_desc_t *desc);
	static bool successfully_loaded();

	static const groundobj_desc_t *random_movingobj_for_climate(climate cl);

	movingobj_t(loadsave_t *file);
	movingobj_t(koord3d pos, const groundobj_desc_t *);
	virtual ~movingobj_t();

	sync_result sync_step(uint32 delta_t);

	// prissi: always free
	virtual bool check_next_tile(const grund_t *) const;
	virtual bool can_enter_tile() { return 1; }
	virtual grund_t* hop_check();
	virtual void hop(grund_t* gr);
	virtual void update_bookkeeping(uint32) {};

	virtual waytype_t get_waytype() const { return get_desc()->get_waytype(); }

	const char *get_name() const {return "Movingobj";}
#ifndef INLINE_OBJ_TYPE
	typ get_typ() const { return movingobj; }
#endif

	/**
	 * Called whenever the season or snowline height changes
	 * return false and the obj_t will be deleted
	 */
	bool check_season(const bool);

	void show_info();

	void info(cbuffer_t & buf, bool dummy = false) const;

	void cleanup(player_t *player);

	const groundobj_desc_t* get_desc() const { return movingobj_typen[movingobjtype]; }

	virtual bool is_flying() const { return get_desc()->get_waytype() == air_wt; }

	void * operator new(size_t s);
	void operator delete(void *p);
};

#endif
