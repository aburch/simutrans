/*
 * simsys_s.c
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifdef __BEOS__
#include "/boot/develop/tools/gnupro/include/SDL/SDL.h"
#elif _MSC_VER
#include <SDL.h>
#else
#include <SDL/SDL.h>
#endif

#ifndef _MSC_VER
#include <unistd.h>
#include <sys/time.h>
#endif

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "simmem.h"
#include "simversion.h"
#include "simsys16.h"

// try to use hardware double buffering ...
// this is equivalent on 16 bpp and much slower on 32 bpp
//#define USE_HW

/*
 * Hajo: flag if sound module should be used
 */
static int use_sound = 0;


/*
 * defines the number of channels avaiable
 */
#define CHANNELS 4


/*
 * this structure contains the data for one sample
 */
typedef struct {
    /* the buffer containing the data for the sample, the format
     * must always be identical to the one of the system output
     * format */
    Uint8 *audio_data;

    Uint32 audio_len;		    /* number of samples in the adiop data */
} sample;


/* this list contains all the samples
 */
static sample samples[32];


/* this structure contains the information about one channel
 */
typedef struct {
    Uint32 sample_pos; /* the current position inside this sample */
    Uint8 sample;		/* which sample is played, 255 for no sample */
    Uint8 volume;		/* the volume this channel should be played */
} channel;


/* this array contains all the information of the currently played samples
 */
static channel channels[CHANNELS];


/* the format of the output audio channel in use
 * all loaded waved need to be converted to this format
 */
static SDL_AudioSpec output_audio_format;


static SDL_Surface *screen;
static int width = 640;
static int height = 480;

static int sync_blit = 0;

struct sys_event sys_event;


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
      if (len + channels[c].sample_pos >= smp->audio_len ) {
	// SDL_MixAudio(stream, smp->audio_data + channels[c].sample_pos, smp->audio_len - channels[c].sample_pos, channels[c].volume);
	channels[c].sample = 255;
      } else {
	SDL_MixAudio(stream, smp->audio_data + channels[c].sample_pos, len, channels[c].volume);
	channels[c].sample_pos += len;
      }
    }
  }
}


/*
 * Hier sind die Basisfunktionen zur Initialisierung der
 * Schnittstelle untergebracht
 * -> init,open,close
 */
int dr_os_init(int n, int *parameter)
{
  // initialize SDL
  int ok = SDL_Init(SDL_INIT_VIDEO|SDL_INIT_NOPARACHUTE);
  if(ok != 0) {
    fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
    return FALSE;
  }

  sync_blit = parameter[1];

  // if SDL gets initialized normally, return zero
  atexit(SDL_Quit); // clean up on exit
  return ok == 0;
}


// open the window
int dr_os_open(int w, int h,int fullscreen)
{
  Uint32 flags = sync_blit ? 0 : SDL_ASYNCBLIT;

#ifdef USE_HW
	{
		SDL_VideoInfo *vi=SDL_GetVideoInfo();
		printf( "hw_available=%i, video_mem=%i, blit_sw=%i\n", vi->hw_available, vi->video_mem, vi->blit_sw );
		printf( "bpp=%i, bytes=%i\n", vi->vfmt->BitsPerPixel, vi->vfmt->BytesPerPixel );
	}
	flags |= SDL_HWSURFACE|SDL_DOUBLEBUF; // bltcopy in graphic memory should be faster ...
#endif

  width = w;
  height = h;

  if(width == 640 ||
     width == 800 ||
     width == 1024 ||
     width == 1280 ||
     fullscreen) {

    flags |= SDL_FULLSCREEN;
  }

  // open the window now
  screen = SDL_SetVideoMode(w, h, 16, flags);
  if (screen == NULL) {
    fprintf(stderr, "Couldn't open the window: %s\n", SDL_GetError());
    return FALSE;
  } else {
    fprintf(stderr, "Screen Flags: requested=%x, actual=%x\n", flags, screen->flags);
  }

  if (screen->pitch != w*2) {
    fprintf(stderr, "Warning: pitch != width\n");
  }

  SDL_EnableUNICODE(TRUE);
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

  // set the caption for the window
  SDL_WM_SetCaption("Simutrans " VERSION_NUMBER,NULL);
  SDL_ShowCursor(0);

  return TRUE;
}


// shut down SDL
int dr_os_close(void)
{
  // Hajo: SDL doc says, screen is free'd by SDL_Quit and should not be
  // free'd by the user
  // SDL_FreeSurface(screen);
  return TRUE;
}



/**
 * Sound initialisation routine
 */
