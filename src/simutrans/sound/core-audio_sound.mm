/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * Apple OSX Core Audio MIDI routine added by Leopard
 */
#include "sound.h"

#import <Foundation/NSArray.h>
#import <Foundation/NSString.h>
#import <QTKit/QTMovie.h>
#import <stdio.h>


static NSMutableArray* movies_WAV;


bool dr_init_sound()
{
	printf("Sound system Initialise\n");
	printf("Wave File database\n");
	movies_WAV = [NSMutableArray arrayWithCapacity: 128];
	printf("Sound system Initialisation complete\n");
	return true;
}


int dr_load_sample(char const* const filename)
{
	NSString* const s = [NSString stringWithUTF8String: filename];
	QTMovie*  const m = [QTMovie movieWithFile: s error: nil];
	if (!m) {
		printf("** Warning, unable to open wav file %s\n", filename);
		return -1;
	}

	// Preload the file into memory.
	[m setVolume: 0];
	[m play];

	[movies_WAV addObject: m];

	int const i = [movies_WAV count] - 1;
	printf("Load WAV (%d): %s\n", i, filename);
	return i;
}


void dr_play_sample(int const key, int const volume)
{
	QTMovie* const m = [movies_WAV objectAtIndex: key];
	[m setVolume: volume / 255.f];
	[m play];
}
