/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Dialog for sound settings
 *
 * @author Hj. Malthaner, Owen Rudge
 * @date 09-Apr-01
 */

#include "gui_frame.h"
#include "components/gui_scrollbar.h"
#include "components/gui_label.h"
#include "components/gui_button.h"
#include "components/action_listener.h"

class sound_frame_t : public gui_frame_t, action_listener_t
{
private:
    scrollbar_t sound_volume_scrollbar;
    scrollbar_t music_volume_scrollbar;
    gui_label_t sound_volume_label;
    gui_label_t music_volume_label;
    button_t sound_mute_button;
    button_t music_mute_button;
    button_t next_song_button;
    button_t previous_song_button;
    button_t shuffle_song_button;
    gui_label_t song_name_label;
    gui_label_t current_playing_label;


    char song_buf[128];
    const char *make_song_name();

public:

    /**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
     * @author Hj. Malthaner
     */
    const char * get_hilfe_datei() const {return "sound.txt";}


    /**
	 * Constructor. Adds all necessary Subcomponents.
     * @author Hj. Malthaner
     */
    sound_frame_t();

    /**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
     * @author Hj. Malthaner
     */
    void zeichnen(koord pos, koord gr);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};
