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
#import <AVFoundation/AVFoundation.h>
#import <stdio.h>


static NSMutableArray* players_WAV;


bool dr_init_sound()
{
	printf("Sound system Initialise\n");
	printf("Wave File database\n");
	players_WAV = [NSMutableArray arrayWithCapacity: 128];
	printf("Sound system Initialisation complete\n");
	return true;
}


int dr_load_sample(char const* const filename)
{
	NSURL* const url = [NSURL fileURLWithPath: [NSString stringWithUTF8String: filename]];
	AVAudioPlayer* const player = [[AVAudioPlayer alloc] initWithContentsOfURL: url error: nil];
	if (!player) {
		printf("** Warning, unable to open wav file %s\n", filename);
		return -1;
	}

	// Preload the file into memory.
	[player setVolume: 0];
	[player play];

	[players_WAV addObject: player];

	int const i = [players_WAV count] - 1;
	printf("Load WAV (%d): %s\n", i, filename);
	return i;
}


void dr_play_sample(int const key, int const volume)
{
	AVAudioPlayer* const m = [players_WAV objectAtIndex: key];
	[m setVolume: volume / 255.f];
	[m play];
}
