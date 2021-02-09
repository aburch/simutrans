/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_SOUND_FRAME_H
#define GUI_SOUND_FRAME_H


#include "gui_frame.h"
#include "components/gui_scrollbar.h"
#include "components/gui_label.h"
#include "components/gui_button.h"
#include "components/gui_numberinput.h"
#include "components/action_listener.h"

/**
 * Dialog for sound settings
 */
class sound_frame_t : public gui_frame_t, action_listener_t
{
private:
	scrollbar_t sound_volume_scrollbar;
	scrollbar_t music_volume_scrollbar;
	scrollbar_t *specific_volume_scrollbar[MAX_SOUND_TYPES+1];
	gui_numberinput_t sound_range;

	button_t sound_mute_button;
	button_t music_mute_button;
	button_t next_song_button;
	button_t previous_song_button;
	button_t shuffle_song_button;
#ifdef USE_FLUIDSYNTH_MIDI
	button_t soundfont_button;
#endif
	gui_label_buf_t song_name_label;

	void update_song_name();

public:
	const char *get_help_filename() const OVERRIDE {return "sound.txt";}

	sound_frame_t();

	virtual ~sound_frame_t();

	// used for updating the song title
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
