/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/* this file has all functions for variable gui elements */


#include "gui_theme.h"
#include "../simskin.h"
#include "../simsys.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/tabfile.h"
#include "components/gui_button.h"
#include "components/gui_tab_panel.h"
#include "../besch/skin_besch.h"
#include "../besch/reader/obj_reader.h"

 // colours
COLOR_VAL gui_theme_t::theme_color_highlight           = MN_GREY4;
COLOR_VAL gui_theme_t::theme_color_shadow              = MN_GREY0;
COLOR_VAL gui_theme_t::theme_color_face                = MN_GREY2;
COLOR_VAL gui_theme_t::theme_color_button_text         = COL_BLACK;
COLOR_VAL gui_theme_t::theme_color_text                = COL_BLACK;
COLOR_VAL gui_theme_t::theme_color_text_highlite       = COL_WHITE;
COLOR_VAL gui_theme_t::theme_color_selected_text       = COL_WHITE;
COLOR_VAL gui_theme_t::theme_color_selected_background = COL_BLUE;
COLOR_VAL gui_theme_t::theme_color_static_text         = COL_BLACK;
COLOR_VAL gui_theme_t::theme_color_disabled_text       = MN_GREY4;

COLOR_VAL gui_theme_t::button_color_text = COL_BLACK;
COLOR_VAL gui_theme_t::button_color_disabled_text = MN_GREY0;

/**
 * Max Kielland
 * These are the built in default theme element sizes and
 * are overridden by the PAK file if a new image is defined.
 */
koord gui_theme_t::gui_button_size        = koord(92,14);
koord gui_theme_t::gui_checkbox_size      = koord(10,10);
koord gui_theme_t::gui_posbutton_size     = koord(14,14);
koord gui_theme_t::gui_arrow_left_size    = koord(14,14);
koord gui_theme_t::gui_arrow_right_size   = koord(14,14);
koord gui_theme_t::gui_arrow_up_size      = koord(14,14);
koord gui_theme_t::gui_arrow_down_size    = koord(14,14);
koord gui_theme_t::gui_scrollbar_size     = koord(14,14);
koord gui_theme_t::gui_scrollknob_size    = koord(14,14);
koord gui_theme_t::gui_indicator_box_size = koord(10, 6);

// default titlebar height
KOORD_VAL gui_theme_t::gui_titlebar_height = 16;

// Max Kielland: default gadget size
KOORD_VAL gui_theme_t::gui_gadget_size = 16;

// dialog borders
KOORD_VAL gui_theme_t::gui_frame_left = 10;
KOORD_VAL gui_theme_t::gui_frame_top = 10;
KOORD_VAL gui_theme_t::gui_frame_right = 10;
KOORD_VAL gui_theme_t::gui_frame_bottom = 10;

// space between two elements
KOORD_VAL gui_theme_t::gui_hspace = 4;
KOORD_VAL gui_theme_t::gui_vspace = 4;

// size of status indicator elements (colored boxes in factories, station and others)
KOORD_VAL gui_theme_t::gui_indicator_width = 20;
KOORD_VAL gui_theme_t::gui_indicator_height = 4;

KOORD_VAL gui_theme_t::gui_divider_height = D_V_SPACE*2;



/**
 * Lazy button image number init
 * @author Hj. Malthaner
 */
