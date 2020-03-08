/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/// stub for no sound

#include "sound.h"


bool dr_init_sound(void)
{
	return false;
}


/**
 * loads a sample
 * @return a handle for that sample or -1 on failure
 */
int dr_load_sample(const char *)
{
	return -1;
}


/**
 * plays a sample
 * @param key the key for the sample to be played
 */
void dr_play_sample(int, int)
{
}
