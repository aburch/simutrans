/*
 * sdl_midi.c
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * SDL_Mixer music routine interfaces
 *
 * author: Kieron Green
 * date:   17-Jan-2007
 */

#include "music.h"
#include <SDL.h>
#include <SDL_mixer.h>

static int midi_number = -1;
int finished_current = 0;

char *midi_filenames[MAX_MIDI];
Mix_Music *music[MAX_MIDI];

/**
 * sets midi playback volume
 * @author Kieron Green
 */
void dr_set_midi_volume(int vol)
{
	Mix_VolumeMusic((vol*MIX_MAX_VOLUME)/256);
}


/**
 * Loads a MIDI file
 * @author Kieron Green
 */
int dr_load_midi(const char * filename)
{
	if (midi_number < MAX_MIDI - 1) {
		const int i = midi_number + 1;

		if(i >= 0 && i < MAX_MIDI) {
			music[i] = NULL;

			// just to make sure that we are loading from correct directory...
			char filename2[255];
			sprintf(filename2,"./%s",filename);
			music[i] = Mix_LoadMUS(filename2);
			if(music[i]) {
				midi_number = i;
			}
		}
	}
	return midi_number;
}

/** sets finished flag **/
void music_finished(void) {
	finished_current = 1;
}

/**
 * Plays a MIDI file
 * @author Kieron Green
 */
void dr_play_midi(int key)
{
	Mix_PlayMusic(music[key], 1);

	// when we are finished playing this file set the finished flag
	Mix_HookMusicFinished(music_finished);
	finished_current = 0;
}


/**
 * Stops playing MIDI file
 * @author Kieron Green
 */
void dr_stop_midi(void)
{
	Mix_HaltMusic();
}


/**
 * Returns the midi_pos variable <- doesn't actually do this
 * Simutrans only needs to know whether file has finished (so that it can start the next music)
 * Returns -1 if current music has finished, else 0
 * @author Kieron Green
 */
long dr_midi_pos(void)
{
	if(finished_current == 1) {
		return -1;
	}
	return 0;
}


/**
 * Midi shutdown/cleanup
 * @author Kieron Green
 */
void dr_destroy_midi(void)
{
}


/**
 * Sets midi pos
 * @author Kieron Green
 */
void set_midi_pos(int pos)
{
}

/**
 * MIDI initialisation routines
 * @author Kieron Green
 */
void dr_init_midi(void)
{
	if(!SDL_WasInit(SDL_INIT_AUDIO)) {
		if(SDL_InitSubSystem(SDL_INIT_AUDIO) != -1) {
			Mix_OpenAudio(22500, AUDIO_S16SYS, 2, 1024);
		}
	}
}
