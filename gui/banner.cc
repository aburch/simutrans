/*
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Intro and everything else
 */

#include "../simcolor.h"
#include "../simevent.h"
#include "../display/simimg.h"
#include "../simworld.h"
#include "../simskin.h"
#include "../gui/simwin.h"
#include "../simsys.h"
#include "../simversion.h"
#include "../display/simgraph.h"
#include "../macros.h"
#include "../besch/skin_besch.h"
#include "../dataobj/environment.h"

#include "banner.h"
#include "loadsave_frame.h"
#include "scenario_frame.h"
#include "server_frame.h"

/* Max Kielland
 * Parameters to tweak GUI layout in this dialog
 * The original values LINESPACE+2, LINESPACE+5 and LINESPACE+7 has
 * been replaced with these defines. These are not standard values
 * and should be replaced later on when we know how to handle
 * LINESPACE and shadow text.
 */
#define L_LINESPACE_EXTRA_2  ( LINESPACE + 2 )
#define L_LINESPACE_EXTRA_5  ( LINESPACE + 5 )
#define L_LINESPACE_EXTRA_7  ( LINESPACE + 7 )

// Local adjustments
#define L_TEXT_INDENT        ( 24 )                            // Shadow text indent
#define L_BANNER_ROWS        ( 5 )                             // Rows of scroll text
#define L_BANNER_TEXT_INDENT ( 4 )                             // Scroll text padding (left/right)
#define L_BANNER_HEIGHT      ( L_BANNER_ROWS * LINESPACE + 2 ) // Banner control height in pixels

#define L_DIALOG_WIDTH (D_MARGIN_LEFT + 3*D_BUTTON_WIDTH + 2*D_H_SPACE + D_MARGIN_RIGHT)

#ifdef _OPTIMIZED
#	define L_DEBUG_TEXT " (optimized)"
#elif defined DEBUG
#	define L_DEBUG_TEXT " (debug)"
#else
#	define L_DEBUG_TEXT
#endif

// colors
#define COL_PT (5)
#define COLOR_RAMP_SIZE ( 5 ) // Number or fade colors + normal color at index 0

// Banner color ramp
// Index 0 is the normal text color
static const PLAYER_COLOR_VAL colors[COLOR_RAMP_SIZE] = { COL_WHITE, COL_GREY3, COL_GREY4, COL_GREY5, COL_GREY6 };


banner_t::banner_t() : gui_frame_t(""),
	logo( skinverwaltung_t::logosymbol->get_bild_nr(0), 0 )
{
	// Pass the upper part drawn by draw()
	scr_coord cursor = scr_coord( D_MARGIN_LEFT, D_MARGIN_TOP + 5*L_LINESPACE_EXTRA_2 + 3*L_LINESPACE_EXTRA_5 + 3*L_LINESPACE_EXTRA_7 + L_BANNER_HEIGHT + D_V_SPACE);
	scr_coord_val width;
	scr_size button_size;

	last_ms = dr_time();
	line = 0;

	// Calculate dialogue width based on a few assumptions.
	// When we have GUI controls for shadow text this will be much simpler...
#ifdef REVISION
	width = max( L_DIALOG_WIDTH, D_MARGINS_X + display_calc_proportional_string_len_width( "Version " VERSION_NUMBER " " VERSION_DATE " r" QUOTEME(REVISION) L_DEBUG_TEXT, -1 ) );
#else
	width = max( L_DIALOG_WIDTH, D_MARGINS_X + display_calc_proportional_string_len_width( "Version " VERSION_NUMBER " " VERSION_DATE L_DEBUG_TEXT, -1 ) );
#endif

	width = max( width, D_MARGINS_X + L_TEXT_INDENT + display_calc_proportional_string_len_width( "Selling of the program is forbidden.", -1) + skinverwaltung_t::logosymbol->get_bild(0)->get_pic()->w + D_H_SPACE);
	button_size = scr_size( (width - D_MARGIN_LEFT - (D_H_SPACE<<1) - D_MARGIN_RIGHT) / 3,D_BUTTON_HEIGHT );

	// Position logo in relation to text drawn by draw()
	logo.set_pos( scr_coord(width - D_MARGIN_RIGHT - skinverwaltung_t::logosymbol->get_bild(0)->get_pic()->w, D_MARGIN_TOP + L_LINESPACE_EXTRA_5 + L_LINESPACE_EXTRA_7 ) );
	add_komponente( &logo );

	// New game button
	new_map.init( button_t::roundbox, "Neue Karte", cursor, button_size );
	new_map.add_listener( this );
	add_komponente( &new_map );
	cursor.x += button_size.w + D_H_SPACE;

	// Load game button
	load_map.init( button_t::roundbox, "Load game", cursor, button_size );
	load_map.add_listener( this );
	add_komponente( &load_map );
	cursor.x += button_size.w + D_H_SPACE;

	// Load scenario button
	load_scenario.init( button_t::roundbox, "Load scenario", cursor, button_size );
	load_scenario.add_listener( this );
	add_komponente( &load_scenario );
	cursor.y += D_BUTTON_HEIGHT + D_V_SPACE;
	cursor.x  = D_MARGIN_LEFT + button_size.w + D_H_SPACE;

	// Play online button
	join_map.init( button_t::roundbox, "join game", cursor, button_size );
	join_map.add_listener( this );
	add_komponente( &join_map );
	cursor.x += button_size.w + D_H_SPACE;

	// Quit button
	quit.init( button_t::roundbox, "Beenden", cursor, button_size );
	quit.add_listener( this );
	add_komponente( &quit );
	cursor += D_BUTTON_SIZE;

	set_windowsize( scr_size( width, D_TITLEBAR_HEIGHT + cursor.y + D_MARGIN_BOTTOM ) );
}


