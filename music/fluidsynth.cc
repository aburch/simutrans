/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */
#include <fluidsynth.h>
#include <ctype.h>
#include <string.h>

#include "../simdebug.h"
#include "../utils/plainstring.h"
#include "../dataobj/environment.h"
#include "music.h"
#ifndef _WIN32
	#include <SDL.h>
	#ifdef HAS_SDL_MIXER
		#include <SDL_mixer.h>
	#endif
#endif

// fluidsynth music routine interfaces
static int         midi_number = -1;
static plainstring midi_filenames[MAX_MIDI];
static bool        is_midi_track[MAX_MIDI];  // true if MIDI, false if WAV

#if !defined(_WIN32) && defined(HAS_SDL_MIXER)
static Mix_Music  *bgm_music         = NULL;
static bool        mixer_initialized = false;
#endif


static void get_ext_lower(const char *filename, char *ext, int maxlen)
{
	const char *p = strrchr(filename, '.');
	ext[0] = 0;
	if(  !p  ) { return; }
	p++;  // skip the '.' so ext contains e.g. "mid", not ".mid"
	int i;
	for(  i = 0;  i < maxlen - 1  &&  p[i];  i++  ) {
		ext[i] = (char)tolower((unsigned char)p[i]);
	}
	ext[i] = 0;
}

static bool is_midi_ext(const char *filename)
{
	char ext[8];
	get_ext_lower(filename, ext, sizeof(ext));
	if(  ext[0] == 0  ) {
		dbg->warning("is_midi_ext()", "BGM file has no extension, assuming MIDI: %s", filename);
		return true;
	}
	return strcmp(ext, "mid") == 0  ||  strcmp(ext, "midi") == 0;
}

static bool is_wav_ext(const char *filename)
{
	char ext[8];
	get_ext_lower(filename, ext, sizeof(ext));
	return strcmp(ext, "wav") == 0;
}

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
#if !defined(_WIN32) && defined(HAS_SDL_MIXER)
	if(  mixer_initialized  ) {
		Mix_VolumeMusic( (vol * MIX_MAX_VOLUME) / 256 );
	}
#endif
}


/**
 * Loads a MIDI or WAV file
 */
int dr_load_midi(const char *filename)
{
	if(  midi_number < MAX_MIDI - 1  ) {
		const int i = midi_number + 1;

		if(  i >= 0  &&  i < MAX_MIDI  ) {
			const bool is_midi = is_midi_ext( filename );
			const bool is_wav  = !is_midi && is_wav_ext( filename );
			if(  !is_midi  &&  !is_wav  ) {
				dbg->warning("dr_load_midi()", "Unsupported BGM format (only MIDI and WAV are supported): %s.", filename);
				return midi_number;
			}
			if(  is_midi  &&  !fluid_is_midifile( filename )  ) {
				dbg->warning("dr_load_midi()", "FluidSynth: Not a valid MIDI file: %s.", filename );
				return midi_number;
			}
			midi_number      = i;
			midi_filenames[i] = filename;
			is_midi_track[i]  = is_midi;
		}
	}
	return midi_number;
}


/**
 * Plays a MIDI or WAV file
 */
