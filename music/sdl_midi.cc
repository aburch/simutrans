/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * SDL_Mixer music routine interfaces
 *
 * author: Kieron Green
 * date:   17-Jan-2007
 */

#include <SDL.h>
#include <SDL_mixer.h>

#include "../simdebug.h"
#include "../utils/plainstring.h"
#include "music.h"

static int         midi_number = -1;
static plainstring midi_filenames[MAX_MIDI];

//Mix_Music *music[MAX_MIDI];
Mix_Music *music = NULL;

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
			music = Mix_LoadMUS(filename);
			if(music) {
				midi_number = i;
				midi_filenames[i] = filename;
			}
			else {
				dbg->warning( "dr_load_midi()", "Failed to load MIDI %s because %s", filename, Mix_GetError() );
			}
			Mix_FreeMusic(music);
			music = NULL;
		}
	}
	return midi_number;
}

/**
 * Plays a MIDI file
 * @author Kieron Green
 */
void dr_play_midi(int key)
{
	if(dr_midi_pos()!= -1) {
		dr_stop_midi();
	}
	music = Mix_LoadMUS(midi_filenames[key]);
	Mix_PlayMusic(music, 1);
}


/**
 * Stops playing MIDI file
 * @author Kieron Green
 */
void dr_stop_midi(void)
{
	if(music) {
		Mix_HaltMusic();
		Mix_FreeMusic(music);
		music = NULL;
	}
}


/**
 * Returns the midi_pos variable <- doesn't actually do this
 * Simutrans only needs to know whether file has finished (so that it can start the next music)
 * Returns -1 if current music has finished, else 0
 * @author Kieron Green
 */
long dr_midi_pos(void)
{
	if(Mix_PlayingMusic()== 0) {
		return -1;
	}
	else {
		return 0;
	}
}


/**
 * Midi shutdown/cleanup
 * @author Kieron Green
 */
void dr_destroy_midi(void)
{
	dr_stop_midi();
	midi_number = -1;
}


/**
 * MIDI initialisation routines
 * @author Kieron Green
 */
bool dr_init_midi(void)
{
	if(!SDL_WasInit(SDL_INIT_AUDIO)) {				//if audio not init
		if(SDL_InitSubSystem(SDL_INIT_AUDIO) != -1) {		//if audio subsys is ok
			if(Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 1024)==-1) {
				//if OpenAudio returns error, dr_init_midi is false
				return false;
			}
		}
		else {
			//if SDL_InitSubSystem returns error, dr_init_midi is false
			return false;
		}
	}
	//if all is fine, return true
	return true;
}
