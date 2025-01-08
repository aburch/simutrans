/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_GUI_THEME_H
#define GUI_GUI_THEME_H


#include "../dataobj/koord.h"
#include "../simcolor.h"
#include "../display/simgraph.h"

class image_t;


/*
 * The following gives positioning aids for elements in dialogues
 * Only those, LINESPACE, and dimensions of elements itself must be
 * exclusively used to calculate positions in dialogues to have a
 * scalable interface
 *
 * Max Kielland:
 * Added more defines for theme testing.
 * These is going to be moved into the theme handling later.
 */

#define D_BUTTON_SIZE          (gui_theme_t::gui_button_size  )
#define D_BUTTON_WIDTH         (gui_theme_t::gui_button_size.w)
#define D_BUTTON_HEIGHT        (gui_theme_t::gui_button_size.h)
#define D_BUTTON_PADDINGS_X    (gui_theme_t::gui_button_text_offset.w + gui_theme_t::gui_button_text_offset_right.x)

#define D_FILTER_BUTTON_SIZE   (gui_theme_t::gui_color_button_size  )
#define D_FILTER_BUTTON_WIDTH  (gui_theme_t::gui_color_button_size.w)
#define D_FILTER_BUTTON_HEIGHT (gui_theme_t::gui_color_button_size.h)

#define D_CHECKBOX_SIZE        (gui_theme_t::gui_checkbox_size  )
#define D_CHECKBOX_WIDTH       (gui_theme_t::gui_checkbox_size.w)
#define D_CHECKBOX_HEIGHT      (gui_theme_t::gui_checkbox_size.h)

#define D_POS_BUTTON_SIZE      (gui_theme_t::gui_pos_button_size  )
#define D_POS_BUTTON_WIDTH     (gui_theme_t::gui_pos_button_size.w)
#define D_POS_BUTTON_HEIGHT    (gui_theme_t::gui_pos_button_size.h)

#define D_ARROW_LEFT_SIZE      (gui_theme_t::gui_arrow_left_size  )
#define D_ARROW_LEFT_WIDTH     (gui_theme_t::gui_arrow_left_size.w)
#define D_ARROW_LEFT_HEIGHT    (gui_theme_t::gui_arrow_left_size.h)

#define D_ARROW_RIGHT_SIZE     (gui_theme_t::gui_arrow_right_size  )
#define D_ARROW_RIGHT_WIDTH    (gui_theme_t::gui_arrow_right_size.w)
#define D_ARROW_RIGHT_HEIGHT   (gui_theme_t::gui_arrow_right_size.h)

#define D_ARROW_UP_SIZE        (gui_theme_t::gui_arrow_up_size  )
#define D_ARROW_UP_WIDTH       (gui_theme_t::gui_arrow_up_size.w)
#define D_ARROW_UP_HEIGHT      (gui_theme_t::gui_arrow_up_size.h)

#define D_ARROW_DOWN_SIZE      (gui_theme_t::gui_arrow_down_size  )
#define D_ARROW_DOWN_WIDTH     (gui_theme_t::gui_arrow_down_size.w)
#define D_ARROW_DOWN_HEIGHT    (gui_theme_t::gui_arrow_down_size.h)

#define D_SCROLLBAR_SIZE       (gui_theme_t::gui_scrollbar_size  )
#define D_SCROLLBAR_HEIGHT     (gui_theme_t::gui_scrollbar_size.h)
#define D_SCROLLBAR_WIDTH      (gui_theme_t::gui_scrollbar_size.w)

#define D_SCROLL_MIN_SIZE      (gui_theme_t::gui_min_scrollbar_size  )
#define D_SCROLL_MIN_WIDTH     (gui_theme_t::gui_min_scrollbar_size.w)
#define D_SCROLL_MIN_HEIGHT    (gui_theme_t::gui_min_scrollbar_size.h)

