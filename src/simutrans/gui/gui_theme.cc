/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/* this file has all functions for variable gui elements */


#include "gui_theme.h"
#include "../world/simworld.h"
#include "../simskin.h"
#include "../tool/simmenu.h"
#include "../sys/simsys.h"
#include "../dataobj/environment.h"
#include "../dataobj/tabfile.h"
#include "components/gui_button.h"
#include "components/gui_tab_panel.h"
#include "../descriptor/skin_desc.h"
#include "../dataobj/pakset_manager.h"


/**
 * Colours
 */
PIXVAL gui_theme_t::gui_color_text;
PIXVAL gui_theme_t::gui_color_text_highlight;
PIXVAL gui_theme_t::gui_color_text_shadow;
PIXVAL gui_theme_t::gui_color_text_title;
PIXVAL gui_theme_t::gui_color_text_strong;
PIXVAL gui_theme_t::gui_color_text_minus;
PIXVAL gui_theme_t::gui_color_text_plus;
PIXVAL gui_theme_t::gui_color_text_unused;
PIXVAL gui_theme_t::gui_color_edit_text;
PIXVAL gui_theme_t::gui_color_edit_text_selected;
PIXVAL gui_theme_t::gui_color_edit_text_disabled;
PIXVAL gui_theme_t::gui_color_edit_background_selected;
PIXVAL gui_theme_t::gui_color_edit_beam;
PIXVAL gui_theme_t::gui_color_chart_background;
PIXVAL gui_theme_t::gui_color_chart_lines_zero;
PIXVAL gui_theme_t::gui_color_chart_lines_odd;
PIXVAL gui_theme_t::gui_color_chart_lines_even;
PIXVAL gui_theme_t::gui_color_list_text_selected_focus;
PIXVAL gui_theme_t::gui_color_list_text_selected_nofocus;
PIXVAL gui_theme_t::gui_color_list_background_selected_f;
PIXVAL gui_theme_t::gui_color_list_background_selected_nf;
PIXVAL gui_theme_t::gui_color_button_text;
PIXVAL gui_theme_t::gui_color_button_text_disabled;
PIXVAL gui_theme_t::gui_color_button_text_selected;
PIXVAL gui_theme_t::gui_color_colored_button_text;
PIXVAL gui_theme_t::gui_color_colored_button_text_selected;
PIXVAL gui_theme_t::gui_color_checkbox_text;
PIXVAL gui_theme_t::gui_color_checkbox_text_disabled;
PIXVAL gui_theme_t::gui_color_ticker_background;
PIXVAL gui_theme_t::gui_color_ticker_divider;
PIXVAL gui_theme_t::gui_color_statusbar_text;
PIXVAL gui_theme_t::gui_color_statusbar_background;
PIXVAL gui_theme_t::gui_color_statusbar_divider;
PIXVAL gui_theme_t::gui_highlight_color;
PIXVAL gui_theme_t::gui_shadow_color;
PIXVAL gui_theme_t::gui_color_loadingbar_inner;
PIXVAL gui_theme_t::gui_color_loadingbar_progress;
PIXVAL gui_theme_t::gui_color_obsolete;
PIXVAL gui_theme_t::gui_color_chat_window_network_transparency;
PIXVAL gui_theme_t::gui_color_empty;

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
scr_size gui_theme_t::gui_dragger_size;
scr_size gui_theme_t::gui_indicator_size;
scr_coord_val gui_theme_t::gui_waitingbar_width;

scr_coord gui_theme_t::gui_focus_offset;
scr_coord gui_theme_t::gui_button_text_offset_right;
scr_coord gui_theme_t::gui_color_button_text_offset_right;

scr_coord_val gui_theme_t::gui_titlebar_height;
scr_coord_val gui_theme_t::gui_frame_left;
scr_coord_val gui_theme_t::gui_frame_top;
scr_coord_val gui_theme_t::gui_frame_right;
scr_coord_val gui_theme_t::gui_frame_bottom;
scr_coord_val gui_theme_t::gui_hspace;
scr_coord_val gui_theme_t::gui_vspace;
scr_coord_val gui_theme_t::gui_filelist_vspace;

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
 */
