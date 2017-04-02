#ifndef obj_wolke_t
#define obj_wolke_t

#include "../descriptor/skin_desc.h"
#include "../ifc/sync_steppable.h"
#include "../tpl/vector_tpl.h"
#include "../display/simimg.h"

class karte_t;

/**
 * smoke clouds (formerly sync_wolke_t)
 * @author Hj. Malthaner
 */
class wolke_t : public obj_no_info_t, public sync_steppable
{
private:
	static vector_tpl<const skin_desc_t *>all_clouds;

	uint16 purchase_time;	// clouds vanish when purchase_time>2500 => maximum 5 images ...
	sint8 base_y_off;
	uint8 cloud_nr;

public:
	static bool register_desc(const skin_desc_t *desc);

	wolke_t(loadsave_t *file);
	wolke_t(koord3d pos, sint8 xoff, sint8 yoff, const skin_desc_t *cloud );
	virtual ~wolke_t();

	sync_result sync_step(uint32 delta_t);

	const char* get_name() const { return "Wolke"; }
#ifdef INLINE_OBJ_TYPE
#else
	typ get_typ() const { return sync_wolke; }
#endif

	image_id get_image() const;

	void rdwr(loadsave_t *file);

	virtual void rotate90();
};


/**
 * following two classes are just for compatibility for old save games
 */
class async_wolke_t : public obj_t
{
public:
	async_wolke_t(loadsave_t *file);
#ifdef INLINE_OBJ_TYPE
#else
	typ get_typ() const { return async_wolke; }
#endif
	image_id get_image() const { return IMG_EMPTY; }
};

class raucher_t : public obj_t
{
public:
	raucher_t(loadsave_t *file);
#ifdef INLINE_OBJ_TYPE
#else
	typ get_typ() const { return raucher; }
#endif
	image_id get_image() const { return IMG_EMPTY; }
};

#endif