#define D_GADGET_SIZE          (gui_theme_t::gui_gadget_size  )
#define D_GADGET_WIDTH         (gui_theme_t::gui_gadget_size.w)
#define D_GADGET_HEIGHT        (gui_theme_t::gui_gadget_size.h)

#define D_DRAGGER_SIZE         (gui_theme_t::gui_dragger_size  )
#define D_DRAGGER_WIDTH        (gui_theme_t::gui_dragger_size.w)
#define D_DRAGGER_HEIGHT       (gui_theme_t::gui_dragger_size.h)

#define D_INDICATOR_BOX_SIZE   (gui_theme_t::gui_indicator_size  )
#define D_INDICATOR_BOX_HEIGHT (gui_theme_t::gui_indicator_size.h)
#define D_INDICATOR_BOX_WIDTH  (gui_theme_t::gui_indicator_size.w)

#define D_INDICATOR_SIZE       (gui_theme_t::gui_indicator_size  )
#define D_INDICATOR_WIDTH      (gui_theme_t::gui_indicator_size.w)
#define D_INDICATOR_HEIGHT     (gui_theme_t::gui_indicator_size.h)

#define D_LABEL_SIZE           (gui_theme_t::gui_label_size  )
#define D_LABEL_WIDTH          (gui_theme_t::gui_label_size.w)
#define D_LABEL_HEIGHT         (gui_theme_t::gui_label_size.h)

#define D_EDIT_SIZE            (gui_theme_t::gui_edit_size  )
#define D_EDIT_WIDTH           (gui_theme_t::gui_edit_size.w)
#define D_EDIT_HEIGHT          (gui_theme_t::gui_edit_size.h)

#define D_FOCUS_OFFSET         (gui_theme_t::gui_focus_offset  )
#define D_FOCUS_OFFSET_H       (gui_theme_t::gui_focus_offset.x)
#define D_FOCUS_OFFSET_V       (gui_theme_t::gui_focus_offset.y)

#define D_TITLEBAR_HEIGHT      (gui_theme_t::gui_titlebar_height)
#define D_DIVIDER_HEIGHT       (gui_theme_t::gui_divider_size.h)
#define D_STATUSBAR_HEIGHT     (max(16,LINESPACE))                    // statusbar bottom of screen
#define D_TAB_HEADER_HEIGHT      (gui_tab_panel_t::header_vsize)        // Tab page params (replace with real values from the skin images)

// Dialog borders
#define D_MARGIN_LEFT          (gui_theme_t::gui_frame_left)
#define D_MARGIN_TOP           (gui_theme_t::gui_frame_top)
#define D_MARGIN_RIGHT         (gui_theme_t::gui_frame_right)
#define D_MARGIN_BOTTOM        (gui_theme_t::gui_frame_bottom)

// Dialogue border helpers
#define D_MARGINS_X            (D_MARGIN_LEFT + D_MARGIN_RIGHT)
#define D_MARGINS_Y            (D_MARGIN_TOP + D_MARGIN_BOTTOM)

// space between two elements
#define D_H_SPACE              (gui_theme_t::gui_hspace)
#define D_V_SPACE              (gui_theme_t::gui_vspace)

// bars of goods waiting in stations
#define D_WAITINGBAR_WIDTH     (gui_theme_t::gui_waitingbar_width)

// Button grid helpers
#define BUTTON1_X     (D_MARGIN_LEFT)
#define BUTTON2_X     (D_MARGIN_LEFT+1*(D_BUTTON_WIDTH+D_H_SPACE))
#define BUTTON3_X     (D_MARGIN_LEFT+2*(D_BUTTON_WIDTH+D_H_SPACE))
#define BUTTON4_X     (D_MARGIN_LEFT+3*(D_BUTTON_WIDTH+D_H_SPACE))
#define BUTTON_X(col) ( (col) * (D_BUTTON_WIDTH  + D_H_SPACE) )
#define BUTTON_Y(row) ( (row) * (D_BUTTON_HEIGHT + D_V_SPACE) )

