/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simcolor.h"
#include "../simevent.h"
#include "../display/simimg.h"
#include "../world/simworld.h"
#include "../simskin.h"
#include "../sys/simsys.h"
#include "../simversion.h"
#include "../display/simgraph.h"
#include "../macros.h"
#include "../descriptor/skin_desc.h"
#include "../dataobj/environment.h"

#include "simwin.h"
#include "banner.h"
#include "loadsave_frame.h"
#include "scenario_frame.h"
#include "server_frame.h"
#include "components/gui_image.h"
#include "optionen.h"


// Local adjustments
#define L_BANNER_ROWS        ( 5 )                             // Rows of scroll text
#define L_BANNER_TEXT_INDENT ( 4 )                             // Scroll text padding (left/right)
#define L_BANNER_HEIGHT      ( L_BANNER_ROWS * LINESPACE + 2 ) // Banner control height in pixels

#ifdef _OPTIMIZED
	#define L_DEBUG_TEXT " (optimized)"
#elif defined DEBUG
#	define L_DEBUG_TEXT " (debug)"
#else
#	define L_DEBUG_TEXT
#endif

// colors
#define COLOR_RAMP_SIZE ( 5 ) // Number or fade colors + normal color at index 0

// Banner color ramp
// Index 0 is the normal text color
static const uint8 colors[COLOR_RAMP_SIZE] = { COL_WHITE, COL_GREY3, COL_GREY4, COL_GREY5, COL_GREY6 };

class banner_text_t : public gui_component_t
{
	sint32 last_ms;
	int line;
public:
	banner_text_t() : last_ms(dr_time() - 70), line(0) {}

	scr_size get_min_size() const OVERRIDE
	{
		return scr_size(0, L_BANNER_HEIGHT);
	}

	void draw(scr_coord offset) OVERRIDE;
};

