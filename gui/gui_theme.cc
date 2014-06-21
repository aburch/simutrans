/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/* this file has all functions for variable gui elements */


#include "gui_theme.h"
#include "../simskin.h"
#include "../simmenu.h"
#include "../simsys.h"
#include "../dataobj/environment.h"
#include "../dataobj/tabfile.h"
#include "components/gui_button.h"
#include "components/gui_tab_panel.h"
#include "../besch/skin_besch.h"
#include "../besch/reader/obj_reader.h"

/**
 * Colours
 */
COLOR_VAL gui_theme_t::gui_color_highlight;
COLOR_VAL gui_theme_t::gui_color_shadow;
COLOR_VAL gui_theme_t::gui_color_face;
COLOR_VAL gui_theme_t::gui_color_button_text;
COLOR_VAL gui_theme_t::gui_color_text;
COLOR_VAL gui_theme_t::gui_color_text_highlight;
COLOR_VAL gui_theme_t::gui_color_selected_text;
COLOR_VAL gui_theme_t::gui_color_selected_background;
COLOR_VAL gui_theme_t::gui_color_static_text;
COLOR_VAL gui_theme_t::gui_color_disabled_text;
COLOR_VAL gui_theme_t::gui_color_workarea;
COLOR_VAL gui_theme_t::gui_color_cursor;
COLOR_VAL gui_theme_t::button_color_text;
COLOR_VAL gui_theme_t::button_color_disabled_text;
COLOR_VAL gui_theme_t::button_color_focus;

/**
 * Max Kielland
 * These are the built in default theme element sizes and
 * are overridden by the PAK file if a new image is defined.
 */
scr_size gui_theme_t::gui_button_size;
scr_size gui_theme_t::gui_button_text_offset;
scr_size gui_theme_t::gui_color_button_size;
scr_size gui_theme_t::gui_color_button_text_offset;
scr_size gui_theme_t::gui_divider_size;
scr_size gui_theme_t::gui_checkbox_size;
scr_size gui_theme_t::gui_pos_button_size;
scr_size gui_theme_t::gui_arrow_left_size;
scr_size gui_theme_t::gui_arrow_right_size;
scr_size gui_theme_t::gui_arrow_up_size;
scr_size gui_theme_t::gui_arrow_down_size;
scr_size gui_theme_t::gui_scrollbar_size;
scr_size gui_theme_t::gui_min_scrollbar_size;
scr_size gui_theme_t::gui_label_size;
scr_size gui_theme_t::gui_edit_size;
scr_size gui_theme_t::gui_gadget_size;
scr_size gui_theme_t::gui_indicator_size;
scr_coord gui_theme_t::gui_focus_offset;

KOORD_VAL gui_theme_t::gui_titlebar_height;
KOORD_VAL gui_theme_t::gui_frame_left;
KOORD_VAL gui_theme_t::gui_frame_top;
KOORD_VAL gui_theme_t::gui_frame_right;
KOORD_VAL gui_theme_t::gui_frame_bottom;
KOORD_VAL gui_theme_t::gui_hspace;
KOORD_VAL gui_theme_t::gui_vspace;

/* those are the 3x3 images which are used for stretching
 * also 1x3 and 3x1 subsets are possible
 * first entry is the normal state
 * second entry is the selected state
 * third entry is disabled state
 * button has a fourth one, which is the mask for the background color blending
 */
stretch_map_t gui_theme_t::button_tiles[3];
stretch_map_t gui_theme_t::button_color_tiles[2];
stretch_map_t gui_theme_t::round_button_tiles[3];
stretch_map_t gui_theme_t::h_scroll_back_tiles;
stretch_map_t gui_theme_t::h_scroll_knob_tiles;
stretch_map_t gui_theme_t::v_scroll_back_tiles;
stretch_map_t gui_theme_t::v_scroll_knob_tiles;
stretch_map_t gui_theme_t::divider;
stretch_map_t gui_theme_t::editfield;
stretch_map_t gui_theme_t::listbox;
stretch_map_t gui_theme_t::windowback;

// and the simple buttons
image_id gui_theme_t::arrow_button_left_img[3];
image_id gui_theme_t::arrow_button_right_img[3];
image_id gui_theme_t::arrow_button_up_img[3];
image_id gui_theme_t::arrow_button_down_img[3];
image_id gui_theme_t::check_button_img[3];
image_id gui_theme_t::pos_button_img[3];

