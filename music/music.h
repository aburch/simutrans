#ifndef MUSIC_H
#define MUSIC_H


#define MAX_MIDI (128)

/**
 * MIDI initialisation routine
 * false: leave midi alone
 */
bool dr_init_midi();


/**
 * sets midi playback volume
 * @param vol volume in range 0..255
 * @author Hj. Malthaner
 */
void dr_set_midi_volume(int vol);


/**
 * Loads a MIDI file
 * @author Owen Rudge
 */
int dr_load_midi(const char* filename);


/**
 * Plays a MIDI file
 * @author Owen Rudge
 */
void dr_play_midi(int key);


/**
 * Stops playing MIDI file
 * @author Owen Rudge
 */
void dr_stop_midi();


/**
 * Returns the midi_pos variable
 * @return -1 if current track has finished, 0 otherwise.
 * @author Owen Rudge
 */
long dr_midi_pos();


/**
 * Midi shutdown/cleanup
 * @author Owen Rudge
 */
void dr_destroy_midi();

#endif
