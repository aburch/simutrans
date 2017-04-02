#ifndef simpeople_h
#define simpeople_h

#include "simroadtraffic.h"

class pedestrian_desc_t;

/**
 * Pedestrians also are road users.
 *
 * @author Hj. Malthaner
 * @see road_user_t
 */
class pedestrian_t : public road_user_t
{
private:
	static stringhashtable_tpl<const pedestrian_desc_t *> table;

private:
	const pedestrian_desc_t *desc;

protected:
	void rdwr(loadsave_t *file);

	void calc_image();

	/**
	 * Creates pedestrian at position given by @p gr.
	 * Does not add pedestrian to the tile!
	 */
	pedestrian_t(grund_t *gr, uint32 time_to_live = 0);

public:
	pedestrian_t(loadsave_t *file);

	virtual ~pedestrian_t();

	const pedestrian_desc_t *get_desc() const { return desc; }

	const char *get_name() const {return "Fussgaenger";}
#ifdef INLINE_OBJ_TYPE
#else
	typ get_typ() const { return pedestrian; }
#endif

	sync_result sync_step(uint32 delta_t);

	virtual grund_t* hop_check();
	virtual void hop(grund_t* gr);

	// class register functions
	static bool register_desc(const pedestrian_desc_t *desc);
	static bool successfully_loaded();

	static void generate_pedestrians_at(koord3d k, uint32 anzahl, uint32 time_to_live = 0);

	static void check_timeline_pedestrians(); 
};

#endif
