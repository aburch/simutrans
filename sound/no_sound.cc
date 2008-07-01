/*
 * stub for no sound
 *
 * This file is part of the Simutrans project under the artistic licence.
 */


#include "sound.h"


void dr_init_sound(void)
{
}


/**
 * loads a sample
 * @return a handle for that sample or -1 on failure
 * @author Hj. Malthaner
 */
int dr_load_sample(const char *filename)
{
    return -1;
}


/**
 * plays a sample
 * @param key the key for the sample to be played
 * @author Hj. Malthaner
 */
void dr_play_sample(int key, int volume)
{
}