void gui_theme_t::init_gui_defaults()
{
	gui_color_text                         = color_idx_to_rgb(COL_BLACK);
	gui_color_text_highlight               = color_idx_to_rgb(COL_WHITE);
	gui_color_text_shadow                  = color_idx_to_rgb(COL_BLACK);
	gui_color_text_title                   = color_idx_to_rgb(207);
	gui_color_text_strong                  = color_idx_to_rgb(COL_RED);
	gui_color_text_minus                   = color_idx_to_rgb(COL_RED);
	gui_color_text_plus                    = color_idx_to_rgb(COL_BLACK);
	gui_color_text_unused                  = color_idx_to_rgb(COL_YELLOW);

	gui_color_edit_text                    = color_idx_to_rgb(COL_WHITE);
	gui_color_edit_text_selected           = color_idx_to_rgb(COL_GREY5);
	gui_color_edit_text_disabled           = color_idx_to_rgb(COL_GREY3);
	gui_color_edit_background_selected     = color_idx_to_rgb(COL_GREY2);
	gui_color_edit_beam                    = color_idx_to_rgb(COL_WHITE);

	gui_color_chart_background             = color_idx_to_rgb(MN_GREY1);
	gui_color_chart_lines_zero             = color_idx_to_rgb(MN_GREY4);
	gui_color_chart_lines_odd              = color_idx_to_rgb(COL_WHITE);
	gui_color_chart_lines_even             = color_idx_to_rgb(MN_GREY0);

	gui_color_list_text_selected_focus     = color_idx_to_rgb(COL_WHITE);
	gui_color_list_text_selected_nofocus   = color_idx_to_rgb(MN_GREY3);
	gui_color_list_background_selected_f   = color_idx_to_rgb(COL_BLUE);
	gui_color_list_background_selected_nf  = color_idx_to_rgb(COL_LIGHT_BLUE);

	gui_color_button_text                  = color_idx_to_rgb(COL_BLACK);
	gui_color_button_text_disabled         = color_idx_to_rgb(MN_GREY0);
	gui_color_button_text_selected         = color_idx_to_rgb(COL_BLACK);

	gui_color_colored_button_text          = color_idx_to_rgb(COL_BLACK);
	gui_color_colored_button_text_selected = color_idx_to_rgb(COL_WHITE);
	gui_color_checkbox_text                = color_idx_to_rgb(COL_BLACK);
	gui_color_checkbox_text_disabled       = color_idx_to_rgb(MN_GREY0);
	gui_color_ticker_background            = color_idx_to_rgb(MN_GREY2);
	gui_color_ticker_divider               = color_idx_to_rgb(COL_BLACK);
	gui_color_statusbar_text               = color_idx_to_rgb(COL_BLACK);
	gui_color_statusbar_background         = color_idx_to_rgb(MN_GREY1);
	gui_color_statusbar_divider            = color_idx_to_rgb(MN_GREY4);

	gui_highlight_color                    = color_idx_to_rgb(MN_GREY4);
	gui_shadow_color                       = color_idx_to_rgb(MN_GREY0);

	gui_color_loadingbar_inner             = color_idx_to_rgb(COL_GREY5);
	gui_color_loadingbar_progress          = color_idx_to_rgb(COL_BLUE);

	gui_color_obsolete                     = color_idx_to_rgb(COL_BLUE);
	gui_color_empty                        = color_idx_to_rgb(COL_WHITE);

	env_t::gui_player_color_bright = 4;
	env_t::gui_player_color_dark   = 1;

	gui_button_size              = scr_size(92,14);
	gui_color_button_size        = scr_size(92,16);
	gui_button_text_offset       = scr_size(0,0);
	gui_color_button_text_offset = scr_size(0,0);
	gui_divider_size             = scr_size(92,2+D_V_SPACE*2);
	gui_checkbox_size            = scr_size(10,10);
	gui_pos_button_size          = scr_size(14,LINESPACE);
	gui_arrow_left_size          = scr_size(14,14);
	gui_arrow_right_size         = scr_size(14,14);
	gui_arrow_up_size            = scr_size(14,14);
	gui_arrow_down_size          = scr_size(14,14);
	gui_scrollbar_size           = scr_size(14,14);
	gui_min_scrollbar_size       = scr_size(3,3);
	gui_label_size               = scr_size(92,LINESPACE);
	gui_edit_size                = scr_size(92,max(LINESPACE+2, max(D_ARROW_LEFT_HEIGHT, D_ARROW_RIGHT_HEIGHT) ));
	gui_gadget_size              = scr_size(16,16);
	gui_indicator_size           = scr_size(20,4);
	gui_focus_offset             = scr_coord(1,1);

	gui_titlebar_height  = 16;
	gui_frame_left       = 10;
	gui_frame_top        = 10;
	gui_frame_right      = 10;
	gui_frame_bottom     = 10;
	gui_hspace           = 4;
	gui_vspace           = 4;
	gui_filelist_vspace  = 0;
	gui_waitingbar_width = 4;
	gui_divider_size.h   = D_V_SPACE*2;

	gui_drop_shadows     = false;

	gui_color_chat_window_network_transparency = color_idx_to_rgb(COL_WHITE);
}


