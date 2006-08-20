/*
 * simsound.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
 * High-Level soundschnittstelle
 * von Hj. Maltahner, 1998, 2000
 */

#include <stdio.h>
#include <string.h>
#include "simdebug.h"
#include "music/music.h"
#include "besch/sound_besch.h"
#include "sound/sound.h"
#include "simsound.h"
#include "simsys.h"
#include "simio.h"

#include "utils/simstring.h"


/**
 * max sound index
 * @author hj. Malthaner
 */
static int new_midi = 0;


#define MAX_MIDI_TITLE 30


static char midi_title[128][MAX_MIDI_TITLE];


/**
 * Gesamtlautstärke
 * @author hj. Malthaner
 */
static int global_volume = 128;


/**
 * MIDI Lautstärke
 * @author hj. Malthaner
 */
static int midi_volume = 128;


static int max_midi = -1; // number of MIDI files


static int current_midi = -1;  // Hajo: init with error condition,
                               // reset during loading


/**
 * setzt lautstaärke für alle effekte
 * @author Hj. Malthaner
 */
void sound_set_global_volume(int volume)
{
  // printf("Message: setting global volume to %d.\n", volume);
  global_volume = volume;
}


/**
 * ermittelt lautstaärke für alle effekte
 * @author Hj. Malthaner
 */
int sound_get_global_volume()
{
  return global_volume;
}


/**
 * spielt sound ab
 * @author Hj. Malthaner
 */
void
sound_play(const struct sound_info info)
{
	if(info.index!=NO_SOUND) {
DBG_MESSAGE("karte_t::interactive_event(event_t &ev)", "play sound %i",info.index);
		dr_play_sample(info.index, (info.volume*global_volume)>>8);
	}
}


/**
 * setzt Lautstärke für MIDI playback
 * @param volume volume in range 0..255
 * @author Hj. Malthaner
 */
void sound_set_midi_volume(int volume)
{
  dr_set_midi_volume(volume);
  midi_volume = volume;
}


/**
 * Sets the MIDI volume variable - for internal use only
 * @param volume Volume in range 0-255
 * @author Owen Rudge
 */
extern "C" void sound_set_midi_volume_var(int volume)
{
   midi_volume = volume;
}


/**
 * ermittelt Lautstärke für MIDI playback
 * @return volume in range 0..255
 * @author Hj. Malthaner
 */
int sound_get_midi_volume()
{
    return midi_volume;
}


/**
 * gets midi title
 * @author Hj. Malthaner
 */
const char * sound_get_midi_title(int index)
{
    if(index >= 0 && index <= max_midi && midi_title[index] != NULL) {
	return midi_title[index];
    } else {
	return "Invalid MIDI index!";
    }
}


/**
 * gets curent midi number
 * @author Hj. Malthaner
 */
int get_current_midi()
{
    return current_midi;
}


/**
 * Load MIDI files
 * By Owen Rudge
 */
void
midi_init()
{
  // read a list of soundfiles

  FILE * file = fopen("music.tab", "rb");

  if(file) {

    dr_init_midi();

    while(!feof(file)) {
      char buf[80];
      char title[80];
      long len;

      read_line(buf, 80, file);
      read_line(title, 80, file);
      if(!feof(file)) {
	len = strlen(buf);

	while(buf[--len] < 32) {
	  buf[len] = 0;
	}
	if(len > 1) {
	  print("  Reading MIDI file '%s' - %s", buf, title);
	  max_midi = dr_load_midi(buf);

	  if(max_midi >= 0 && max_midi < MAX_MIDI_TITLE) {
	    tstrncpy(midi_title[max_midi], title, 128);
	  }
	}
      }
    }

    fclose(file);
  } else {
    dbg->warning("midi_init()",
		 "can't open file 'music.tab' for reading, turning music off.");
  }

  if(max_midi >= 0) {
    current_midi = 0;
  }
}


void midi_play(const int no)
{
  if (no > max_midi) {
    dbg->warning("midi_play()",
		 "MIDI index %d too high (total loaded: %d)",
		 no, max_midi);
  } else {
    dr_play_midi(no);
  }
}


void midi_stop()
{
  dr_stop_midi();
}


/*
 * Check if need to play new MIDI
 */
void check_midi()
{
  if((dr_midi_pos() < 0 || new_midi == 1) && max_midi > -1) {
    current_midi++;
    if (current_midi > max_midi)
      current_midi = 0;

    midi_play(current_midi);
    DBG_MESSAGE("check_midi()", "Playing MIDI %d", current_midi);
    new_midi = 0;
  }
}


/**
 * shuts down midi playing
 * @author Owen Rudge
 */
void close_midi()
{
  dr_destroy_midi();
}


void midi_next_track()
{
  new_midi = 1;
}


void midi_last_track()
{
  if (current_midi == 0)
    current_midi = max_midi - 1;
  else
    current_midi = current_midi - 2;

  new_midi = 1;
}
