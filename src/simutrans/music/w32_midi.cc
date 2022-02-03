/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <mmsystem.h>

#include "../utils/plainstring.h"
#include "music.h"
#include "../simsound.h"
#include "../simdebug.h"


// native win32 midi playing routines

static int         midi_number = -1;
static plainstring midi_filenames[MAX_MIDI];


static int OldMIDIVol[2] = {-1, -1};


#define __MIDI_VOL_SIMU    1  // 0-255
#define __MIDI_VOL_WIN32   2  // 0-65535


void __win32_set_midi_volume(int type, int left, int right);


/**
 * sets midi playback volume
 */
void dr_set_midi_volume(int vol)
{
	__win32_set_midi_volume(__MIDI_VOL_SIMU, vol, vol);
}


/**
 * Loads a MIDI file
 */

int dr_load_midi(const char *filename)
{
	if(midi_number < MAX_MIDI-1) {
		const int i = midi_number + 1;

		if (i >= 0 && i < MAX_MIDI) {

			// MCI doesn't like relative paths
			// but we get absolute ones anyway
			// already absolute path
			midi_filenames[i] = filename;

			// need to make dos path separators
			for (char* j = midi_filenames[i]; *j != '\0'; ++j) {
				if (*j == '/') {
					*j = '\\';
				}
			}

			midi_number = i;
		}
	}
	return midi_number;
}


/**
 * Plays a MIDI file
 * Key: The index of the MIDI file to be played
 */
void dr_play_midi(int key)
{
	char str[200], retstr[200];

	if (midi_number > 0) {

		if (key >= 0 && key <= midi_number) {
			sprintf(str, "open \"%s\" type sequencer alias SimuMIDI", midi_filenames[key].c_str());
			dbg->debug("dr_play_midi(w32)", "MCI string: %s", str);

			if (mciSendStringA(str, NULL, 0, NULL) != 0) {
				dbg->warning("dr_play_midi(w32)", "Unable to load MIDI %d", key);
			}
			else if (mciSendStringA("play SimuMIDI", retstr, 200, NULL) != 0) {
				dbg->warning("dr_play_midi(w32)", "Unable to play MIDI %d - %s\n", key, retstr);
			}
		}
		else {
			dbg->warning("dr_play_midi(w32)", "Unable to play MIDI %d", key);
		}
	}
}



/**
 * Stops playing MIDI file
 */
void dr_stop_midi()
{
	//   stop_midi();
	char retstr[200];

	mciSendStringA("stop SimuMIDI", retstr, 200, NULL);
	mciSendStringA("close SimuMIDI", retstr, 200, NULL);
}


/**
 * Returns the midi_pos variable
 */
sint32 dr_midi_pos()
{
	char retstr[200];
	long length;

	mciSendStringA("set SimuMIDI time format milliseconds", retstr, 200, NULL);
	mciSendStringA("status SimuMIDI length", retstr, 200, NULL);
	length = atol(retstr);

	if (mciSendStringA("status SimuMIDI position", retstr, 200, NULL) == 0) {
		const long pos = atol(retstr);
		if (pos == length) {
			mciSendStringA("stop SimuMIDI", retstr, 200, NULL);  // We must stop ourselves
			mciSendStringA("close SimuMIDI", retstr, 200, NULL);
			return (-1);
		}
		else {
			return pos;
		}
	}
	return 0;
}


/**
 * Midi shutdown/cleanup
 */
void dr_destroy_midi()
{
	__win32_set_midi_volume(__MIDI_VOL_WIN32, OldMIDIVol[0], OldMIDIVol[1]);
	midi_number = -1;
}


/**
 * MIDI initialisation routines
 */