// The width of a typical dialogue (either list/covoi/factory) and initial width when it makes sense
#define D_DEFAULT_WIDTH (D_MARGINS_X + 4*D_BUTTON_WIDTH + 3*D_H_SPACE)
#define D_DEFAULT_HEIGHT (max(56, get_base_tile_raster_width() * 7 / 8) + 208 + D_SCROLLBAR_HEIGHT)

// Max Kielland: align helper, returns the offset to apply to N1 for a center alignment around N2
#define D_GET_CENTER_ALIGN_OFFSET(N1,N2) ((N2-N1)>>1)
#define D_GET_FAR_ALIGN_OFFSET(N1,N2) (N2-N1)

#define D_FILELIST_V_SPACE (gui_theme_t::gui_filelist_vspace)

#define TOOLTIP_MOUSE_OFFSET_X (16)
#define TOOLTIP_MOUSE_OFFSET_Y (12)

// these define the offset of images in their definitions
enum {
	SKIN_WINDOW_BACKGROUND=0,

	// gadget (window GUI buttons)
	SKIN_GADGET_CLOSE=0,
	SKIN_GADGET_HELP,
	SKIN_GADGET_MINIMIZE,
	SKIN_BUTTON_PREVIOUS,
	SKIN_BUTTON_NEXT,
	SKIN_GADGET_NOTPINNED,
	SKIN_GADGET_PINNED,
	SKIN_WINDOW_RESIZE,
	SKIN_GADGET_GOTOPOS,
//	SKIN_GADGET_BUTTON,
	SKIN_GADGET_COUNT, // maximum number, NOT AN IMAGE

	// scrollbars horizontal
	SKIN_BUTTON_ARROW_LEFT = 0,
	SKIN_BUTTON_ARROW_LEFT_PRESSED,
	SKIN_BUTTON_ARROW_LEFT_DISABLED,
	SKIN_BUTTON_ARROW_RIGHT,
	SKIN_BUTTON_ARROW_RIGHT_PRESSED,
	SKIN_BUTTON_ARROW_RIGHT_DISABLED,
	SKIN_SCROLLBAR_H_BACKGROUND_LEFT,
	SKIN_SCROLLBAR_H_BACKGROUND,
	SKIN_SCROLLBAR_H_BACKGROUND_RIGHT,
	SKIN_SCROLLBAR_H_KNOB_LEFT,
	SKIN_SCROLLBAR_H_KNOB_BODY,
	SKIN_SCROLLBAR_H_KNOB_RIGHT,
	// and vertical
	SKIN_BUTTON_ARROW_UP,
	SKIN_BUTTON_ARROW_UP_PRESSED,
	SKIN_BUTTON_ARROW_UP_DISABLED,
	SKIN_BUTTON_ARROW_DOWN,
	SKIN_BUTTON_ARROW_DOWN_PRESSED,
	SKIN_BUTTON_ARROW_DOWN_DISABLED,
	SKIN_SCROLLBAR_V_BACKGROUND_TOP,
	SKIN_SCROLLBAR_V_BACKGROUND,
	SKIN_SCROLLBAR_V_BACKGROUND_BOTTOM,
	SKIN_SCROLLBAR_V_KNOB_TOP,
	SKIN_SCROLLBAR_V_KNOB_BODY,
	SKIN_SCROLLBAR_V_KNOB_BOTTOM,

	// squarebutton
	SKIN_BUTTON_CHECKBOX = 0,
	SKIN_BUTTON_CHECKBOX_PRESSED,
	SKIN_BUTTON_CHECKBOX_DISABLED,

	// posbutton
	SKIN_BUTTON_POS = 0,
	SKIN_BUTTON_POS_PRESSED,
	SKIN_BUTTON_POS_DISABLED,

