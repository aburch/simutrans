#ifndef simpeople_h
#define simpeople_h

#include "simroadtraffic.h"

class fussgaenger_besch_t;

/**
 * Pedestrians also are road users.
 *
 * @author Hj. Malthaner
 * @see road_user_t
 */
class pedestrian_t : public road_user_t
{
private:
	static stringhashtable_tpl<const fussgaenger_besch_t *> table;

private:
	const fussgaenger_besch_t *besch;

protected:
	void rdwr(loadsave_t *file);

	void calc_image();

	/**
	 * Creates pedestrian at position given by @p gr.
	 * Does not add pedestrian to the tile!
	 */
	pedestrian_t(grund_t *gr);

public:
	pedestrian_t(loadsave_t *file);

	virtual ~pedestrian_t();

	const fussgaenger_besch_t *get_besch() const { return besch; }

	const char *get_name() const {return "Fussgaenger";}
#ifdef INLINE_OBJ_TYPE
#else
	typ get_typ() const { return pedestrian; }
#endif

	bool sync_step(long delta_t);

	// prissi: always free
	virtual bool can_enter_tile() { return true; }
	virtual grund_t* hop_check();
	virtual void hop(grund_t* gr);

	// class register functions
	static bool register_besch(const fussgaenger_besch_t *besch);
	static bool alles_geladen();

	static void generate_pedestrians_at(koord3d k, int &anzahl);
};

#endif
