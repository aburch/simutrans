/* simsound.h
 *
 * Beschreibung der Schnittstelle zum Soundsystem.
 * von Hj. Malthaner, 1998, 2000
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef simsound_h
#define simsound_h

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
    short index;

    /**
     * Lautstaerke des soundeffekts 0=stille, 255=maximum
     * @author Hj. Malthaner
     */
    unsigned char volume;

    /**
     * Prioritaet des sounds. Falls die Anzahl der abspielbaren Sounds
     * vom System begrenzt wird, werden nur die Sounds hoher Priorität
     * abgespielt
     * @author Hj. Malthaner
     */
    unsigned char pri;
};


/**
 * laedt soundfiles gemaess soundconfiguration.
 * koennen soundfiles nicht geladen werden, dann wird
 * spaeter kein sound abgespielt.
 * @author Hj. Malthaner
 */
void sound_init();


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
void sound_play(const struct sound_info inf);



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
const char * sound_get_midi_title(int index);


/**
 * gets curent midi number
 * @author Hj. Malthaner
 */
int get_current_midi();



/* OWEN: MIDI routines */
extern void midi_init();
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