void dr_play_midi(int key)
{
	if(  dr_midi_pos() != -1  ) {
		dr_stop_midi();
	}
	if(  !is_midi_track[key]  ) {
		// non-MIDI: play via SDL_mixer
#if !defined(_WIN32) && defined(HAS_SDL_MIXER)
		if(  mixer_initialized  ) {
			bgm_music = Mix_LoadMUS( midi_filenames[key] );
			if(  bgm_music  ) {
				Mix_PlayMusic( bgm_music, 1 );
			}
			else {
				dbg->warning("dr_play_midi()", "SDL_mixer: Failed to load %s: %s", midi_filenames[key].c_str(), Mix_GetError());
			}
		}
		else {
			dbg->warning("dr_play_midi()", "SDL_mixer not initialized; cannot play WAV BGM: %s", midi_filenames[key].c_str());
		}
#else
		dbg->warning("dr_play_midi()", "WAV BGM not supported in this build: %s", midi_filenames[key].c_str());
#endif
		return;
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
 * Stops playing MIDI or WAV file
 */
void dr_stop_midi(void)
{
#if !defined(_WIN32) && defined(HAS_SDL_MIXER)
	if(  bgm_music  ) {
		Mix_HaltMusic();
		Mix_FreeMusic( bgm_music );
		bgm_music = NULL;
	}
#endif
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
#if !defined(_WIN32) && defined(HAS_SDL_MIXER)
	if(  bgm_music  ) {
		return Mix_PlayingMusic() ? 0 : -1;
	}
#endif
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
#if !defined(_WIN32) && defined(HAS_SDL_MIXER)
	if(  mixer_initialized  ) {
		Mix_CloseAudio();
		mixer_initialized = false;
	}
#endif
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


static void fluid_log(int level, const char *message, void *)
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

	if(  !(settings = new_fluid_settings())  ) {
		dbg->warning("dr_init_midi()", "FluidSynth: MIDI settings failed.");
		return false;
	}

	fluid_settings_setint( settings, "synth.cpu-cores", env_t::num_threads );
	fluid_settings_setstr( settings, "synth.midi-bank-select", "gm" );

	if(  !(synth = new_fluid_synth( settings ))  ) {
		dbg->warning("dr_init_midi()", "FluidSynth: Synth setup failed.");
		delete_fluid_settings(settings);
		return false;
	}

#ifdef _WIN32
	fluid_settings_setstr( settings, "audio.driver", "dsound" );
	adriver = new_fluid_audio_driver( settings, synth );
#elif defined(__APPLE__) && __APPLE__
	fluid_settings_setstr( settings, "audio.driver", "coreaudio" );
	adriver = new_fluid_audio_driver( settings, synth );
#elif __ANDROID__
	fluid_settings_setstr( settings, "audio.driver", "oboe" );
	adriver = new_fluid_audio_driver( settings, synth );
#else
	// On Linux, prefer non-SDL2 drivers so SDL_mixer can use SDL audio independently.
	// Try pulseaudio/pipewire/alsa/oss first; fall back to sdl2 if needed.
	{
		static const char *linux_drivers[] = { "pulseaudio", "pipewire", "alsa", "oss", "sdl2", NULL };
		for(  int di = 0;  linux_drivers[di]  &&  !adriver;  di++  ) {
			if(  fluid_settings_setstr( settings, "audio.driver", linux_drivers[di] ) == FLUID_OK  ) {
				if(  strcmp( linux_drivers[di], "sdl2" ) == 0  ) {
					// sdl2 driver requires SDL audio to be initialized
					if(  !SDL_WasInit(SDL_INIT_AUDIO)  ) {
						SDL_InitSubSystem( SDL_INIT_AUDIO );
					}
				}
				adriver = new_fluid_audio_driver( settings, synth );
				if(  adriver  ) {
					dbg->message("dr_init_midi()", "FluidSynth: Using audio driver: %s", linux_drivers[di]);
				}
			}
		}
	}
#endif

	if(  !adriver  ) {
		dbg->warning("dr_init_midi()", "FluidSynth: Audio driver setup failed.");
		delete_fluid_synth(synth);
		delete_fluid_settings(settings);
		return false;
	}

#if !defined(_WIN32) && defined(HAS_SDL_MIXER)
	// Initialize SDL_mixer for WAV BGM support.
	// This requires SDL audio to be free (i.e., FluidSynth not using sdl2 driver).
	if(  !SDL_WasInit(SDL_INIT_AUDIO)  ) {
		SDL_InitSubSystem( SDL_INIT_AUDIO );
	}
	if(  Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 4096 ) == 0  ) {
		mixer_initialized = true;
		dbg->message("dr_init_midi()", "SDL_mixer initialized for WAV BGM support.");
	}
	else {
		dbg->warning("dr_init_midi()", "SDL_mixer: Could not open audio (WAV BGM disabled): %s. "
			"This typically happens when FluidSynth is using the sdl2 audio driver and already owns the SDL audio device.", Mix_GetError());
	}
#endif

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