banner_t::banner_t() : gui_frame_t("")
{
	set_table_layout(1,0);

	new_component<gui_label_t>("This is Simutrans" SIM_VERSION_BUILD_STRING, SYSCOL_TEXT_TITLE, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);

#ifdef REVISION
	new_component<gui_label_t>("Version " VERSION_NUMBER " " VERSION_DATE " r" QUOTEME(REVISION)
#else
	new_component<gui_label_t>("Version " VERSION_NUMBER " " VERSION_DATE
#endif
#ifdef GIT_HASH
		" hash " QUOTEME(GIT_HASH)
#endif
		L_DEBUG_TEXT, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);

	add_table(5,0);
	{
		new_component_span<gui_label_t>("The version is developed by", SYSCOL_TEXT_TITLE, gui_label_t::left, 5)->set_shadow(SYSCOL_TEXT_SHADOW, true);
		new_component<gui_fill_t>();

		add_table(1,0);
		new_component<gui_label_t>("the simutrans team", SYSCOL_TEXT_HIGHLIGHT, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);
		new_component<gui_label_t>("under the Artistic Licence", SYSCOL_TEXT_HIGHLIGHT, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);
		new_component<gui_label_t>("based on Simutrans 84.22.1", SYSCOL_TEXT_HIGHLIGHT, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);
		end_table();

		new_component<gui_fill_t>();
		new_component<gui_image_t>()->set_image(skinverwaltung_t::logosymbol->get_image_id(0), true);
		new_component<gui_fill_t>();
	}
	end_table();

	new_component<gui_label_t>("Selling of the program is forbidden.", color_idx_to_rgb(COL_ORANGE), gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);
	new_component<gui_label_t>("For questions and support please visit:", SYSCOL_TEXT_TITLE, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);

	add_table(3,0);
	{
		new_component<gui_fill_t>();
		add_table(1,0);
		new_component<gui_label_t>("https://www.simutrans.com", SYSCOL_TEXT_HIGHLIGHT, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);
		new_component<gui_label_t>("https://forum.simutrans.com", SYSCOL_TEXT_HIGHLIGHT, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);
		new_component<gui_label_t>("https://wiki.simutrans.com", SYSCOL_TEXT_HIGHLIGHT, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);
		end_table();
		new_component<gui_fill_t>();
	}

	end_table();

	// scrolling text ...
	new_component<banner_text_t>();

	add_table(3,2)->set_force_equal_columns(true);;
	{
		// New game button
		new_map.init( button_t::roundbox | button_t::flexible, "Neue Karte");
		new_map.add_listener( this );
		add_component( &new_map );

		// Load game button
		load_map.init( button_t::roundbox | button_t::flexible, "Load game");
		load_map.add_listener( this );
		add_component( &load_map );

		// Load scenario button
		load_scenario.init( button_t::roundbox | button_t::flexible, "Load scenario");
		load_scenario.add_listener( this );
		add_component( &load_scenario );

		// Options button
		options.init( button_t::roundbox | button_t::flexible, "Optionen");
		options.add_listener( this );
		add_component( &options );

		// Play online button
		join_map.init( button_t::roundbox | button_t::flexible, "join game");
		join_map.add_listener( this );
		add_component( &join_map );

		// Quit button
		quit.init( button_t::roundbox | button_t::flexible, "Beenden");
		quit.add_listener( this );
		add_component( &quit );
	}
	end_table();

	reset_min_windowsize();
}


bool banner_t::infowin_event(const event_t *ev)
{
	if(  gui_frame_t::is_hit( ev->cx, ev->cy  )  ) {
		gui_frame_t::infowin_event( ev );
	}
	return false;
}


bool banner_t::action_triggered( gui_action_creator_t *comp, value_t)
{
	if(  comp == &quit  ) {
		env_t::quit_simutrans = true;
		destroy_all_win(true);
	}
	else if(  comp == &new_map  ) {
		destroy_all_win(true);
	}
	else if(  comp == &load_map  ) {
		destroy_all_win(true);
		create_win( new loadsave_frame_t(true), w_info, magic_load_t);
	}
	else if(  comp == &load_scenario  ) {
		destroy_all_win(true);
		create_win( new scenario_frame_t(), w_info, magic_load_t );
	}
	else if(  comp == &options  ) {
		destroy_all_win(true);
		create_win( new optionen_gui_t(), w_info, magic_optionen_gui_t );
	}
	else if(  comp == &join_map  ) {
		destroy_all_win(true);
		create_win( new server_frame_t(), w_info, magic_server_frame_t );
	}
	return true;
}


void banner_text_t::draw(scr_coord offset)
{
	// now the scrolling
	//                    BANNER_ROWS defines size and how many rows of text are shown in banner
	//                    BANNER_TEXT_INDENT defines left and right padding inside banner area

	static const char* const scrolltext[] = {
		#include "../scrolltext.h"
	};

	scr_coord cursor(get_pos() + offset);
	const scr_coord_val text_line = (line / LINESPACE) * 2;
	const scr_coord_val text_offset = line % LINESPACE;
	const scr_coord_val left = cursor.x + D_MARGIN_LEFT;
	const scr_coord_val width = size.w - D_MARGIN_LEFT - D_MARGIN_RIGHT;
	const scr_coord_val height = size.h;


	PUSH_CLIP_FIT( left, cursor.y, width, height );

	display_fillbox_wh_clip_rgb(left, cursor.y,          width, height, color_idx_to_rgb(COL_GREY1), true);
	display_fillbox_wh_clip_rgb(left, cursor.y - 1,      width, 1,      color_idx_to_rgb(COL_GREY3), false);
	display_fillbox_wh_clip_rgb(left, cursor.y + height, width, 1,      color_idx_to_rgb(COL_GREY6), false);

	cursor.y++;

	for(  int row = 0;  row < L_BANNER_ROWS+1;  row++  ) {

		PIXVAL color;
		if(  row > L_BANNER_ROWS-COLOR_RAMP_SIZE+1  ) {
			color = color_idx_to_rgb(colors[L_BANNER_ROWS-row+1]);
		}
		else {
			color = color_idx_to_rgb(colors[0]);
		}

		display_proportional_clip_rgb( left + L_BANNER_TEXT_INDENT,         cursor.y - text_offset, scrolltext[text_line + row*2    ], ALIGN_LEFT,  color, false);
		display_proportional_clip_rgb( left + width - L_BANNER_TEXT_INDENT, cursor.y - text_offset, scrolltext[text_line + row*2 + 1], ALIGN_RIGHT, color, false);

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
