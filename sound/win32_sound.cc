/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include <windows.h>
#include <mmsystem.h>

#include "sound.h"
#include "../sys/simsys.h"


/*
 * flag if sound module should be used
 * with Win32 the number+1 of the device used
 */
static int use_sound = 0;

/* this list contains all the samples
 */
static void *samples[64];
static int sample_number = 0;


/**
 * Sound initialisation routine
 */
bool dr_init_sound()
{
	use_sound = 1;
	return true;
}



/**
 * loads a single sample
 * @return a handle for that sample or -1 on failure
 */
int dr_load_sample(char const* filename)
{
	if(use_sound  &&  sample_number>=0  &&  sample_number<64) {
		if (FILE* const fIn = dr_fopen(filename, "rb")) {
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
	return -1;
}



/**
 * plays a sample
 * @param key the key for the sample to be played
 */
void dr_play_sample(int sample_number, int volume)
{
	if(use_sound!=0  &&  sample_number>=0  &&  sample_number<64  &&  volume>1) {
		static int oldvol = -1;
		volume = (volume<<8)-1;
		if(oldvol!=volume) {
			DWORD vol = (volume<<16)|volume;
			waveOutSetVolume( 0, vol );
			oldvol = volume;
		}

		UINT flags = SND_MEMORY | SND_ASYNC | SND_NODEFAULT;
		// Terminate the current sound, if not already the requested one.
		static int last_sample_nr = -1;
		if(  last_sample_nr == sample_number  ) {
			flags |= SND_NOSTOP;
		}
		last_sample_nr = sample_number;
		sndPlaySound(static_cast<TCHAR const*>(samples[sample_number]), flags);
	}
}
