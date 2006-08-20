/*
 * simsys_s.c
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#define USE_SOUND
// #define USE_MIDI


#ifdef _MSC_VER
#include <SDL.h>
#else
#include <SDL/SDL.h>
#include <unistd.h>
#include <sys/time.h>
#endif
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "simsys.h"
#include "simmem.h"

#ifdef USE_SOUND

/*
 * defines the number of channels avaiable
 */
#define CHANNELS 4

/*
 * this structure contains the data for one sample
 */
typedef struct {
    /*
     * the buffer containing the data for the sample, the format
     * must always be identical to the one of the system output
     * format
     */
    Uint8 *audio_data;

    /*
     * number of samples in the adiop data
     */
    Uint32 audio_len;
} sample;


/*
 * this list contains all the samples
 */
static sample samples[10];


/*
 * this structure contains the information about one channel
 */
typedef struct {

    /*
     * which sample is played, 255 for no sample
     */
    Uint8 sample;

    /*
     * the current position inside this sample
     */
    Uint32 sample_pos;

    /*
     * the volume this channel should be played
     */
    Uint8 volume;
} channel;


/*
 * this array contains all the information of the currently played samples
 */
static channel channels[CHANNELS];


/*
 * the format of the output audio channel in use
 * all loaded waved need to be converted to this format
 */
static SDL_AudioSpec output_audio_format;


/*
 * identifies the state of the sound module
 */
static Uint8 sound_ok = 0;


#endif

static SDL_Surface *screen;
static int width = 640;
static int height = 480;

struct sys_event sys_event;


#ifdef USE_SOUND

void sdl_sound_callback(void *ptr, Uint8 * stream, int len)
{
    int c;

    /*
     * add all the sample that need to be played
     */
    for (c = 0; c < CHANNELS; c++) {

        /*
         * only do something, if the channel is used
         */
        if (channels[c].sample != 255) {

            sample * smp = &samples[channels[c].sample];

            /*
             * add sample
             */
            if (len > smp->audio_len - channels[c].sample_pos) {
                SDL_MixAudio(stream, smp->audio_data + channels[c].sample_pos, smp->audio_len - channels[c].sample_pos, channels[c].volume);
                channels[c].sample = 255;
            } else {
                SDL_MixAudio(stream, smp->audio_data + channels[c].sample_pos, len, channels[c].volume);
                channels[c].sample_pos += len;
            }
        }
    }
}

#endif


/*
 * Hier sind die Basisfunktionen zur Initialisierung der Schnittstelle untergebracht
 * -> init,open,close
 */

// open libraries, allocate memory...
int dr_os_init(int n, int *parameter)
{
        // initialize SDL
        int ok = SDL_Init(SDL_INIT_VIDEO||);
        if(ok != 0) {
                printf("Couldn't initialize SDL: %s\n", SDL_GetError());
                return FALSE;
        }

#ifdef USE_SOUND

    sound_ok = 0;

    // initialize SDL sound subsystem
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != -1) {

        // open an audio channel
        SDL_AudioSpec desired;

        desired.freq = 22500;
        desired.channels = 1;
        // desired.format = AUDIO_S16SYS;
        desired.format = AUDIO_U8;
        desired.samples = 1024;
        desired.userdata = NULL;

        desired.callback = sdl_sound_callback;

        if (SDL_OpenAudio(&desired, &output_audio_format) != -1) {

            // check if we got the right audi format
            if (output_audio_format.format == AUDIO_U8) {

                int i;

                // finished initializing
                sound_ok = 1;

                for (i = 0; i < CHANNELS; i++)
                    channels[i].sample = 255;

                // start playing sounds
                SDL_PauseAudio(0);

            } else {
                printf("Open audio channel doesn't meet requirements. Muting\n");
                SDL_CloseAudio();
                SDL_QuitSubSystem(SDL_INIT_AUDIO);
            }


        } else {
            printf("Could not open required audio channel. Muting\n");
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
        }
    } else {
        printf("Could not initialize sound system. Muting\n");
    }

#endif

        // if SDL gets initialized normally, return zero
        atexit(SDL_Quit); // clean up on exit
        return ok == 0;
}

// open the window
int dr_os_open(int w, int h)
{

        width = w;
        height = h;

        // open the window now
        screen = SDL_SetVideoMode(w, h, 8, SDL_HWPALETTE);
        if (screen == NULL) {
                printf("Couldn't open the window: %s\n", SDL_GetError());
                return FALSE;
        }

        if (screen->pitch != w) {
            printf("!!!!!!!!!!!!!!!!!warnign, pitch != width\n");
        }

	SDL_EnableUNICODE(TRUE);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

        // set the caption for the window
        SDL_WM_SetCaption("Simutrans",NULL);
        SDL_ShowCursor(0);

        return TRUE;
}

// shut down SDL
int dr_os_close(void)
{
        SDL_FreeSurface(screen);
        return TRUE;
}


unsigned char *dr_textur_init()
{
        return (unsigned char *) (screen->pixels);
}