void gui_theme_t::init_gui_images()
{
	const bild_t *image;

	if(  skinverwaltung_t::squarebutton  ) {
		// Calculate checkbox size
		button_t::square_button_normal    = skinverwaltung_t::squarebutton->get_bild_nr( SKIN_BUTTON_CHECKBOX );
		button_t::square_button_pushed    = skinverwaltung_t::squarebutton->get_bild_nr( SKIN_BUTTON_CHECKBOX_PRESSED );
		image = skinverwaltung_t::squarebutton->get_bild( SKIN_BUTTON_CHECKBOX )->get_pic();
		gui_checkbox_size = koord(image->w,image->h);
	}

	if(  skinverwaltung_t::posbutton  ) {
		// Calculate posbutton size
		button_t::pos_button_normal    = skinverwaltung_t::posbutton->get_bild_nr( SKIN_BUTTON_POS );
		button_t::pos_button_pushed    = skinverwaltung_t::posbutton->get_bild_nr( SKIN_BUTTON_POS_PRESSED );
		image = skinverwaltung_t::posbutton->get_bild( SKIN_BUTTON_CHECKBOX )->get_pic();
		gui_posbutton_size = koord(image->w,image->h);
	}

	if(  skinverwaltung_t::button  ) {
		// Normal buttons
		button_t::b_cap_left              = skinverwaltung_t::button->get_bild_nr( SKIN_BUTTON_SIDE_LEFT  );
		button_t::b_cap_right             = skinverwaltung_t::button->get_bild_nr( SKIN_BUTTON_SIDE_RIGHT );
		button_t::b_body                  = skinverwaltung_t::button->get_bild_nr( SKIN_BUTTON_BODY       );
		button_t::b_cap_left_p            = skinverwaltung_t::button->get_bild_nr( SKIN_BUTTON_SIDE_LEFT_PRESSED  );
		button_t::b_cap_right_p           = skinverwaltung_t::button->get_bild_nr( SKIN_BUTTON_SIDE_RIGHT_PRESSED );
		button_t::b_body_p                = skinverwaltung_t::button->get_bild_nr( SKIN_BUTTON_BODY_PRESSED       );
		// Calculate round button size
		if(  (button_t::b_cap_left != IMG_LEER)  &&  (button_t::b_cap_right != IMG_LEER)  &&  (button_t::b_body != IMG_LEER)  ) {

			// calculate button width
			scr_coord_val button_width = skinverwaltung_t::button->get_bild( SKIN_BUTTON_SIDE_LEFT  )->get_pic()->w +
											skinverwaltung_t::button->get_bild( SKIN_BUTTON_SIDE_RIGHT )->get_pic()->w +
											skinverwaltung_t::button->get_bild( SKIN_BUTTON_BODY       )->get_pic()->w;
			gui_button_size.x = max(gui_button_size.x,button_width);

			// calculate button height
			// gui_button_size.y = max(gui_button_size.y, skinverwaltung_t::window_skin->get_bild(SKIN_BUTTON_BODY)->get_pic()->h);
			gui_button_size.y = max(gui_button_size.y, skinverwaltung_t::button->get_bild(SKIN_BUTTON_BODY)->get_pic()->h);
		}
	}

	if(  skinverwaltung_t::scrollbar  ) {
		// scrollbars
		button_t::arrow_left_normal       = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_BUTTON_ARROW_LEFT             );
		button_t::arrow_left_pushed       = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_BUTTON_ARROW_LEFT_PRESSED     );
		button_t::arrow_right_normal      = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_BUTTON_ARROW_RIGHT            );
		button_t::arrow_right_pushed      = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_BUTTON_ARROW_RIGHT_PRESSED    );
		button_t::scrollbar_left          = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_SCROLLBAR_H_BACKGROUND_LEFT   );
		button_t::scrollbar_right         = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_SCROLLBAR_H_BACKGROUND_RIGHT  );
		button_t::scrollbar_middle        = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_SCROLLBAR_H_BACKGROUND        );
		button_t::scrollbar_slider_left   = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_SCROLLBAR_H_KNOB_LEFT         );
		button_t::scrollbar_slider_right  = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_SCROLLBAR_H_KNOB_RIGHT        );
		button_t::scrollbar_slider_middle = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_SCROLLBAR_H_KNOB_BODY         );
		button_t::arrow_up_normal         = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_BUTTON_ARROW_UP               );
		button_t::arrow_up_pushed         = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_BUTTON_ARROW_UP_PRESSED       );
		button_t::arrow_down_normal       = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_BUTTON_ARROW_DOWN             );
		button_t::arrow_down_pushed       = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_BUTTON_ARROW_DOWN_PRESSED     );
		button_t::scrollbar_top           = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_SCROLLBAR_V_BACKGROUND_TOP    );
		button_t::scrollbar_bottom        = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_SCROLLBAR_V_BACKGROUND_BOTTOM );
		button_t::scrollbar_center        = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_SCROLLBAR_V_BACKGROUND        );
		button_t::scrollbar_slider_top    = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_SCROLLBAR_V_KNOB_TOP          );
		button_t::scrollbar_slider_bottom = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_SCROLLBAR_V_KNOB_BOTTOM       );
		button_t::scrollbar_slider_center = skinverwaltung_t::scrollbar->get_bild_nr( SKIN_SCROLLBAR_V_KNOB_BODY         );

		// Calculate arrow left size
		if(  button_t::arrow_left_normal != IMG_LEER  ) {
			image = skinverwaltung_t::scrollbar->get_bild( SKIN_BUTTON_ARROW_LEFT )->get_pic();
			gui_arrow_left_size = koord(image->w,image->h);
		}

		// Calculate arrow right size
		if(  button_t::pos_button_normal != IMG_LEER  ) {
			image = skinverwaltung_t::scrollbar->get_bild( SKIN_BUTTON_ARROW_RIGHT )->get_pic();
			gui_arrow_right_size = koord(image->w,image->h);
		}

		// Calculate arrow up size
		if(  button_t::arrow_up_normal != IMG_LEER  ) {
			image = skinverwaltung_t::scrollbar->get_bild( SKIN_BUTTON_ARROW_UP )->get_pic();
			gui_arrow_up_size = koord(image->w,image->h);
		}

		// Calculate arrow down size
		if(  button_t::arrow_down_normal != IMG_LEER  ) {
			image = skinverwaltung_t::scrollbar->get_bild( SKIN_BUTTON_ARROW_DOWN )->get_pic();
			gui_arrow_down_size = koord(image->w,image->h);
		}

		// Calculate V scrollbar size
		image = NULL;
		if(  button_t::scrollbar_center != IMG_LEER  ) {
			image = skinverwaltung_t::scrollbar->get_bild( SKIN_SCROLLBAR_V_BACKGROUND )->get_pic();
		}
		else if(  button_t::scrollbar_slider_center != IMG_LEER  ) {
			image = skinverwaltung_t::scrollbar->get_bild( SKIN_SCROLLBAR_V_KNOB_BODY )->get_pic();
		}
		gui_scrollbar_size.x = (image) ? image->w : gui_arrow_up_size.x;

		if(  button_t::scrollbar_slider_center != IMG_LEER  ) {
			image = skinverwaltung_t::scrollbar->get_bild( SKIN_SCROLLBAR_V_KNOB_BODY )->get_pic();
			gui_scrollknob_size.x = image->w;
		}
		else {
			gui_scrollknob_size.x = gui_scrollbar_size.x;
		}

		// Calculate H scrollbar size
		image = NULL;
		if(  button_t::scrollbar_middle != IMG_LEER  ) {
			image = skinverwaltung_t::scrollbar->get_bild( SKIN_SCROLLBAR_H_BACKGROUND )->get_pic();
		}
		else if(  button_t::scrollbar_slider_middle != IMG_LEER  ) {
			image = skinverwaltung_t::scrollbar->get_bild( SKIN_SCROLLBAR_H_KNOB_BODY )->get_pic();
		}
		gui_scrollbar_size.y = (image) ? image->h : gui_arrow_left_size.y;

		if ( button_t::scrollbar_slider_middle != IMG_LEER ) {
			image = skinverwaltung_t::scrollbar->get_bild( SKIN_SCROLLBAR_H_KNOB_BODY )->get_pic();
			gui_scrollknob_size.y = image->h;
		}
		else {
			gui_scrollknob_size.y = gui_scrollbar_size.y;
		}
	}
}


