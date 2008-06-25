/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * Dummy midi routines - only provide interface, does nothing
 *
 * author: Hj. Malthaner
 * date:   25-May-2002
 */

#include "music.h"


/**
 * sets midi playback volume
 * @author Hj. Malthaner
 */
void dr_set_midi_volume(int vol)
{
}


/**
 * Loads a MIDI file
 * @author Hj. Malthaner
 */
int dr_load_midi(const char * filename)
{
    return -1;
}


/**
 * Plays a MIDI file
 * @author Hj. Malthaner
 */
void dr_play_midi(int key)
{
}


/**
 * Stops playing MIDI file
 * @author Hj. Malthaner
 */
void dr_stop_midi(void)
{
}


/**
 * Returns the midi_pos variable
 * @author Hj. Malthaner
 */
long dr_midi_pos(void)
{
    return 0;
}


/**
 * Midi shutdown/cleanup
 * @author Hj. Malthaner
 */
void dr_destroy_midi(void)
{
}


/**
 * Sets midi pos
 * @author Hj. Malthaner
 */
void set_midi_pos(int pos)
{
}

/**
 * MIDI initialisation routines
 * @author Owen Rudge
 */
void dr_init_midi(void)
{
}
