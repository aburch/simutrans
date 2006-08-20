/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <allegro.h>
#include <stdio.h>
#include "sound.h"

// #define DIGMID_SUPPORT   // for Simutrans to work with DIGMID, define this


static int sound_ok = 0;

static int sample_number = 0;
static SAMPLE * sound_samples[1024];

static int voice[4];
static int vc;


/**
 * initializes sound device
 * @author Hj. Malthaner
 */
void dr_init_sound()
{
        int voices = detect_digi_driver(DIGI_AUTODETECT);

        if(voices > 0) {
                int ok;

                // assume: all ok, override on error
                sound_ok = 1;

                fprintf(stderr, "Message: %d voices available\n", voices);

#ifndef DIGMID_SUPPORT
                reserve_voices(MIN(voices,4), 0);
#endif

                ok = install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL); /***** OCR *****/

                if(ok == -1) {
                        fprintf(stderr, "Error: %s\n", allegro_error);
                        sound_ok = 0;
                } else {

               	}

               	voice[0] = -1;
               	voice[1] = -1;
               	voice[2] = -1;
               	voice[3] = -1;

                set_volume(255, 128);
        } else {
                fprintf(stderr, "Warning: No sound available!\n");
                sound_ok = 0;
        }
}


/*
 * loads a sample
 * @return a handle for that sample or -1 on failure
 * @author Hj. Malthaner
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
 * @author Hj. Malthaner
 */
void dr_play_sample(int key, int volume)
{
//        fprintf(stderr, "playing sound %d\n", key);



        if(sound_ok && key < 1024 && sound_samples[key] != NULL) {
/*
                play_sample(sound_samples[key],
                            volume,
                            128,
                            1000,
                            0);
*/

		int v = voice[vc & 3];

		if(v == -1) {
			// noch nie benutzt

			voice[vc & 3] = v = allocate_voice(sound_samples[key]);
		} else {
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
