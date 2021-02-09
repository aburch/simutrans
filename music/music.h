/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef MUSIC_MUSIC_H
#define MUSIC_MUSIC_H


#include "../simtypes.h"


#define MAX_MIDI (128)

/**
 * MIDI initialisation routine
 * false: leave midi alone
 */
bool dr_init_midi();


/**
 * sets midi playback volume
 * @param vol volume in range 0..255
 */
void dr_set_midi_volume(int vol);


/**
 * Loads a MIDI file
 */
int dr_load_midi(const char* filename);


/**
 * Plays a MIDI file
 */
void dr_play_midi(int key);


/**
 * Stops playing MIDI file
 */
void dr_stop_midi();


/**
 * @return -1 if current track has finished, 0 otherwise.
 */
sint32 dr_midi_pos();


/**
 * Midi shutdown/cleanup
 */
void dr_destroy_midi();


/**
 * Load a soundfont. Only available if we are using FluidSynth.
 */
#ifdef USE_FLUIDSYNTH_MIDI
bool dr_load_sf(const char * filename);
#endif

#endif
