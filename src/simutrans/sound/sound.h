/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SOUND_SOUND_H
#define SOUND_SOUND_H


/**
 * Sound initialisation routine
 */
bool dr_init_sound();

/**
 * loads a sample
 * @return a handle for that sample or -1 on failure
 */
int dr_load_sample(const char* filename);

/**
 * plays a sample
 * @param key the handle for the sample to be played
 */
void dr_play_sample(int key, int volume);

#endif
