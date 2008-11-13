/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "sound_frame.h"
#include "../simsound.h"
#include "../simintr.h"



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
  : gui_frame_t("Sound settings"),
    digi(scrollbar_t::horizontal),
    midi(scrollbar_t::horizontal),
    dlabel("Sound volume:"),
    mlabel("Music volume:"),
    curlabel(make_song_name()),
    cplaying("Currently playing:")
{
	dlabel.setze_pos(koord(10,10));
	add_komponente(&dlabel);

	digi.setze_groesse(koord(255, 10));
	digi.setze_pos(koord(10, 22));
	digi.setze_knob(32, 255+32);
	digi.setze_knob_offset(sound_get_global_volume());
	add_komponente(&digi);
	digi.add_listener( this );

	digi_mute.init( button_t::square_state, "mute sound", koord(10,36) );
	digi_mute.pressed = sound_get_mute();
	add_komponente(&digi_mute);
	digi_mute.add_listener( this );

	// now midi
	mlabel.setze_pos(koord(10,58));
	add_komponente(&mlabel);

	midi.setze_groesse(koord(255, 10));
	midi.setze_pos(koord(10, 70));
	midi.setze_knob(32, 255+32);
	midi.setze_knob_offset(sound_get_midi_volume());
	midi.add_listener( this );
	add_komponente(&midi);

	midi_mute.init( button_t::square_state, "disable midi", koord(10,84) );;
	midi_mute.pressed = midi_get_mute();
	midi_mute.add_listener( this );
	add_komponente(&midi_mute);

	// song selection
	cplaying.setze_pos(koord(10,106)); // "Currently Playing:"
	add_komponente(&cplaying);

	prevbtn.setze_pos(koord(10,118));
	prevbtn.setze_typ(button_t::arrowleft);
	prevbtn.add_listener(this);
	add_komponente(&prevbtn);

	nextbtn.setze_pos(koord(24,118));
	nextbtn.setze_typ(button_t::arrowright);
	nextbtn.add_listener(this);
	add_komponente(&nextbtn);

	curlabel.setze_pos(koord(42,118)); // "Jazz"
	add_komponente(&curlabel);

	shufflebtn.setze_pos(koord(10,130));
	shufflebtn.setze_typ(button_t::square_state);
	shufflebtn.setze_text("shuffle midis");
	shufflebtn.pressed = sound_get_shuffle_midi();
	shufflebtn.add_listener(this);
	add_komponente(&shufflebtn);

	setze_fenstergroesse(koord(276, 164));
}



bool
sound_frame_t::action_triggered(gui_komponente_t *komp,value_t p)
{
	if (komp == &nextbtn) {
		midi_stop();
		midi_next_track();
		check_midi();
		curlabel.setze_text(make_song_name());
	}
	else if (komp == &prevbtn) {
		midi_stop();
		midi_last_track();
		check_midi();
		curlabel.setze_text(make_song_name());
	}
	else if (komp == &shufflebtn) {
		sound_set_shuffle_midi( !sound_get_shuffle_midi() );
		shufflebtn.pressed = sound_get_shuffle_midi();
	}
	else if (komp == &digi_mute) {
		sound_set_mute( !digi_mute.pressed );
		digi_mute.pressed = sound_get_mute();
	}
	else if (komp == &midi_mute) {
		midi_set_mute( !midi_mute.pressed );
		midi_mute.pressed = midi_get_mute();
	}
	else if (komp == &digi) {
		sound_set_global_volume(p.i);
	}
	else if (komp == &midi) {
		sound_set_midi_volume(p.i);
	}
	return true;
}


/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 * @author Hj. Malthaner
 */
void sound_frame_t::zeichnen(koord pos, koord gr)
{
	// update song name label
	curlabel.setze_text(make_song_name());
	gui_frame_t::zeichnen(pos, gr);
}