// helper for easier init
void gui_theme_t::init_size_from_image( const image_t *image, scr_size &k )
{
	if(  image  ) {
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

	init_size_from_image( skinverwaltung_t::posbutton->get_image( SKIN_BUTTON_POS ), gui_pos_button_size );
	init_size_from_image( skinverwaltung_t::check_button->get_image( SKIN_BUTTON_CHECKBOX ), gui_checkbox_size );
	for(  int i=0;  i<3;  i++  ) {
		pos_button_img[i] = skinverwaltung_t::posbutton->get_image_id( SKIN_BUTTON_POS+i );
		check_button_img[i] = skinverwaltung_t::check_button->get_image_id( SKIN_BUTTON_CHECKBOX+i );
	}

	// Normal buttons (colorful ones)
	scr_coord_val y = gui_button_size.h;
	scr_size k;
	init_size_from_image( skinverwaltung_t::button->get_image( SKIN_BUTTON_SIDE_LEFT ), k );
	y = max( y, k.h );
	init_size_from_image( skinverwaltung_t::button->get_image( SKIN_BUTTON_SIDE_RIGHT ), k );
	y = max( y, k.h );
	init_size_from_image( skinverwaltung_t::button->get_image( SKIN_BUTTON_BODY ), k );
	y = max( y, k.h );
	for(  int i=0;  i<3;  i++  ) {
		for(  int j=0;  j<9;  j++  ) {
			button_tiles[i][j%3][j/3] = skinverwaltung_t::button->get_image_id( i*9+j );
		}
	}
	image_id has_second_mask = 0xFFFF;
	for(  int i=0;  i<2;  i++  ) {
		has_second_mask = 0xFFFF;
		for(  int j=0;  j<9;  j++  ) {
			button_color_tiles[i][j%3][j/3] = skinverwaltung_t::button->get_image_id( i*9+j+27 );
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
			round_button_tiles[i][j%3][j/3] = skinverwaltung_t::round_button->get_image_id( i*9+j );
		}
	}

	// background for editfields, listbuttons, and windows
	for(  int j=0;  j<9;  j++  ) {
		editfield[j%3][j/3] = skinverwaltung_t::editfield->get_image_id( j );
		listbox[j%3][j/3] = skinverwaltung_t::listbox->get_image_id( j );
		windowback[j%3][j/3] = skinverwaltung_t::back->get_image_id( j );
	}

	// Divider (vspace will be added later on)
	init_size_from_image( skinverwaltung_t::divider->get_image(1), gui_divider_size );
	for(  int i=0;  i<3;  i++  ) {
		divider[i][0] = skinverwaltung_t::divider->get_image_id( i );
		divider[i][1] = IMG_EMPTY;
		divider[i][2] = IMG_EMPTY;
	}

	// Calculate arrow size
	init_size_from_image( skinverwaltung_t::scrollbar->get_image( SKIN_BUTTON_ARROW_LEFT ), gui_arrow_left_size );
	init_size_from_image( skinverwaltung_t::scrollbar->get_image( SKIN_BUTTON_ARROW_RIGHT ), gui_arrow_right_size );
	init_size_from_image( skinverwaltung_t::scrollbar->get_image( SKIN_BUTTON_ARROW_UP ), gui_arrow_up_size );
	init_size_from_image( skinverwaltung_t::scrollbar->get_image( SKIN_BUTTON_ARROW_DOWN ), gui_arrow_down_size );
	for(  int i=0;  i<3;  i++  ) {
		arrow_button_left_img[i] = skinverwaltung_t::scrollbar->get_image_id( SKIN_BUTTON_ARROW_LEFT+i );
		arrow_button_right_img[i] = skinverwaltung_t::scrollbar->get_image_id( SKIN_BUTTON_ARROW_RIGHT+i );
		arrow_button_up_img[i] = skinverwaltung_t::scrollbar->get_image_id( SKIN_BUTTON_ARROW_UP+i );
		arrow_button_down_img[i] = skinverwaltung_t::scrollbar->get_image_id( SKIN_BUTTON_ARROW_DOWN+i );
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
		h_scroll_back_tiles[i][0] = skinverwaltung_t::scrollbar->get_image_id( SKIN_SCROLLBAR_H_BACKGROUND_LEFT+i );
		h_scroll_back_tiles[i][1] = IMG_EMPTY;
		h_scroll_back_tiles[i][2] = IMG_EMPTY;
		h_scroll_knob_tiles[i][0] = skinverwaltung_t::scrollbar->get_image_id( SKIN_SCROLLBAR_H_KNOB_LEFT+i );
		h_scroll_knob_tiles[i][1] = IMG_EMPTY;
		h_scroll_knob_tiles[i][2] = IMG_EMPTY;
	}

	// init vertical scrollbar buttons
	for(  int i=0;  i<3;  i++  ) {
		v_scroll_back_tiles[0][i] = skinverwaltung_t::scrollbar->get_image_id( SKIN_SCROLLBAR_V_BACKGROUND_TOP+i );
		v_scroll_back_tiles[1][i] = IMG_EMPTY;
		v_scroll_back_tiles[2][i] = IMG_EMPTY;
		v_scroll_knob_tiles[0][i] = skinverwaltung_t::scrollbar->get_image_id( SKIN_SCROLLBAR_V_KNOB_TOP+i );
		v_scroll_knob_tiles[1][i] = IMG_EMPTY;
		v_scroll_knob_tiles[2][i] = IMG_EMPTY;
	}

	// Calculate V scrollbar size
	{
		scr_size back, front;
		init_size_from_image( skinverwaltung_t::scrollbar->get_image( SKIN_SCROLLBAR_V_BACKGROUND ), back );
		init_size_from_image( skinverwaltung_t::scrollbar->get_image( SKIN_SCROLLBAR_V_KNOB_BODY ), front );
		gui_scrollbar_size.w = max(front.w, back.w);

		// Calculate H scrollbar size
		init_size_from_image( skinverwaltung_t::scrollbar->get_image( SKIN_SCROLLBAR_H_BACKGROUND ), back );
		init_size_from_image( skinverwaltung_t::scrollbar->get_image( SKIN_SCROLLBAR_H_KNOB_BODY ), front );
		gui_scrollbar_size.h = max(front.h, back.h);

		// calculate minimum width
		init_size_from_image( skinverwaltung_t::scrollbar->get_image( SKIN_SCROLLBAR_H_KNOB_LEFT ), back );
		init_size_from_image( skinverwaltung_t::scrollbar->get_image( SKIN_SCROLLBAR_H_KNOB_RIGHT ), front );
		gui_min_scrollbar_size.w = back.w + front.w;

		// calculate minimum height
		init_size_from_image( skinverwaltung_t::scrollbar->get_image( SKIN_SCROLLBAR_V_KNOB_TOP ), back );
		init_size_from_image( skinverwaltung_t::scrollbar->get_image( SKIN_SCROLLBAR_V_KNOB_BOTTOM ), front );
		gui_min_scrollbar_size.h = back.h + front.h;
	}

	// gadgets
	gui_dragger_size = gui_scrollbar_size;
	if (skinverwaltung_t::gadget) {
		init_size_from_image( skinverwaltung_t::gadget->get_image( SKIN_GADGET_CLOSE ), gui_gadget_size );
	}
}


/**
 * Reads theme configuration data, still not final
 *
 * Note, there will be a theme manager later on and
 * each gui object will find their own parameters by
 * themselves after registering its class to the theme
 * manager. This will be done as the last step in
 * the chain when loading a theme.
 */
bool gui_theme_t::themes_init(const char *file_name, bool init_fonts, bool init_tools )
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

	// reload current font if requested size differs and we are allowed to do so
	uint8 new_size = contents.get_int("font_size", env_t::fontsize );
	if(  init_fonts  &&  new_size!=0  &&  LINESPACE!=new_size  ) {
		if(  display_load_font( env_t::fontname.c_str() )  ) {
			env_t::fontsize = new_size;
		}
	}

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
		dr_chdir( pathname );
		pakset_manager_t::load_pak_file(buttonpak);
		gui_theme_t::init_gui_from_images();
		free(pathname);
		dr_chdir(env_t::user_dir);
	}

	// first the stuff for the dialogues
	gui_theme_t::gui_titlebar_height = (uint32)contents.get_int("gui_titlebar_height", gui_theme_t::gui_titlebar_height );
	gui_theme_t::gui_frame_left =      (uint32)contents.get_int("gui_frame_left",      gui_theme_t::gui_frame_left );
	gui_theme_t::gui_frame_top =       (uint32)contents.get_int("gui_frame_top",       gui_theme_t::gui_frame_top );
	gui_theme_t::gui_frame_right =     (uint32)contents.get_int("gui_frame_right",     gui_theme_t::gui_frame_right );
	gui_theme_t::gui_frame_bottom =    (uint32)contents.get_int("gui_frame_bottom",    gui_theme_t::gui_frame_bottom );
	gui_theme_t::gui_hspace =          (uint32)contents.get_int("gui_hspace",          gui_theme_t::gui_hspace );
	gui_theme_t::gui_vspace =          (uint32)contents.get_int("gui_vspace",          gui_theme_t::gui_vspace );
	gui_theme_t::gui_filelist_vspace = (uint32)contents.get_int("gui_filelist_vspace", gui_theme_t::gui_filelist_vspace );

	// the divider needs the vspace added to it for know
	gui_divider_size.h += gui_vspace*2;
	gui_theme_t::gui_divider_size.h = contents.get_int("gui_divider_vsize",  gui_theme_t::gui_divider_size.h );

	gui_theme_t::gui_button_size.w = (uint32)contents.get_int("gui_button_width",  gui_theme_t::gui_button_size.w );
	gui_theme_t::gui_button_size.h = (uint32)contents.get_int("gui_button_height", gui_theme_t::gui_button_size.h );
	gui_theme_t::gui_edit_size.h = (uint32)contents.get_int("gui_edit_height", gui_theme_t::gui_edit_size.h );

	gui_theme_t::gui_checkbox_size.w = (uint32)contents.get_int("gui_checkbox_width",  gui_theme_t::gui_checkbox_size.w );
	gui_theme_t::gui_checkbox_size.h = (uint32)contents.get_int("gui_checkbox_height", gui_theme_t::gui_checkbox_size.h );

	gui_theme_t::gui_gadget_size.w = (uint32)contents.get_int("gui_gadget_width",  gui_theme_t::gui_gadget_size.w );
	gui_theme_t::gui_gadget_size.h = (uint32)contents.get_int("gui_gadget_height", gui_theme_t::gui_gadget_size.h );

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

	// dragger size must be as large as scrollbar size
	if (skinverwaltung_t::gadget) {
		init_size_from_image( skinverwaltung_t::gadget->get_image( SKIN_WINDOW_RESIZE), gui_dragger_size );
		gui_dragger_size.clip_lefttop(scr_coord(gui_scrollbar_size.w, gui_scrollbar_size.h));
	}

	// in practice, posbutton min height better is LINESPACE
	gui_theme_t::gui_pos_button_size.w = (uint32)contents.get_int("gui_posbutton_width",  gui_theme_t::gui_pos_button_size.w );
	gui_theme_t::gui_pos_button_size.h = (uint32)contents.get_int("gui_posbutton_height", gui_theme_t::gui_pos_button_size.h );

	// read ../dataobj/tabfile.h for clarification of this area
	vector_tpl<int> color_button_text_offsets = contents.get_ints("gui_color_button_text_offset");
	if(  color_button_text_offsets.get_count() > 2  ) {
		gui_theme_t::gui_color_button_text_offset = scr_size(color_button_text_offsets[0], color_button_text_offsets[1]);
		gui_theme_t::gui_color_button_text_offset_right = scr_coord(color_button_text_offsets[2], 0);
	}

	vector_tpl<int> button_text_offsets = contents.get_ints("gui_button_text_offset");
	if(  button_text_offsets.get_count() > 2  ) {
		gui_theme_t::gui_button_text_offset = scr_size(button_text_offsets[0], button_text_offsets[1]);
		gui_theme_t::gui_button_text_offset_right = scr_coord(button_text_offsets[2], 0);
	}

	// default iconsize (square for now)
	env_t::iconsize.h = env_t::iconsize.w = contents.get_int("icon_width",env_t::iconsize.w );

	// maybe not the best place, rather use simwin for the static defines?
	gui_theme_t::gui_color_text                         = (PIXVAL)contents.get_color("gui_color_text", SYSCOL_TEXT);
	gui_theme_t::gui_color_text_highlight               = (PIXVAL)contents.get_color("gui_color_text_highlight", SYSCOL_TEXT_HIGHLIGHT);
	gui_theme_t::gui_color_text_shadow                  = (PIXVAL)contents.get_color("gui_color_text_shadow", SYSCOL_TEXT_SHADOW);
	gui_theme_t::gui_color_text_title                   = (PIXVAL)contents.get_color("gui_color_text_title", SYSCOL_TEXT_TITLE);
	gui_theme_t::gui_color_text_strong                  = (PIXVAL)contents.get_color("gui_color_text_strong", SYSCOL_TEXT_STRONG);
	gui_theme_t::gui_color_text_minus                   = (PIXVAL)contents.get_color("gui_color_text_minus", MONEY_MINUS);
	gui_theme_t::gui_color_text_plus                    = (PIXVAL)contents.get_color("gui_color_text_plus", MONEY_PLUS);
	gui_theme_t::gui_color_text_unused                  = (PIXVAL)contents.get_color("gui_color_text_unused", SYSCOL_TEXT_UNUSED);
	gui_theme_t::gui_color_edit_text                    = (PIXVAL)contents.get_color("gui_color_edit_text", SYSCOL_EDIT_TEXT);
	gui_theme_t::gui_color_edit_text_selected           = (PIXVAL)contents.get_color("gui_color_edit_text_selected", SYSCOL_EDIT_TEXT_SELECTED);
	gui_theme_t::gui_color_edit_text_disabled           = (PIXVAL)contents.get_color("gui_color_edit_text_disabled", SYSCOL_EDIT_TEXT_DISABLED);
	gui_theme_t::gui_color_edit_background_selected     = (PIXVAL)contents.get_color("gui_color_edit_background_selected", SYSCOL_EDIT_BACKGROUND_SELECTED);
	gui_theme_t::gui_color_edit_beam                    = (PIXVAL)contents.get_color("gui_color_edit_beam", SYSCOL_CURSOR_BEAM);
	gui_theme_t::gui_color_chart_background             = (PIXVAL)contents.get_color("gui_color_chart_background", SYSCOL_CHART_BACKGROUND);
	gui_theme_t::gui_color_chart_lines_zero             = (PIXVAL)contents.get_color("gui_color_chart_lines_zero", SYSCOL_CHART_LINES_ZERO);
	gui_theme_t::gui_color_chart_lines_odd              = (PIXVAL)contents.get_color("gui_color_chart_lines_odd", SYSCOL_CHART_LINES_ODD);
	gui_theme_t::gui_color_chart_lines_even             = (PIXVAL)contents.get_color("gui_color_chart_lines_even", SYSCOL_CHART_LINES_EVEN);
	gui_theme_t::gui_color_list_text_selected_focus     = (PIXVAL)contents.get_color("gui_color_list_text_selected_focus", SYSCOL_LIST_TEXT_SELECTED_FOCUS);
	gui_theme_t::gui_color_list_text_selected_nofocus   = (PIXVAL)contents.get_color("gui_color_list_text_selected_nofocus", SYSCOL_LIST_TEXT_SELECTED_NOFOCUS);
	gui_theme_t::gui_color_list_background_selected_f   = (PIXVAL)contents.get_color("gui_color_list_background_selected_focus", SYSCOL_LIST_BACKGROUND_SELECTED_F);
	gui_theme_t::gui_color_list_background_selected_nf  = (PIXVAL)contents.get_color("gui_color_list_background_selected_nofocus", SYSCOL_LIST_BACKGROUND_SELECTED_NF);
	gui_theme_t::gui_color_button_text                  = (PIXVAL)contents.get_color("gui_color_button_text", SYSCOL_BUTTON_TEXT);
	gui_theme_t::gui_color_button_text_disabled         = (PIXVAL)contents.get_color("gui_color_button_text_disabled", SYSCOL_BUTTON_TEXT_DISABLED);
	gui_theme_t::gui_color_button_text_selected         = (PIXVAL)contents.get_color("gui_color_button_text_selected", SYSCOL_BUTTON_TEXT_SELECTED);
	gui_theme_t::gui_color_colored_button_text          = (PIXVAL)contents.get_color("gui_color_colored_button_text", SYSCOL_COLORED_BUTTON_TEXT);
	gui_theme_t::gui_color_colored_button_text_selected = (PIXVAL)contents.get_color("gui_color_colored_button_text_selected", SYSCOL_COLORED_BUTTON_TEXT_SELECTED);
	gui_theme_t::gui_color_checkbox_text                = (PIXVAL)contents.get_color("gui_color_checkbox_text", SYSCOL_CHECKBOX_TEXT);
	gui_theme_t::gui_color_checkbox_text_disabled       = (PIXVAL)contents.get_color("gui_color_checkbox_text_disabled", SYSCOL_CHECKBOX_TEXT_DISABLED);
	gui_theme_t::gui_color_ticker_background            = (PIXVAL)contents.get_color("gui_color_ticker_background", SYSCOL_TICKER_BACKGROUND);
	gui_theme_t::gui_color_ticker_divider               = (PIXVAL)contents.get_color("gui_color_ticker_divider", SYSCOL_TICKER_DIVIDER);
	gui_theme_t::gui_color_statusbar_text               = (PIXVAL)contents.get_color("gui_color_statusbar_text", SYSCOL_STATUSBAR_TEXT);
	gui_theme_t::gui_color_statusbar_background         = (PIXVAL)contents.get_color("gui_color_statusbar_background", SYSCOL_STATUSBAR_BACKGROUND);
	gui_theme_t::gui_color_statusbar_divider            = (PIXVAL)contents.get_color("gui_color_statusbar_divider", SYSCOL_STATUSBAR_DIVIDER);
	gui_theme_t::gui_highlight_color                    = (PIXVAL)contents.get_color("gui_highlight_color", SYSCOL_HIGHLIGHT);
	gui_theme_t::gui_shadow_color                       = (PIXVAL)contents.get_color("gui_shadow_color", SYSCOL_SHADOW);
	gui_theme_t::gui_color_loadingbar_inner             = (PIXVAL)contents.get_color("gui_color_loadingbar_inner", SYSCOL_LOADINGBAR_INNER);
	gui_theme_t::gui_color_loadingbar_progress          = (PIXVAL)contents.get_color("gui_color_loadingbar_progress", SYSCOL_LOADINGBAR_PROGRESS);
	gui_theme_t::gui_color_obsolete                     = (PIXVAL)contents.get_color("gui_color_obsolete", SYSCOL_OBSOLETE);
	gui_theme_t::gui_color_empty                        = (PIXVAL)contents.get_color("gui_color_empty", SYSCOL_EMPTY);
	gui_theme_t::gui_color_chat_window_network_transparency = (PIXVAL)contents.get_color("gui_color_chat_window_network_transparency", gui_color_chat_window_network_transparency);

	gui_theme_t::gui_waitingbar_width = (uint32)contents.get_int("gui_waitingbar_width", gui_theme_t::gui_waitingbar_width);

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
	env_t::bottom_window_darkness =    contents.get_int("bottom_window_darkness",    env_t::bottom_window_darkness );
	env_t::menupos                   = contents.get_int("menubar_position",          env_t::menupos);
	env_t::gui_player_color_bright =   contents.get_int("gui_player_color_bright",   env_t::gui_player_color_bright );
	env_t::gui_player_color_dark =     contents.get_int("gui_player_color_dark",     env_t::gui_player_color_dark );

	env_t::default_window_title_color = contents.get_color("default_window_title_color", env_t::default_window_title_color,  &env_t::default_window_title_color_rgb );
	env_t::front_window_text_color =    contents.get_color("front_window_text_color",    env_t::front_window_text_color,  &env_t::front_window_text_color_rgb );
	env_t::bottom_window_text_color =   contents.get_color("bottom_window_text_color",   env_t::bottom_window_text_color, &env_t::bottom_window_text_color_rgb );
	env_t::cursor_overlay_color =       contents.get_color("cursor_overlay_color",       env_t::cursor_overlay_color,     &env_t::cursor_overlay_color_rgb );
	env_t::tooltip_color =              contents.get_color("tooltip_background_color",   env_t::tooltip_color,            &env_t::tooltip_color_rgb );
	env_t::tooltip_textcolor =          contents.get_color("tooltip_text_color",         env_t::tooltip_textcolor,        &env_t::tooltip_textcolor_rgb );

	env_t::show_tooltips =        contents.get_int("show_tooltips",              env_t::show_tooltips );
	env_t::tooltip_delay =        contents.get_int("tooltip_delay",              env_t::tooltip_delay );
	env_t::tooltip_duration =     contents.get_int("tooltip_duration",           env_t::tooltip_duration );
	env_t::toolbar_max_width =    contents.get_int("toolbar_max_width",          env_t::toolbar_max_width );
	env_t::toolbar_max_height =   contents.get_int("toolbar_max_height",         env_t::toolbar_max_height );

	env_t::chat_window_transparency =   100 - contents.get_int("gui_chat_window_network_transparency", 100 - env_t::chat_window_transparency);

	if(  toolbar_last_used_t::last_used_tools  &&  init_tools  ) {
		// only re-init if already inited
		tool_t::update_toolbars();
	}
	env_t::default_theme = file_name;

	return true;
}
