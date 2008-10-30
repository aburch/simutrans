/*
   Apple OSX Core Audio MIDI routine added by Leopard
   Modified from no_midi.cc
	 Written as objective-c
   Date: 2008-07-27
 *
 * This file is part of the Simutrans project under the artistic licence.
 *
 */


#import "music.h"


#import <stdio.h>
#import <QTKit/QTMovie.h>
#import <QTKit/QTKit.h>
#import <Cocoa/Cocoa.h>


float defaultVolume = 0.5;	// a nice default volume
int nowPlaying = -1;				// the number of the track currently being played

NSMutableArray *files;
NSMutableArray *movies;

/**
 * sets midi playback volume
 * @author Hj. Malthaner
 */
void dr_set_midi_volume(int vol)
{
	// we are given an integer from 0 - 255, we need a float between 0 and 1
	defaultVolume =((float)vol / 255);
	if(  nowPlaying != -1  ) {
		[[movies objectAtIndex:nowPlaying] setVolume:defaultVolume];
	}
}


/**
 * Loads a MIDI file
 * @author Hj. Malthaner
 */
int dr_load_midi(const char * filename)
{
	// return a reference number, which will be used to 'call' this file for later playback
	// we store the filename and preload the file into memory ready to play

	static int cntr = 0;

	// load filename into the array of such things, in case we need it
	[files addObject: [[NSString alloc] initWithUTF8String:filename]];

	// preload the file into memory
	[movies addObject: [[QTMovie alloc] initWithFile: [[NSString alloc] initWithUTF8String:filename] error:nil] ];

	cntr++;

	printf("Load MIDI (%d): %s", cntr-1, filename);
	return (cntr-1);	// allow for zero based array
}


/**
 * Plays a MIDI file
 * @author Hj. Malthaner
 */
void dr_play_midi(int key)
{
	// play the file referenced by the supplied key

	// the following would show some basic info
	//printf("\nPlay Midi: %d (%s) \nVolume: %f", key, [[files objectAtIndex:key] cString], defaultVolume);
	//printf("\n Array holds: %d", [movies count]);

	// set the volume to whatever the current default is
	[[movies objectAtIndex:key] setVolume:defaultVolume];

	// start the playback
	[[movies objectAtIndex:key] play];
	nowPlaying = key;
}


/**
 * Stops playing MIDI file
 * @author Hj. Malthaner
 */
void dr_stop_midi(void)
{
	// stop whatever is playing
	// we assume the 'Now_playing' key holds the most recently started track
	[[movies objectAtIndex:nowPlaying] stop];
}


/**
 * Returns the midi_pos variable
 * @return -1 if current track has finished, 0 otherwise.
 */
long dr_midi_pos(void)
{
	float rate;

	rate =  [[movies objectAtIndex:nowPlaying] rate];

	return rate>0 ? 0 : -1;
}


/**
 * Midi shutdown/cleanup
 * @author Hj. Malthaner
 */
void dr_destroy_midi(void)
{
	// closedown midi routines
}


/**
 * Sets midi pos
 * @author Hj. Malthaner
 */
void set_midi_pos(int pos)
{
	// presumably set the position within the current midi routine?
	// the sdl_midi.cc file has a placeholder routine here only.
}

/**
 * MIDI initialisation routines
 * @author Owen Rudge
 */
void dr_init_midi(void)
{
	// startup midi routines

	// configure arrays
	files = [[NSMutableArray alloc] initWithCapacity: MAX_MIDI];
	movies = [[NSMutableArray alloc] initWithCapacity: MAX_MIDI];
}


