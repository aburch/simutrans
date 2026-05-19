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


static int            midi_number = -1;
static NSString      *midi_filenames[MAX_MIDI];
static int            nowPlaying  = -1;
static AVMIDIPlayer  *current_player = nil;


void dr_set_midi_volume(int const vol)
{
	// Not supported by AVMIDIPlayer
}


int dr_load_midi(char const* const filename)
{
	if (midi_number < MAX_MIDI - 1) {
		const int i = midi_number + 1;
		midi_filenames[i] = [NSString stringWithUTF8String: filename];
		midi_number = i;
	}
	return midi_number;
}


void dr_play_midi(int const key)
{
	dr_stop_midi();

	if (key < 0 || key > midi_number || midi_filenames[key] == nil) {
		return;
	}

	NSURL *url = [NSURL fileURLWithPath: midi_filenames[key]];
	current_player = [[AVMIDIPlayer alloc] initWithContentsOfURL: url soundBankURL: nil error: nil];
	if (current_player) {
		[current_player prepareToPlay];
		[current_player play: ^{}];
		nowPlaying = key;
	}
	else {
		dbg->warning("dr_play_midi()", "AVFoundation: Failed to create player for key %d", key);
	}
}


void dr_stop_midi()
{
	if (current_player) {
		[current_player stop];
		current_player = nil;
	}
	nowPlaying = -1;
}


sint32 dr_midi_pos()
{
	if (current_player == nil) {
		return -1;
	}
	return [current_player rate] > 0 ? 0 : -1;
}


void dr_destroy_midi()
{
	dr_stop_midi();
	for (int i = 0; i <= midi_number; i++) {
		midi_filenames[i] = nil;
	}
	midi_number = -1;
}


bool dr_init_midi()
{
	return true;
}
