/*
 * native win32 midi playing routines
 *
 * author: Owen Rudge, Hj. Malthaner
 */
#ifdef _MSC_VER
#include <direct.h>
#else
#include <dir.h>
#endif

#include <stdio.h>
#include <windows.h>
#include <mmsystem.h>
#include "music.h"


/*
 * Hajo: flag if midi module should be used
 */
static int use_midi = 0;


/*
 * MIDI: Owen Rudge
 */
static int midi_number = -1;

char *midi_filenames[MAX_MIDI];


static int OldMIDIVol[2] = {-1, -1};


extern void sound_set_midi_volume_var(int volume);


#define __MIDI_VOL_SIMU    1  // 0-255
#define __MIDI_VOL_WIN32   2  // 0-65535


void __win32_set_midi_volume(int type, int left, int right);


/**
 * sets midi playback volume
 * @author Owen Rudge
 */
void dr_set_midi_volume(int vol)
{
	if(use_midi) {
		__win32_set_midi_volume(__MIDI_VOL_SIMU, vol, vol);
	}
}


/**
 * Loads a MIDI file
 * @author Owen Rudge, changes by Hj. Malthaner
 */

int dr_load_midi(const char *filename)
{
  if(use_midi) {

    unsigned j;

    //   printf("dr_load_midi(%s)\n", filename);
    if(midi_number < MAX_MIDI-1) {
      const int i = midi_number + 1;

      if (i >= 0 && i < MAX_MIDI) {

				// MCI doesn't like relative paths
				// but we get absolute ones anyway
				// already absolute path
				midi_filenames[i] = malloc( strlen(filename)+1 );
				strcpy(midi_filenames[i], filename );

				// need to make dos path seperators
				for (j = 0; j < strlen(midi_filenames[i]); j++)	{
					if (midi_filenames[i][j] == '/') {
						midi_filenames[i][j] = '\\';
					}
				}

				midi_number = i;
      }
    }
    return midi_number;
  } else {
    return -1;
  }
}


/**
 * Plays a MIDI file
 * Key: The index of the MIDI file to be played
 * By Owen Rudge
 */
void dr_play_midi(int key)
{
	if(use_midi) {
		char str[200], retstr[200];

		if (midi_number > 0) {

			if (key >= 0 && key <= midi_number) {
				sprintf(str, "open \"%s\" type sequencer alias SimuMIDI", midi_filenames[key]);
				printf("MCI string: %s\n", str);

				if (mciSendStringA(str, NULL, 0, NULL) != 0) {
					printf("\nMessage: MIDI: Unable to load MIDI %d\n", key);
				}
				else {
					if (mciSendStringA("play SimuMIDI", NULL, 0, NULL) != 0)
						printf("\nMessage: MIDI: Unable to play MIDI %d - %s\n", key, retstr);
					}
				}
			else {
					printf("\nMessage: MIDI: Unable to play MIDI %d\n", key);
			}
		}
	}
}


/**
 * Stops playing MIDI file
 * By Owen Rudge
 */
void dr_stop_midi(void)
{
	if(use_midi) {
		//   stop_midi();
		char retstr[200];

		mciSendStringA("stop SimuMIDI", retstr, 200, NULL);
		mciSendStringA("close SimuMIDI", retstr, 200, NULL);
	}
}


/**
 * Returns the midi_pos variable
 * By Owen Rudge
 */
long dr_midi_pos(void)
{
	if(use_midi) {
		char retstr[200];
		static long lastpos;
		long length;

		mciSendStringA("set SimuMIDI time format milliseconds", retstr, 200, NULL);
		mciSendStringA("status SimuMIDI length", retstr, 200, NULL);
		length = atol(retstr);

		if (mciSendStringA("status SimuMIDI position", retstr, 200, NULL) == 0) {
		//         printf("Position: %ld (%ld)\n", atol(retstr), lastpos);
			const long pos = atol(retstr);
			if (pos == length) {
				mciSendStringA("stop SimuMIDI", retstr, 200, NULL);  // We must stop ourselves
				mciSendStringA("close SimuMIDI", retstr, 200, NULL);
				return (-1);
			}
			else {
				lastpos = pos;
				return pos;
			}
		}
	}
	return 0;
}


/**
 * Midi shutdown/cleanup
 * By Owen Rudge
 */
void dr_destroy_midi(void)
{
	if(use_midi) {
		__win32_set_midi_volume(__MIDI_VOL_WIN32, OldMIDIVol[0], OldMIDIVol[1]);
		midi_number = -1;

		use_midi = 0;
	}
}


/**
 * MIDI initialisation routines
 * @author Owen Rudge
 */
void dr_init_midi(void)
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
	long old_volume;

	if( midiOutGetNumDevs()== 0 ) {
		return;
	}
	midiOutGetVolume( 0, &old_volume );
	OldMIDIVol[0] = old_volume>>24;
	OldMIDIVol[1] = (old_volume&0x0000FF00)>>8;
#endif

  // Hajo: assuming if we got here, all is set up to work properly
  use_midi = 1;
}

// CURRENTLY UNSUPPORTED
void set_midi_pos(int pos)
{
//   midi_pos = pos;
}

// Sets the MIDI volume - internal routine
void __win32_set_midi_volume(int type, int left, int right)
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
#else
	// prissis short version
	long vol = (left<<24)|(right<<8);

	if( midiOutGetNumDevs()== 0 ) {
		return;
	}
	midiOutSetVolume( 0, vol );
#endif
}
