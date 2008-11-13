#ifndef MUSIC_H
#define MUSIC_H


#define MAX_MIDI (128)

/**
 * MIDI initialisation routine
 * false: leave midi alone
 */
bool dr_init_midi(void);


/**
 * sets midi playback volume
 * @param vol volume in range 0..255
 * @author Hj. Malthaner
 */
void dr_set_midi_volume(int vol);


/**
 * gets midi title
 * @author Hj. Malthaner
 */
const char* dr_get_midi_title(int index);


/**
 * MIDI playing routines
 * @author Owen Rudge
 */
int dr_load_midi(const char* filename);


/**
 * MIDI playing routines
 * @author Owen Rudge
 */
void dr_play_midi(int key);


/**
 * MIDI playing routines
 * @author Owen Rudge
 */
void dr_stop_midi(void);


/**
 * MIDI playing routines
 * @author Owen Rudge
 */
long dr_midi_pos(void);


/**
 * Midi shutdown/cleanup
 * @author Owen Rudge
 */
void dr_destroy_midi(void);

void set_midi_pos(int);

#endif