bool gui_theme_t::gui_drop_shadows;

/**
 * Initializes theme related parameters to hard coded default values.
 * @author  Max Kielland
 */
void gui_theme_t::init_gui_defaults()
{
	gui_color_highlight           = MN_GREY4;
	gui_color_shadow              = MN_GREY0;
	gui_color_face                = MN_GREY2;
	gui_color_button_text         = COL_BLACK;
	gui_color_text                = COL_WHITE;
	gui_color_text_highlight      = COL_WHITE;
	gui_color_selected_text       = COL_GREY5;
	gui_color_selected_background = COL_GREY2;
	gui_color_static_text         = COL_BLACK;
	gui_color_disabled_text       = MN_GREY4;
	gui_color_workarea            = MN_GREY1;
	gui_color_cursor              = COL_WHITE;
	button_color_text             = COL_BLACK;
	button_color_disabled_text    = MN_GREY0;
	button_color_focus            = COL_WHITE;

	gui_button_size        = scr_size(92,14);
	gui_color_button_size  = scr_size(92,16);
	gui_button_text_offset = scr_size(0,0);
	gui_color_button_text_offset = scr_size(0,0);
	gui_divider_size       = scr_size(92,2+D_V_SPACE*2);
	gui_checkbox_size      = scr_size(10,10);
	gui_pos_button_size    = scr_size(14,LINESPACE);
	gui_arrow_left_size    = scr_size(14,14);
	gui_arrow_right_size   = scr_size(14,14);
	gui_arrow_up_size      = scr_size(14,14);
	gui_arrow_down_size    = scr_size(14,14);
	gui_scrollbar_size     = scr_size(14,14);
	gui_min_scrollbar_size = scr_size(3,3);
	gui_label_size         = scr_size(92,LINESPACE);
	gui_edit_size          = scr_size(92,max(LINESPACE+2, max(D_ARROW_LEFT_HEIGHT, D_ARROW_RIGHT_HEIGHT) ));
	gui_gadget_size        = scr_size(16,16);
	gui_indicator_size     = scr_size(20,4);
	gui_focus_offset       = scr_coord(1,1);

	gui_titlebar_height  = 16;
	gui_frame_left       = 10;
	gui_frame_top        = 10;
	gui_frame_right      = 10;
	gui_frame_bottom     = 10;
	gui_hspace           = 4;
	gui_vspace           = 4;
	gui_divider_size.h   = D_V_SPACE*2;

	gui_drop_shadows     = false;
}


// helper for easier init
void gui_theme_t::init_size_from_bild( const bild_besch_t *pic, scr_size &k )
{
	if(  pic  ) {
		const bild_t *image = pic->get_pic();
		k = scr_size(image->x+image->w,image->y+image->h);
	}
}


/**
 * Lazy button image number init
 */
