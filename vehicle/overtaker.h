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
 * It is the superclass of convois and city cars (stadtauto_t)
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
	sint8 diff;

public:
	overtaker_t():tiles_overtaking(0), diff(0) {}
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
	virtual bool can_overtake(overtaker_t *other_overtaker, int other_speed, int steps_other, int diagonal_length) = 0;
};

#endif
