/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <allegro.h>
#include <stdio.h>
#include "music.h"

static int midi_number = -1;

#define MAX_MIDI 30

MIDI *midi_samples[MAX_MIDI];


/**
 * nothing to do here
 */
void dr_init_midi(void)
{
}


/**
 * sets midi playback volume
 * @author Hj. Malthaner
 */
void dr_set_midi_volume(int vol)
{
    set_volume(-1, vol);
}


/**
 * Loads a MIDI file
 * @author Owen Rudge, changes by Hj. Malthaner
 */
int dr_load_midi(const char *filename)
{
    if (midi_number < MAX_MIDI - 1)
    {
	const int i = midi_number + 1;

	if(i >= 0 && i < MAX_MIDI) {

	    midi_samples[i] = load_midi(filename);

	    if(midi_samples[i]) {
		midi_number = i;
	    }
	}
    }

    return midi_number;
}


/**
 * Plays a MIDI file
 * Key: The index of the MIDI file to be played
 * By Owen Rudge
 */
void dr_play_midi(int key)
{
   if (midi_number > 0)
   {
      if (key >= 0 && key <= midi_number && midi_samples[key] != NULL)
         play_midi(midi_samples[key], FALSE); // loop
      else
	printf("\nMessage: MIDI: Unable to play MIDI %d\n", key);
   }
}


/**
 * Stops playing MIDI file
 * By Owen Rudge
 */
void dr_stop_midi(void)
{
   stop_midi();
}


/**
 * Returns the midi_pos variable
 * By Owen Rudge
 */
long dr_midi_pos(void)
{
  return midi_pos;
}


/**
 * Midi shutdown/cleanup
 * By Owen Rudge
 */
void dr_destroy_midi(void)
{
   int i;

   for (i = 0; i <= midi_number; i++) {
      if (midi_samples[i] != NULL) {
	destroy_midi(midi_samples[i]);
      }
   }
   midi_number = -1;
}


void set_midi_pos(int pos)
{
   midi_pos = pos;
}
