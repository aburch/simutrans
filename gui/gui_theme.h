/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_theme_h
#define gui_theme_h

#include "../dataobj/koord.h"
#include "../simcolor.h"
#include "../display/simgraph.h"

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

#define D_INDICATOR_BOX_HEIGHT (gui_theme_t::gui_indicator_box_size.y)
#define D_INDICATOR_BOX_WIDTH  (gui_theme_t::gui_indicator_box_size.x)

// default edit field height
#define D_EDIT_HEIGHT (LINESPACE+3)

// default square button xy size (replace with real values from the skin images)
#define D_BUTTON_SQUARE (gui_theme_t::gui_checkbox_size.y)

// statusbar bottom of screen
#define D_STATUSBAR_HEIGHT (16)

// gadget size
#define D_GADGET_SIZE (D_TITLEBAR_HEIGHT)

// Scrollbar params (replace with real values from the skin images)
#define KNOB_SIZE        (32)

// default button width (may change with language and font)
#define D_BUTTON_WIDTH (gui_theme_t::gui_button_size.x)
#define D_BUTTON_HEIGHT (gui_theme_t::gui_button_size.y)

// scrollbar sizes
#define D_SCROLLBAR_WIDTH (gui_theme_t::gui_scrollbar_size.x)
#define D_SCROLLBAR_HEIGHT (gui_theme_t::gui_scrollbar_size.y)

// titlebar height
#define D_TITLEBAR_HEIGHT (gui_theme_t::gui_titlebar_height)

#define D_DIVIDER_HEIGHT (gui_theme_t::gui_divider_height)

// Tab page params (replace with real values from the skin images)
#define TAB_HEADER_V_SIZE (gui_tab_panel_t::header_vsize)

// dialog borders
#define D_MARGIN_LEFT (gui_theme_t::gui_frame_left)
#define D_MARGIN_TOP (gui_theme_t::gui_frame_top)
#define D_MARGIN_RIGHT (gui_theme_t::gui_frame_right)
#define D_MARGIN_BOTTOM (gui_theme_t::gui_frame_bottom)

#define D_MARGINS_X (D_MARGIN_LEFT + D_MARGIN_RIGHT)
#define D_MARGINS_Y (D_MARGIN_TOP + D_MARGIN_BOTTOM)

// space between two elements
#define D_H_SPACE (gui_theme_t::gui_hspace)
#define D_V_SPACE (gui_theme_t::gui_vspace)

#define BUTTON1_X (D_MARGIN_LEFT)
#define BUTTON2_X (D_MARGIN_LEFT+1*(D_BUTTON_WIDTH+D_H_SPACE))
#define BUTTON3_X (D_MARGIN_LEFT+2*(D_BUTTON_WIDTH+D_H_SPACE))
#define BUTTON4_X (D_MARGIN_LEFT+3*(D_BUTTON_WIDTH+D_H_SPACE))

#define BUTTON_X(col) ( (col) * (D_BUTTON_WIDTH  + D_H_SPACE) )
#define BUTTON_Y(row) ( (row) * (D_BUTTON_HEIGHT + D_V_SPACE) )

// Max Kielland: align helper, returns the offset to apply to N1 for a center alignment around N2
#define D_GET_CENTER_ALIGN_OFFSET(N1,N2) ((N2-N1)>>1)
#define D_GET_FAR_ALIGN_OFFSET(N1,N2) (N2-N1)

// The width of a typical dialogue (either list/covoi/factory) and initial width when it makes sense
#define D_DEFAULT_WIDTH (D_MARGIN_LEFT + 4*D_BUTTON_WIDTH + 3*D_H_SPACE + D_MARGIN_RIGHT)