static std::string theme_name;

const char *gui_theme_t::get_current_theme()
{
	return theme_name.c_str();
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

	tabfileobj_t contents;
	themesconf.read(contents);

	// theme name to find out current theme
	theme_name = contents.get( "name" );

	// first the stuff for the dialogues
	gui_theme_t::gui_titlebar_height = (uint32)contents.get_int("gui_titlebar_height", gui_theme_t::gui_titlebar_height );
	gui_theme_t::gui_frame_left =      (uint32)contents.get_int("gui_frame_left",      gui_theme_t::gui_frame_left );
	gui_theme_t::gui_frame_top =       (uint32)contents.get_int("gui_frame_top",       gui_theme_t::gui_frame_top );
	gui_theme_t::gui_frame_right =     (uint32)contents.get_int("gui_frame_right",     gui_theme_t::gui_frame_right );
	gui_theme_t::gui_frame_bottom =    (uint32)contents.get_int("gui_frame_bottom",    gui_theme_t::gui_frame_bottom );
	gui_theme_t::gui_hspace =          (uint32)contents.get_int("gui_hspace",          gui_theme_t::gui_hspace );
	gui_theme_t::gui_vspace =          (uint32)contents.get_int("gui_vspace",          gui_theme_t::gui_vspace );

	// those two will be anyway set whenever the buttons are reinitialized
	// Max Kielland: This has been moved to button_t
	gui_theme_t::gui_button_size.x = (uint32)contents.get_int("gui_button_width",  gui_theme_t::gui_button_size.x );
	gui_theme_t::gui_button_size.y = (uint32)contents.get_int("gui_button_height", gui_theme_t::gui_button_size.y );

	gui_theme_t::button_color_text = (uint32)contents.get_color("button_color_text", gui_theme_t::button_color_text );
	gui_theme_t::button_color_disabled_text = (uint32)contents.get_color("button_color_disabled_text", gui_theme_t::button_color_disabled_text );

	// maybe not the best place, rather use simwin for the static defines?
	gui_theme_t::theme_color_text =          (COLOR_VAL)contents.get_color("gui_text_color",          SYSCOL_TEXT);
	gui_theme_t::theme_color_text_highlite = (COLOR_VAL)contents.get_color("gui_text_highlite",       SYSCOL_TEXT_HIGHLITE);
	gui_theme_t::theme_color_static_text =   (COLOR_VAL)contents.get_color("gui_static_text_color",   SYSCOL_STATIC_TEXT);
	gui_theme_t::theme_color_disabled_text = (COLOR_VAL)contents.get_color("gui_disabled_text_color", SYSCOL_DISABLED_TEXT);
	gui_theme_t::theme_color_highlight =     (COLOR_VAL)contents.get_color("gui_highlight_color",     SYSCOL_HIGHLIGHT);
	gui_theme_t::theme_color_shadow =        (COLOR_VAL)contents.get_color("gui_shadow_color",        SYSCOL_SHADOW);
	gui_theme_t::theme_color_face =          (COLOR_VAL)contents.get_color("gui_face_color",          SYSCOL_FACE);
	gui_theme_t::theme_color_button_text =   (COLOR_VAL)contents.get_color("gui_button_text_color",   SYSCOL_BUTTON_TEXT);

	// those two may be rather an own control later on?
	gui_theme_t::gui_indicator_width =  (uint32)contents.get_int("gui_indicator_width",  gui_theme_t::gui_indicator_width );
	gui_theme_t::gui_indicator_height = (uint32)contents.get_int("gui_indicator_height", gui_theme_t::gui_indicator_height );

	// other gui parameter
	// Max Kielland: Scrollbar size is set by the arrow size in button_t
	//scrollbar_t::BAR_SIZE = (uint32)contents.get_int("gui_scrollbar_width", scrollbar_t::BAR_SIZE );
	gui_tab_panel_t::header_vsize = (uint32)contents.get_int("gui_tab_header_vsize", gui_tab_panel_t::header_vsize );

	// stuff in umgebung_t but rather GUI
	umgebung_t::window_buttons_right =      contents.get_int("window_buttons_right",      umgebung_t::window_buttons_right );
	umgebung_t::left_to_right_graphs =      contents.get_int("left_to_right_graphs",      umgebung_t::left_to_right_graphs );
	umgebung_t::window_frame_active =       contents.get_int("window_frame_active",       umgebung_t::window_frame_active );
	umgebung_t::second_open_closes_win =    contents.get_int("second_open_closes_win",    umgebung_t::second_open_closes_win );
	umgebung_t::remember_window_positions = contents.get_int("remember_window_positions", umgebung_t::remember_window_positions );
	umgebung_t::window_snap_distance = contents.get_int("window_snap_distance", umgebung_t::window_snap_distance );

	umgebung_t::front_window_bar_color =   contents.get_color("front_window_bar_color",   umgebung_t::front_window_bar_color );
	umgebung_t::front_window_text_color =  contents.get_color("front_window_text_color",  umgebung_t::front_window_text_color );
	umgebung_t::bottom_window_bar_color =  contents.get_color("bottom_window_bar_color",  umgebung_t::bottom_window_bar_color );
	umgebung_t::bottom_window_text_color = contents.get_color("bottom_window_text_color", umgebung_t::bottom_window_text_color );

	umgebung_t::show_tooltips =        contents.get_int("show_tooltips",              umgebung_t::show_tooltips );
	umgebung_t::tooltip_color =        contents.get_color("tooltip_background_color", umgebung_t::tooltip_color );
	umgebung_t::tooltip_textcolor =    contents.get_color("tooltip_text_color",       umgebung_t::tooltip_textcolor );
	umgebung_t::tooltip_delay =        contents.get_int("tooltip_delay",              umgebung_t::tooltip_delay );
	umgebung_t::tooltip_duration =     contents.get_int("tooltip_duration",           umgebung_t::tooltip_duration );
	umgebung_t::toolbar_max_width =    contents.get_int("toolbar_max_width",          umgebung_t::toolbar_max_width );
	umgebung_t::toolbar_max_height =   contents.get_int("toolbar_max_height",         umgebung_t::toolbar_max_height );
	umgebung_t::cursor_overlay_color = contents.get_color("cursor_overlay_color",     umgebung_t::cursor_overlay_color );

	const std::string buttonpak = contents.get("themeimages");
	if(  buttonpak.length()>0  ) {
		std::string path;
		if(  char *s = (char *)strrchr( file_name, '/' )  ) {
			*s = 0;
			chdir( file_name );
		}
		else if(  char *s = (char *)strrchr( file_name, '\\' )  ) {
			*s = 0;
			chdir( file_name );
		}
		obj_reader_t::read_file(buttonpak.c_str());
		gui_theme_t::init_gui_images();
	}

	// parsing buttons still needs to be done after agreement what to load
	return false; //hence we return false for now ...
}
