/*
 * Beschreibung der Schnittstelle zum Soundsystem.
 * von Hj. Malthaner, 1998, 2000
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef simsound_h
#define simsound_h

#include "simtypes.h"


/**
 * info zu einem abzuspielenden sound
 * @author Hj. Malthaner
 */
struct sound_info
{
	/**
	 * Index des beschriebenen sounds
	 * @author Hj. Malthaner
	 */
	uint16 index;

	/**
	 * Lautstaerke des soundeffekts 0=stille, 255=maximum
	 * @author Hj. Malthaner
	 */
	uint8 volume;

	/**
	 * Prioritaet des sounds. Falls die Anzahl der abspielbaren Sounds
	 * vom System begrenzt wird, werden nur die Sounds hoher Priorität
	 * abgespielt
	 *
	 * Priority of the sounds. If the number of playable sounds in the
	 * system is limited, only the sounds played high priority (Google)
	 * @author Hj. Malthaner
	 */
	uint8 pri;
};


// when muted, sound is not played (and also volume is not touched)
void sound_set_mute(bool on);
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


/**
 * spielt sound ab
 * @author Hj. Malthaner
 */
void sound_play(const sound_info inf);


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
