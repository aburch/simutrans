/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

/*
 * High-Level soundschnittstelle
 * von Hj. Maltahner, 1998, 2000
 */

#include <stdio.h>
#include <string.h>
#include "macros.h"
#include "music/music.h"
#include "besch/sound_besch.h"
#include "sound/sound.h"
#include "simsound.h"
#include "simsys.h"
#include "simio.h"
#include "simtools.h"
#include "simdebug.h"

#include "dataobj/umgebung.h"
#include "utils/plainstring.h"
#include "utils/simstring.h"


/**
 * max sound index
 * @author hj. Malthaner
 */
static bool        new_midi = false;
static plainstring midi_title[MAX_MIDI];


/**
 * Gesamtlautstärke
 * @author hj. Malthaner
 */

static int max_midi = -1; // number of MIDI files

static int current_midi = -1;  // Hajo: init with error condition,
                               // reset during loading



/**
 * setzt lautstärke für alle effekte
 * @author Hj. Malthaner
 */
void sound_set_global_volume(int volume)
{
	umgebung_t::global_volume = volume;
}


/**
 * ermittelt lautstaärke für alle effekte
 * @author Hj. Malthaner
 */
int sound_get_global_volume()
{
	return umgebung_t::global_volume;
}


void sound_set_mute(bool on)
{
	umgebung_t::mute_sound = on;
}

bool sound_get_mute()
{
	return (  umgebung_t::mute_sound  ||  SFX_CASH == NO_SOUND  );
}


void sound_play(uint16 const idx, uint8 const volume)
{
	if(  idx != (uint16)NO_SOUND  &&  !umgebung_t::mute_sound  ) {
	  dr_play_sample(idx, volume * (umgebung_t::global_volume >> 8));
	}
}




bool sound_get_shuffle_midi()
{
	return umgebung_t::shuffle_midi;
}

void sound_set_shuffle_midi( bool shuffle )
{
	umgebung_t::shuffle_midi = shuffle;
}



/**
 * setzt Lautstärke für MIDI playback
 * @param volume volume in range 0..255
 * @author Hj. Malthaner
 */
void sound_set_midi_volume(int volume)
{
	if(  !umgebung_t::mute_midi  &&  max_midi > -1  ) {
	  dr_set_midi_volume(volume);
	}
	umgebung_t::midi_volume = volume;
}



/**
 * ermittelt Lautstärke für MIDI playback
 * @return volume in range 0..255
 * @author Hj. Malthaner
 */
int sound_get_midi_volume()
{
	return umgebung_t::midi_volume;
}



/**
 * gets midi title
 * @author Hj. Malthaner
 */
const char *sound_get_midi_title(int index)
{
	if (  index >= 0  &&  index <= max_midi  ) {
	  return midi_title[index];
	}
	else {
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
int midi_init(const char *directory)
{
	// read a list of soundfiles
	char full_path[1024];

	sprintf( full_path, "%smusic/music.tab", directory );
	if(  FILE* const file = fopen(full_path, "rb")  ) {
	  while(!feof(file)) {
	    char buf[256];
	    char title[256];
	    size_t len;

	    read_line(buf,   sizeof(buf),   file);
	    read_line(title, sizeof(title), file);
	    if(  !feof(file)  ) {
	      len = strlen(buf);
	      while(  len>0  &&  buf[--len] <= 32  ) {
	        buf[len] = 0;
	      }

	      if(  len > 1  ) {
	        sprintf( full_path, "%s%s", directory, buf );
	        printf("  Reading MIDI file '%s' - %s", full_path, title);
	        max_midi = dr_load_midi(full_path);

	        if(  max_midi >= 0  ) {
	          len = strlen(title);
	          while(  len > 0  &&  title[--len] <= 32  ) {
	            title[len] = 0;
	          }
	          midi_title[max_midi] = title;
	        }
	      }
	    }
	  }

	  fclose(file);
	}
	else {
	  dbg->warning("midi_init()","can't open file '%s' for reading.", full_path);
	}

	if(  max_midi >= 0  ) {
	  current_midi = 0;
	}
	// success?
	return (  max_midi >= 0  );
}


void midi_play(const int no)
{
	if(  no > max_midi  ) {
	  dbg->warning("midi_play()", "MIDI index %d too high (total loaded: %d)", no, max_midi);
	}
	else if(  !midi_get_mute()  ) {
	  dr_play_midi(no);
	}
}


void midi_stop()
{
	if(  !midi_get_mute()  ) {
	  dr_stop_midi();
	}
}



void midi_set_mute(bool on)
{
	on |= (  max_midi == -1  );
	if(  on  ) {
	  if(  !umgebung_t::mute_midi  ) {
	    dr_stop_midi();
	  }
	  umgebung_t::mute_midi = true;
	}
	else {
	  if(  umgebung_t::mute_midi  ) {
	    umgebung_t::mute_midi = false;
	    midi_play(current_midi);
	  }
	  dr_set_midi_volume(umgebung_t::midi_volume);
	}
}



bool midi_get_mute()
{
	return  (  umgebung_t::mute_midi  ||  max_midi == -1  );
}



/*
 * Check if need to play new MIDI
 * Max Kielland:
 * Made it possible to get next song
 * even if we are muted.
 */
void check_midi()
{
	// Check for next sound
	if(  dr_midi_pos() < 0  ||  new_midi == true  ) {
	  if(  umgebung_t::shuffle_midi  &&  max_midi > 1  ) {

	    // shuffle songs (must not use simrand()!)
	    int new_song = sim_async_rand(max_midi);

	    if(  new_song >= current_midi  ) {
	      new_song ++;
	    }
	    current_midi = new_song;
	  }
	  else {
	    current_midi++;
	    if(  current_midi > max_midi  ) {
	      current_midi = 0;
	    }
	  }

	  // Are we in playing mode?
	  if(  false == midi_get_mute()  ) {
	    midi_play(current_midi);
	    DBG_MESSAGE("check_midi()", "Playing MIDI %d", current_midi);
	  }
	}

	new_midi = false;
}


/**
 * shuts down midi playing
 * @author Owen Rudge
 */
void close_midi()
{
	if(  max_midi > -1  ) {
	  dr_destroy_midi();
	}
}


void midi_next_track()
{
	new_midi = true;
}


void midi_last_track()
{
	if (  current_midi == 0  ) {
	  current_midi = max_midi - 1;
	}
	else {
	  current_midi = current_midi - 2;
	}
	new_midi = true;
}
