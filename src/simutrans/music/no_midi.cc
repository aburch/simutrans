/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "music.h"

// Dummy midi routines - only provide interface, does nothing

/**
 * sets midi playback volume
 */
void dr_set_midi_volume(int)
{
}


/**
 * Loads a MIDI file
 */
int dr_load_midi(const char *)
{
	return -1;
}


/**
 * Plays a MIDI file
 */
void dr_play_midi(int)
{
}


/**
 * Stops playing MIDI file
 */
void dr_stop_midi(void)
{
}


/**
 * Returns the midi_pos variable
 */
sint32 dr_midi_pos(void)
{
	return 0;
}


/**
 * Midi shutdown/cleanup
 */
void dr_destroy_midi(void)
{
}


/**
 * MIDI initialisation routines
 */
bool dr_init_midi(void)
{
	return false;
}
