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

#import <string.h>

NSMutableArray *files_WAV;
NSMutableArray *movies_WAV;

void dr_init_sound(void)
{

	printf("\nSound system Initialise");
	printf("\nFilename Database");
	files_WAV = [[NSMutableArray alloc] initWithCapacity: 128];
	printf("\nWave File database\n");
	movies_WAV = [[NSMutableArray alloc] initWithCapacity: 128];
	printf("\nSound system Initalisation complete\n");
}


/**
 * loads a sample
 * @return a handle for that sample or -1 on failure
 * @author Hj. Malthaner
 */
int dr_load_sample(const char *filename)
{
	// return a reference number, which will be used to 'call' this file for later playback
	// we store the filename and preload the file into memory ready to play

	char *realFile = strstr(filename, "//")+1;

	static int cntr = 0;

	printf("\nLoad WAV (%d): %s", cntr, realFile);

	// we need to 'demangle' the filename
	// to do this wee need to find the location of '//' in the string

	// load filename into the array of such things, in case we need it
	NSString *myFile = [[NSString alloc] initWithUTF8String:realFile];

	//[files_WAV addObject: [[NSString alloc] initWithUTF8String:filename]];
	[files_WAV addObject: myFile];

	// preload the file into memory
	// need to validate file is present
	// it appears the filename supplied will be 'mangled', in effect '~' + absolute path, hence the path
	// is malformed with a '//' in the middle, the last character of which is the start of the correct absolute
	// file path.

	if(  [[NSFileManager defaultManager] fileExistsAtPath:myFile]  ) {
		[movies_WAV addObject: [[QTMovie alloc] initWithFile: myFile error:nil] ];
		[[movies_WAV objectAtIndex:cntr] setVolume:1];
		[[movies_WAV objectAtIndex:cntr] play];
	}
	else {
		printf("** Warning, unable to open wav file %s\n", filename);
	}
	cntr++;

	return (cntr-1);	// allow for zero based array
}


/**
 * plays a sample
 * @param key the key for the sample to be played
 * @author Hj. Malthaner
 */
void dr_play_sample(int key, int volume)
{
	// play the file referenced by the supplied key


	// show some basic info
	printf("\nPlay WAV: %d (%s) \nVolume: %d", key, [[files_WAV objectAtIndex:key] cString], volume);

	printf("\n Array holds: %d", [movies_WAV count]);

	// set the volume to whatever the current default is
	[[movies_WAV objectAtIndex:key] setVolume:((float)volume / 255)];

	// start the playback
	[[movies_WAV objectAtIndex:key] play];
}