void gui_theme_t::init_gui_from_images()
{
	// Calculate posbutton size
	if(  skinverwaltung_t::divider == NULL  ) {
		// usually there should be a default theme which would provided missing images even for outdated themes
		dbg->fatal( "gui_theme_t::init_gui_themes", "Wrong theme loaded" );
	}

	init_size_from_bild( skinverwaltung_t::posbutton->get_bild( SKIN_BUTTON_POS ), gui_pos_button_size );
	init_size_from_bild( skinverwaltung_t::check_button->get_bild( SKIN_BUTTON_CHECKBOX ), gui_checkbox_size );
	for(  int i=0;  i<3;  i++  ) {
		pos_button_img[i] = skinverwaltung_t::posbutton->get_bild_nr( SKIN_BUTTON_POS+i );
		check_button_img[i] = skinverwaltung_t::check_button->get_bild_nr( SKIN_BUTTON_CHECKBOX+i );
	}

	// Normal buttons (colorful ones)
	scr_coord_val y = gui_button_size.h;
	scr_size k;
	init_size_from_bild( skinverwaltung_t::button->get_bild( SKIN_BUTTON_SIDE_LEFT ), k );
	y = max( y, k.h );
	init_size_from_bild( skinverwaltung_t::button->get_bild( SKIN_BUTTON_SIDE_RIGHT ), k );
	y = max( y, k.h );
	init_size_from_bild( skinverwaltung_t::button->get_bild( SKIN_BUTTON_BODY ), k );
	y = max( y, k.h );
	for(  int i=0;  i<3;  i++  ) {
		for(  int j=0;  j<9;  j++  ) {
			button_tiles[i][j%3][j/3] = skinverwaltung_t::button->get_bild_nr( i*9+j );
		}
	}
	image_id has_second_mask;
	for(  int i=0;  i<2;  i++  ) {
		has_second_mask = 0xFFFF;
		for(  int j=0;  j<9;  j++  ) {
			button_color_tiles[i][j%3][j/3] = skinverwaltung_t::button->get_bild_nr( i*9+j+27 );
			has_second_mask &= button_color_tiles[i][j%3][j/3];
		}
	}
	if(  has_second_mask == 0xFFFF  ) {
		// has no second mask => copy first
		for(  int j=0;  j<9;  j++  ) {
			button_color_tiles[1][j%3][j/3] = button_color_tiles[0][j%3][j/3];
		}
	}

	// Round buttons
	for(  int i=0;  i<3;  i++  ) {
		for(  int j=0;  j<9;  j++  ) {
			round_button_tiles[i][j%3][j/3] = skinverwaltung_t::round_button->get_bild_nr( i*9+j );
		}
	}

	// background for editfields, listbuttons, and windows
	for(  int j=0;  j<9;  j++  ) {
		editfield[j%3][j/3] = skinverwaltung_t::editfield->get_bild_nr( j );
		listbox[j%3][j/3] = skinverwaltung_t::listbox->get_bild_nr( j );
		windowback[j%3][j/3] = skinverwaltung_t::back->get_bild_nr( j );
	}

	// Divider (vspace will be added later on)
	init_size_from_bild( skinverwaltung_t::divider->get_bild(1), gui_divider_size );
	for(  int i=0;  i<3;  i++  ) {
		divider[i][0] = skinverwaltung_t::divider->get_bild_nr( i );
		divider[i][1] = IMG_LEER;
		divider[i][2] = IMG_LEER;
	}

	// Calculate arrow size
	init_size_from_bild( skinverwaltung_t::scrollbar->get_bild( SKIN_BUTTON_ARROW_LEFT ), gui_arrow_left_size );
	init_size_from_bild( skinverwaltung_t::scrollbar->get_bild( SKIN_BUTTON_ARROW_RIGHT ), gui_arrow_right_size );
	init_size_from_bild( skinverwaltung_t::scrollbar->get_bild( SKIN_BUTTON_ARROW_UP ), gui_arrow_up_size );
	init_size_from_bild( skinverwaltung_t::scrollbar->get_bild( SKIN_BUTTON_ARROW_DOWN ), gui_arrow_down_size );
	for(  int i=0;  i<3;  i++  ) {
		arrow_button_left_img[i] = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_BUTTON_ARROW_LEFT+i );
		arrow_button_right_img[i] = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_BUTTON_ARROW_RIGHT+i );
		arrow_button_up_img[i] = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_BUTTON_ARROW_UP+i );
		arrow_button_down_img[i] = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_BUTTON_ARROW_DOWN+i );
	}
	if(  gui_theme_t::gui_arrow_right_size != gui_theme_t::gui_arrow_left_size  ) {
		dbg->warning( "gui_theme_t::themes_init()", "Size of left and right arrows differ" );
	}
	if(  gui_theme_t::gui_arrow_up_size != gui_theme_t::gui_arrow_down_size  ) {
		dbg->warning( "gui_theme_t::themes_init()", "Size of up and down arrows differ" );
	}

	// now init this button dependent size here too
	gui_edit_size = scr_size(92,max(LINESPACE+2, max(D_ARROW_LEFT_HEIGHT, D_ARROW_RIGHT_HEIGHT) ));

	// init horizontal scrollbar buttons
	for(  int i=0;  i<3;  i++  ) {
		h_scroll_back_tiles[i][0] = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_SCROLLBAR_H_BACKGROUND_LEFT+i );
		h_scroll_back_tiles[i][1] = IMG_LEER;
		h_scroll_back_tiles[i][2] = IMG_LEER;
		h_scroll_knob_tiles[i][0] = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_SCROLLBAR_H_KNOB_LEFT+i );
		h_scroll_knob_tiles[i][1] = IMG_LEER;
		h_scroll_knob_tiles[i][2] = IMG_LEER;
	}

	// init vertical scrollbar buttons
	for(  int i=0;  i<3;  i++  ) {
		v_scroll_back_tiles[0][i] = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_SCROLLBAR_V_BACKGROUND_TOP+i );
		v_scroll_back_tiles[1][i] = IMG_LEER;
		v_scroll_back_tiles[2][i] = IMG_LEER;
		v_scroll_knob_tiles[0][i] = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_SCROLLBAR_V_KNOB_TOP+i );
		v_scroll_knob_tiles[1][i] = IMG_LEER;
		v_scroll_knob_tiles[2][i] = IMG_LEER;
	}

	// Calculate V scrollbar size
	{
		scr_size back, front;
		init_size_from_bild( skinverwaltung_t::scrollbar->get_bild( SKIN_SCROLLBAR_V_BACKGROUND ), back );
		init_size_from_bild( skinverwaltung_t::scrollbar->get_bild( SKIN_SCROLLBAR_V_KNOB_BODY ), front );
		gui_scrollbar_size.w = max(front.w, back.w);

		// Calculate H scrollbar size
		init_size_from_bild( skinverwaltung_t::scrollbar->get_bild( SKIN_SCROLLBAR_H_BACKGROUND ), back );
		init_size_from_bild( skinverwaltung_t::scrollbar->get_bild( SKIN_SCROLLBAR_H_KNOB_BODY ), front );
		gui_scrollbar_size.h = max(front.h, back.h);

		// calculate minimum width
		init_size_from_bild( skinverwaltung_t::scrollbar->get_bild( SKIN_SCROLLBAR_H_KNOB_LEFT ), back );
		init_size_from_bild( skinverwaltung_t::scrollbar->get_bild( SKIN_SCROLLBAR_H_KNOB_RIGHT ), front );
		gui_min_scrollbar_size.w = back.w + front.w;

		// calculate minimum height
		init_size_from_bild( skinverwaltung_t::scrollbar->get_bild( SKIN_SCROLLBAR_V_KNOB_TOP ), back );
		init_size_from_bild( skinverwaltung_t::scrollbar->get_bild( SKIN_SCROLLBAR_V_KNOB_BOTTOM ), front );
		gui_min_scrollbar_size.h = back.h + front.h;
	}

	// gadgets
	init_size_from_bild( skinverwaltung_t::gadget->get_bild( SKIN_GADGET_CLOSE ), gui_gadget_size );
}


