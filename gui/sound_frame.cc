/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Dialog for sound settings
 */
#include "sound_frame.h"
#include "../simsound.h"
#include "../dataobj/translator.h"
#include "components/gui_divider.h"

#define L_KNOB_SIZE (32)

void sound_frame_t::update_song_name()
{
	const int current_midi = get_current_midi();

	if(current_midi >= 0) {
		song_name_label.buf().printf("%d - %s", current_midi+1, sound_get_midi_title(current_midi));
	}
	else {
		song_name_label.buf().printf("Music playing disabled/not available" );
	}
	song_name_label.update();
}


sound_frame_t::sound_frame_t()
  : gui_frame_t( translator::translate("Sound settings") ),
    sound_volume_scrollbar(scrollbar_t::horizontal),
    music_volume_scrollbar(scrollbar_t::horizontal)
{
	set_table_layout(1,0);

	// Sound volume label
	new_component<gui_label_t>("Sound volume:");

	sound_volume_scrollbar.set_knob(L_KNOB_SIZE, 255+L_KNOB_SIZE);
	sound_volume_scrollbar.set_knob_offset(sound_get_global_volume());
	sound_volume_scrollbar.set_scroll_discrete(false);
	sound_volume_scrollbar.add_listener( this );
	add_component(&sound_volume_scrollbar);

	sound_mute_button.init( button_t::square_state, "mute sound");
	sound_mute_button.pressed = sound_get_mute();
	add_component(&sound_mute_button);
	sound_mute_button.add_listener( this );

	new_component<gui_divider_t>();
	// Music
	new_component<gui_label_t>("Music volume:");

	music_volume_scrollbar.set_knob(L_KNOB_SIZE, 255+L_KNOB_SIZE);
	music_volume_scrollbar.set_knob_offset(sound_get_midi_volume());
	music_volume_scrollbar.set_scroll_discrete(false);
	music_volume_scrollbar.add_listener( this );
	add_component(&music_volume_scrollbar);

	music_mute_button.init( button_t::square_state, "disable midi");
	music_mute_button.pressed = midi_get_mute();
	music_mute_button.add_listener( this );
	add_component(&music_mute_button);

	new_component<gui_divider_t>();
	// song selection
	new_component<gui_label_t>("Currently playing:");

	add_table(3,1);
	{
		previous_song_button.set_typ(button_t::arrowleft);
		previous_song_button.add_listener(this);
		add_component(&previous_song_button);

		next_song_button.set_typ(button_t::arrowright);
		next_song_button.add_listener(this);
		add_component(&next_song_button);

		add_component(&song_name_label);
		update_song_name();
	}
	end_table();

	shuffle_song_button.init( button_t::square_state, "shuffle midis");
	shuffle_song_button.pressed = sound_get_shuffle_midi();
	shuffle_song_button.add_listener(this);
	add_component(&shuffle_song_button);


	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
}


bool sound_frame_t::action_triggered( gui_action_creator_t *comp, value_t p)
{
	if (comp == &next_song_button) {
		midi_stop();
		midi_next_track();
		check_midi();
		update_song_name();
	}
	else if (comp == &previous_song_button) {
		midi_stop();
		midi_last_track();
		check_midi();
		update_song_name();
	}
	else if (comp == &shuffle_song_button) {
		sound_set_shuffle_midi( !sound_get_shuffle_midi() );
		shuffle_song_button.pressed = sound_get_shuffle_midi();
	}
	else if (comp == &sound_mute_button) {
		sound_set_mute( !sound_mute_button.pressed );
		sound_mute_button.pressed = sound_get_mute();
	}
	else if (comp == &music_mute_button) {
		midi_set_mute( !music_mute_button.pressed );
		music_mute_button.pressed = midi_get_mute();
		previous_song_button.enable(!music_mute_button.pressed);
	}
	else if (comp == &sound_volume_scrollbar) {
		sound_set_global_volume(p.i);
	}
	else if (comp == &music_volume_scrollbar) {
		sound_set_midi_volume(p.i);
	}
	return true;
}


/**
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 * @author Hj. Malthaner
 */
void sound_frame_t::draw(scr_coord pos, scr_size size)
{
	// update song name label
	update_song_name();
	gui_frame_t::draw(pos, size);
}
