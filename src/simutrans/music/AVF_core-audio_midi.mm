/*
 * Apple OSX Core Audio MIDI routine added by Leopard
 *
 * This file is part of the Simutrans project under the artistic licence.
 *
 */

#import "music.h"
#include "../simdebug.h"

#import <Foundation/NSArray.h>
#import <Foundation/NSString.h>
#import <AVFoundation/AVFoundation.h>


static int   nowPlaying    = -1;  // the number of the track currently being played

static NSMutableArray* players;


void dr_set_midi_volume(int const vol)
{
	// Not supportd by AVMIDIPlayer
}


int dr_load_midi(char const* const filename)
{
	NSURL* const url = [NSURL fileURLWithPath: [NSString stringWithUTF8String: filename]];
	AVMIDIPlayer*  const player = [[AVMIDIPlayer alloc] initWithContentsOfURL:url soundBankURL: nil error: nil];
	if (player) {
		[player prepareToPlay];
		[players addObject: player];
	}
	return [players count] - 1;
}


void dr_play_midi(int const key)
{
	// Play the file referenced by the supplied key.
	AVMIDIPlayer* const player = [players objectAtIndex: key];
	double duration = [player duration];
	double currentPosition = [player currentPosition];
	if (currentPosition >= duration) {
		[player setCurrentPosition: 0.0];
	}
	try {
		[player play: ^{}];
	} catch (NSException *e) {
		dbg->warning("dr_play_midi()", "AVFoundation: Error playing midi");
	}
	nowPlaying = key;
}


void dr_stop_midi()
{
	if(  nowPlaying!= -1  ) {
		// We assume the 'nowPlaying' key holds the most recently started track.
		AVMIDIPlayer* const player = [players objectAtIndex: nowPlaying];
		[player stop];
	}
}


sint32 dr_midi_pos()
{
	if (nowPlaying == -1) {
		return -1;
	}
	float const rate = [[players objectAtIndex: nowPlaying] rate];
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
	players = [NSMutableArray arrayWithCapacity: MAX_MIDI];
	return true;
}
