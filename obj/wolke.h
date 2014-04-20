#ifndef obj_wolke_t
#define obj_wolke_t

#include "../besch/skin_besch.h"
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
	static vector_tpl<const skin_besch_t *>all_clouds;

	uint16 insta_zeit;	// clouds vanish when insta_zeit>2500 => maximum 5 images ...
	sint8 base_y_off;
	sint8 cloud_nr;

public:
	static bool register_besch(const skin_besch_t *besch);

	wolke_t(loadsave_t *file);
	wolke_t(koord3d pos, sint8 xoff, sint8 yoff, const skin_besch_t *cloud );
	~wolke_t();

	bool sync_step(uint32 delta_t);

	const char* get_name() const { return "Wolke"; }
	typ get_typ() const { return sync_wolke; }

	image_id get_bild() const;

	void rdwr(loadsave_t *file);

	virtual void rotate90();
};


/**
 * follwoing two classes are just for compatibility for old save games
 */
class async_wolke_t : public obj_t
{
public:
	async_wolke_t(loadsave_t *file);
	typ get_typ() const { return async_wolke; }
	image_id get_bild() const { return IMG_LEER; }
};

class raucher_t : public obj_t
{
public:
	raucher_t(loadsave_t *file);
	typ get_typ() const { return raucher; }
	image_id get_bild() const { return IMG_LEER; }
};

#endif