// dimensions of indicator bars (not yet a gui element ...)
#define D_INDICATOR_WIDTH (gui_theme_t::gui_indicator_width)
#define D_INDICATOR_HEIGHT (gui_theme_t::gui_indicator_height)

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
	SKIN_GADGET_COUNT,	// maximum number, NOT AN IMAGE

	// scrollbars horizontal
	SKIN_BUTTON_ARROW_LEFT = 0,
	SKIN_BUTTON_ARROW_LEFT_PRESSED,
	SKIN_BUTTON_ARROW_RIGHT,
	SKIN_BUTTON_ARROW_RIGHT_PRESSED,
	SKIN_SCROLLBAR_H_BACKGROUND_LEFT,
	SKIN_SCROLLBAR_H_BACKGROUND_RIGHT,
	SKIN_SCROLLBAR_H_BACKGROUND,
	SKIN_SCROLLBAR_H_KNOB_LEFT,
	SKIN_SCROLLBAR_H_KNOB_RIGHT,
	SKIN_SCROLLBAR_H_KNOB_BODY,
	// and vertical
	SKIN_BUTTON_ARROW_UP,
	SKIN_BUTTON_ARROW_UP_PRESSED,
	SKIN_BUTTON_ARROW_DOWN,
	SKIN_BUTTON_ARROW_DOWN_PRESSED,
	SKIN_SCROLLBAR_V_BACKGROUND_TOP,
	SKIN_SCROLLBAR_V_BACKGROUND_BOTTOM,
	SKIN_SCROLLBAR_V_BACKGROUND,
	SKIN_SCROLLBAR_V_KNOB_TOP,
	SKIN_SCROLLBAR_V_KNOB_BOTTOM,
	SKIN_SCROLLBAR_V_KNOB_BODY,

	// squarebutton
	SKIN_BUTTON_CHECKBOX = 0,
	SKIN_BUTTON_CHECKBOX_PRESSED,

	// posbutton
	SKIN_BUTTON_POS = 0,
	SKIN_BUTTON_POS_PRESSED,

	// normal buttons
	SKIN_BUTTON_SIDE_LEFT = 0,
	SKIN_BUTTON_SIDE_RIGHT,
	SKIN_BUTTON_BODY,
	SKIN_BUTTON_SIDE_LEFT_PRESSED,
	SKIN_BUTTON_SIDE_RIGHT_PRESSED,
	SKIN_BUTTON_BODY_PRESSED
};


class gui_theme_t
{
public:
	// default button sizes
	//static KOORD_VAL gui_button_width;
	//static KOORD_VAL gui_button_height;

	static COLOR_VAL button_color_text;
	static COLOR_VAL button_color_disabled_text;

	// button sizes
	static koord gui_button_size;
	static koord gui_checkbox_size;
	static koord gui_posbutton_size;
	static koord gui_arrow_left_size;
	static koord gui_arrow_right_size;
	static koord gui_arrow_up_size;
	static koord gui_arrow_down_size;
	static koord gui_scrollbar_size;
	static koord gui_scrollknob_size;
	static koord gui_indicator_box_size;

	// titlebar height
	static KOORD_VAL gui_titlebar_height;

	// gadget size
	static KOORD_VAL gui_gadget_size;

	// dialog borders
	static KOORD_VAL gui_frame_left;
	static KOORD_VAL gui_frame_top;
	static KOORD_VAL gui_frame_right;
	static KOORD_VAL gui_frame_bottom;

	// space between two elements
	static KOORD_VAL gui_hspace;
	static KOORD_VAL gui_vspace;

	// and the indicator box dimension
	static KOORD_VAL gui_indicator_width;
	static KOORD_VAL gui_indicator_height;

	// default divider height
	static KOORD_VAL gui_divider_height;

	/// @name system colours used by gui components
	/// @{
	static COLOR_VAL theme_color_highlight;           //@< Colour to draw highlighs in dividers, buttons etc... MN_GREY4
	static COLOR_VAL theme_color_shadow;              //@< Colour to draw shadows in dividers, buttons etc...   MN_GREY0
	static COLOR_VAL theme_color_face;                //@< Colour to draw surface in buttons etc...             MN_GREY2
	static COLOR_VAL theme_color_button_text;         //@< Colour to draw button text
	static COLOR_VAL theme_color_text;                //@< Colour to draw interactive text checkbox etc...
	static COLOR_VAL theme_color_text_highlight;       //@< Colour to draw different text (mostly in labels)
	static COLOR_VAL theme_color_static_text;         //@< Colour to draw non interactive text in labels etc...
	static COLOR_VAL theme_color_disabled_text;       //@< Colour to draw disabled text in buttons etc...

	// Not implemented yet
	static COLOR_VAL theme_color_selected_text;       //@< Colour to draw selected text in edit box etc...
	static COLOR_VAL theme_color_selected_background; //@< Colour to draw selected text background in edit box etc...
	/// @}

public:
	// init the skin dimensions form file
	static void init_gui_images();

	// returns the current theme name
	static const char *get_current_theme();

	/**
	 * Reads theme configuration data, still not final
	 * searches a theme.tab inside the specified folder
	 * @author prissi
	 */
	static bool themes_init(const char *dir_name);
};
#endif
