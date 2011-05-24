/*
   Apple OSX Core Audio MIDI routine added by Leopard
   Modified from no_midi.cc
	 Written as objective-c
   Date: 2008-07-27
*
 * This file is part of the Simutrans project under the artistic licence.
 *
 */


#include "sound.h"

#import <stdio.h>
#import <QTKit/QTMovie.h>
#import <QTKit/QTKit.h>
#import <Cocoa/Cocoa.h>


NSMutableArray *movies_WAV;

bool dr_init_sound(void)
{

	printf("Sound system Initialise\n");
	printf("Wave File database\n");
	movies_WAV = [[NSMutableArray alloc] initWithCapacity: 128];
	printf("Sound system Initalisation complete\n");
	return true;
}


/**
 * loads a sample
 * @return a handle for that sample or -1 on failure
 * @author Hj. Malthaner
 */
int dr_load_sample(const char *filename)
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


/**
 * plays a sample
 * @param key the key for the sample to be played
 * @author Hj. Malthaner
 */
void dr_play_sample(int key, int volume)
{
	// play the file referenced by the supplied key

	// set the volume to whatever the current default is
	[[movies_WAV objectAtIndex:key] setVolume:((float)volume / 255)];

	// start the playback
	[[movies_WAV objectAtIndex:key] play];
}
