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

	uint16 insta_zeit;	// clouds vanish when insta_zeit>2500 => maximum 5 images ...
	sint8 base_y_off;
	uint8 cloud_nr;

public:
	static bool register_desc(const skin_desc_t *desc);

	wolke_t(loadsave_t *file);
	wolke_t(koord3d pos, sint8 xoff, sint8 yoff, const skin_desc_t *cloud );
	~wolke_t();

	sync_result sync_step(uint32 delta_t) OVERRIDE;

	const char* get_name() const OVERRIDE { return "Wolke"; }
	typ get_typ() const OVERRIDE { return sync_wolke; }

	image_id get_image() const OVERRIDE;

	void rdwr(loadsave_t *file) OVERRIDE;

	void rotate90() OVERRIDE;
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
