/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#define UNICODE 1
#include <windows.h>
#include <mmsystem.h>
#include "../tpl/debug_helper.h"

/*
 * Hajo: flag if sound module should be used
 */
static int use_sound = 0;

//#define USE_LOW_LEVEL_AUDIO

/* this list contains all the samples
 */
static void *samples[64];
static int sample_number = 0;

// do not use it: Its a pain to program and has no benefits. Only direct sound would work
// #define USE_LOW_LEVEL_AUDIO
#ifdef USE_LOW_LEVEL_AUDIO
static HWAVEOUT wave_out=NULL;
#endif

/**
 * Sound initialisation routine
 */
void dr_init_sound()
{
#ifdef USE_LOW_LEVEL_AUDIO
	WAVEFORMATEX wf = { WAVE_FORMAT_PCM, 1, 44100, 88200, 2, 16, 0 };
	UINT dev_count=waveOutGetNumDevs();
	UINT i;
	for(  i=0;  i<dev_count;  i++  ) {
		WAVEOUTCAPS wc;
		if(	waveOutGetDevCaps( i, &wc, sizeof(wc) )==MMSYSERR_NOERROR) {
			if((wc.dwSupport & WAVECAPS_PLAYBACKRATE)!=0) {
				use_sound = waveOutOpen( &wave_out, i, &wf, 0, 0, 0 );
				if(use_sound==MMSYSERR_NOERROR) {
					use_sound = 1;
					return;
				}
			}
		}
	}
	if(!use_sound) {
		WARNING( "dr_init_sound()", "waveOutOpen() no matching device found!" );
		return;
	}
#else
	use_sound = 1;
#endif
}



/**
 * loads a single sample
 * @return a handle for that sample or -1 on failure
 * @author Hj. Malthaner
 */
int dr_load_sample(char *filename)
{
	if(use_sound  &&  sample_number>=0  &&  sample_number<64) {

#ifdef USE_LOW_LEVEL_AUDIO
		WAVEFORMATEX FormatDescription;
		HMMIO AudioHandle;
		MMCKINFO ckInfoRIFF;
		MMCKINFO ckInfoChunk;
		DWORD len;
		WAVEHDR *whdr;
		DWORD sps;
		// Datei wird geöffnet
		AudioHandle = mmioOpenA(filename, NULL, MMIO_READ);
		if(AudioHandle==NULL) {
			return -1;
		}
		ckInfoRIFF.fccType = mmioFOURCC('W', 'A', 'V', 'E');
		if(mmioDescend(AudioHandle, &ckInfoRIFF, NULL,MMIO_FINDRIFF)!=MMSYSERR_NOERROR) {
			// no wave!
			mmioClose(AudioHandle, 0);
			WARNING("dr_load_sample()","%i is not a PCM wave file!",filename);
			return -1;
		}
		ckInfoChunk.ckid = mmioFOURCC('f', 'm', 't', ' ');
		if(mmioDescend(AudioHandle, &ckInfoChunk, &ckInfoRIFF,MMIO_FINDCHUNK)!=MMSYSERR_NOERROR) {
			// no format description there
			mmioClose(AudioHandle, 0);
			WARNING("dr_load_sample()","%i is not a PCM wave file!",filename);
			return -1;
		}
		mmioRead(AudioHandle, (char*) &FormatDescription,sizeof(FormatDescription));
		if(FormatDescription.wFormatTag!=WAVE_FORMAT_PCM) {
			// only PCM supported
			mmioClose(AudioHandle, 0);
			WARNING("dr_load_sample()","%i is not a PCM wave file!",filename);
			return -1;
		}
		// Und wieder zurueck...
		if(mmioAscend(AudioHandle, &ckInfoChunk, 0) != MMSYSERR_NOERROR) {
			mmioClose(AudioHandle, 0);
			return -1;
		}
		// and now we may try to read it
		ckInfoChunk.ckid = mmioFOURCC('d', 'a', 't', 'a');
		if(mmioDescend(AudioHandle, &ckInfoChunk, &ckInfoRIFF,MMIO_FINDCHUNK) != MMSYSERR_NOERROR) {
			// no data?!?
			mmioClose(AudioHandle, 0);
			WARNING("dr_load_sample()","%i is damaged!",filename);
			return -1;
		}
		// now we know everything, we can start allocing a structure and read the data
		len = ckInfoChunk.cksize;
		samples[sample_number] = GlobalLock( GlobalAlloc(  GMEM_MOVEABLE, (len+4)&0x7FFFFFFCl ) );
		mmioRead(AudioHandle, samples[sample_number], len);	// assuming success, if we finally go here ...
		mmioClose(AudioHandle, 0);
		// now convert it to a header
		whdr=GlobalLock( GlobalAlloc(  GMEM_MOVEABLE, sizeof(WAVEHDR) ) );
		whdr->lpData = samples[sample_number];
		whdr->dwBufferLength = len;
		sps = ((FormatDescription.nSamplesPerSec/44100l)<<16) + ((65536*(FormatDescription.nSamplesPerSec%44100l))/44100l);
		whdr->dwUser = FormatDescription.nSamplesPerSec;//sps;
MESSAGE( "dr_load_sample()","sample rate %i to with sample rate factor %x",FormatDescription.nSamplesPerSec,sps );
		whdr->dwFlags = 0;
		whdr->dwLoops = 0;
		waveOutPrepareHeader( wave_out, whdr, sizeof(WAVEHDR) );
		samples[sample_number] = whdr;
		return sample_number++;
	}
#else
		FILE *fIn=fopen(filename,"rb");
		if(fIn) {
			long len;
			fseek( fIn, 0, SEEK_END );
			len = ftell( fIn );

			if(len>0) {
				samples[sample_number] = GlobalLock( GlobalAlloc(  GMEM_MOVEABLE, (len+4)&0x7FFFFFFCu ) );

				rewind( fIn );
				fread( samples[sample_number], len, 1, fIn );
				fclose( fIn );

				return sample_number++;
			}
		}
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
	if(use_sound!=0  &&  sample_number>=0  &&  sample_number<64  &&  volume>1) {

#ifdef USE_LOW_LEVEL_AUDIO
   		// prissis short version
		static int oldvol = -1;
		int i;
		volume = (volume<<8)-1;
		if(oldvol!=volume) {
			long vol = (volume<<16)|volume;
			waveOutSetVolume( wave_out, vol );
			oldvol = volume;
		}
		waveOutSetPlaybackRate( wave_out, ((WAVEHDR *)samples[sample_number])->dwUser );
		waveOutWrite( wave_out,  (WAVEHDR *)samples[sample_number], sizeof(WAVEHDR) );
#else
//MESSAGE("dr_play_sample()", "%i sample %i, volume %i ",use_sound,sample_number,volume);
		// prissis short version
		static int oldvol = -1;
		volume = (volume<<8)-1;
		if(oldvol!=volume) {
			long vol = (volume<<16)|volume;
			waveOutSetVolume( 0, vol );
			oldvol = volume;
		}
		// terminate the current sound
		sndPlaySound( NULL, SND_ASYNC );
		// now play
		sndPlaySound( samples[sample_number], SND_MEMORY|SND_ASYNC|SND_NODEFAULT );
#endif
	}
}
