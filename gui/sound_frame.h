/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_SOUND_FRAME_H
#define GUI_SOUND_FRAME_H


#include "gui_frame.h"
#include "components/gui_scrollbar.h"
#include "components/gui_label.h"
#include "components/gui_button.h"
#include "components/action_listener.h"

/**
 * Dialog for sound settings
 */
class sound_frame_t : public gui_frame_t, action_listener_t
{
private:
	scrollbar_t sound_volume_scrollbar;
	scrollbar_t music_volume_scrollbar;
	button_t sound_mute_button;
	button_t music_mute_button;
	button_t next_song_button;
	button_t previous_song_button;
	button_t shuffle_song_button;
	gui_label_buf_t song_name_label;

	void update_song_name();

public:

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
     */
	const char * get_help_filename() const OVERRIDE {return "sound.txt";}


	/**
	 * Constructor. Adds all necessary Subcomponents.
     */
    sound_frame_t();

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
     */
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
