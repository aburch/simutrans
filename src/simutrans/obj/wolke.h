/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef OBJ_WOLKE_H
#define OBJ_WOLKE_H


#include "simobj.h"

#include "../descriptor/skin_desc.h"
#include "../tpl/vector_tpl.h"
#include "../display/simimg.h"

#include "../tpl/freelist_iter_tpl.h"


class karte_t;


#define DEFAULT_EXHAUSTSMOKE_TIME (2497)


/**
 * smoke clouds (formerly sync_wolke_t)
 */
class wolke_t : public obj_no_info_t
{
private:
	static vector_tpl<const skin_desc_t *>all_clouds;

	uint16 insta_zeit; // clouds vanish when insta_zeit>2500 => maximum 5 images ...
	sint16 base_y_off; // since sint8 may overflow wiht larger pak sizes
	uint16 lifetime;   // since factories generate other smoke than
	uint16 uplift;
	uint8 cloud_nr;

	sint16 calc_yoff() const; // calculate the smoke height using uplift and insta_zeit

	static freelist_iter_tpl<wolke_t> fl; // if not declared static, it would consume 4 bytes due to empty class nonzero rules

public:
	static bool register_desc(const skin_desc_t *desc);

	wolke_t(loadsave_t *file);
	wolke_t(koord3d pos, sint8 xoff, sint8 yoff, sint16 hoff, uint16 lifetime, uint16 uplift, const skin_desc_t *cloud );
	~wolke_t();

	sync_result sync_step(uint32 delta_t);

	void* operator new(size_t) { return fl.gimme_node(); }
	void operator delete(void* p) { return fl.putback_node(p); }

	static void sync_handler(uint32 delta_t) { fl.sync_step(delta_t); }

	const char* get_name() const OVERRIDE { return "Wolke"; }
	typ get_typ() const OVERRIDE { return cloud; }

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
	typ get_typ() const OVERRIDE { return old_async_wolke; }
	image_id get_image() const OVERRIDE { return IMG_EMPTY; }
};

class raucher_t : public obj_t
{
public:
	raucher_t(loadsave_t *file);
	typ get_typ() const OVERRIDE { return old_raucher; }
	image_id get_image() const OVERRIDE { return IMG_EMPTY; }
};

#endif