/**
 * Reads theme configuration data, still not final
 * @author prissi
 *
 * Max Kielland:
 * Note, there will be a theme manager later on and
 * each gui object will find their own parameters by
 * themselves after registering its class to the theme
 * manager. This will be done as the last step in
 * the chain when loading a theme.
 */
bool gui_theme_t::themes_init(const char *file_name)
{
	tabfile_t themesconf;

	// first take user data, then user global data
	if(  !themesconf.open(file_name)  ) {
		dbg->warning("simwin.cc themes_init()", "Can't read themes from %s", file_name );
		return false;
	}

	// define a default even when stuff is missing from the table
	init_gui_defaults();

	tabfileobj_t contents;
	themesconf.read(contents);

	// theme name to find out current theme
	std::string theme_name = contents.get( "name" );

	// first get the images ( to be able to overload default sizes)
	const std::string buttonpak = contents.get("themeimages");
	if(  buttonpak.length()>0  ) {
		std::string path;
		char *pathname = strdup(file_name);
		if(  char *s = strrchr( pathname, '/' )  ) {
			*s = 0;
		}
		else if(  char *s = strrchr( pathname, '\\' )  ) {
			*s = 0;
		}
		chdir( pathname );
		obj_reader_t::read_file(buttonpak.c_str());
		gui_theme_t::init_gui_from_images();
		free(pathname);
	}

	// first the stuff for the dialogues
	gui_theme_t::gui_titlebar_height = (uint32)contents.get_int("gui_titlebar_height", gui_theme_t::gui_titlebar_height );
	gui_theme_t::gui_frame_left =      (uint32)contents.get_int("gui_frame_left",      gui_theme_t::gui_frame_left );
	gui_theme_t::gui_frame_top =       (uint32)contents.get_int("gui_frame_top",       gui_theme_t::gui_frame_top );
	gui_theme_t::gui_frame_right =     (uint32)contents.get_int("gui_frame_right",     gui_theme_t::gui_frame_right );
	gui_theme_t::gui_frame_bottom =    (uint32)contents.get_int("gui_frame_bottom",    gui_theme_t::gui_frame_bottom );
	gui_theme_t::gui_hspace =          (uint32)contents.get_int("gui_hspace",          gui_theme_t::gui_hspace );
	gui_theme_t::gui_vspace =          (uint32)contents.get_int("gui_vspace",          gui_theme_t::gui_vspace );

	// the divider needs the vspace added to it for know
	gui_divider_size.h += gui_vspace*2;
	gui_theme_t::gui_divider_size.h = contents.get_int("gui_divider_vsize",  gui_theme_t::gui_divider_size.h );

	gui_theme_t::gui_button_size.w = (uint32)contents.get_int("gui_button_width",  gui_theme_t::gui_button_size.w );
	gui_theme_t::gui_button_size.h = (uint32)contents.get_int("gui_button_height", gui_theme_t::gui_button_size.h );
	gui_theme_t::gui_edit_size.h = (uint32)contents.get_int("gui_edit_height", gui_theme_t::gui_edit_size.h );

	// make them fit at least the font height
	gui_theme_t::gui_titlebar_height = max( LINESPACE+2, gui_theme_t::gui_titlebar_height );
	gui_theme_t::gui_button_size.h = max( LINESPACE+2, gui_theme_t::gui_button_size.h );
	gui_theme_t::gui_edit_size.h = max( LINESPACE+2, gui_theme_t::gui_edit_size.h );


	// since the arrows are used in scrollbars, the need similar sizes
	gui_theme_t::gui_arrow_left_size.w = (uint32)contents.get_int("gui_horizontal_arrow_width",  gui_theme_t::gui_arrow_left_size.w );
	gui_theme_t::gui_arrow_left_size.h = (uint32)contents.get_int("gui_horizontal_arrow_height", gui_theme_t::gui_arrow_left_size.h );
	gui_theme_t::gui_arrow_right_size = gui_theme_t::gui_arrow_left_size;

	gui_theme_t::gui_arrow_up_size.w = (uint32)contents.get_int("gui_vertical_arrow_width",  gui_theme_t::gui_arrow_up_size.w );
	gui_theme_t::gui_arrow_up_size.h = (uint32)contents.get_int("gui_vertical_arrow_height", gui_theme_t::gui_arrow_up_size.h );
	gui_theme_t::gui_arrow_down_size = gui_theme_t::gui_arrow_up_size;

	// since scrollbar must have a certain size
	gui_theme_t::gui_scrollbar_size.w = max( gui_min_scrollbar_size.w, (uint32)contents.get_int("gui_scrollbar_width",  gui_theme_t::gui_scrollbar_size.w ) );
	gui_theme_t::gui_scrollbar_size.h = max( gui_min_scrollbar_size.h, (uint32)contents.get_int("gui_scrollbar_height", gui_theme_t::gui_scrollbar_size.h ) );

	// in practice, posbutton min height beeter is LINESPACE
	gui_theme_t::gui_pos_button_size.w = (uint32)contents.get_int("gui_posbutton_width",  gui_theme_t::gui_pos_button_size.w );
	gui_theme_t::gui_pos_button_size.h = (uint32)contents.get_int("gui_posbutton_height", gui_theme_t::gui_pos_button_size.h );

	gui_theme_t::button_color_text = (uint32)contents.get_color("gui_button_color_text", gui_theme_t::button_color_text );
	gui_theme_t::button_color_disabled_text = (uint32)contents.get_color("gui_button_color_disabled_text", gui_theme_t::button_color_disabled_text );
	gui_theme_t::gui_color_button_text_offset = contents.get_scr_size("gui_color_button_text_offset", gui_theme_t::gui_color_button_text_offset );
	gui_theme_t::gui_button_text_offset = contents.get_scr_size("gui_button_text_offset", gui_theme_t::gui_button_text_offset );

	// default iconsize (square for now)
	env_t::iconsize.h = env_t::iconsize.w = contents.get_int("icon_width",env_t::iconsize.w );

	// maybe not the best place, rather use simwin for the static defines?
	gui_theme_t::gui_color_text =          (COLOR_VAL)contents.get_color("gui_text_color",          SYSCOL_TEXT);
	gui_theme_t::gui_color_text_highlight =(COLOR_VAL)contents.get_color("gui_text_highlight",      SYSCOL_TEXT_HIGHLIGHT);
	gui_theme_t::gui_color_static_text =   (COLOR_VAL)contents.get_color("gui_static_text_color",   SYSCOL_STATIC_TEXT);
	gui_theme_t::gui_color_disabled_text = (COLOR_VAL)contents.get_color("gui_disabled_text_color", SYSCOL_DISABLED_TEXT);
	gui_theme_t::gui_color_highlight =     (COLOR_VAL)contents.get_color("gui_highlight_color",     SYSCOL_HIGHLIGHT);
	gui_theme_t::gui_color_shadow =        (COLOR_VAL)contents.get_color("gui_shadow_color",        SYSCOL_SHADOW);
	gui_theme_t::gui_color_face =          (COLOR_VAL)contents.get_color("gui_face_color",          SYSCOL_FACE);
	gui_theme_t::gui_color_button_text =   (COLOR_VAL)contents.get_color("gui_button_text_color",   SYSCOL_BUTTON_TEXT);

	// those two may be rather an own control later on?
	gui_theme_t::gui_indicator_size = contents.get_scr_size("gui_indicator_size",  gui_theme_t::gui_indicator_size );

	gui_tab_panel_t::header_vsize = (uint32)contents.get_int("gui_tab_header_vsize", gui_tab_panel_t::header_vsize );

	// stuff in env_t but rather GUI
	env_t::window_buttons_right =      contents.get_int("window_buttons_right",      env_t::window_buttons_right );
	env_t::left_to_right_graphs =      contents.get_int("left_to_right_graphs",      env_t::left_to_right_graphs );
	env_t::window_frame_active =       contents.get_int("window_frame_active",       env_t::window_frame_active );
	env_t::second_open_closes_win =    contents.get_int("second_open_closes_win",    env_t::second_open_closes_win );
	env_t::remember_window_positions = contents.get_int("remember_window_positions", env_t::remember_window_positions );
	env_t::window_snap_distance =      contents.get_int("window_snap_distance",      env_t::window_snap_distance );
	gui_theme_t::gui_drop_shadows =    contents.get_int("gui_drop_shadows",          gui_theme_t::gui_drop_shadows );

	env_t::front_window_bar_color =   contents.get_color("front_window_bar_color",   env_t::front_window_bar_color );
	env_t::front_window_text_color =  contents.get_color("front_window_text_color",  env_t::front_window_text_color );
	env_t::bottom_window_bar_color =  contents.get_color("bottom_window_bar_color",  env_t::bottom_window_bar_color );
	env_t::bottom_window_text_color = contents.get_color("bottom_window_text_color", env_t::bottom_window_text_color );

	env_t::show_tooltips =        contents.get_int("show_tooltips",              env_t::show_tooltips );
	env_t::tooltip_color =        contents.get_color("tooltip_background_color", env_t::tooltip_color );
	env_t::tooltip_textcolor =    contents.get_color("tooltip_text_color",       env_t::tooltip_textcolor );
	env_t::tooltip_delay =        contents.get_int("tooltip_delay",              env_t::tooltip_delay );
	env_t::tooltip_duration =     contents.get_int("tooltip_duration",           env_t::tooltip_duration );
	env_t::toolbar_max_width =    contents.get_int("toolbar_max_width",          env_t::toolbar_max_width );
	env_t::toolbar_max_height =   contents.get_int("toolbar_max_height",         env_t::toolbar_max_height );
	env_t::cursor_overlay_color = contents.get_color("cursor_overlay_color",     env_t::cursor_overlay_color );

	werkzeug_t::update_toolbars();
	env_t::default_theme = theme_name.c_str();

	return true;
}
