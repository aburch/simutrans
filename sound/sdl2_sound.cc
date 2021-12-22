/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/// sdl-sound without SDL_mixer.dll

#ifndef __APPLE__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#include "sound.h"
#include "../simmem.h"
#include "../simdebug.h"

#include <cstdio>
#include <cstring>


/// flag if sound module should be used
/// -1: error during initialization
///  0: not initialized
///  1: successfully initialized
static int use_sound = 0;

/// defines the number of channels available
#define NUM_AUDIO_CHANNELS 4

/// Used to indicate that a channel is empty
#define NO_SAMPLE (0xFF)


/// this structure contains the data for one sample
struct sample
{
	// the buffer containing the data for the sample, the format
	// must always be identical to the one of the system output
	// format
	Uint8 *audio_data;

	Uint32 audio_len; // number of samples in the audio data
};


/// this list contains all the samples
static sample samples[64];

/// all samples are stored chronologically there
static int samplenumber = 0;


/// this structure contains the information about one channel
struct channel
{
	Uint32 sample_pos; ///< the current position inside this sample
	Uint8 sample;      ///< which sample is played, or NO_SAMPLE if there is none
	Uint8 volume;      ///< the volume this channel should be played
};


/// this array contains all the information of the currently played samples
static channel channels[NUM_AUDIO_CHANNELS];


/// the format of the output audio channel in use
/// all loaded waved need to be converted to this format
static SDL_AudioSpec output_audio_format;

static SDL_AudioDeviceID audio_device;


void sdl_sound_callback(void *, Uint8 * stream, int len)
{
	memset(stream, 0, len); // SDL2 requires the output stream to be fully written on every callback.

	// add all the sample that need to be played
	for(  int c = 0;  c < NUM_AUDIO_CHANNELS;  c++  ) {
		if (channels[c].sample == NO_SAMPLE) {
			// only do something if the channel is used
			continue;
		}

		sample *smp = &samples[channels[c].sample];

		// add sample
		if (len + channels[c].sample_pos >= smp->audio_len ) {
			// SDL_MixAudio(stream, smp->audio_data + channels[c].sample_pos, smp->audio_len - channels[c].sample_pos, channels[c].volume);
			channels[c].sample = NO_SAMPLE;
		}
		else {
			SDL_MixAudioFormat(stream, smp->audio_data + channels[c].sample_pos, output_audio_format.format, len, channels[c].volume);
			channels[c].sample_pos += len;
		}
	}
}


bool dr_init_sound()
{
	// avoid init twice
	if (use_sound != 0) {
		return use_sound > 0;
	}

	// initialize SDL sound subsystem
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) == -1) {
		dbg->error("dr_init_sound(SDL2)", "Could not initialize sound system. Muting.");
		use_sound = -1;
		return false;
	}

	// open an audio channel
	SDL_AudioSpec desired;

	desired.freq     = 22050;
	desired.channels = 1;
	desired.format   = AUDIO_S16SYS;
	desired.samples  = 1024;
	desired.userdata = NULL;
	desired.callback = sdl_sound_callback;

	// allow any change as loaded .wav files are converted to output format upon loading
	audio_device = SDL_OpenAudioDevice( NULL, 0, &desired, &output_audio_format, SDL_AUDIO_ALLOW_ANY_CHANGE );

	if(  !audio_device  ) {
		dbg->error("dr_init_sound(SDL2)", "Could not open required audio channel. Muting.");
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		use_sound = -1;
		return false;
	}

	// finished initializing
	for (int i = 0; i < NUM_AUDIO_CHANNELS; i++) {
		channels[i].sample = NO_SAMPLE;
	}

	// start playing sounds
	SDL_PauseAudioDevice(audio_device, 0);

	use_sound = 1;
	return true;
}


int dr_load_sample(const char *filename)
{
	if(use_sound<=0  ||  samplenumber>=64) {
		return -1;
	}

	SDL_AudioSpec wav_spec;
	SDL_AudioCVT  wav_cvt;
	Uint8 *wav_data;
	Uint32 wav_length;
	sample smp;

	// load the sample
	if (SDL_LoadWAV(filename, &wav_spec, &wav_data, &wav_length) == NULL) {
		dbg->warning("dr_load_sample", "Could not load wav: %s", SDL_GetError());
		return -1;
	}

	/* convert the loaded wav to the internally used sound format */
	if (SDL_BuildAudioCVT(&wav_cvt,
			wav_spec.format, wav_spec.channels, wav_spec.freq,
			output_audio_format.format,
			output_audio_format.channels,
			output_audio_format.freq) < 0) {
		dbg->error("dr_load_sample", "Could not create conversion structure");
		SDL_FreeWAV(wav_data);
		return -1;
	}

	wav_cvt.buf = MALLOCN(Uint8, wav_length * wav_cvt.len_mult);
	wav_cvt.len = wav_length;
	memcpy(wav_cvt.buf, wav_data, wav_length);

	SDL_FreeWAV(wav_data);

	if (SDL_ConvertAudio(&wav_cvt) < 0) {
		dbg->error("dr_load_sample", "Could not convert wav to output format");
		return -1;
	}

	// save the data
	smp.audio_data = wav_cvt.buf;
	smp.audio_len = wav_cvt.len_cvt;
	samples[samplenumber] = smp;

	dbg->message("dr_load_sample", "Loaded '%s' to sample %i.", filename, samplenumber);
	return samplenumber++;
}


void dr_play_sample(int sample_number, int volume)
{
	if (use_sound <= 0 || sample_number == -1) {
		return;
	}

	// find an empty channel, and play
	for (int c = 0; c < NUM_AUDIO_CHANNELS; c++) {
		if (channels[c].sample == NO_SAMPLE) {
			channels[c].sample = sample_number;
			channels[c].sample_pos = 0;
			channels[c].volume = volume * SDL_MIX_MAXVOLUME >> 8;
			break;
		}
	}
}