/**
 * Does this system wrapper need software cursor?
 * @return true if a software cursor is needed
 * @author Hj. Malthaner
 */
int dr_use_softpointer()
{
    return 0;
}


void dr_flush()
{
//    SDL_UpdateRect(screen, 0, 0, width, height);
}


void
dr_textur(int xp, int yp, int w, int h)
{
    // make sure the given rectangle is completely on screen
    if ((xp + w) > screen->w) w = screen->w-xp;
    if ((yp + h) > screen->h) h = screen->h-yp;
    SDL_UpdateRect(screen, xp, yp, w, h);
}


void dr_setRGB8(int n, int r, int g, int b)
{
        SDL_Color rgb;
        rgb.r = r;
        rgb.g = g;
        rgb.b = b;

        SDL_SetColors(screen, &rgb, n, 1);
}


void dr_setRGB8multi(int first, int count, unsigned char * data)
{
#ifdef _MSC_VER
    SDL_Color *rgb = guarded_malloc(sizeof(SDL_Color) * count);
#else
    SDL_Color rgb[count];
#endif
    int n;

    for(n=0; n<count; n++) {
	rgb[n].r = *data++;
	rgb[n].g = *data++;
	rgb[n].b = *data++;
    }

    SDL_SetColors(screen, rgb, first, count);
#ifdef _MSC_VER
    guarded_free(rgb);
#endif
}


/*
 * move cursor to the specified location
 */
void move_pointer(int x, int y)
{
        SDL_WarpMouse(x,y);
}


/*
 * Hier sind die Funktionen zur Messageverarbeitung
 */
static void internal_GetEvents(int wait)
{
    SDL_Event event;
    event.type = 1;

    if(wait) {
        int n = 0;

        do {
            SDL_WaitEvent(&event);
            n = SDL_PollEvent(NULL);
        }  while (n != 0 && event.type==SDL_MOUSEMOTION);

    } else {
        int n = 0;
        int got_one = FALSE;

        do {
            n = SDL_PollEvent(&event);

	    if(n != 0) {
		got_one = TRUE;

	    	if(event.type == SDL_MOUSEMOTION) {
        		sys_event.type=SIM_MOUSE_MOVE;
        		sys_event.code= SIM_MOUSE_MOVED;
        		sys_event.mx=event.motion.x;
        		sys_event.my=event.motion.y;
	    	}
	    }

	} while (n != 0 && event.type==SDL_MOUSEMOTION);

        if(!got_one) {
            return;
        }
    }


    switch(event.type) {
    case SDL_MOUSEBUTTONDOWN:     /* originally ButtonPress */
        sys_event.type=SIM_MOUSE_BUTTONS;
        sys_event.mx=event.button.x;
        sys_event.my=event.button.y;
        switch(event.button.button) {
        case 1:
            sys_event.code=SIM_MOUSE_LEFTBUTTON;
            break;
        case 2:
            sys_event.code=SIM_MOUSE_MIDBUTTON;
            break;
        case 3:
            sys_event.code=SIM_MOUSE_RIGHTBUTTON;
            break;
        }
        break;
   case SDL_MOUSEBUTTONUP:     /* originally ButtonRelease */
        sys_event.type=SIM_MOUSE_BUTTONS;
        sys_event.mx=event.button.x;
        sys_event.my=event.button.y;
        switch(event.button.button) {
        case 1:
            sys_event.code=SIM_MOUSE_LEFTUP;
            break;
        case 2:
            sys_event.code=SIM_MOUSE_MIDUP;
            break;
        case 3:
            sys_event.code=SIM_MOUSE_RIGHTUP;
            break;
        }
        break;
    case SDL_KEYDOWN:   /* originally KeyPress */
        sys_event.type=SIM_KEYBOARD;

	if(event.key.keysym.sym >= 32) {
	    sys_event.code=event.key.keysym.unicode;
	} else {
	    sys_event.code=event.key.keysym.sym;
	}

        printf("Key '%c' (%d) was pressed\n", sys_event.code, sys_event.code);

        break;
    case SDL_MOUSEMOTION:    /* originally MotionNotify */
        sys_event.type=SIM_MOUSE_MOVE;
        sys_event.code= SIM_MOUSE_MOVED;
        sys_event.mx=event.motion.x;
        sys_event.my=event.motion.y;
        break;
    case 15:
    case 14:
    case SDL_KEYUP: /* originally KeyRelease */
        sys_event.type=SIM_IGNORE_EVENT;
        sys_event.code=0;
        break;
    default:
        printf("Unbekanntes Ereignis # %d!\n",event.type);
        sys_event.type=SIM_IGNORE_EVENT;
        sys_event.code=0;
    }
}

void GetEvents()
{
    internal_GetEvents(TRUE);
}

void GetEventsNoWait()
{
    sys_event.type = SIM_NOEVENT;
    sys_event.code = 0;

    internal_GetEvents(FALSE);
}

// I added this because it's needed by simgraph.c
void show_pointer(int yesno)
{
    if (yesno)
	SDL_ShowCursor(1);
    else
	SDL_ShowCursor(0);
}


