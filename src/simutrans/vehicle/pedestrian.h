/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef VEHICLE_PEDESTRIAN_H
#define VEHICLE_PEDESTRIAN_H


#include "simroadtraffic.h"
#include "../tpl/freelist_iter_tpl.h"


class pedestrian_desc_t;


/**
 * Pedestrians also are road users.
 * @see road_user_t
 */
class pedestrian_t : public road_user_t
{
private:
	static stringhashtable_tpl<const pedestrian_desc_t *> table;

private:
	const pedestrian_desc_t *desc;
	uint16 animation_steps;

	uint16 steps_offset;
	uint16 ped_offset;
	bool on_left;

	bool list_empty();

	static freelist_iter_tpl<pedestrian_t> fl; // if not declared static, it would consume 4 bytes due to empty class nonzero rules

protected:
	void rdwr(loadsave_t *file) OVERRIDE;

	void calc_image() OVERRIDE;

	/**
	 * Creates pedestrian at position given by @p gr.
	 * Does not add pedestrian to the tile!
	 */
	pedestrian_t(grund_t *gr);

public:
	pedestrian_t(loadsave_t *file);

	static void sync_handler(uint32 delta_t) { fl.sync_step(delta_t); }

	const pedestrian_desc_t *get_desc() const { return desc; }

	const char *get_name() const OVERRIDE {return "Fussgaenger";}
	typ get_typ() const OVERRIDE { return pedestrian; }

	void info(cbuffer_t & buf) const OVERRIDE;

	sync_result sync_step(uint32 delta_t) OVERRIDE;

	///@ returns true if pedestrian walks on the left side of the road
	bool is_on_left() const { return on_left; }

	void calc_disp_lane();

	void rotate90() OVERRIDE;

	// overloaded to enable animations
	image_id get_image() const OVERRIDE;

	void get_screen_offset( int &xoff, int &yoff, const sint16 raster_width ) const OVERRIDE;

	grund_t* hop_check() OVERRIDE;
	void hop(grund_t* gr) OVERRIDE;

	void* operator new(size_t) { return fl.gimme_node(); }
	void operator delete(void* p) { return fl.putback_node(p); }

	// class register functions
	static bool register_desc(const pedestrian_desc_t *desc);
	static bool successfully_loaded();

	static void generate_pedestrians_at(koord3d k, int &count);

	static void build_timeline_list( karte_t *welt );
};

#endif
