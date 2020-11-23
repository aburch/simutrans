/*
 * Apple OSX Core Audio MIDI routine added by Leopard
 *
 * This file is part of the Simutrans project under the artistic licence.
 *
 */

#import "music.h"

#import <Foundation/NSArray.h>
#import <Foundation/NSString.h>
#import <QTKit/QTMovie.h>


static float defaultVolume = 0.5; // a nice default volume
static int   nowPlaying    = -1;  // the number of the track currently being played

static NSMutableArray* movies;


void dr_set_midi_volume(int const vol)
{
	// We are given an integer from 0 - 255, we need a float between 0 and 1.
	defaultVolume = vol / 255.f;
	if (nowPlaying != -1) {
		[[movies objectAtIndex: nowPlaying] setVolume: defaultVolume];
	}
}


int dr_load_midi(char const* const filename)
{
	NSString* const s = [NSString stringWithUTF8String: filename];
	QTMovie*  const m = [QTMovie movieWithFile: s error: nil];
	if (m) {
		[movies addObject: m];
	}
	return [movies count] - 1;
}


void dr_play_midi(int const key)
{
	// Play the file referenced by the supplied key.
	QTMovie* const m = [movies objectAtIndex: key];
	[m setVolume:defaultVolume];
	[m play];
	nowPlaying = key;
}


void dr_stop_midi()
{
	if(  nowPlaying!= -1  ) {
		// We assume the 'nowPlaying' key holds the most recently started track.
		QTMovie* const m = [movies objectAtIndex: nowPlaying];
		[m stop];
	}
}


sint32 dr_midi_pos()
{
	if (nowPlaying == -1) {
		return -1;
	}
	float const rate = [[movies objectAtIndex: nowPlaying] rate];
	return rate > 0 ? 0 : -1;
}


void dr_destroy_midi()
{
	if (nowPlaying != -1) {
		dr_stop_midi();
	}
}


bool dr_init_midi()
{
	movies = [NSMutableArray arrayWithCapacity: MAX_MIDI];
	return true;
}
