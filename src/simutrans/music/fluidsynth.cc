/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */
#include <fluidsynth.h>

#include "../simdebug.h"
#include "../utils/plainstring.h"
#include "../dataobj/environment.h"
#include "music.h"
#ifndef _WIN32
#ifndef __APPLE__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#endif

// fluidsynth music routine interfaces
static int         midi_number = -1;
static plainstring midi_filenames[MAX_MIDI];

fluid_settings_t* settings;
fluid_synth_t* synth;
fluid_audio_driver_t* adriver;
fluid_player_t* player;

// Predefined list of paths to search for soundfonts
static const char * default_sf_paths[] = {
	/* RedHat/Fedora/Arch path */
	"/usr/share/soundfonts/",
	/* Debian/Ubuntu/OpenSUSE path */
	"/usr/share/sounds/sf2/",
	NULL
};

// Soundfonts included on linux distros or bundled with Simutrans
static const char * default_sf_names[] = {
	"default.sf3",
	"default.sf2",
	"freepats-general-midi.sf2",
	"PCLite.sf2",
	"TimGM6mb.sf2",
	"FluidR3_GM.sf2",
	"FluidR3_GS.sf2",
	NULL
};

#ifdef __ANDROID__
/* Fluidsynth on Android is too old and does not export some functions */
extern "C" int fluid_synth_all_notes_off(fluid_synth_t* synth, int chan);
#endif


/**
 * sets midi playback volume
 */
void dr_set_midi_volume(int vol)
{
	/* Allowed range of synth.gain is 0.0 to 10.0 */
	/* fluidsynth's default gain is 0.2, to avoid possible clipping.
	 * Set gain using Simutrans's volume, as a number between 0
	 * and 0.7 to balance with sound effects.
	 */
	if(  fluid_settings_setnum( settings, "synth.gain", 0.7 * vol / 255.0 ) != FLUID_OK  ) {
		dbg->warning("dr_set_midi_volume()", "FluidSynth: Could not set volume.");
	}
}


/**
 * Loads a MIDI file
 */
int dr_load_midi(const char *filename)
{
	if(  midi_number < MAX_MIDI - 1  ) {
		const int i = midi_number + 1;

		if(  i >= 0  &&  i < MAX_MIDI  &&  fluid_is_midifile( filename )  ) {
			midi_number = i;
			midi_filenames[i] = filename;
		}
		else {
			dbg->warning("dr_load_midi()", "FluidSynth: Failed to load MIDI %s.", filename );
		}
	}
	return midi_number;
}


/**
 * Plays a MIDI file
 */
void dr_play_midi(int key)
{
	if(  dr_midi_pos() !=  -1  ) {
		dr_stop_midi();
	}
	if(  !(player = new_fluid_player( synth ))  ) {
		dbg->warning("dr_play_midi()", "FluidSynth: MIDI player setup failed.");
		return;
	}
	if(  fluid_player_add( player, midi_filenames[key] ) != FLUID_OK  ) {
		dbg->warning("dr_play_midi()", "FluidSynth: %s MIDI file load failed.", midi_filenames[key].c_str() );
		return;
	}
	if(  fluid_player_play( player ) != FLUID_OK  ) {
		dbg->warning("dr_play_midi()", "FluidSynth: MIDI player start failed.");
		return;
	}
}


/**
 * Stops playing MIDI file
 */
void dr_stop_midi(void)
{
	if(  !player  ) {
		return;
	}

	fluid_player_stop( player );
	if(  fluid_player_join( player ) != FLUID_OK  ) {
		dbg->warning("dr_stop_midi()", "FluidSynth: Player join failed.");
	}
	fluid_synth_all_notes_off( synth, -1 );
	delete_fluid_player( player );
	player = NULL;
}


/**
 * Returns the midi_pos variable <- doesn't actually do this
 * Simutrans only needs to know whether file has finished (so that it can start the next music)
 * Returns -1 if current music has finished, else 0
 */
sint32 dr_midi_pos(void)
{
	if(  !player  ||  fluid_player_get_status( player ) != FLUID_PLAYER_PLAYING  ) {
		return -1;
	}
	return 0;
}


/**
 * Midi shutdown/cleanup
 */
void dr_destroy_midi(void)
{
	dr_stop_midi();
	delete_fluid_audio_driver(adriver);
	delete_fluid_synth(synth);
	delete_fluid_settings(settings);
	midi_number = -1;
}


