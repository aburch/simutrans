/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Dialog for sound settings
 */

#include <stdio.h>

#include "sound_frame.h"
#include "../simsound.h"
#include "../dataobj/translator.h"

#define L_KNOB_SIZE (32)
#define DIALOG_WIDTH (276)

const char *sound_frame_t::make_song_name()
{
	const int current_midi = get_current_midi();

	if(current_midi >= 0) {
		sprintf(song_buf, "%d - %s", current_midi+1, sound_get_midi_title(current_midi));
	}
	else {
		sprintf(song_buf, "Music playing disabled/not available" );
	}
	return(song_buf);
}


sound_frame_t::sound_frame_t()
  : gui_frame_t( translator::translate("Sound settings") ),
    sound_volume_scrollbar(scrollbar_t::horizontal),
    music_volume_scrollbar(scrollbar_t::horizontal),
    sound_volume_label("Sound volume:"),
    music_volume_label("Music volume:"),
    song_name_label(make_song_name()),
    current_playing_label("Currently playing:")
{

	scr_coord cursor = scr_coord(D_MARGIN_LEFT,D_MARGIN_TOP);

	// Sound volume label
	sound_volume_label.set_pos(cursor);
	add_komponente(&sound_volume_label);
	cursor.y += LINESPACE + D_V_SPACE;

	sound_volume_scrollbar.set_pos(cursor);
	sound_volume_scrollbar.set_size(scr_size(DIALOG_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT, D_SCROLLBAR_HEIGHT));
	sound_volume_scrollbar.set_knob(L_KNOB_SIZE, 255+L_KNOB_SIZE);
	sound_volume_scrollbar.set_knob_offset(sound_get_global_volume());
	sound_volume_scrollbar.set_scroll_discrete(false);
	add_komponente(&sound_volume_scrollbar);
	sound_volume_scrollbar.add_listener( this );
	cursor.y += D_SCROLLBAR_HEIGHT + D_V_SPACE;

	sound_mute_button.init( button_t::square_state, "mute sound", cursor ); // 1 = align with scrollbar background
	sound_mute_button.pressed = sound_get_mute();
	add_komponente(&sound_mute_button);
	sound_mute_button.add_listener( this );
	cursor.y += D_CHECKBOX_HEIGHT + D_V_SPACE*2;

	// Music
	music_volume_label.set_pos( cursor );
	add_komponente(&music_volume_label);
	cursor.y += LINESPACE + D_V_SPACE;

	music_volume_scrollbar.set_pos( cursor );
	music_volume_scrollbar.set_size(scr_size(DIALOG_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT, D_SCROLLBAR_HEIGHT));
	music_volume_scrollbar.set_knob(L_KNOB_SIZE, 255+L_KNOB_SIZE);
	music_volume_scrollbar.set_knob_offset(sound_get_midi_volume());
	music_volume_scrollbar.set_scroll_discrete(false);
	music_volume_scrollbar.add_listener( this );
	add_komponente(&music_volume_scrollbar);
	cursor.y += D_SCROLLBAR_HEIGHT + D_V_SPACE;

	music_mute_button.init( button_t::square_state, "disable midi",cursor ); // 1 = align with scrollbar background
	music_mute_button.pressed = midi_get_mute();
	music_mute_button.add_listener( this );
	add_komponente(&music_mute_button);
	cursor.y += LINESPACE + D_V_SPACE*2;

	// song selection
	current_playing_label.set_pos( cursor ); // "Currently Playing:"
	add_komponente(&current_playing_label);
	cursor.y += LINESPACE + D_V_SPACE;

	previous_song_button.init(button_t::arrowleft, "", cursor );
	//previous_song_button.set_typ(button_t::arrowleft);
	previous_song_button.add_listener(this);
	add_komponente(&previous_song_button);
	cursor.x += previous_song_button.get_size().w + D_H_SPACE;

	next_song_button.init(button_t::arrowright, "", cursor);
	//next_song_button.set_typ(button_t::arrowright);
	next_song_button.add_listener(this);
	add_komponente(&next_song_button);
	cursor.x += next_song_button.get_size().w + D_H_SPACE;

	song_name_label.set_pos(cursor); // "Jazz"
	add_komponente(&song_name_label);
	cursor.y += LINESPACE + D_V_SPACE;
	cursor.x = D_MARGIN_LEFT;

	previous_song_button.align_to(&song_name_label,ALIGN_CENTER_V);
	next_song_button.align_to(&song_name_label,ALIGN_CENTER_V);

	shuffle_song_button.init( button_t::square_state, "shuffle midis", cursor );
	shuffle_song_button.pressed = sound_get_shuffle_midi();
	shuffle_song_button.add_listener(this);
	add_komponente(&shuffle_song_button);
	cursor.y += LINESPACE;

	set_windowsize(scr_size(DIALOG_WIDTH, D_TITLEBAR_HEIGHT + cursor.y + D_MARGIN_BOTTOM));
}


bool sound_frame_t::action_triggered( gui_action_creator_t *komp, value_t p)
{
	if (komp == &next_song_button) {
		midi_stop();
		midi_next_track();
		check_midi();
		song_name_label.set_text(make_song_name());
	}
	else if (komp == &previous_song_button) {
		midi_stop();
		midi_last_track();
		check_midi();
		song_name_label.set_text(make_song_name());
	}
	else if (komp == &shuffle_song_button) {
		sound_set_shuffle_midi( !sound_get_shuffle_midi() );
		shuffle_song_button.pressed = sound_get_shuffle_midi();
	}
	else if (komp == &sound_mute_button) {
		sound_set_mute( !sound_mute_button.pressed );
		sound_mute_button.pressed = sound_get_mute();
	}
	else if (komp == &music_mute_button) {
		midi_set_mute( !music_mute_button.pressed );
		music_mute_button.pressed = midi_get_mute();
		previous_song_button.enable(!music_mute_button.pressed);
	}
	else if (komp == &sound_volume_scrollbar) {
		sound_set_global_volume(p.i);
	}
	else if (komp == &music_volume_scrollbar) {
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
	song_name_label.set_text(make_song_name());
	gui_frame_t::draw(pos, size);
}
