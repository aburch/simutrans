/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * Dialog for sound settings
 */
#include "sound_frame.h"
#include "../simsound.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "components/gui_divider.h"
#ifdef USE_FLUIDSYNTH_MIDI
#include "loadsoundfont_frame.h"
#endif

#define L_KNOB_SIZE (32)

void sound_frame_t::update_song_name()
{
	const int current_midi = get_current_midi();
	if(  current_midi < 0  ) {
		song_name_label.buf().printf( translator::translate("Music playing disabled/not available") );
	}
#ifdef USE_FLUIDSYNTH_MIDI
	else if(  strcmp( env_t::soundfont_filename.c_str(), "Error") == 0  ){
		song_name_label.buf().printf( translator::translate("Soundfont not found. Please select a soundfont below.") );
	}
#endif
	else {
		song_name_label.buf().printf("%d - %s", current_midi + 1, sound_get_midi_title( current_midi ) );
	}
	song_name_label.update();
	song_name_label.set_size( song_name_label.get_min_size() );

	// Loadsoundfont dialog may unmute us, update mute status
	music_mute_button.pressed = midi_get_mute();
	previous_song_button.enable( !music_mute_button.pressed );
	next_song_button.enable( !music_mute_button.pressed );
}


const char *specific_volume_names[ MAX_SOUND_TYPES ] = {
	"TOOL_SOUND",
	"TRAFFIC_SOUND",
	"AMBIENT_SOUND",
	"FACTORY_SOUND",
	"CROSSING_SOUND",
	"CASH_SOUND"
};


sound_frame_t::sound_frame_t() :
	gui_frame_t( translator::translate("Sound settings") ),
	sound_volume_scrollbar(scrollbar_t::horizontal),
	music_volume_scrollbar(scrollbar_t::horizontal)
{
	set_table_layout(1,0);

	sound_mute_button.init( button_t::square_state, "mute sound");
	sound_mute_button.pressed = sound_get_mute();
	add_component(&sound_mute_button);
	sound_mute_button.add_listener( this );

	add_table(2,0);
	{
		// Sound volume label
		new_component<gui_label_t>( "Sound volume:" );

		sound_volume_scrollbar.set_knob( L_KNOB_SIZE, 255 + L_KNOB_SIZE );
		sound_volume_scrollbar.set_knob_offset( sound_get_global_volume() );
		sound_volume_scrollbar.set_scroll_discrete( false );
		sound_volume_scrollbar.add_listener( this );
		add_component( &sound_volume_scrollbar );

		new_component<gui_label_t>( "Sound range:" );

		sound_range.set_value( env_t::sound_distance_scaling);
		sound_range.set_limits( 1, 32 );
		sound_range.add_listener(this);
//		sound_range.set_tooltip( "Lower values mean more local sounds" )
		add_component(&sound_range);

		for( int i = 0; i < MAX_SOUND_TYPES; i++ ) {
			new_component<gui_label_t>( specific_volume_names[i] );

			specific_volume_scrollbar[ i ] = new scrollbar_t( scrollbar_t::horizontal );
			specific_volume_scrollbar[i]->set_knob( L_KNOB_SIZE, 255 + L_KNOB_SIZE );
			specific_volume_scrollbar[i]->set_knob_offset( sound_get_specific_volume((sound_type_t)i) );
			specific_volume_scrollbar[i]->set_scroll_discrete( false );
			specific_volume_scrollbar[i]->add_listener( this );
			add_component( specific_volume_scrollbar[i] );
		}
	}
	end_table();

	new_component<gui_divider_t>();

	// Music
	music_mute_button.init( button_t::square_state, "disable midi");
	music_mute_button.pressed = midi_get_mute();
	music_mute_button.add_listener( this );
#ifdef USE_FLUIDSYNTH_MIDI
	if(  strcmp( env_t::soundfont_filename.c_str(), "Error" ) == 0  ){
		music_mute_button.enable( !music_mute_button.pressed );
	}
#endif
	add_component(&music_mute_button);

	add_table(2,0);
	{
		new_component<gui_label_t>( "Music volume:" );

		music_volume_scrollbar.set_knob( L_KNOB_SIZE, 255 + L_KNOB_SIZE );
		music_volume_scrollbar.set_knob_offset( sound_get_midi_volume() );
		music_volume_scrollbar.set_scroll_discrete( false );
		music_volume_scrollbar.add_listener( this );
		add_component( &music_volume_scrollbar );
	}
	end_table();

	// song selection
	new_component<gui_label_t>( "Currently playing:" );

	add_table( 3, 1 );
	{
		previous_song_button.set_typ( button_t::arrowleft );
		previous_song_button.add_listener( this );
		previous_song_button.enable( !music_mute_button.pressed );
		add_component( &previous_song_button );

		next_song_button.set_typ( button_t::arrowright );
		next_song_button.add_listener( this );
		next_song_button.enable( !music_mute_button.pressed );
		add_component( &next_song_button );

		add_component( &song_name_label );
		update_song_name();
	}
	end_table();

	shuffle_song_button.init( button_t::square_state, "shuffle midis" );
	shuffle_song_button.pressed = sound_get_shuffle_midi();
	shuffle_song_button.add_listener(this);
	add_component(&shuffle_song_button);

#ifdef USE_FLUIDSYNTH_MIDI
	// Soundfont selection
	soundfont_button.init( button_t::roundbox_state | button_t::flexible, "Select soundfont" );
	soundfont_button.add_listener(this);
	add_component( &soundfont_button );
#endif

	set_resizemode(diagonal_resize);
	reset_min_windowsize();
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
		previous_song_button.enable( !music_mute_button.pressed );
		next_song_button.enable( !music_mute_button.pressed );
	}
	else if (comp == &sound_volume_scrollbar) {
		sound_set_global_volume(p.i);
	}
	else if (comp == &music_volume_scrollbar) {
		sound_set_midi_volume(p.i);
	}
	else if (comp == &sound_range) {
		env_t::sound_distance_scaling = p.i;
	}
#ifdef USE_FLUIDSYNTH_MIDI
	else if(  comp == &soundfont_button  ) {
		create_win( new loadsoundfont_frame_t(), w_info, magic_soundfont );
	}
#endif
	else {
		for( int i = 0; i < MAX_SOUND_TYPES; i++ ) {
			if( comp == specific_volume_scrollbar[ i ] ) {
				sound_set_specific_volume( p.i, (sound_type_t)i );
			}
		}
	}
	return true;
}


/**
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 */
void sound_frame_t::draw(scr_coord pos, scr_size size)
{
	// update song name label
	update_song_name();
#ifdef USE_FLUIDSYNTH_MIDI
	if(  strcmp(env_t::soundfont_filename.c_str(), "Error") != 0  ){
		music_mute_button.enable( true );
	}
#endif
	gui_frame_t::draw(pos, size);
}


// need to delete scroll bars
sound_frame_t::~sound_frame_t()
{
	for( int i = 0; i < MAX_SOUND_TYPES; i++ ) {
		delete specific_volume_scrollbar[ i ];
	}
}