void ex_ord_update_mx_my()
{
        // this procedure is left empty for the SDL version
}


unsigned long dr_time(void)
{
  return SDL_GetTicks();
}


void dr_sleep(unsigned long usec)
{
    if(usec >= 1024) {
	// schlaeft meist etwas zu kurz,
        // usec/1024 statt usec/1000
        SDL_Delay(usec >> 10);
    }
}


/**
 * loads a sample
 * @return a handle for that sample or -1 on failure
 * @author Hj. Malthaner
 */
int dr_load_sample(const char *filename)
{

#ifdef USE_SOUND

    static int samplenumber = 0;

    if (sound_ok) {

        SDL_AudioSpec wav_spec;
        SDL_AudioCVT  wav_cvt;
        Uint8 *wav_data;
        Uint32 wav_length;
        sample smp;

        /* load the sample */
        if (SDL_LoadWAV(filename, &wav_spec, &wav_data, &wav_length) == NULL) {
            printf("could not load wav (%s)\n", SDL_GetError());
            return -1;
        }

        /* convert the loaded wav to the internally used sound format */
        if (SDL_BuildAudioCVT(&wav_cvt,
                              wav_spec.format, wav_spec.channels, wav_spec.freq,
                              output_audio_format.format,
                              output_audio_format.channels,
                              output_audio_format.freq) < 0) {
            printf("could not create conversion structure\n");
            SDL_FreeWAV(wav_data);
            return -1;
        }

        wav_cvt.buf = (Uint8 *)malloc(wav_length*wav_cvt.len_mult);
        wav_cvt.len = wav_length;
        memcpy(wav_cvt.buf, wav_data, wav_length);

        SDL_FreeWAV(wav_data);

        if (SDL_ConvertAudio(&wav_cvt) < 0) {
            printf("could not convert wav to output format\n");
            return -1;
        }

        /* save the data */
        smp.audio_data = wav_cvt.buf;
        smp.audio_len = wav_cvt.len_cvt;
        samples[samplenumber] = smp;

        return samplenumber++;
    }

#endif

    return -1;

}

/**
 * plays a sample
 * @param key the key for the sample to be played
 * @author Hj. Malthaner
 */
void dr_play_sample(int sample_number, int volume)
{
#ifdef USE_SOUND

    if (sound_ok) {

        int c;

        if (sample_number == -1) {
            return;
        }

        /* find an empty channel, and play */
        for (c = 0; c < CHANNELS; c++) {
            if (channels[c].sample == 255) {
                channels[c].sample = sample_number;
                channels[c].sample_pos = 0;
                channels[c].volume = volume * SDL_MIX_MAXVOLUME >> 8;
                break;
            }
        }
    }

#endif
}

/**
 * sets midi playback volume
 * @author Hj. Malthaner
 */
void dr_set_midi_volume(int vol)
{
#ifdef USE_MIDI
    set_volume(-1, vol);
#endif
}


/**
 * Loads a MIDI file
 * @author Owen Rudge, changes by Hj. Malthaner
 */

int dr_load_midi(const char *filename)
{
#ifdef USE_MIDI
    if(sound_ok && midi_number < MAX_MIDI-1)
    {
	const int i = midi_number + 1;

	if(i >= 0 && i < MAX_MIDI) {

	    midi_samples[i] = load_midi(filename);

	    if(midi_samples[i]) {
		midi_number = i;
	    }
	}
    }

    return midi_number;
#else
    return -1;
#endif
}



/**
 * Plays a MIDI file
 * Key: The index of the MIDI file to be played
 * By Owen Rudge
 */

void dr_play_midi(int key)
{
#ifdef USE_MIDI
   if (midi_number > 0)
   {
      if(sound_ok && key >= 0 && key <= midi_number && midi_samples[key] != NULL)
         play_midi(midi_samples[key], FALSE); // loop
      else
	printf("\nMessage: MIDI: Unable to play MIDI %d\n", key);
   }
#endif
}



/**
 * Stops playing MIDI file
 * By Owen Rudge
 */
void dr_stop_midi()
{
#ifdef USE_MIDI
   stop_midi();
#endif
}

/**
 * Returns the midi_pos variable
 * By Owen Rudge
 */
long dr_midi_pos()
{
#ifdef USE_MIDI
    if(sound_ok) {
	return (midi_pos);
    } else {
	return 0;
    }
#endif
}

/**
 * Midi shutdown/cleanup
 * By Owen Rudge
 */
void dr_destroy_midi()
{
#ifdef USE_MIDI
   int i;

   for (i = 0; i <= midi_number; i++) {
      if (midi_samples[i] != NULL) {
	destroy_midi(midi_samples[i]);
      }
   }
   midi_number = -1;
#endif
}


void set_midi_pos(int pos)
{
#ifdef USE_MIDI
   midi_pos = pos;
#endif
}



int simu_main(int argc, char **argv);

int main(int argc, char **argv)
{
    return simu_main(argc, argv);
}