	// normal buttons and round buttons
	SKIN_BUTTON_SIDE_LEFT = 0,
	SKIN_BUTTON_BODY,
	SKIN_BUTTON_SIDE_RIGHT,
	SKIN_BUTTON_SIDE_LEFT_PRESSED,
	SKIN_BUTTON_BODY_PRESSED,
	SKIN_BUTTON_SIDE_RIGHT_PRESSED,
	SKIN_BUTTON_SIDE_RIGHT_DISABLED,
	SKIN_BUTTON_BODY_DISABLED,
	SKIN_BUTTON_SIDE_LEFT_DISABLED,
	SKIN_BUTTON_COLOR_MASK_LEFT,
	SKIN_BUTTON_COLOR_MASK_BODY,
	SKIN_BUTTON_COLOR_MASK_RIGHT
};


class gui_theme_t {
public:
	/// @name system colours used by gui components
	/// @{
	static PIXVAL gui_color_text;                         //@< Color to draw standard text
	static PIXVAL gui_color_text_highlight;               //@< Color to draw highlighted text (tabs, finance window headlines, ware list bonus text, fps info in video options, it and em tags)
	static PIXVAL gui_color_text_title;                   //@< Color to draw title text (banner, h1 and a tags)
	static PIXVAL gui_color_text_shadow;                  //@< Color to draw text shadow
	static PIXVAL gui_color_text_strong;                  //@< Color to draw strong text (strong tags)
	static PIXVAL gui_color_text_minus;                   //@< Color to draw negative values
	static PIXVAL gui_color_text_plus;                    //@< Color to draw positive values
	static PIXVAL gui_color_text_unused;                  //@< Color to draw unused items
	static PIXVAL gui_color_edit_text;                    //@< Color to draw text in edit areas
	static PIXVAL gui_color_edit_text_selected;           //@< Color to draw selected text in edit areas
	static PIXVAL gui_color_edit_text_disabled;           //@< Color to draw disabled text in edit areas
	static PIXVAL gui_color_edit_background_selected;     //@< Color to draw background of selected text in edit areas
	static PIXVAL gui_color_edit_beam;                    //@< Color to draw the cursor beam
	static PIXVAL gui_color_chart_background;             //@< Color to draw background of charts
	static PIXVAL gui_color_chart_lines_zero;             //@< Color to draw in-chart horizontal zero line
	static PIXVAL gui_color_chart_lines_odd;              //@< Color to draw in-chart vertical odd lines and text
	static PIXVAL gui_color_chart_lines_even;             //@< Color to draw in-chart vertical even lines and text
	static PIXVAL gui_color_list_text_selected_focus;     //@< Colour to draw the selected element text in list when window has focus
	static PIXVAL gui_color_list_text_selected_nofocus;   //@< Colour to draw the selected element text in list when window is not in focus
	static PIXVAL gui_color_list_background_selected_f;   //@< Colour to draw the selected element background in list when window has focus
	static PIXVAL gui_color_list_background_selected_nf;  //@< Colour to draw the selected element background in list when window is not in focus
	static PIXVAL gui_color_button_text;                  //@< Color to draw text in normal buttons
	static PIXVAL gui_color_button_text_disabled;         //@< Color to draw text in disabled buttons
	static PIXVAL gui_color_button_text_selected;         //@< Color to draw text in pressed normal buttons
	static PIXVAL gui_color_colored_button_text;          //@< Color to draw text in colored buttons
	static PIXVAL gui_color_colored_button_text_selected; //@< Color to draw text in pressed colored buttons
	static PIXVAL gui_color_checkbox_text;                //@< Color to draw text in checkboxes
	static PIXVAL gui_color_checkbox_text_disabled;       //@< Color to draw text in disabled checkboxes
	static PIXVAL gui_color_ticker_background;            //@< Color to draw ticker background
	static PIXVAL gui_color_ticker_divider;               //@< Color to draw ticker divider
	static PIXVAL gui_color_statusbar_text;               //@< Color to draw text in statusbar
	static PIXVAL gui_color_statusbar_background;         //@< Color to draw statusbar background
	static PIXVAL gui_color_statusbar_divider;            //@< Color to draw statusbar divider
	static PIXVAL gui_highlight_color;                    //@< Color to draw highlight dividers (tabs)
	static PIXVAL gui_shadow_color;                       //@< Color to draw shadowed dividers (tabs)
	static PIXVAL gui_color_loadingbar_inner;
	static PIXVAL gui_color_loadingbar_progress;
	static PIXVAL gui_color_obsolete;                     //@< Color for obsolete convois/server entries
	static PIXVAL gui_color_empty;                        //@< Color for empty entries
	static PIXVAL gui_color_chat_window_network_transparency; //@< Color if chat window is transparent in network mode
	/// @}