void dr_init_sound()
{
  int sound_ok = 0;

  use_sound = 1;


  // initialize SDL sound subsystem
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) != -1) {

    // open an audio channel
    SDL_AudioSpec desired;

    desired.freq = 22500;
    desired.channels = 1;
    desired.format = AUDIO_S16SYS;
    desired.samples = 1024;
    desired.userdata = NULL;

    desired.callback = sdl_sound_callback;

    if (SDL_OpenAudio(&desired, &output_audio_format) != -1) {

      // check if we got the right audi format
      if (output_audio_format.format == AUDIO_S16SYS) {

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


  use_sound &= sound_ok;
}


unsigned short *dr_textur_init()
{
#ifdef USE_HW
	SDL_LockSurface( screen );
#endif
    return (unsigned short *) (screen->pixels);
}


/**
 * Transform a 24 bit RGB color into the system format.
 * @return converted color value
 * @author Hj. Malthaner
 */
unsigned int get_system_color(unsigned int r, unsigned int g, unsigned int b)
{
  return SDL_MapRGB(screen->format, (Uint8)r, (Uint8)g, (Uint8)b);
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
#ifdef USE_HW
	SDL_UnlockSurface( screen );
	SDL_Flip(screen);
	SDL_LockSurface( screen );
#endif
}


void
dr_textur(int xp, int yp, int w, int h)
{
#ifdef USE_HW
#else
  // make sure the given rectangle is completely on screen
  if ((xp + w) > screen->w) w = screen->w-xp;
  if ((yp + h) > screen->h) h = screen->h-yp;
  SDL_UpdateRect(screen, xp, yp, w, h);
#endif
}


// move cursor to the specified location
void move_pointer(int x, int y)
{
  SDL_WarpMouse((Uint16)x,(Uint16)y);
}


/**
 * Some wrappers can save screenshots.
 * @return 1 on success, 0 if not implemented for a particular wrapper and -1
 *         in case of error.
 * @author Hj. Malthaner
 */
int dr_screenshot(const char *filename)
{
  /*
   * Speichert Screenshot mit SDL-Funktion
   * @author hellmade
   */
  SDL_SaveBMP (SDL_GetVideoSurface(), filename);

  return 1;
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

	// read mod key state from SDL layer
	sys_event.key_mod = SDL_GetModState();

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
        case 4:
            sys_event.code=SIM_MOUSE_WHEELUP;
            break;
        case 5:
            sys_event.code=SIM_MOUSE_WHEELDOWN;
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

    // do low level special stuff here
    switch(event.key.keysym.sym) {

    case SDLK_KP0:
      sys_event.code = '0';
      break;
    case SDLK_KP1:
      sys_event.code = '1';
      break;
    case SDLK_KP2:
    // case SDLK_DOWN:
      sys_event.code = '2';
      break;
    case SDLK_KP3:
      sys_event.code = '3';
      break;
    case SDLK_KP4:
    // case SDLK_LEFT:
      sys_event.code = '4';
      break;
    case SDLK_KP5:
      sys_event.code = '5';
      break;
    case SDLK_KP6:
    // case SDLK_RIGHT:
      sys_event.code = '6';
      break;
    case SDLK_KP7:
      sys_event.code = '7';
      break;
    case SDLK_KP8:
    // case SDLK_UP:
      sys_event.code = '8';
      break;
    case SDLK_KP9:
      sys_event.code = '9';
      break;
    case SDLK_PAGEUP:
      sys_event.code = '>';
      break;
    case SDLK_PAGEDOWN:
      sys_event.code = '<';
      break;

    default:

        // Hajo: process keys to report to application
	if ((event.key.keysym.sym >= 32) && (event.key.keysym.sym <= 255)) {

	    sys_event.code=event.key.keysym.unicode;

	    // Hajo: some mapping of 'special' keys
	    if (event.key.keysym.sym == SDLK_DELETE) {
	      sys_event.code = 127;
	    }

	} else {

	    // Hajo: need to remap some codes
	    switch(event.key.keysym.sym) {
	    case SDLK_F1:
	      sys_event.code = SIM_F1;
	      break;
	    default:
	      sys_event.code = event.key.keysym.sym;
	    }
	}
    }
    printf("Key '%c' (%d) was pressed\n", (int)sys_event.code, (int)sys_event.code);
    break;

    case SDL_MOUSEMOTION:
        sys_event.type=SIM_MOUSE_MOVE;
        sys_event.code= SIM_MOUSE_MOVED;
        sys_event.mx=event.motion.x;
        sys_event.my=event.motion.y;
        break;

    case 1:
    case SDL_KEYUP:
        sys_event.type=SIM_KEYBOARD;
        sys_event.code=0;
        break;

    case SDL_QUIT:
        sys_event.type=SIM_SYSTEM;
        sys_event.code=SIM_SYSTEM_QUIT;
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


void show_pointer(int yesno)
{
  if (yesno) {
    SDL_ShowCursor(1);
  } else {
    SDL_ShowCursor(0);
  }
}


void ex_ord_update_mx_my()
{
  SDL_PumpEvents();
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
  if(use_sound) {

    static int samplenumber = 0;

    if(use_sound) {

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

      wav_cvt.buf = (Uint8 *)guarded_malloc(wav_length*wav_cvt.len_mult);
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
  if(use_sound) {

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
}


#ifdef __linux__

#include "system/no_midi.c"

#else

#ifdef __BEOS__
#include "system/no_midi.c"
#else
#include "system/w32_midi.c"
#endif

#endif



int simu_main(int argc, char **argv);

int main(int argc, char **argv)
{
    return simu_main(argc, argv);
}
