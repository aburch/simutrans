#ifndef _overtaker_h
#define _overtaker_h

/**
 * All vehicles that can overtake must include this class
 * These are convois and city cars
 *
 * Oct 2008
 */

#include "../simtypes.h"

/**
 * Class dealing with overtaking
 * It is the superclass of convois and city cars (private_car_t)
 *
 * @author isidoro
 */
class overtaker_t
{
protected:
	/* if >0, number of tiles for overtaking left
	 * if <0, number of tiles that is prevented being overtaken
	 *        (other vehicle is overtaking us)
	 */
	sint8 tiles_overtaking;
	sint8 prev_tiles_overtaking;
	sint8 diff;

	sint32 max_power_speed; // max achievable speed at current power/weight
public:
	overtaker_t():tiles_overtaking(0), diff(0), max_power_speed(SPEED_UNLIMITED) {}
	virtual ~overtaker_t() {}

	bool is_overtaking() const { return diff < 0; }

	bool is_overtaken() const { return diff > 0; }

	bool can_be_overtaken() const { return tiles_overtaking == 0; }

	void set_tiles_overtaking(sint8 v) {
		prev_tiles_overtaking = tiles_overtaking;
		tiles_overtaking = v;
		diff = 0;
		if(tiles_overtaking>0) {
			diff --;
		}
		else if(tiles_overtaking<0) {
			diff ++;
		}
		refresh(prev_tiles_overtaking, tiles_overtaking);
	}

	// change counter for overtaking
	void update_tiles_overtaking() {
		prev_tiles_overtaking = tiles_overtaking;
		tiles_overtaking += diff;
		if(tiles_overtaking==0) {
			diff = 0;
		}
		refresh(prev_tiles_overtaking, tiles_overtaking);
	}

	// since citycars and convois can react quite different
	virtual bool can_overtake(overtaker_t *other_overtaker, sint32 other_speed, sint16 steps_other) = 0;

	sint32 get_max_power_speed() const { return max_power_speed; }

	sint8 get_tiles_overtaking() const { return tiles_overtaking; }

	sint8 get_prev_tiles_overtaking() const { return prev_tiles_overtaking; }

	virtual void refresh(sint8 prev_tiles_overtaking, sint8 current_tiles_overtaking) =0; // When the overtaker starts or ends overtaking, Mark image dirty to prevent glitch.
};

#endif