	/// @name GUI element sizes used by gui components
	/// @{
	static scr_size gui_divider_size;
	static scr_size gui_button_size;
	static scr_size gui_color_button_text_offset; // extra offset for the text (in case of asymmetric or buttons with color on the left)
	static scr_size gui_button_text_offset;       // extra offset for the text (in case of asymmetric or buttons with checkmark on the left)
	static scr_size gui_color_button_size;
	static scr_size gui_checkbox_size;
	static scr_size gui_pos_button_size;
	static scr_size gui_arrow_left_size;
	static scr_size gui_arrow_right_size;
	static scr_size gui_arrow_up_size;
	static scr_size gui_arrow_down_size;
	static scr_size gui_scrollbar_size;
	static scr_size gui_min_scrollbar_size; // minimum width and height of a scrollbar slider
	static scr_size gui_label_size;
	static scr_size gui_edit_size;
	static scr_size gui_indicator_size;
	static scr_size gui_gadget_size;
	static scr_size gui_dragger_size;
	static scr_coord gui_focus_offset;
	static scr_coord gui_color_button_text_offset_right; // extra right offset for the text (in case of asymmetric or buttons with color on the right)
	static scr_coord gui_button_text_offset_right;       // extra right offset for the text (in case of asymmetric or buttons with checkmark on the right)

	static scr_coord_val gui_titlebar_height;
	static scr_coord_val gui_frame_left;
	static scr_coord_val gui_frame_top;
	static scr_coord_val gui_frame_right;
	static scr_coord_val gui_frame_bottom;
	static scr_coord_val gui_hspace;
	static scr_coord_val gui_vspace;
	static scr_coord_val gui_waitingbar_width;

	// one special entries, since there are lot of lists with files/fonts/paks/... where zero spacing could fit more entires on the screen
	static scr_coord_val gui_filelist_vspace;
	/// @}

	// those are the 3x3 images which are used for stretching
	static stretch_map_t button_tiles[3];
	static stretch_map_t button_color_tiles[2];
	static stretch_map_t round_button_tiles[3];
	static stretch_map_t h_scroll_back_tiles;
	static stretch_map_t h_scroll_knob_tiles;
	static stretch_map_t v_scroll_back_tiles;
	static stretch_map_t v_scroll_knob_tiles;
	static stretch_map_t divider;
	static stretch_map_t editfield;
	static stretch_map_t listbox;
	static stretch_map_t windowback;

	// those are the normal, selected and disabled simple buttons
	static image_id arrow_button_left_img[3];
	static image_id arrow_button_right_img[3];
	static image_id arrow_button_up_img[3];
	static image_id arrow_button_down_img[3];
	static image_id check_button_img[3];
	static image_id pos_button_img[3];

	static bool gui_drop_shadows;

public:
	// default dimensions and colors
	static void init_gui_defaults();

	// assings k with the dimension of this image
	static void init_size_from_image( const image_t *pic, scr_size &k );

	// init the skin dimensions form file
	static void init_gui_from_images();

	/**
	 * Reads theme configuration data, still not final
	 * searches a theme.tab inside the specified folder
	 */
	static bool themes_init(const char *dir_name,bool init_font,bool init_tools);
};
#endif
