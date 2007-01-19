/*
 * no_midi.c
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * Dummy midi routines - only provide interface, does nothing
 *
 * author: Hj. Malthaner
 * date:   25-May-2002
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
 * @author Hj. Malthaner
 */
void dr_set_midi_volume(int vol)
{
	Mix_VolumeMusic((vol*MIX_MAX_VOLUME)/256);
}


/**
 * Loads a MIDI file
 * @author Hj. Malthaner
 */
int dr_load_midi(const char * filename)
{
	if (midi_number < MAX_MIDI - 1) {
		const int i = midi_number + 1;

		if(i >= 0 && i < MAX_MIDI) {
			music[i] = NULL;
			char filename2[255];
			sprintf(filename2,"./%s",filename);
			printf("Loading %s",filename2);
			music[i] = Mix_LoadMUS(filename2);
    			printf("Mix_LoadMUS(\"music.mp3\"): %s\n", Mix_GetError());
			if(music[i]) {
				printf("Loaded a midi");
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
 * @author Hj. Malthaner
 */
void dr_play_midi(int key)
{
	Mix_PlayMusic(music[key], 1);
	Mix_HookMusicFinished(music_finished);
	finished_current = 0;
}


/**
 * Stops playing MIDI file
 * @author Hj. Malthaner
 */
void dr_stop_midi(void)
{
	Mix_HaltMusic();
}


/**
 * Returns the midi_pos variable
 * @author Hj. Malthaner
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
	if(!SDL_WasInit(SDL_INIT_AUDIO)) {
		if(SDL_InitSubSystem(SDL_INIT_AUDIO) != -1) {
			Mix_OpenAudio(22500, AUDIO_S16SYS, 2, 1024);
		}
	}
}
