/*
 * sound with SDL_mixer.dll (not chaning the volume of other programs)
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <string.h>
#include "sound.h"
#include "../simmem.h"

/*
 * Hajo: flag if sound module should be used
 */
static int use_sound = 0;

/* this list contains all the samples
 */
static Mix_Chunk *samples[64];

/* all samples are stored chronologically there
 */
static int samplenumber = 0;


/**
 * Sound initialisation routine
 */
void dr_init_sound()
{
	int sound_ok = 0;
	if(use_sound!=0) {
		return;	// avoid init twice
	}
	use_sound = 1;

	// initialize SDL sound subsystem
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) != -1) {

		// open an audio channel

		int freq = 22500;
		int channels = 1;
		unsigned short int format = AUDIO_S16SYS;
		int samples = 1024;

		if (Mix_OpenAudio(freq, format, channels, samples) != -1) {
			Mix_QuerySpec(&freq, &format,  &channels);
			// check if we got the right audi format
			if (format == AUDIO_S16SYS) {
				// finished initializing
				sound_ok = 1;

				// allocate 16 mixing channels
				Mix_AllocateChannels(16);

				// start playing sounds
				Mix_ResumeMusic();

			} else {
				printf("Open audio channel doesn't meet requirements. Muting\n");
				Mix_CloseAudio();
				SDL_QuitSubSystem(SDL_INIT_AUDIO);
			}


		} else {
			printf("Could not open required audio channel. Muting\n");
			SDL_QuitSubSystem(SDL_INIT_AUDIO);
		}
	} else {
		printf("Could not initialize sound system. Muting\n");
	}

	use_sound = sound_ok ? 1: -1;
}


/**
 * loads a sample
 * @return a handle for that sample or -1 on failure
 * @author Hj. Malthaner
 */
int dr_load_sample(const char *filename)
{
	if(use_sound>0  &&  samplenumber<64) {

		Mix_Chunk *smp;

		/* load the sample */
		smp = Mix_LoadWAV(filename);
		if (smp == NULL) {
			printf("could not load wav (%s)\n", SDL_GetError());
			return -1;
		}

		samples[samplenumber] = smp;
		printf("Loaded %s to sample %i.\n",filename,samplenumber);

		return samplenumber++;
	}
	return -1;
}


/**
 * plays a sample
 * @param key the key for the sample to be played
 * @author Hj. Malthaner
 */
void dr_play_sample(int sample_number, int volume)
{
	// sound enabled and a valid sample
	if(use_sound>0 && sample_number != -1) {
		// sdl_mixer finds free channel, we then play at correct volume
		int play_channel = Mix_PlayChannel(-1, samples[sample_number], 0);
		Mix_Volume(play_channel,(volume*MIX_MAX_VOLUME)/256);
	}
}