/**
 * MIDI initialisation routines
 */
bool dr_load_sf(const char * filename){
	static int previous_id = -1;

	if(  synth  &&  fluid_is_soundfont( filename )  ) {
		int next_id = fluid_synth_sfload( synth, filename, 1 );
		if(  next_id != FLUID_FAILED  ) {
			if(  previous_id != -1  ) {
				fluid_synth_sfunload( synth, previous_id, 1 );
			}
			previous_id = next_id;
			env_t::soundfont_filename = filename;
			return true;
		}
	}
	return false;
}


#if 	FLUIDSYNTH_VERSION_MAJOR >= 2
static void fluid_log(int level, const char* message, void*)
#else
static void fluid_log(int level, char *message, void *)
#endif
{
	switch (level) {
	case FLUID_PANIC: dbg->fatal("FluidSynth", "%s", message);
	case FLUID_ERR:   dbg->error("FluidSynth", "%s", message);   break;
	case FLUID_WARN:  dbg->warning("FluidSynth", "%s", message); break;
	case FLUID_INFO:  dbg->message("FluidSynth", "%s", message); break;
	case FLUID_DBG:   dbg->debug("FluidSynth", "%s", message);   break;
	}
}


bool dr_init_midi()
{
	fluid_set_log_function(FLUID_PANIC, fluid_log, NULL);
	fluid_set_log_function(FLUID_ERR,   fluid_log, NULL);
	fluid_set_log_function(FLUID_WARN,  fluid_log, NULL);
	fluid_set_log_function(FLUID_INFO,  fluid_log, NULL);
	fluid_set_log_function(FLUID_DBG,   fluid_log, NULL);

#ifdef _WIN32
	std::string fluidsynth_driver = "dsound";
#elif defined(__APPLE__) && __APPLE__
	std::string fluidsynth_driver = "coreaudio";
#elif defined(__ANDROID__) && __ANDROID__
	std::string fluidsynth_driver = "oboe";
#else
	std::string fluidsynth_driver = "sdl2";

	if(  !SDL_WasInit(SDL_INIT_AUDIO)  ) {
		if(  SDL_InitSubSystem( SDL_INIT_AUDIO ) != 0  ) {
			dbg->warning("dr_init_midi()", "FluidSynth: SDL_INIT_AUDIO failed.");
			return false;
		}
	}
#endif

	if(  !(settings = new_fluid_settings())  ) {
		dbg->warning("dr_init_midi()", "FluidSynth: MIDI settings failed.");
		return false;
	}

	fluid_settings_setint( settings, "synth.cpu-cores", env_t::num_threads );
	fluid_settings_setstr( settings, "synth.midi-bank-select", "gm" );

	if(  fluid_settings_setstr( settings, "audio.driver", fluidsynth_driver.c_str() ) != FLUID_OK  ) {
		dbg->warning("dr_init_midi()", "FluidSynth: Set MIDI driver %s failed.", fluidsynth_driver.c_str());
		return false;
	}

	if(  !(synth = new_fluid_synth( settings ))  ) {
		dbg->warning("dr_init_midi()", "FluidSynth: Synth setup failed.");
		return false;
	}

	if(  !(adriver = new_fluid_audio_driver( settings, synth ))  ) {
		dbg->warning("dr_init_midi()", "FluidSynth: Audio driver setup failed.");
		return false;
	}

	// User defined font first
	if(  dr_load_sf( env_t::soundfont_filename.c_str() ) || dr_load_sf( ((std::string)env_t::data_dir + "music/" + env_t::soundfont_filename).c_str() )  ) {
		return true;
	}

	// Bundled soundfonts second
	for(  int i = 0;  default_sf_names[i];  i++  ) {
		if(  dr_load_sf( ((std::string)env_t::data_dir + "music/" + (std::string)default_sf_names[i] ).c_str() )  ) {
			return true;
		}
	}

	// System soundfonts at last
	for(  int i = 0;  default_sf_paths[i];  i++  ) {
		for(  int j = 0; default_sf_names[j]; j++  ){
			if(  dr_load_sf(  ((std::string)default_sf_paths[i] + (std::string)default_sf_names[j]).c_str()  )  ) {
				return true;
			}
		}
	}

	env_t::soundfont_filename = "Error";
	dbg->warning("dr_init_midi()", "FluidSynth: No soundfont was found.");
	return true; // MIDI system has been initialed even if no soundfont was loaded. A user can load a soundfont after.
}
