/*
 * The interface to the simutrans sound system.
 * von Hj. Malthaner, 1998, 2000
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef simsound_h
#define simsound_h

#include "simtypes.h"

// sound can be selectively muted (but volume is not touched)
void sound_set_mute(bool new_flag);
bool sound_get_mute();

/**
 * setzt Lautstärke für alle effekte
 * @author Hj. Malthaner
 */
void sound_set_global_volume(int volume);


/**
 * ermittelt Lautstärke für alle effekte
 * @author Hj. Malthaner
 */
int sound_get_global_volume();

/// and settign volume for specific sopunds
void sound_set_specific_volume( int volume, sound_type_t t );
int sound_get_specific_volume( sound_type_t t );

/**
 * Play a sound.
 *
 * @param idx    Index of the sound
 * @param volume Volume of the sound, 0 = silence, 255 = max
 */
void sound_play(uint16 idx, uint8 volume, sound_type_t t );


// shuffle enable/disable for midis
bool sound_get_shuffle_midi();
void sound_set_shuffle_midi( bool shuffle );


/**
 * setzt Lautstärke für MIDI playback
 * @param volume volume in range 0..255
 * @author Hj. Malthaner
 */
void sound_set_midi_volume(int volume);


/**
 * ermittelt Lautstärke für MIDI playback
 * @return volume in range 0..255
 * @author Hj. Malthaner
 */
int sound_get_midi_volume();


/**
 * gets midi title
 * @author Hj. Malthaner
 */
const char *sound_get_midi_title(int index);


/**
 * gets curent midi number
 * @author Hj. Malthaner
 */
int get_current_midi();

// when muted, midi is not used
void midi_set_mute(bool on);
bool midi_get_mute();

/* OWEN: MIDI routines */
extern int midi_init(const char *path);
extern void midi_play(const int no);
extern void check_midi();


/**
 * shuts down midi playing
 * @author Owen Rudge
 */
extern void close_midi();

extern void midi_next_track();
extern void midi_last_track();
extern void midi_stop();

#endif