bool banner_t::infowin_event(const event_t *ev)
{
	if(  gui_frame_t::getroffen( ev->cx, ev->cy  )  ) {
		gui_frame_t::infowin_event( ev );
	}
	return false;
}


bool banner_t::action_triggered( gui_action_creator_t *komp, value_t)
{
	if(  komp == &quit  ) {
		env_t::quit_simutrans = true;
		destroy_all_win(true);
	}
	else if(  komp == &new_map  ) {
		destroy_all_win(true);
	}
	else if(  komp == &load_map  ) {
		destroy_all_win(true);
		create_win( new loadsave_frame_t(true), w_info, magic_load_t);
	}
	else if(  komp == &load_scenario  ) {
		destroy_all_win(true);
		create_win( new scenario_frame_t(), w_info, magic_load_t );
	}
	else if(  komp == &join_map  ) {
		destroy_all_win(true);
		create_win( new server_frame_t(), w_info, magic_server_frame_t );
	}
	return true;
}


void banner_t::draw(scr_coord pos, scr_size size )
{
	scr_coord cursor = pos + scr_coord( D_MARGIN_LEFT, D_TITLEBAR_HEIGHT + D_MARGIN_TOP);
	gui_frame_t::draw( pos, size );

	// Hajo: add white line on top since this frame has no title bar.
	display_fillbox_wh(pos.x, pos.y + D_TITLEBAR_HEIGHT, size.w, 1, COL_GREY6, false);

	// Max Kielland: Add shadow as property to label_t so we can use the label_t class instead...
	display_shadow_proportional( cursor.x, cursor.y, COL_PT, COL_BLACK, "This is Simutrans" SIM_VERSION_BUILD_STRING, true );
	cursor.y += L_LINESPACE_EXTRA_5;

#ifdef REVISION
	display_shadow_proportional( cursor.x, cursor.y, SYSCOL_TEXT_HIGHLIGHT, COL_BLACK, "Version " VERSION_NUMBER " " VERSION_DATE " r" QUOTEME(REVISION) L_DEBUG_TEXT, true );
#else
	display_shadow_proportional( cursor.x, cursor.y, SYSCOL_TEXT_HIGHLIGHT, COL_BLACK, "Version " VERSION_NUMBER " " VERSION_DATE L_DEBUG_TEXT, true );
#endif
	cursor.y += L_LINESPACE_EXTRA_7;

	display_shadow_proportional( cursor.x, cursor.y, COL_PT, COL_BLACK, "The version is developed by", true );
	cursor += scr_coord (L_TEXT_INDENT,L_LINESPACE_EXTRA_5);

	display_shadow_proportional( cursor.x, cursor.y, SYSCOL_TEXT_HIGHLIGHT, COL_BLACK, "the simutrans team", true );
	cursor.y += L_LINESPACE_EXTRA_2;

	display_shadow_proportional( cursor.x, cursor.y, SYSCOL_TEXT_HIGHLIGHT, COL_BLACK, "under the Artistic Licence", true );
	cursor.y += L_LINESPACE_EXTRA_2;

	display_shadow_proportional( cursor.x, cursor.y, SYSCOL_TEXT_HIGHLIGHT, COL_BLACK, "based on Simutrans 84.22.1", true );
	cursor += scr_coord (-L_TEXT_INDENT,L_LINESPACE_EXTRA_7);

	display_shadow_proportional( cursor.x, cursor.y, COL_ORANGE, COL_BLACK, "Selling of the program is forbidden.", true );
	cursor.y += L_LINESPACE_EXTRA_5;

	display_shadow_proportional( cursor.x, cursor.y, COL_PT, COL_BLACK, "For questions and support please visit:", true );
	cursor += scr_coord (L_TEXT_INDENT,L_LINESPACE_EXTRA_2);

	display_shadow_proportional( cursor.x, cursor.y, SYSCOL_TEXT_HIGHLIGHT, COL_BLACK, "http://www.simutrans.com", true );
	cursor.y += L_LINESPACE_EXTRA_2;

	display_shadow_proportional( cursor.x, cursor.y, SYSCOL_TEXT_HIGHLIGHT, COL_BLACK, "http://forum.simutrans.com", true );
	cursor.y += L_LINESPACE_EXTRA_2;

	display_shadow_proportional( cursor.x, cursor.y, SYSCOL_TEXT_HIGHLIGHT, COL_BLACK, "http://wiki.simutrans-germany.com/", true );
	cursor.y += L_LINESPACE_EXTRA_7;

	// now the scrolling
	// Max Kielland TODO: Convert this to a gui component
	//                    BANNER_ROWS defines size and how many rows of text are shown in banner
	//                    BANNER_TEXT_INDENT defines left and right padding inside banner area

	static const char* const scrolltext[] = {
		#include "../scrolltext.h"
	};

	const scr_coord_val text_line = (line / LINESPACE) * 2;
	const scr_coord_val text_offset = line % LINESPACE;
	const scr_coord_val left = pos.x + D_MARGIN_LEFT;
	const scr_coord_val width = size.w - D_MARGIN_LEFT - D_MARGIN_RIGHT;
	PLAYER_COLOR_VAL color;

	display_fillbox_wh(left, cursor.y, width, L_BANNER_HEIGHT, COL_GREY1, true);
	display_fillbox_wh(left, cursor.y - 1, width, 1, COL_GREY3, false);
	display_fillbox_wh(left, cursor.y + L_BANNER_HEIGHT, width, 1, COL_GREY6, false);

	PUSH_CLIP( left, cursor.y, width, L_BANNER_HEIGHT );
	cursor.y++;

	for(  int row = 0;  row < L_BANNER_ROWS+1;  row++  ) {

		if(  row > L_BANNER_ROWS-COLOR_RAMP_SIZE+1  ) {
			color = colors[L_BANNER_ROWS-row+1];
		}
		else {
			color = colors[0];
		}

		if(  row == L_BANNER_ROWS  ||  row == 0  ) {
			display_proportional_clip( left + L_BANNER_TEXT_INDENT,         cursor.y - text_offset, scrolltext[text_line + row*2    ], ALIGN_LEFT,  color, false);
			display_proportional_clip( left + width - L_BANNER_TEXT_INDENT, cursor.y - text_offset, scrolltext[text_line + row*2 + 1], ALIGN_RIGHT, color, false);
		}
		else {
			display_proportional( left + L_BANNER_TEXT_INDENT,              cursor.y - text_offset, scrolltext[text_line + row*2    ], ALIGN_LEFT,  color, false);
			display_proportional( left + width - L_BANNER_TEXT_INDENT,      cursor.y - text_offset, scrolltext[text_line + row*2 + 1], ALIGN_RIGHT, color, false);
		}
		cursor.y += LINESPACE;

	}

	POP_CLIP();

	// scroll on every 70 ms
	if(dr_time()>last_ms+70u) {
		last_ms += 70u;
		line ++;
	}

	if (scrolltext[text_line + (L_BANNER_ROWS+1)*2] == 0) {
		line = 0;
	}

}
