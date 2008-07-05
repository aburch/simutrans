#ifndef SOUND_H
#define SOUND_H


/**
 * Sound initialisation routine
 */
void dr_init_sound(void);

/**
 * loads a sample
 * @return a handle for that sample or -1 on failure
 * @author Hj. Malthaner
 */
int dr_load_sample(const char* filename);

/**
 * plays a sample
 * @param key the key for the sample to be played
 * @author Hj. Malthaner
 */
void dr_play_sample(int key, int volume);

#endif
