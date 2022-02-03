/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef OBJ_WOLKE_H
#define OBJ_WOLKE_H


#include "simobj.h"

#include "../descriptor/skin_desc.h"
#include "../obj/sync_steppable.h"
#include "../tpl/vector_tpl.h"
#include "../display/simimg.h"


class karte_t;


#define DEFAULT_EXHAUSTSMOKE_TIME (2497)


/**
 * smoke clouds (formerly sync_wolke_t)
 */
class wolke_t : public obj_no_info_t, public sync_steppable
{
private:
	static vector_tpl<const skin_desc_t *>all_clouds;

	uint16 insta_zeit; // clouds vanish when insta_zeit>2500 => maximum 5 images ...
	sint16 base_y_off; // since sint8 may overflow wiht larger pak sizes
	uint16 lifetime;   // since factories generate other smoke than
	uint16 uplift;
	uint8 cloud_nr;

	sint16 calc_yoff() const; // calculate the smoke height using uplift and insta_zeit

public:
	static bool register_desc(const skin_desc_t *desc);

	void * operator new(size_t s);
	void operator delete(void *p);

	wolke_t(loadsave_t *file);
	wolke_t(koord3d pos, sint8 xoff, sint8 yoff, sint16 hoff, uint16 lifetime, uint16 uplift, const skin_desc_t *cloud );
	~wolke_t();

	sync_result sync_step(uint32 delta_t) OVERRIDE;

	const char* get_name() const OVERRIDE { return "Wolke"; }
	typ get_typ() const OVERRIDE { return sync_wolke; }

	image_id get_image() const OVERRIDE { return IMG_EMPTY; }

	image_id get_front_image() const OVERRIDE;

	void rdwr(loadsave_t *file) OVERRIDE;

	void rotate90() OVERRIDE;

	/**
	* Draw foreground image
	* (everything that is in front of vehicles)
	*/
#ifdef MULTI_THREAD
	virtual void display_after( int xpos, int ypos, const sint8 clip_num ) const OVERRIDE;
#else
	virtual void display_after( int xpos, int ypos, bool is_global ) const OVERRIDE;
#endif
};


/**
 * following two classes are just for compatibility for old save games
 */
class async_wolke_t : public obj_t
{
public:
	async_wolke_t(loadsave_t *file);
	typ get_typ() const OVERRIDE { return async_wolke; }
	image_id get_image() const OVERRIDE { return IMG_EMPTY; }
};

class raucher_t : public obj_t
{
public:
	raucher_t(loadsave_t *file);
	typ get_typ() const OVERRIDE { return raucher; }
	image_id get_image() const OVERRIDE { return IMG_EMPTY; }
};

#endif