bool dr_init_midi()
{
 #ifdef MIXER_VOLUME
	UINT nMIDIDevices;
	MIXERLINECONTROLS mlc;
	MIXERCONTROL MixControl;
	MIXERCONTROLDETAILS MixControlDetails;
	MIXERCONTROLDETAILS_UNSIGNED MixControlDetailsUnsigned[2];
	MIXERLINE MixerLine;
	HMIXER hMixer;
	MIXERCAPS DevCaps;

	// Reset MIDI volume

	nMIDIDevices = midiOutGetNumDevs();

	if (nMIDIDevices == 0)
		return;

	mixerOpen(&hMixer, 0, 0, 0, MIXER_OBJECTF_MIDIOUT);
	mixerGetDevCaps((UINT) hMixer, &DevCaps, sizeof(DevCaps));
	mixerClose(hMixer);

	MixerLine.cbStruct = sizeof(MixerLine);
	MixerLine.Target.dwType = MIXERLINE_TARGETTYPE_MIDIOUT;
	MixerLine.Target.wMid = DevCaps.wMid;
	MixerLine.Target.wPid = DevCaps.wPid;
	MixerLine.Target.vDriverVersion = DevCaps.vDriverVersion;
	strcpy(MixerLine.Target.szPname, DevCaps.szPname);

	mixerGetLineInfo(0, &MixerLine, MIXER_GETLINEINFOF_TARGETTYPE | MIXER_OBJECTF_MIDIOUT);

	mlc.cbStruct = sizeof(mlc);
	mlc.dwLineID = MixerLine.dwLineID;
	mlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
	mlc.cControls = 1;
	mlc.cbmxctrl = sizeof(MixControl);
	mlc.pamxctrl = &MixControl;

	MixControl.cbStruct = sizeof(MixControl);
	MixControl.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;

	mixerGetLineControls(0, &mlc, MIXER_OBJECTF_MIDIOUT | MIXER_GETLINECONTROLSF_ONEBYTYPE);

	MixControlDetails.cbStruct = sizeof(MixControlDetails);
	MixControlDetails.dwControlID = MixControl.dwControlID;
	MixControlDetails.cChannels = MixerLine.cChannels;
	MixControlDetails.hwndOwner = NULL;
	MixControlDetails.cMultipleItems = 0;
	MixControlDetails.cbDetails = sizeof(MixControlDetailsUnsigned[0])*2;
	MixControlDetails.paDetails = &MixControlDetailsUnsigned[0];

	mixerGetControlDetails(0, &MixControlDetails, MIXER_OBJECTF_MIDIOUT | MIXER_GETCONTROLDETAILSF_VALUE);

	OldMIDIVol[0] = MixControlDetailsUnsigned[0].dwValue;  // Save the old volume
	OldMIDIVol[1] = MixControlDetailsUnsigned[0].dwValue;

	sound_set_midi_volume_var(OldMIDIVol[0] >> 8); // Set the MIDI volume

	mixerSetControlDetails(0, &MixControlDetails, MIXER_OBJECTF_MIDIOUT | MIXER_SETCONTROLDETAILSF_VALUE);
#else
	if( midiOutGetNumDevs()== 0 ) {
		return false;
	}
	DWORD old_volume;
	midiOutGetVolume( 0, &old_volume );
	OldMIDIVol[0] = old_volume>>24;
	OldMIDIVol[1] = (old_volume&0x0000FF00)>>8;
#endif
	// assuming if we got here, all is set up to work properly
	return true;
}


#ifdef MIXER_VOLUME
// Sets the MIDI volume - internal routine
void __win32_set_midi_volume(int type, int left, int right)
{
	UINT nMIDIDevices;
	MIXERLINECONTROLS mlc;
	MIXERCONTROL MixControl;
	MIXERCONTROLDETAILS MixControlDetails;
	MIXERCONTROLDETAILS_UNSIGNED MixControlDetailsUnsigned[2];
	MIXERLINE MixerLine;
	HMIXER hMixer;
	MIXERCAPS DevCaps;

	nMIDIDevices = midiOutGetNumDevs();
	if (nMIDIDevices == 0) {
		return;
	}

	mixerOpen(&hMixer, 0, 0, 0, MIXER_OBJECTF_MIDIOUT);
	mixerGetDevCaps((UINT) hMixer, &DevCaps, sizeof(DevCaps));
	mixerClose(hMixer);

	MixerLine.cbStruct = sizeof(MixerLine);
	MixerLine.Target.dwType = MIXERLINE_TARGETTYPE_MIDIOUT;
	MixerLine.Target.wMid = DevCaps.wMid;
	MixerLine.Target.wPid = DevCaps.wPid;
	MixerLine.Target.vDriverVersion = DevCaps.vDriverVersion;
	strcpy(MixerLine.Target.szPname, DevCaps.szPname);

	mixerGetLineInfo(0, &MixerLine, MIXER_GETLINEINFOF_TARGETTYPE | MIXER_OBJECTF_MIDIOUT);

	mlc.cbStruct = sizeof(mlc);
	mlc.dwLineID = MixerLine.dwLineID;
	mlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
	mlc.cControls = 1;
	mlc.cbmxctrl = sizeof(MixControl);
	mlc.pamxctrl = &MixControl;

	MixControl.cbStruct = sizeof(MixControl);
	MixControl.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;

	mixerGetLineControls(0, &mlc, MIXER_OBJECTF_MIDIOUT | MIXER_GETLINECONTROLSF_ONEBYTYPE);

	MixControlDetails.cbStruct = sizeof(MixControlDetails);
	MixControlDetails.dwControlID = MixControl.dwControlID;
	MixControlDetails.cChannels = MixerLine.cChannels;
	MixControlDetails.hwndOwner = NULL;
	MixControlDetails.cMultipleItems = 0;
	MixControlDetails.cbDetails = sizeof(MixControlDetailsUnsigned[0])*2;
	MixControlDetails.paDetails = &MixControlDetailsUnsigned[0];

	mixerGetControlDetails(0, &MixControlDetails, MIXER_OBJECTF_MIDIOUT | MIXER_GETCONTROLDETAILSF_VALUE);

	if (type == __MIDI_VOL_SIMU)
	{
		MixControlDetailsUnsigned[0].dwValue = (left << 8);
		MixControlDetailsUnsigned[1].dwValue = (right << 8);
	}
	else
	{
		MixControlDetailsUnsigned[0].dwValue = left;
		MixControlDetailsUnsigned[1].dwValue = right;
	}

	mixerSetControlDetails(0, &MixControlDetails, MIXER_OBJECTF_MIDIOUT | MIXER_SETCONTROLDETAILSF_VALUE);

	// Phew, I'm glad that's over! What a horrible API...
}
#else
// Sets the MIDI volume - internal routine
void __win32_set_midi_volume(int , int left, int right)
{
	// short version
	long vol = (left<<24)|(right<<8);

	if( midiOutGetNumDevs()== 0 ) {
		return;
	}
	midiOutSetVolume( 0, vol );
}
#endif
