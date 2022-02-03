/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef VEHICLE_OVERTAKER_H
#define VEHICLE_OVERTAKER_H


#include "../simtypes.h"
#include "../simunits.h"


/**
 * Class dealing with overtaking
 * It is the superclass of convois and city cars (private_car_t)
 */
class overtaker_t
{
protected:
	/* if >0, number of tiles for overtaking left
	 * if <0, number of tiles that is prevented being overtaken
	 *        (other vehicle is overtaking us)
	 */
	sint8 tiles_overtaking;
	sint8 diff;

	sint32 max_power_speed; // max achievable speed at current power/weight
public:
	overtaker_t():tiles_overtaking(0), diff(0), max_power_speed(SPEED_UNLIMITED) {}
	virtual ~overtaker_t() {}

	bool is_overtaking() const { return diff < 0; }

	bool is_overtaken() const { return diff > 0; }

	bool can_be_overtaken() const { return tiles_overtaking == 0; }

	void set_tiles_overtaking(sint8 v) {
		tiles_overtaking = v;
		diff = 0;
		if(tiles_overtaking>0) {
			diff --;
		}
		else if(tiles_overtaking<0) {
			diff ++;
		}
	}

	// change counter for overtaking
	void update_tiles_overtaking() {
		tiles_overtaking += diff;
		if(tiles_overtaking==0) {
			diff = 0;
		}
	}

	// since citycars and convois can react quite different
	virtual bool can_overtake(overtaker_t *other_overtaker, sint32 other_speed, sint16 steps_other) = 0;

	sint32 get_max_power_speed() const { return max_power_speed; }
};

#endif
