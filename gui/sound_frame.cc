/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
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
    digi.setze_groesse(koord(256, 10));
    digi.setze_pos(koord(22, 30));
    digi.setze_opaque( true );
    digi.setze_knob(32, 255+32);
    digi.setze_knob_offset(sound_get_global_volume());
    digi.add_listener( this );

    dlabel.setze_pos(koord(22,14));

    midi.setze_groesse(koord(255, 10));
    midi.setze_pos(koord(22, 80));
    midi.setze_opaque( true );
    midi.setze_knob(32, 255+32);
    midi.setze_knob_offset(sound_get_midi_volume());
    midi.add_listener( this );

    mlabel.setze_pos(koord(22,64));

    nextbtn.setze_pos(koord(38,133));
    nextbtn.setze_typ(button_t::arrowright);

    prevbtn.setze_pos(koord(22,133));
    prevbtn.setze_typ(button_t::arrowleft);

    shufflebtn.setze_pos(koord(22,146));
    shufflebtn.setze_typ(button_t::square_state);
    shufflebtn.setze_text("shuffle midis");
		shufflebtn.pressed = sound_get_shuffle_midi();

		cplaying.setze_pos(koord(22,114)); // "Currently Playing:"
    curlabel.setze_pos(koord(60,134)); // "Jazz"

    add_komponente(&dlabel);
    add_komponente(&digi);

    add_komponente(&mlabel);
    add_komponente(&midi);

    add_komponente(&cplaying);
    add_komponente(&curlabel);

    nextbtn.add_listener(this);
    prevbtn.add_listener(this);
    shufflebtn.add_listener(this);

    add_komponente(&nextbtn);
    add_komponente(&prevbtn);
    add_komponente(&shufflebtn);

    setze_fenstergroesse(koord(300, 180));
}



bool
sound_frame_t::action_triggered(gui_komponente_t *komp,value_t p)
{
	if (komp == &nextbtn) {
		midi_stop();
		midi_next_track();
		check_midi();
		curlabel.setze_text(make_song_name());
		intr_refresh_display(true);
	}
	else if (komp == &prevbtn) {
		midi_stop();
		midi_last_track();
		check_midi();
		curlabel.setze_text(make_song_name());
		intr_refresh_display(true);
	}
	else if (komp == &shufflebtn) {
		sound_set_shuffle_midi( !sound_get_shuffle_midi() );
		shufflebtn.pressed = sound_get_shuffle_midi();
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
