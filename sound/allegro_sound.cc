/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <allegro.h>
#include <stdio.h>
#include "sound.h"
#include "../simdebug.h"

// #define DIGMID_SUPPORT   // for Simutrans to work with DIGMID, define this


static bool sound_ok = false;

static int sample_number = 0;
static SAMPLE * sound_samples[1024];

static int voice[4];
static int vc;


/**
 * initializes sound device
 */
bool dr_init_sound()
{
	if(sound_ok) {
		return sound_ok; // already initialized
	}

	const int voices = detect_digi_driver(DIGI_AUTODETECT);

	if(voices > 0) {
		// assume: all ok, override on error
		sound_ok = true;

		dbg->message("dr_init_sound(allegro)", "%d voices available", voices);

#ifndef DIGMID_SUPPORT
		reserve_voices(MIN(voices,4), 0);
#endif

		const int ok = install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL); /***** OCR *****/
		if(ok == -1) {
			dbg->warning("dr_init_sound(allegro)", "Failed to install_sound: %s", allegro_error);
			sound_ok = false;
		}

		voice[0] = -1;
		voice[1] = -1;
		voice[2] = -1;
		voice[3] = -1;

		set_volume(255, 128);
	}
	else {
		dbg->warning("dr_init_sound(allegro)", "No sound available!");
		sound_ok = false;
	}

	return sound_ok;
}



/**
 * loads a sample
 * @return a handle for that sample or -1 on failure
 */
int dr_load_sample(const char *filename)
{
	if(sound_ok) {
		sound_samples[sample_number] = load_sample(filename);
		lock_sample(sound_samples[sample_number]);
	}
	return sample_number++;
}


/**
 * plays a sample
 * @param key the key for the sample to be played
 */
void dr_play_sample(int key, int volume)
{
	if(sound_ok && key < 1024 && sound_samples[key] != NULL) {
		int v = voice[vc & 3];
		if(v == -1) {
			// never used
			voice[vc & 3] = v = allocate_voice(sound_samples[key]);
		}
		else {
			voice_stop( v );
		}

		reallocate_voice(v, sound_samples[key]);
		voice_set_volume(v, volume);
		voice_set_pan(v, 128);
		voice_set_frequency(v, sound_samples[key]->freq);
		voice_set_playmode(v, PLAYMODE_PLAY);
		voice_start(v);

		vc ++;
	}
}
