/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../simcolor.h"
#include "../simevent.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../simmenu.h"
#include "../simskin.h"
#include "../sys/simsys.h"
#include "../simticker.h"
#include "simwin.h"
#include "../simintr.h"
#include "../simhalt.h"
#include "../simworld.h"

#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/tabfile.h"

#include "../descriptor/skin_desc.h"

#include "../obj/zeiger.h"


#include "../player/simplay.h"
#include "../tpl/inthashtable_tpl.h"
#include "../tpl/vector_tpl.h"
#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "map_frame.h"
#include "help_frame.h"
#include "messagebox.h"
#include "gui_frame.h"

// needed to restore/save them
#include "tool_selector.h"
#include "player_frame_t.h"
#include "money_frame.h"
#include "halt_info.h"
#include "convoi_detail_t.h"
#include "convoi_frame.h"
#include "convoi_info_t.h"
#include "line_management_gui.h"
#include "schedule_list.h"
#include "city_info.h"
#include "citylist_frame_t.h"
#include "message_frame_t.h"
#include "message_option_t.h"
#include "fabrik_info.h"
#include "themeselector.h"
#include "goods_frame_t.h"
#include "loadfont_frame.h"
#ifdef USE_FLUIDSYNTH_MIDI
#include "loadsoundfont_frame.h"
#endif
#include "scenario_info.h"
#include "depot_frame.h"
#include "depotlist_frame.h"
#include "halt_list_frame.h"
#include "vehiclelist_frame.h"
#include "curiositylist_frame_t.h"
#include "factorylist_frame_t.h"
#include "labellist_frame_t.h"
#include "display_settings.h"
#include "optionen.h"

#include "../simversion.h"

class inthashtable_tpl<ptrdiff_t,scr_coord> old_win_pos;

// hash-table: magic number to windowsize
class inthashtable_tpl<ptrdiff_t, scr_size> saved_windowsizes;


// I added a button to the map window to fix it's size to the best one.
// This struct is the flow back to the object of the refactoring.
class simwin_gadget_flags_t
{
public:
	simwin_gadget_flags_t(  ) : title(true), close( false ), help( false ), prev( false ), size( false ), next( false ), sticky( false ), gotopos( false ) { }

	bool title:1;
	bool close:1;
	bool help:1;
	bool prev:1;
	bool size:1;
	bool next:1;
	bool sticky:1;
	bool gotopos:1;
};

class simwin_t
{
public:
	scr_coord pos;          // Window position
	uint32 dauer;           // How long should the window stay open?
	uint8 wt;               // the flags for the window type
	ptrdiff_t magic_number; // either magic number or this pointer (which is unique too)
	gui_frame_t *gui;
	uint16 gadget_state;    // which buttons to hilite
	bool sticky;            // true if window is sticky
	bool rollup;
	bool dirty;

	simwin_gadget_flags_t flags; // See Above.

	simwin_t() : flags() {}

	bool operator== (const simwin_t &) const;
};

bool simwin_t::operator== (const simwin_t &other) const { return gui == other.gui; }


#define MAX_WIN (64)
static vector_tpl<simwin_t> wins(MAX_WIN);
static vector_tpl<simwin_t> kill_list(MAX_WIN);

static karte_t* wl = NULL; // Pointer to current world is set in win_set_world

static int top_win(int win, bool keep_state );
static void display_win(int win);


// tooltip data
static scr_coord_val tooltip_xpos = 0;
static scr_coord_val tooltip_ypos = 0;
static const char * tooltip_text = 0;
static std::string static_tooltip_text;

// For timed tooltip with initial delay and finite visible duration.
// Valid owners are required for timing. Invalid (NULL) owners disable timing.
static const void * tooltip_owner = 0;   // owner of the registered tooltip
static const void * tooltip_group = 0;   // group to which the owner belongs
static uint32 tooltip_register_time = 0; // time at which a tooltip is initially registered

static bool show_ticker=0;

/* if we are inside the event handler,
 * the window handler has gui pointer as value,
 * to defer destruction if this window
 */
static void *inside_event_handling = NULL;

// only this gui element can set a tooltip
static void *tooltip_element = NULL;

static bool destroy_framed_win(simwin_t *win);

//=========================================================================
// Helper Functions

#define REVERSE_GADGETS (!env_t::window_buttons_right)

/**
 * Display a window gadget
 */
static int display_gadget_box(sint8 code,
			      int const x, int const y,
			      PIXVAL lighter, PIXVAL darker,
			      bool const pushed)
{

	// If we have a skin, get gadget image data
	const image_t *img = NULL;
	if(  skinverwaltung_t::gadget  ) {
		// "x", "?", "=", "<<", ">>"
		img = skinverwaltung_t::gadget->get_image(code);
	}

	if(pushed) {
		// mark_rect_dirty_wc(x, y, D_GADGET_WIDTH, D_TITLEBAR_HEIGHT);
		display_fillbox_wh_clip_rgb(x, y, D_GADGET_WIDTH, D_TITLEBAR_HEIGHT, lighter, false);
	}

	// Do we have a gadget image?
	if(  img != NULL  ) {

		// Max Kielland: This center the gadget image and compensates for any left/top margins within the image to be backward compatible with older PAK sets.
		display_img_aligned(img->imageid, scr_rect(x, y, D_GADGET_WIDTH, D_TITLEBAR_HEIGHT), ALIGN_CENTER_H | ALIGN_CENTER_V, true);

	}
	else {
		const char *gadget_text = "#";
		static const char *gadget_texts[5]={ "X", "?", "=", "<", ">" };

		if(  code <= SKIN_BUTTON_NEXT  ) {
			gadget_text = gadget_texts[code];
		}
		else if(  code == SKIN_GADGET_GOTOPOS  ) {
			gadget_text = "*";
		}
		else if(  code == SKIN_GADGET_NOTPINNED  ) {
			gadget_text = "s";
		}
		else if(  code == SKIN_GADGET_PINNED  ) {
			gadget_text = "S";
		}
		display_proportional_rgb( x+4, y+4, gadget_text, ALIGN_LEFT, color_idx_to_rgb(COL_BLACK), false );
	}

	int side = x+REVERSE_GADGETS*D_GADGET_WIDTH-1;
	display_vline_wh_clip_rgb(side+1, y+1, D_TITLEBAR_HEIGHT-2, lighter, false);
	display_vline_wh_clip_rgb(side,   y+1, D_TITLEBAR_HEIGHT-2, darker,  false);

	// return width of gadget
	return D_GADGET_WIDTH;
}


static int display_gadget_boxes(
	simwin_gadget_flags_t* flags,
	int x, int y,
	PIXVAL lighter,
	PIXVAL darker,
	uint16 gadget_state,
	bool sticky_pushed,
	bool goto_pushed
) {
	int width = 0;
	const int k=(REVERSE_GADGETS?1:-1);

	// Only the close and sticky gadget can be pushed.
	if(  flags->close  ) {
		width += k*display_gadget_box( SKIN_GADGET_CLOSE, x + width, y, lighter, darker, gadget_state & (1<<SKIN_GADGET_CLOSE) );
	}
	if(  flags->size  ) {
		width += k*display_gadget_box( SKIN_GADGET_MINIMIZE, x + width, y, lighter, darker, gadget_state & (1<<SKIN_GADGET_MINIMIZE) );
	}
	if(  flags->help  ) {
		width += k*display_gadget_box( SKIN_GADGET_HELP, x + width, y, lighter, darker, gadget_state & (1<<SKIN_GADGET_HELP) );
	}
	if(  flags->prev  ) {
		width += k*display_gadget_box( SKIN_BUTTON_PREVIOUS, x + width, y, lighter, darker, gadget_state & (1<<SKIN_BUTTON_PREVIOUS) );
	}
	if(  flags->next  ) {
		width += k*display_gadget_box( SKIN_BUTTON_NEXT, x + width, y, lighter, darker, gadget_state & (1<<SKIN_BUTTON_NEXT) );
	}
	if(  flags->gotopos  ) {
		width += k*display_gadget_box( SKIN_GADGET_GOTOPOS, x + width, y, lighter, darker, goto_pushed  ||  (gadget_state & (1<<SKIN_GADGET_GOTOPOS)) );
	}
	if(  flags->sticky  ) {
		width += k*display_gadget_box( sticky_pushed ? SKIN_GADGET_PINNED : SKIN_GADGET_NOTPINNED, x + width, y, lighter, darker, gadget_state & (1<<SKIN_GADGET_NOTPINNED) );
	}

	return abs( width );
}


static sint8 decode_gadget_boxes(simwin_gadget_flags_t const * const flags, int const x,int const px)
{
	int offset = px-x;
	const int w=(REVERSE_GADGETS?-D_GADGET_WIDTH:D_GADGET_WIDTH);

//DBG_MESSAGE("simwin_gadget_et decode_gadget_boxes()","offset=%i, w=%i",offset, w );

	// Only the close gadget can be pushed.
	if( flags->close ) {
		if( offset >= 0  &&  offset<D_GADGET_WIDTH  ) {
//DBG_MESSAGE("simwin_gadget_et decode_gadget_boxes()","close" );
			return SKIN_GADGET_CLOSE;
		}
		offset += w;
	}
	if( flags->size ) {
		if( offset >= 0  &&  offset<D_GADGET_WIDTH  ) {
//DBG_MESSAGE("simwin_gadget_et decode_gadget_boxes()","size" );
			return SKIN_GADGET_MINIMIZE;
		}
		offset += w;
	}
	if( flags->help ) {
		if( offset >= 0  &&  offset<D_GADGET_WIDTH  ) {
//DBG_MESSAGE("simwin_gadget_et decode_gadget_boxes()","help" );
			return SKIN_GADGET_HELP;
		}
		offset += w;
	}
	if( flags->prev ) {
		if( offset >= 0  &&  offset<D_GADGET_WIDTH  ) {
			return SKIN_BUTTON_PREVIOUS;
		}
		offset += w;
	}
	if( flags->next ) {
		if( offset >= 0  &&  offset<D_GADGET_WIDTH  ) {
			return SKIN_BUTTON_NEXT;
		}
		offset += w;
	}
	if( flags->gotopos ) {
		if( offset >= 0  &&  offset<D_GADGET_WIDTH  ) {
			return SKIN_GADGET_GOTOPOS;
		}
		offset += w;
	}
	if( flags->sticky ) {
		if( offset >= 0  &&  offset<D_GADGET_WIDTH  ) {
			return SKIN_GADGET_NOTPINNED;
		}
		offset += w;
	}
	return SKIN_GADGET_COUNT;
}


static void win_draw_window_title(const scr_coord pos, const scr_size size,
		const FLAGGED_PIXVAL title_color,
		const char * const text,
		const FLAGGED_PIXVAL text_color,
		const koord3d welt_pos,
		const uint16 gadget_state,
		const bool sticky,
		const bool goto_pushed,
		simwin_gadget_flags_t &flags )
{
	PUSH_CLIP_FIT(pos.x, pos.y, size.w, size.h);

	PIXVAL lighter = display_blend_colors(title_color, color_idx_to_rgb(COL_WHITE), 25);
	PIXVAL darker  = display_blend_colors(title_color, color_idx_to_rgb(COL_BLACK), 25);

	// fill title bar with color
	display_fillbox_wh_clip_rgb(pos.x, pos.y, size.w, D_TITLEBAR_HEIGHT, title_color, false);

	// border of title bar
	display_fillbox_wh_clip_rgb( pos.x + 1, pos.y,                         size.w - 2, 1, lighter, false ); // top
	display_fillbox_wh_clip_rgb( pos.x + 1, pos.y + D_TITLEBAR_HEIGHT - 1, size.w - 2, 1, darker,  false ); // bottom

	display_vline_wh_clip_rgb( pos.x,              pos.y, D_TITLEBAR_HEIGHT, lighter, false ); // left
	display_vline_wh_clip_rgb( pos.x + size.w - 1, pos.y, D_TITLEBAR_HEIGHT, darker,  false ); // right

	// Draw the gadgets and then move left and draw text.
	flags.gotopos = (welt_pos != koord3d::invalid);
	int width = display_gadget_boxes( &flags, pos.x+(REVERSE_GADGETS?0:size.w-D_GADGET_WIDTH), pos.y, lighter, darker, gadget_state, sticky, goto_pushed );
	int titlewidth = display_proportional_clip_rgb( pos.x + (REVERSE_GADGETS?width+4:4), pos.y+(D_TITLEBAR_HEIGHT-LINEASCENT)/2, text, ALIGN_LEFT, text_color, false );
	if(  flags.gotopos  ) {
		display_proportional_clip_rgb( pos.x + (REVERSE_GADGETS?width+4:4)+titlewidth+8, pos.y+(D_TITLEBAR_HEIGHT-LINEASCENT)/2, welt_pos.get_2d().get_fullstr(), ALIGN_LEFT, text_color, false );
	}
	POP_CLIP();
}


//-------------------------------------------------------------------------

/**
 * Draw dragger widget
 */
static void win_draw_window_dragger(scr_coord pos, scr_size size)
{
	pos += size;
	if(  skinverwaltung_t::gadget  &&  skinverwaltung_t::gadget->get_image_id(SKIN_WINDOW_RESIZE)!=IMG_EMPTY  ) {
		const image_t *dragger = skinverwaltung_t::gadget->get_image(SKIN_WINDOW_RESIZE);
		display_color_img( dragger->get_id(), pos.x-dragger->get_pic()->w, pos.y-dragger->get_pic()->h, 0, false, false);
	}
	else {
		int dragger_size = min(D_DRAGGER_WIDTH, D_DRAGGER_HEIGHT);
		for(  int x=1;  x<dragger_size;  x++  ) {
			display_fillbox_wh_clip_rgb( pos.x-x, pos.y-dragger_size+x, x, 1, color_idx_to_rgb((x & 1) ? COL_BLACK : MN_GREY4), true);
		}
	}
}

// Functions to save & restore windowsize

ptrdiff_t guess_magic_number(simwin_t *win, ptrdiff_t magic = magic_none)
{
	if (win) {
		magic = win->magic_number;
	}
	// reduce player-wise magic numbers and magic numbers derived from handles
	const ptrdiff_t magic_pl[] = {
		magic_finances_t, magic_convoi_list, magic_convoi_list_filter, magic_line_list, magic_halt_list,
		magic_line_management_t, magic_ai_options_t, magic_ai_selector, magic_pwd_t, magic_jump, magic_headquarter, magic_headquarter + MAX_PLAYER_COUNT,
		magic_none,
		magic_convoi_info, magic_halt_info, magic_toolbar,
	};

	for (uint i=1; i<lengthof(magic_pl); i++) {
		if (magic_pl[i-1] == magic_none) {
			continue;
		}
		if (magic_pl[i-1] <= magic  &&  magic < magic_pl[i]) {
			return magic_pl[i-1];
		}
	}
	if (win == NULL) {
		return magic;
	}
	// try rdwr_id
	uint32 id = win->gui->get_rdwr_id();
	if (id != magic_reserved) {
		return id;
	}

	// anything outside the magic number's ranges will be ignored
	if (magic <= magic_reserved  ||  magic >= magic_max) {
		magic = magic_none;
	}
	return magic;
}

void save_windowsize(simwin_t *win)
{
	ptrdiff_t magic = guess_magic_number(win);
	if (magic != magic_none) {
		saved_windowsizes.set(magic, win->gui->get_windowsize() );
	}
}

scr_size get_stored_windowsize(simwin_t *win)
{
	ptrdiff_t magic = guess_magic_number(win);
	if (magic != magic_none) {
		return saved_windowsizes.get(magic);
	}
	return scr_size();
}

void rdwr_win_settings(loadsave_t *file)
{
	if (file->is_loading()) {
		saved_windowsizes.clear();
		ptrdiff_t magic;
		do {
			sint64 rd_magic;
			file->rdwr_longlong(rd_magic);
			magic = (ptrdiff_t)rd_magic;
			if (magic != magic_none) {
				scr_size s;
				file->rdwr_long(s.w);
				file->rdwr_long(s.h);
				// ignore stuff that will not be used anymore
				if (magic == guess_magic_number(NULL, magic)) {
					saved_windowsizes.put(magic, s);
				}
			}
		} while (magic != magic_none);
	}
	else {
		typedef inthashtable_tpl<ptrdiff_t, scr_size> stupid_table_t;
		FOR(stupid_table_t, it, saved_windowsizes) {
			sint64 m = it.key;
			file->rdwr_longlong(m);
			file->rdwr_long(it.value.w);
			file->rdwr_long(it.value.h);
		}
		sint64 m = magic_none;
		file->rdwr_longlong(m);
	}
}
//=========================================================================



// returns the window (if open) otherwise zero
gui_frame_t *win_get_magic(ptrdiff_t magic)
{
	if(  magic!=-1  &&  magic!=0  ) {
		// there is at most one window with a positive magic number
		FOR( vector_tpl<simwin_t>, const& i, wins ) {
			if(  i.magic_number == magic  ) {
				// if 'special' magic number, return it
				return i.gui;
			}
		}
	}
	return NULL;
}


// sets the magic of a gui_frame_t (needed during reload of windows)
bool win_set_magic( gui_frame_t *gui, ptrdiff_t magic )
{
	if(  magic!=-1  &&  magic!=0  ) {
		// there is at most one window with a positive magic number
		FOR( vector_tpl<simwin_t>, &i, wins ) {
			if(  i.gui == gui  ) {
				i.magic_number = magic;
				return true;
			}
		}
	}
	return false;
}


/**
 * Returns top window
 */
gui_frame_t *win_get_top()
{
	return wins.empty() ? 0 : wins.back().gui;
}


/**
 * returns the focused component of the top window
 */
gui_component_t *win_get_focus()
{
	return wins.empty() ? 0 : wins.back().gui->get_focus();
}


bool win_is_textinput()
{
	if(  gui_component_t *comp = win_get_focus()  ) {
		return dynamic_cast<gui_textinput_t *>(comp) || dynamic_cast<gui_combobox_t *>(comp)  ||  dynamic_cast<gui_numberinput_t *>(comp);
	}
	return false;
}


uint32 win_get_open_count()
{
	return wins.get_count();
}


gui_frame_t* win_get_index(uint32 i)
{
	if (i < wins.get_count()) {
		return wins[i].gui;
	}
	return NULL;
}


// brings a window to front, if open
bool top_win( const gui_frame_t *gui, bool keep_rollup )
{
	for(  uint i=0;  i<wins.get_count()-1;  i++  ) {
		if(wins[i].gui==gui) {
			top_win(i,keep_rollup);
			return true;
		}
	}
	// not open
	return false;
}


/**
 * Checks if a window is a top level window
 */
bool win_is_top(const gui_frame_t *ig)
{
	return !wins.empty() && wins.back().gui == ig;
}


// window functions


// save/restore all dialogues
void rdwr_all_win(loadsave_t *file)
{
	if(  file->is_version_atleast(120, 8)  ) {
		if(  file->is_saving()  ) {
			FOR(vector_tpl<simwin_t>, & i, wins) {
				uint32 id = i.gui->get_rdwr_id();
				if(  id!=magic_reserved  ) {
					file->rdwr_long( id );
					i.pos.rdwr(file);
					file->rdwr_byte(i.wt);
					file->rdwr_bool(i.sticky);
					file->rdwr_bool(i.rollup);
					i.gui->rdwr(file);
				}
			}
			uint32 end = magic_none;
			file->rdwr_long( end );
		}
		else {
			// restore windows
			while(1) {
				uint32 id;
				file->rdwr_long(id);
				// create the matching
				gui_frame_t *w = NULL;
				switch(magic_numbers(id)) {

					// end of dialogues
					case magic_none: return;

					// actual dialogues to restore
					case magic_convoi_info:    w = new convoi_info_t(); break;
					case magic_themes:         w = new themeselector_t(); break;
					case magic_halt_info:      w = new halt_info_t(); break;
					case magic_reliefmap:      w = new map_frame_t(); break;
					case magic_ki_kontroll_t:  w = new ki_kontroll_t(); break;
					case magic_line_schedule_rdwr_dummy: w = new line_management_gui_t(); break;
					case magic_city_info_t:    w = new city_info_t(); break;
					case magic_messageframe:   w = new message_frame_t(); break;
					case magic_message_options: w = new message_option_t(); break;
					case magic_factory_info:   w = new fabrik_info_t(); break;
					case magic_goodslist:      w = new goods_frame_t(); break;
					case magic_font:           w = new loadfont_frame_t(); break;
#ifdef USE_FLUIDSYNTH_MIDI
					case magic_soundfont:      w = new loadsoundfont_frame_t(); break;
#endif
					case magic_scenario_info:  w = new scenario_info_t(); break;
					case magic_depot:          w = new depot_frame_t(); break;
					case magic_convoi_list:    w = new convoi_frame_t(); break;
					case magic_depotlist:      w = new depotlist_frame_t(); break;
					case magic_vehiclelist:    w = new vehiclelist_frame_t(); break;
					case magic_halt_list:      w = new halt_list_frame_t(); break;
					case magic_citylist_frame_t: w = new citylist_frame_t(); break;
					case magic_curiositylist:  w = new curiositylist_frame_t(); break;
					case magic_factorylist:    w = new factorylist_frame_t(); break;
					case magic_labellist:      w = new labellist_frame_t(); break;
					case magic_color_gui_t:    w = new color_gui_t(); break;
					case magic_optionen_gui_t: w = new optionen_gui_t(); break;

					default:
						if(  id>=magic_finances_t  &&  id<magic_finances_t+MAX_PLAYER_COUNT  ) {
							w = new money_frame_t( wl->get_player(id-magic_finances_t) );
						}
						else if(  id>=magic_line_management_t  &&  id<magic_line_management_t+MAX_PLAYER_COUNT  ) {
							w = new schedule_list_gui_t( wl->get_player(id-magic_line_management_t) );
						}
						else if(  id>=magic_toolbar  &&  id<magic_toolbar+256  ) {
							tool_t::toolbar_tool[id-magic_toolbar]->update(wl->get_active_player());
							w = tool_t::toolbar_tool[id-magic_toolbar]->get_tool_selector();
						}
						else {
							dbg->error( "rdwr_all_win()", "No idea how to restore magic 0x%X", id );
							return;
						}
				}
				/* sequence is now the same for all dialogues
				 * restore coordinates
				 * create window
				 * read state
				 * restore content
				 * restore state - gui_frame_t::rdwr() might create its own window ->> want to restore state to that window
				 */
				scr_coord p;
				p.rdwr(file);
				uint8 win_type;
				file->rdwr_byte( win_type );
				create_win( p.x, p.y, w, (wintype)win_type, id, true );
				bool sticky, rollup;
				file->rdwr_bool( sticky );
				file->rdwr_bool( rollup );
				// now load the window
				uint32 count = wins.get_count();
				w->rdwr( file );

				// restore sticky / rollup status
				// ensure that the new status is to currently loaded window
				if (wins.get_count() >= count) {
					wins.back().sticky = sticky;
					wins.back().rollup = rollup;
				}
			}
		}
	}
}


int create_win(gui_frame_t* const gui, wintype const wt, ptrdiff_t const magic)
{
	return create_win( -1, -1, gui, wt, magic);
}


/* tries to get a window on the screen
 * without titlebar hidden by any menubar/satus bar elements
 */
void win_clamp_xywh_position( scr_coord_val &x, scr_coord_val &y, scr_size wh, bool move_topleft )
{
	scr_coord_val add_menuwidth = env_t::iconsize.w;
	scr_coord_val add_menuheight = env_t::iconsize.h;

	scr_rect clip_rr(0, add_menuheight, display_get_width(), display_get_height() - add_menuwidth - win_get_statusbar_height());
	switch (env_t::menupos) {
	case MENU_TOP:
		// rect default
		break;
	case MENU_BOTTOM:
		clip_rr = scr_rect(0, win_get_statusbar_height(), display_get_width(), clip_rr.h );
		break;
	case MENU_LEFT:
		clip_rr = scr_rect(add_menuwidth, 0, display_get_width() - add_menuwidth, display_get_height() - win_get_statusbar_height());
		break;
	case MENU_RIGHT:
		clip_rr = scr_rect(0, 0, display_get_width() - add_menuwidth, display_get_height() - win_get_statusbar_height());
		break;
	}


	// should we move to be fully on screen?
	if (move_topleft) {
		if (x + wh.w > clip_rr.x + clip_rr.w) {
			x = clip_rr.x + clip_rr.w - wh.w;
		}
		if (y + wh.h > clip_rr.y + clip_rr.h) {
			y = clip_rr.y + clip_rr.h - wh.h;
		}
	}

	// now do not hide titlebar by menubar
	x = max(x, clip_rr.x);
	y = max(y, clip_rr.y);
}




int create_win(scr_coord_val x, scr_coord_val y, gui_frame_t* const gui, wintype const wt, ptrdiff_t const magic, bool move_to_full_view)
{
	assert(gui!=NULL  &&  magic!=0);

	if(  gui_frame_t *win = win_get_magic(magic)  ) {
		if(  env_t::second_open_closes_win  ) {
			destroy_win( win );
			if(  !( wt & w_do_not_delete )  ) {
				delete gui;
			}
		}
		else {
			top_win( win );
		}
		return -1;
	}

	if(  x==-1  &&  y==-1  &&  env_t::remember_window_positions  ) {
		// look for window in hash table
		if(  scr_coord *k = old_win_pos.access(magic)  ) {
			x = k->x;
			y = k->y;
		}
	}

	/* if there are too many handles (likely in large games)
	 * we search for any error/news message at the bottom of the stack and delete it
	 * => newer information might be more important ...
	 * This is a stopgap to avoid simutrans to freeze due to many error etc windows
	 */
	if(  wins.get_count()==MAX_WIN  ) {
		// we try to remove one of the lowest news windows (magic_none)
		for(  uint i=0;  i<MAX_WIN;  i++  ) {
			if(  wins[i].magic_number == magic_none  &&  dynamic_cast<news_window *>(wins[i].gui)!=NULL  ) {
				destroy_win( wins[i].gui );
				break;
			}
		}
	}

	if(  wins.get_count() < MAX_WIN  ) {

		if (!wins.empty()) {
			// mark old dirty
			const scr_size size = wins.back().gui->get_windowsize();
			mark_rect_dirty_wc( wins.back().pos.x - 1, wins.back().pos.y - 1, wins.back().pos.x + size.w + 2, wins.back().pos.y + size.h + 2 + D_TITLEBAR_HEIGHT ); // -1, +2 for env_t::window_frame_active
		}

		wins.append( simwin_t() );
		simwin_t& win = wins.back();

		// Make Sure Closes Aren't Forgotten.
		// Must Reset as the entries and thus flags are reused
		win.flags.close = true;
		win.flags.title = gui->has_title();
		win.flags.help = ( gui->get_help_filename() != NULL );
		win.flags.prev = gui->has_prev();
		win.flags.next = gui->has_next();
		win.flags.size = gui->has_min_sizer();
		win.flags.sticky = gui->has_sticky();
		win.gui = gui;

		// take care of time delete windows ...
		win.wt    = wt & w_time_delete ? w_info : wt;
		win.dauer = (wt&w_time_delete) ? MESG_WAIT : -1;
		win.magic_number = magic;
		win.gadget_state = 0;
		win.rollup = false;
		win.sticky = false;
		win.dirty = true;

		// Notify window to be shown
		assert(gui);
		event_t ev;

		ev.ev_class = INFOWIN;
		ev.ev_code = WIN_OPEN;
		ev.mx = 0;
		ev.my = 0;
		ev.cx = 0;
		ev.cy = 0;
		ev.button_state = 0;

		void *old = inside_event_handling;
		inside_event_handling = gui;
		gui->infowin_event(&ev);
		inside_event_handling = old;

		// restore windowsize
		scr_size stored = get_stored_windowsize(&win);

		if (stored == scr_size()) {
			// not stored, take current
			stored = gui->get_windowsize();
		}

		// use default width
		stored.clip_lefttop(scr_size(D_DEFAULT_WIDTH, D_DEFAULT_HEIGHT));
		// clip to display size
		stored.clip_rightbottom( scr_size(display_get_width(), display_get_height() - env_t::iconsize.h - win_get_statusbar_height() ) );

		if (stored != gui->get_windowsize()) {
			// send tailored resize event
			scr_size delta = stored - gui->get_windowsize();
			event_t wev;
			wev.ev_class = WINDOW_RESIZE;
			wev.ev_code = 0;
			wev.mx = delta.w;
			wev.my = delta.h;

			inside_event_handling = gui;
			gui->infowin_event(&wev);
			inside_event_handling = old;
		}

		// try to go next to mouse bar
		if (x == -1) {
			move_to_full_view = true;
			x = get_mouse_x() - gui->get_windowsize().w / 2;
			y = get_mouse_y() - gui->get_windowsize().h - get_tile_raster_width()/4;
		}

		// make sure window is on screen
		win_clamp_xywh_position(x, y, gui->get_windowsize(), move_to_full_view);


		win.pos = scr_coord(x,y);
		win.dirty = true;
		return wins.get_count();
	}
	else {
		if(  !( wt & w_do_not_delete )  ) {
			delete gui;
		}
		return -1;
	}
}


/* sometimes a window cannot destroyed while it is still handled;
 * in those cases it will added to kill list and it is only destructed
 * by this function
 */
static void process_kill_list()
{
	FOR(vector_tpl<simwin_t>, & i, kill_list) {
		if (inside_event_handling != i.gui) {
			destroy_framed_win(&i); // we call this first, otherwise the focus may not be recognized
			wins.remove(i);
		}
	}
	kill_list.clear();
}


/**
 * Destroy a framed window
 */
static bool destroy_framed_win(simwin_t *wins)
{
	bool r = true;

	// mark dirty
	const scr_size size = wins->gui->get_windowsize();
	mark_rect_dirty_wc( wins->pos.x - 1, wins->pos.y - 1, wins->pos.x + size.w + 2, wins->pos.y + size.h + 2 ); // -1, +2 for env_t::window_frame_active

	// save windowsize for later
	save_windowsize(wins);

	// save pointer to gui window: might be modified in event handling,
	// or could be modified if wins points to value in kill_list and kill_list is modified! nasty surprise
	gui_frame_t* gui = wins->gui;
	if(  gui  ) {
		event_t ev;

		ev.ev_class = INFOWIN;
		ev.ev_code = WIN_CLOSE;
		ev.mx = 0;
		ev.my = 0;
		ev.cx = 0;
		ev.cy = 0;
		ev.button_state = 0;

		void *old = inside_event_handling;
		inside_event_handling = gui;
		wins->gui->infowin_event(&ev);
		inside_event_handling = old;
	}

	if(  (wins->wt&w_do_not_delete) == 0  ) {
		if(  wins->gui == gui  ) {
			// remove from kill list first, otherwise delete will be called again on that window
			for(  uint32 j = 0;  j < kill_list.get_count();  j++  ) {
				if(  kill_list[j].gui == gui  ) {
					kill_list.remove_at(j);
					break;
				}
			}
			delete gui;
		}
		else {
			// wins likely modified during event handling. Assume this was just a schedule window destroying itself, and top_win() therefore modifying wins.
			// return false to signal destroy_all_win() that wins was already modified.
			r = false;
		}
	}
	// set dirty flag to refill background
	if(  wl  ) {
		wl->set_background_dirty();
	}
	return r;
}


bool destroy_win(const ptrdiff_t magic)
{
	const gui_frame_t *gui = win_get_magic(magic);
	if(gui) {
		return destroy_win( gui );
	}
	return false;
}


bool destroy_win(const gui_frame_t *gui)
{
	if(wins.get_count() > 1  &&  wins.back().gui == gui) {
		// destroy topped window, top the next window before removing
		// do it here as top_win manipulates the win vector
		top_win(wins.get_count() - 2, true);
	}

	for(  uint i=0;  i<wins.get_count();  i++  ) {
		if(wins[i].gui == gui) {
			if(inside_event_handling==wins[i].gui) {
				kill_list.append_unique(wins[i]);
			}
			else {
				simwin_t win = wins[i];
				wins.remove_at(i);
				if(  win.magic_number < magic_max  ) {
					// save last pos
					if(  scr_coord *k = old_win_pos.access(win.magic_number)  ) {
						*k = win.pos;
					}
					else {
						old_win_pos.put( win.magic_number, win.pos );
					}
				}
				destroy_framed_win(&win);
			}
			return true;
		}
	}
	return false;
}


void destroy_all_win(bool destroy_sticky)
{
	for(  sint32 curWin = 0;  curWin < (sint32)wins.get_count();  curWin++  ) {
		if(  destroy_sticky  ||  !wins[curWin].sticky  ) {
			if(  inside_event_handling == wins[curWin].gui  ) {
				// only add this, if not already added
				kill_list.append_unique(wins[curWin]);
				// compact the window list
				wins.remove_at(curWin);
				curWin--;
			}
			else {
				if(  destroy_framed_win(&wins[curWin])  ) {
					// compact the window list
					wins.remove_at(curWin);
					curWin--;
				}
				// else wins was already modified - assume by the schedule window closing itself during event handling
			}
		}
	}
}

void rollup_all_win()
{
	bool all_rolldown_flag = true; // If any dialog is open, all rolldown will not be performed
	for (  sint32 curWin = 0; curWin < (sint32)wins.get_count(); curWin++  ) {
		if (  !wins[curWin].rollup  ) {
			wins[curWin].rollup = true;
			gui_frame_t *gui = wins[curWin].gui;
			scr_size size = gui->get_windowsize();
			mark_rect_dirty_wc( wins[curWin].pos.x, wins[curWin].pos.y, wins[curWin].pos.x+size.w, wins[curWin].pos.y+size.h );
			all_rolldown_flag = false;
		}
	}

	if (  !all_rolldown_flag  ) {
		wl->set_background_dirty();
	}
	else if (  wins.get_count()  ) {
		rolldown_all_win();
	}
}

void rolldown_all_win()
{
	for (  sint32 curWin = 0; curWin < (sint32)wins.get_count(); curWin++  ) {
		wins[curWin].rollup = false;
		gui_frame_t *gui = wins[curWin].gui;
		scr_size size = gui->get_windowsize();
		mark_rect_dirty_wc( wins[curWin].pos.x, wins[curWin].pos.y, wins[curWin].pos.x+size.w, wins[curWin].pos.y+size.h );
	}
}


int top_win(int win, bool keep_state )
{
	if(  (uint32)win==wins.get_count()-1  ) {
		return win;
	} // already topped

	// mark old dirty
	scr_size size = wins.back().gui->get_windowsize();
	mark_rect_dirty_wc( wins.back().pos.x - 1, wins.back().pos.y - 1, wins.back().pos.x + size.w + 2, wins.back().pos.y + size.h + 2 ); // -1, +2 for env_t::window_frame_active
	wins.back().dirty = true;

	simwin_t tmp = wins[win];
	wins.remove_at(win);
	if(  !keep_state  ) {
		tmp.rollup = false; // make visible when topping
	}
	wins.append(tmp);

	 // mark new dirty
	size = wins.back().gui->get_windowsize();
	mark_rect_dirty_wc( wins.back().pos.x - 1, wins.back().pos.y - 1, wins.back().pos.x + size.w + 2, wins.back().pos.y + size.h + 2 ); // -1, +2 for env_t::window_frame_active

	event_t ev;

	ev.ev_class = INFOWIN;
	ev.ev_code = WIN_TOP;
	ev.mx = 0;
	ev.my = 0;
	ev.cx = 0;
	ev.cy = 0;
	ev.button_state = 0;

	void *old = inside_event_handling;
	inside_event_handling = tmp.gui;
	tmp.gui->infowin_event(&ev);
	inside_event_handling = old;

	return wins.get_count()-1;
}


void display_win(int win)
{
	// ok, now process it
	gui_frame_t *comp = wins[win].gui;
	scr_size size = comp->get_windowsize();
	scr_coord pos = wins[win].pos;
	FLAGGED_PIXVAL title_color = (comp->get_titlecolor()&0xFFFF);
	FLAGGED_PIXVAL text_color = env_t::front_window_text_color;
	if(  (unsigned)win!=wins.get_count()-1  ) {
		// not top => darker
		title_color = display_blend_colors(title_color, color_idx_to_rgb(COL_BLACK), env_t::bottom_window_darkness);
		text_color = env_t::bottom_window_text_color;
	}
	bool need_dragger = comp->get_resizemode() != gui_frame_t::no_resize;

	// HACK  So draw will know if gadget is needed.
	wins[win].flags.help = ( comp->get_help_filename() != NULL );
	if(  wins[win].flags.title  ) {
		win_draw_window_title(wins[win].pos,
				size,
				title_color,
				comp->get_name(),
				text_color,
				comp->get_weltpos(false),
				wins[win].gadget_state,
				wins[win].sticky,
				comp->is_weltpos(),
				wins[win].flags );
	}
	if(  wins[win].dirty  ) {
		// not sure this is still a useful call
		mark_rect_dirty_wc( wins[win].pos.x, wins[win].pos.y, wins[win].pos.x+size.w+1, wins[win].pos.y+2 );
		wins[win].dirty = false;
	}
	// mark top window, if requested
	if(env_t::window_frame_active  &&  (unsigned)win==wins.get_count()-1) {
		if(!wins[win].rollup) {
			display_ddd_box_clip_rgb( wins[win].pos.x-1, wins[win].pos.y-1, size.w+2, size.h+2, title_color, title_color);
		}
		else {
			display_ddd_box_clip_rgb( wins[win].pos.x-1, wins[win].pos.y-1, size.w+2, D_TITLEBAR_HEIGHT + 2, title_color, title_color);
		}
	}
	if(!wins[win].rollup) {
		comp->draw(wins[win].pos, size);

		// draw dragger
		if(need_dragger) {
			win_draw_window_dragger( pos, size);
		}
	}
}


void display_all_win()
{
	// first: empty kill list
	process_kill_list();

	// check which window can set tooltip
	const sint16 x = get_mouse_x();
	const sint16 y = get_mouse_y();
	tooltip_element = NULL;
	for(  uint32 i = wins.get_count(); i-- != 0;  ) {
		if(  (!wins[i].rollup  &&  wins[i].gui->is_hit(x-wins[i].pos.x,y-wins[i].pos.y))  ||
		     (wins[i].rollup  &&  x>=wins[i].pos.x  &&  x<wins[i].pos.x+wins[i].gui->get_windowsize().w  &&  y>=wins[i].pos.y  &&  y<wins[i].pos.y+D_TITLEBAR_HEIGHT)
		) {
			// tooltips are only allowed for this window
			tooltip_element = wins[i].gui;
			break;
		}
	}

	// then display windows
	for(  uint i=0;  i<wins.get_count();  i++  ) {
		void *old_gui = inside_event_handling;
		inside_event_handling = wins[i].gui;
		display_win(i);
		inside_event_handling = old_gui;
	}
}


void win_rotate90( sint16 new_ysize )
{
	FOR(vector_tpl<simwin_t>, const& i, wins) {
		i.gui->map_rotate90(new_ysize);
	}
}


static void remove_old_win()
{
	// Destroy (close) old window when life time expire
	for(  int i=wins.get_count()-1;  i>=0;  i=min(i,(int)wins.get_count())-1  ) {
		if(wins[i].dauer > 0) {
			wins[i].dauer --;
			if(wins[i].dauer == 0) {
				destroy_win( wins[i].gui );
			}
		}
	}
}


static inline void snap_check_distance( scr_coord_val *r, const scr_coord_val a, const scr_coord_val b )
{
	if(  abs(a-b)<=env_t::window_snap_distance  ) {
		*r = a;
	}
}


void snap_check_win( const int win, scr_coord *r, const scr_coord from_pos, const scr_coord from_size, const scr_coord to_pos, const scr_coord to_size )
{
	bool resize;
	if(  from_size==to_size  &&  from_pos!=to_pos  ) { // check if we're moving
		resize = false;
	}
	else if(  from_size!=to_size  &&  from_pos==from_pos  ) { // or resizing the window
		resize = true;
	}
	else {
		return; // or nothing to do.
	}

	const int wins_count = wins.get_count();

	for(  int i=0;  i<=wins_count;  i++  ) {
		if(  i==win  ) {
			// Don't snap to self
			continue;
		}

		scr_coord other_pos;
		scr_coord other_size;

		if(  i==wins_count  ) {
			// Allow snap to screen edge
			other_pos.x = (env_t::menupos==MENU_LEFT)*env_t::iconsize.w;
			other_pos.y = (env_t::menupos==MENU_TOP)*env_t::iconsize.h + (env_t::menupos==MENU_BOTTOM)*win_get_statusbar_height();
			other_size.x = display_get_width() - other_pos.x - (env_t::menupos==MENU_RIGHT)*env_t::iconsize.w;
			other_size.y = display_get_height()-win_get_statusbar_height()-env_t::iconsize.h;
			if(  show_ticker  ) {
				other_size.y -= TICKER_HEIGHT;
			}
		}
		else {
			// Snap to other window
			other_size = wins[i].gui->get_windowsize();
			other_pos = wins[i].pos;
			if(  wins[i].rollup  ) {
				other_size.y = D_TITLEBAR_HEIGHT;
			}
		}

		// my bottom below other top  and  my top above other bottom  ---- in same vertical band
		if(  from_pos.y+from_size.y >= other_pos.y  &&  from_pos.y <= other_pos.y+other_size.y  ) {
			if(  resize  ) {
				// other right side and my new right side within snap
				snap_check_distance( &r->x, other_pos.x+other_size.x-from_pos.x, to_size.x );  // snap right - align right sides
			}
			else {
				// other right side and my new right side within snap
				snap_check_distance( &r->x, other_pos.x+other_size.x-from_size.x, to_pos.x );  // snap right - align right sides

				// other left side and my new left side within snap
				snap_check_distance( &r->x, other_pos.x, to_pos.x );  // snap left - align left sides
			}
		}

		// my new bottom below other top  and  my new top above other bottom  ---- in same vertical band
		if(  resize  ) {
			if(  from_pos.y+to_size.y>other_pos.y  &&  from_pos.y<other_pos.y+other_size.y  ) {
				// other left side and my new right side within snap
				snap_check_distance( &r->x, other_pos.x-from_pos.x, to_size.x );  // snap right - align my right to other left
			}
		}
		else {
			if(  to_pos.y+from_size.y>other_pos.y  &&  to_pos.y<other_pos.y+other_size.y  ) {
				// other left side and my new right side within snap
				snap_check_distance( &r->x, other_pos.x-from_size.x, to_pos.x );  // snap right - align my right to other left

				// other right side and my new left within snap
				snap_check_distance( &r->x, other_pos.x+other_size.x, to_pos.x );  // snap left - align my left to other right
			}
		}

		// my right side right of other left side  and  my left side left of other right side  ---- in same horizontal band
		if(  from_pos.x+from_size.x>=other_pos.x  &&  from_pos.x<=other_pos.x+other_size.x  ) {
			if(  resize  ) {
				// other bottom and my new bottom within snap
				snap_check_distance( &r->y, other_pos.y+other_size.y-from_pos.y, to_size.y );  // snap down - align bottoms
			}
			else {
				// other bottom and my new bottom within snap
				snap_check_distance( &r->y, other_pos.y+other_size.y-from_size.y, to_pos.y );  // snap down - align bottoms

				// other top and my new top within snap
				snap_check_distance( &r->y, other_pos.y, to_pos.y );  // snap up - align tops
			}
		}

		// my new right side right of other left side  and  my new left side left of other right side  ---- in same horizontal band
		if (  resize  ) {
			if(  from_pos.x+to_size.x>other_pos.x  &&  from_pos.x<other_pos.x+other_size.x  ) {
				// other top and my new bottom within snap
				snap_check_distance( &r->y, other_pos.y-from_pos.y, to_size.y );  // snap down - align my bottom to other top
			}
		}
		else {
			if(  to_pos.x+from_size.x>other_pos.x  &&  to_pos.x<other_pos.x+other_size.x  ) {
				// other top and my new bottom within snap
				snap_check_distance( &r->y, other_pos.y-from_size.y, to_pos.y );  // snap down - align my bottom to other top

				// other bottom and my new top within snap
				snap_check_distance( &r->y, other_pos.y+other_size.y, to_pos.y );  // snap up - align my top to other bottom
			}
		}
	}
}


void move_win(int win, event_t *ev)
{
	const scr_coord mouse_from( ev->cx, ev->cy );
	const scr_coord mouse_to( ev->mx, ev->my );

	const scr_coord from_pos = wins[win].pos;
	scr_coord from_size = scr_coord(wins[win].gui->get_windowsize().w,wins[win].gui->get_windowsize().h);
	if(  wins[win].rollup  ) {
		from_size.y = D_TITLEBAR_HEIGHT + 2;
	}

	scr_coord to_pos = wins[win].pos+(mouse_to-mouse_from);
	const scr_coord to_size = from_size;

	if(  env_t::window_snap_distance>0  ) {
		snap_check_win( win, &to_pos, from_pos, from_size, to_pos, to_size );
	}

	// CLIP(wert,min,max)
	win_clamp_xywh_position(to_pos.x, to_pos.y, wins[win].gui->get_windowsize(), false);

	// delta is actual window movement.
	const scr_coord delta = to_pos - from_pos;

	wins[win].pos += delta;
	// need to mark all of old and new positions dirty. -1, +2 for env_t::window_frame_active
	mark_rect_dirty_wc( from_pos.x - 1, from_pos.y - 1, from_pos.x + from_size.x + 2, from_pos.y + from_size.y + 2 );
	wins[win].dirty = true;
	// set dirty flag to refill background
	if(wl) {
		wl->set_background_dirty();
	}

	change_drag_start( delta.x, delta.y );
}


void resize_win(int win, event_t *ev)
{
	event_t wev = *ev;
	wev.ev_class = WINDOW_RESIZE;
	wev.ev_code = 0;

	const scr_coord mouse_from( wev.cx, wev.cy );
	const scr_coord mouse_to( wev.mx, wev.my );

	const scr_coord from_pos = wins[win].pos;
	const scr_coord from_size = scr_coord(wins[win].gui->get_windowsize().w,wins[win].gui->get_windowsize().h);

	const scr_coord to_pos = from_pos;
	scr_coord to_size = from_size+(mouse_to-mouse_from);

	if(  env_t::window_snap_distance>0  ) {
		snap_check_win( win, &to_size, from_pos, from_size, to_pos, to_size );
	}

	// since we may be smaller afterwards
	mark_rect_dirty_wc( from_pos.x - 1, from_pos.y - 1, from_pos.x + from_size.x + 2, from_pos.y + from_size.y + 2 ); // -1, +2 for env_t::window_frame_active
	// set dirty flag to refill background
	if(wl) {
		wl->set_background_dirty();
	}

	// adjust event mouse scr_coord per snap
	wev.mx = wev.cx + to_size.x - from_size.x;
	wev.my = wev.cy + to_size.y - from_size.y;

	wins[win].gui->infowin_event( &wev );
}


// returns true, if gui is a open window handle
bool win_is_open(gui_frame_t *gui)
{
	FOR(vector_tpl<simwin_t>, const& i, wins) {
		if (i.gui == gui) {
			FOR(vector_tpl<simwin_t>, const& j, kill_list) {
				if (j.gui == gui) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
}


scr_coord const& win_get_pos(gui_frame_t const* const gui)
{
	for(  uint32 i = wins.get_count(); i-- != 0;  ) {
		if(  wins[i].gui == gui  ) {
			return wins[i].pos;
		}
	}
	static scr_coord const bad(-1, -1);
	return bad;
}


void win_set_pos(gui_frame_t *gui, int x, int y)
{
	for(  uint32 i = wins.get_count(); i-- != 0;  ) {
		if(  wins[i].gui == gui  ) {
			wins[i].pos.x = x;
			wins[i].pos.y = y;
			wins[i].dirty = true;
			return;
		}
	}
}


bool last_drag_is_caught = false;

// since check_pos_win is processed before i.e. scrolling map
// we do not want to catch the mouse, if we use it already
void catch_dragging()
{
	last_drag_is_caught = true;
}

/*
 * main window event handler
 */
bool check_pos_win(event_t *ev)
{
	static int is_resizing = -1;
	static int is_moving = -1;

	bool swallowed = false;

	const int x = ev->ev_class==EVENT_MOVE?ev->mx:ev->cx;
	const int y = ev->ev_class==EVENT_MOVE?ev->my:ev->cy;

	if( last_drag_is_caught ) {
		if( ev->ev_class == EVENT_DRAG ) {
			// somebody else drags already => do nothing
			return false;
		}
		if( ev->ev_class == EVENT_RELEASE ) {
			// we will handle dragging events again after this
			last_drag_is_caught = false;
			return false;
		}
	}

	// for the moment, no none events
	if (ev->ev_class == EVENT_NONE) {
		process_kill_list();
		return false;
	}

	// we stop resizing once the user releases the button
	if(  (is_resizing>=0  ||  is_moving>=0)  &&  (IS_LEFTRELEASE(ev)  ||  (ev->button_state&1)==0)  ) {
		is_resizing = -1;
		is_moving = -1;
		if(  IS_LEFTRELEASE(ev)  ) {
			// should not proceed, otherwise the left release event will be fed to other components;
			// return true (i.e. event swallowed) to prevent propagation back to the main view
			return true;
		}
	}

	// disable any active tooltip upon mouse click by forcing expiration of tooltip duration
	if(  ev->ev_class==EVENT_CLICK  ) {
		tooltip_register_time = 0;
	}

	// click in main menu?
	scr_coord menuoffset((env_t::menupos == MENU_RIGHT) * (display_get_width() - env_t::iconsize.w), (env_t::menupos == MENU_BOTTOM) * (display_get_height() - env_t::iconsize.h) - D_TITLEBAR_HEIGHT);
	if (!tool_t::toolbar_tool.empty()  &&
		tool_t::toolbar_tool[0]->get_tool_selector()  &&
		tool_t::toolbar_tool[0]->get_tool_selector()->is_hit(x-menuoffset.x, y-menuoffset.y)  &&
		y > menuoffset.y+D_TITLEBAR_HEIGHT  &&
		ev->ev_class != EVENT_KEYBOARD) {

		event_t wev = *ev;
		wev.move_origin(menuoffset);

		inside_event_handling = tool_t::toolbar_tool[0];
		tool_t::toolbar_tool[0]->get_tool_selector()->infowin_event( &wev );
		inside_event_handling = NULL;

		// swallow event
		return true;
	}

	// cursor event only go to top window (but not if rolled up)
	if(  (ev->ev_class == EVENT_KEYBOARD  ||  ev->ev_class == EVENT_STRING)  &&  !wins.empty()  ) {
		simwin_t &win  = wins.back();
		if(  !win.rollup  )  {
			inside_event_handling = win.gui;
			swallowed = win.gui->infowin_event(ev);
		}
		inside_event_handling = NULL;
		process_kill_list();
		return swallowed;
	}

	// just move top window until button release
	if(  is_moving>=0  &&  (unsigned)is_moving<wins.get_count()  &&  (IS_LEFTDRAG(ev)  ||  IS_LEFTREPEAT(ev))  ) {
		move_win( is_moving, ev );
		return true;
	}

	// just resize window until button release
	if(  is_resizing>=0  &&  (unsigned)is_resizing<wins.get_count()  &&  (IS_LEFTDRAG(ev)  ||  IS_LEFTREPEAT(ev))  ) {
		resize_win( is_resizing, ev );
		return true;
	}

	// swallow all other events in the infobar
	if(  !(ev->ev_class == EVENT_KEYBOARD  ||  ev->ev_class == EVENT_STRING)  &&  y > display_get_height()- win_get_statusbar_height()  ) {
		// swallow event
		return true;
	}

	// swallow all other events in ticker (if there)
	if(  !(ev->ev_class == EVENT_KEYBOARD  ||  ev->ev_class == EVENT_STRING)  &&  show_ticker  &&  y > display_get_height()- win_get_statusbar_height() - TICKER_HEIGHT  ) {
		if(  IS_LEFTCLICK(ev)  ) {
			// goto infowin koordinate, if ticker is active
			koord p = ticker::get_welt_pos();
			if(wl->is_within_limits(p)) {
				wl->get_viewport()->change_world_position(koord3d(p,wl->min_hgt(p)));
			}
		}
		// swallow event
		return true;
	}

	// handle all the other events
	for(  int i=wins.get_count()-1;  i>=0  &&  !swallowed;  i=min(i,(int)wins.get_count())-1  ) {

		if(  wins[i].gui->is_hit( x-wins[i].pos.x, y-wins[i].pos.y )  ) {

			// all events in window are swallowed
			swallowed = true;

			inside_event_handling = wins[i].gui;

			// Top window first
			if(  (int)wins.get_count()-1>i  &&  IS_LEFTCLICK(ev)  &&  (!wins[i].rollup  ||  ev->cy<wins[i].pos.y+D_TITLEBAR_HEIGHT)  ) {
				i = top_win(i,false);
			}

			// if within title bar && window needs decoration
			// Max Kielland: Use title height
			if(  y<wins[i].pos.y+D_TITLEBAR_HEIGHT  &&  wins[i].flags.title  ) {
				// no more moving
				is_moving = -1;
				wins[i].dirty = true;

				// HACK So decode will know if gadget is needed.
				wins[i].flags.help = ( wins[i].gui->get_help_filename() != NULL );

				// Where Was It ?
				sint8 code = decode_gadget_boxes( ( & wins[i].flags ), wins[i].pos.x + (REVERSE_GADGETS?0:wins[i].gui->get_windowsize().w-D_GADGET_WIDTH), x );

				if(  code < SKIN_GADGET_COUNT  ) {
					if(  IS_LEFTCLICK(ev)  ) {
						wins[i].gadget_state |= (1 << code);
					}
					else if(  IS_LEFTRELEASE(ev)  ) {
						wins[i].gadget_state &= ~(1 << code);
						if(  ev->my >= wins[i].pos.y  &&  ev->my < wins[i].pos.y+D_TITLEBAR_HEIGHT  &&  decode_gadget_boxes( ( & wins[i].flags ), wins[i].pos.x + (REVERSE_GADGETS?0:wins[i].gui->get_windowsize().w-D_GADGET_WIDTH), ev->mx )==code  ) {
							// do whatever needs to be done
							switch(  code  ) {
								case SKIN_GADGET_CLOSE :
									destroy_win(wins[i].gui);
									break;
								case SKIN_GADGET_MINIMIZE: // (Mathew Hounsell)
									ev->ev_class = WINDOW_MAKE_MIN_SIZE;
									ev->ev_code = 0;
									wins[i].gui->infowin_event( ev );
									break;
								case SKIN_GADGET_HELP :
									help_frame_t::open_help_on( wins[i].gui->get_help_filename() );
									break;
								case SKIN_BUTTON_PREVIOUS:
									ev->ev_class = WINDOW_CHOOSE_NEXT;
									ev->ev_code = PREV_WINDOW;  // backward
									wins[i].gui->infowin_event( ev );
									break;
								case SKIN_BUTTON_NEXT:
									ev->ev_class = WINDOW_CHOOSE_NEXT;
									ev->ev_code = NEXT_WINDOW;  // forward
									wins[i].gui->infowin_event( ev );
									break;
								case SKIN_GADGET_GOTOPOS:
									{
										// change position on map (or follow)
										koord3d k = wins[i].gui->get_weltpos(true);
										if(  k!=koord3d::invalid  ) {
											wl->get_viewport()->change_world_position( k );
											wl->get_zeiger()->change_pos( k );
										}
									}
									break;
								case SKIN_GADGET_NOTPINNED:
									wins[i].sticky = !wins[i].sticky;
									break;
							}
						}
					}
				}
				else {
					// Somewhere on the titlebar
					if (IS_LEFTDRAG(ev)) {
						i = top_win(i,false);
						move_win(i, ev);
						is_moving = i;
					}
					if(IS_RIGHTCLICK(ev)  ||  IS_LEFTDBLCLK(ev)  ) {
						wins[i].rollup ^= 1;
						gui_frame_t *gui = wins[i].gui;
						scr_size size = gui->get_windowsize();
						mark_rect_dirty_wc( wins[i].pos.x, wins[i].pos.y, wins[i].pos.x+size.w, wins[i].pos.y+size.h );
						if(  wins[i].rollup  ) {
							wl->set_background_dirty();
						}
					}
				}

				// It has been handled so stop checking.
				break;

			}
			else {
				if(!wins[i].rollup) {
					// click in Window / Resize?
					//11-May-02   markus weber added

					scr_size size = wins[i].gui->get_windowsize();

					// resizer hit ?
					const bool canresize = is_resizing>=0  ||
												(ev->cx > wins[i].pos.x + size.w - D_DRAGGER_WIDTH  &&
												 ev->cy > wins[i].pos.y + size.h - D_DRAGGER_HEIGHT);

					if((IS_LEFTCLICK(ev)  ||  IS_LEFTDRAG(ev)  ||  IS_LEFTREPEAT(ev))  &&  canresize  &&  wins[i].gui->get_resizemode()!=gui_frame_t::no_resize) {
						resize_win( i, ev );
						is_resizing = i;
					}
					else {
						is_resizing = -1;
						// click in Window
						event_t wev = *ev;
						wev.move_origin(wins[i].pos);
						wins[i].gui->infowin_event( &wev );
					}
				}
				else {
					swallowed = false;
				}
			}
			inside_event_handling = NULL;
		}
	}

	inside_event_handling = NULL;
	process_kill_list();

	return swallowed;
}


void win_poll_event(event_t* const ev)
{
	display_poll_event(ev);
	// main window resized
	if(  ev->ev_class==EVENT_SYSTEM  &&  ev->ev_code==SYSTEM_RESIZE  ) {
		// main window resized
		simgraph_resize( ev->new_window_size );
		ticker::redraw();
		tool_t::update_toolbars();
		for( uint i = 0; i<wins.get_count(); i++ ) {
			scr_coord_val x = wins[i].pos.x;
			scr_coord_val y = wins[i].pos.y;
			win_clamp_xywh_position( x, y, wins[i].gui->get_min_windowsize(), true );
			wins[i].pos.x = x;
			wins[i].pos.y = y;
		}
		wl->set_dirty();
		wl->get_viewport()->metrics_updated();
		ev->ev_class = IGNORE_EVENT;
	}
	// save and reload all windows (currently only used when a new theme is applied)
	if(  ev->ev_class==EVENT_SYSTEM  &&  ev->ev_code==SYSTEM_RELOAD_WINDOWS  ) {
		dr_chdir( env_t::user_dir );
		loadsave_t dlg;

		if(  dlg.wr_open( "dlgpos.xml", loadsave_t::xml_zipped, 1, "temp", SERVER_SAVEGAME_VER_NR ) == loadsave_t::FILE_STATUS_OK  ) {
			// save all
			rdwr_all_win( &dlg );
			dlg.close();
			destroy_all_win( true );
			if(  dlg.rd_open( "dlgpos.xml" ) == loadsave_t::FILE_STATUS_OK  ) {
				// and reload them ...
				rdwr_all_win( &dlg );
			}
		}
		wl->set_dirty();
		ev->ev_class = IGNORE_EVENT;
		ticker::redraw();
	}
	if(  ev->ev_class==EVENT_SYSTEM  &&  ev->ev_code==SYSTEM_THEME_CHANGED  ) {
		// called when font is changed
		ev->mx = ev->my = ev->cx = ev->cy = 0;
		FOR(vector_tpl<simwin_t>, const& i, wins) {
			i.gui->infowin_event(ev);
		}
		ev->ev_class = IGNORE_EVENT;
		ticker::redraw();
	}
}


uint16 win_get_statusbar_height()
{
	return max(LINESPACE + 2, 15);
}

// finally updates the display
void win_display_flush(double konto)
{
	const sint16 disp_width = display_get_width();
	const sint16 disp_height = display_get_height();

	// display main menu
	tool_selector_t *main_menu = tool_t::toolbar_tool[0]->get_tool_selector();
	scr_coord menu_pos(0,0);
	scr_size menu_size(disp_width, env_t::iconsize.h);
	scr_rect clip_rr(0, env_t::iconsize.h, disp_width, disp_height - env_t::iconsize.h);
	switch (env_t::menupos) {
		case MENU_TOP:
			// pos default (see above)
			// size default
			// rect default
			break;
		case MENU_BOTTOM:
			menu_pos = scr_coord(0, disp_height - env_t::iconsize.h);
			// size default
			clip_rr.y = 0;
			break;
		case MENU_LEFT:
			// pos default (see above)
			menu_size = scr_size(env_t::iconsize.w, disp_height-win_get_statusbar_height()-show_ticker*TICKER_HEIGHT);
			clip_rr = scr_rect(env_t::iconsize.h, 0, disp_width - env_t::iconsize.w, disp_height);
			break;
		case MENU_RIGHT:
			menu_pos.x = disp_width - env_t::iconsize.w;
			menu_size = scr_size(env_t::iconsize.w, disp_height - win_get_statusbar_height()-show_ticker*TICKER_HEIGHT );
			clip_rr = scr_rect(0, 0, disp_width - env_t::iconsize.w, disp_height);
			break;
	}

	display_set_clip_wh( menu_pos.x, menu_pos.y, menu_size.w, menu_size.h );

	if(  skinverwaltung_t::toolbar_background  &&  skinverwaltung_t::toolbar_background->get_image_id(0) != IMG_EMPTY  ) {
		const image_id back_img = skinverwaltung_t::toolbar_background->get_image_id(0);
		display_fit_img_to_width( back_img, env_t::iconsize.w );

		stretch_map_t imag = { {IMG_EMPTY, IMG_EMPTY, IMG_EMPTY}, {IMG_EMPTY, back_img, IMG_EMPTY}, {IMG_EMPTY, IMG_EMPTY, IMG_EMPTY} };
		display_img_stretch(imag, scr_rect(menu_pos, menu_size));
	}
	else {
		display_fillbox_wh_rgb( menu_pos.x, menu_pos.y, menu_size.w, menu_size.h, color_idx_to_rgb(MN_GREY2), false );
	}
	// .. extra logic to enable tooltips
	tooltip_element = main_menu->is_hit( get_mouse_x()-menu_pos.x, get_mouse_y()-menu_pos.y) ? main_menu : NULL;
	void *old_inside_event_handling = inside_event_handling;
	inside_event_handling = main_menu;
	menu_pos.y -= D_TITLEBAR_HEIGHT;
	main_menu->draw(menu_pos, menu_size);
	inside_event_handling = old_inside_event_handling;

	display_set_clip_wh(clip_rr.x, clip_rr.y, clip_rr.w, clip_rr.h);

	if(show_ticker  ||  !ticker::empty()) {
		if (!show_ticker  ||  wl->is_dirty()) {
			ticker::redraw();
		}
		show_ticker = !ticker::empty();
		ticker::draw();
	}
	else {
		show_ticker = false;
	}

	if(  skinverwaltung_t::compass_iso  &&  env_t::compass_screen_position  ) {
		display_img_aligned( skinverwaltung_t::compass_iso->get_image_id( wl->get_settings().get_rotation() ), scr_rect(D_MARGIN_LEFT, env_t::iconsize.h+D_MARGIN_TOP,disp_width-2*4,disp_height- env_t::iconsize.h -D_MARGIN_TOP-D_MARGIN_BOTTOM-win_get_statusbar_height()-(TICKER_HEIGHT)*show_ticker), env_t::compass_screen_position, false );
	}

	{
		// clip windows to avoid drawing into menu, ticker, or status-bar
		PUSH_CLIP(clip_rr.x, clip_rr.y+(env_t::menupos==MENU_BOTTOM ? win_get_statusbar_height() + show_ticker*TICKER_HEIGHT:0), clip_rr.w, clip_rr.h - (TICKER_HEIGHT)*show_ticker - win_get_statusbar_height() );

		display_all_win();
		remove_old_win();

		if(env_t::show_tooltips) {
			// check if there is a tooltip to display
			if(  tooltip_text  &&  *tooltip_text  ) {
				// display tooltip when current owner is invalid or when it is within visible duration
				uint32 elapsed_time;
				if(  !tooltip_owner  ||  ((elapsed_time=dr_time()-tooltip_register_time)>env_t::tooltip_delay  &&  elapsed_time<=env_t::tooltip_delay+env_t::tooltip_duration)  ) {
					const sint16 width = proportional_string_width(tooltip_text)+(LINESPACE/2);
					scr_coord_val x = tooltip_xpos;
					scr_coord_val y = tooltip_ypos;
					win_clamp_xywh_position( x, y, scr_size( width, (LINESPACE*9)/7 ), true );
					display_ddd_proportional_clip( x, y, env_t::tooltip_color, env_t::tooltip_textcolor, tooltip_text, true);
					if(wl) {
						wl->set_background_dirty();
					}
				}
			}
			else if(!static_tooltip_text.empty()) {
				const sint16 width = proportional_string_width(static_tooltip_text.c_str())+ (LINESPACE/2);
				scr_coord_val x = get_mouse_x();
				scr_coord_val y = get_mouse_y();
				win_clamp_xywh_position(x, y, scr_size(width, (LINESPACE*9)/7), true);
				display_ddd_proportional_clip(x, y, env_t::tooltip_color, env_t::tooltip_textcolor, static_tooltip_text.c_str(), true);
				if(wl) {
					wl->set_background_dirty();
				}
			}
			// reset owner and group if no tooltip has been registered
			if(  !tooltip_text  ) {
				tooltip_owner = 0;
				tooltip_group = 0;
			}
			// clear tooltip text to avoid sticky tooltips
			tooltip_text = 0;
		}

		POP_CLIP();

		if(!wl) {
			// no infos during loading etc
			return;
		}
	}

	char const *time = tick_to_string( wl->get_ticks() );

	// statusbar background
	scr_coord_val const status_bar_height = win_get_statusbar_height();
	scr_coord_val const status_bar_y =env_t::menupos == MENU_BOTTOM ? 0 : disp_height - status_bar_height;
	scr_coord_val const status_bar_text_y = status_bar_y + (status_bar_height - LINESPACE) / 2;
	scr_coord_val const status_bar_icon_y = status_bar_y + (status_bar_height - 15) / 2;
	display_set_clip_wh( 0, 0, disp_width, disp_height );
	display_fillbox_wh_rgb(0, status_bar_y - 1, disp_width, 1, SYSCOL_STATUSBAR_DIVIDER, false);
	display_fillbox_wh_rgb(0, env_t::menupos == MENU_BOTTOM ? status_bar_height : status_bar_y - 1, disp_width, 1, SYSCOL_STATUSBAR_DIVIDER, false);
	display_fillbox_wh_rgb(0, status_bar_y, disp_width, status_bar_height, SYSCOL_STATUSBAR_BACKGROUND, false);

	bool tooltip_check = env_t::menupos == MENU_BOTTOM ? get_mouse_y() < status_bar_height : get_mouse_y() > status_bar_y;
	if(  tooltip_check  ) {
		tooltip_xpos = get_mouse_x();
		tooltip_ypos = env_t::menupos == MENU_BOTTOM ? status_bar_height + 10 + TICKER_HEIGHT * show_ticker : status_bar_y - 10 - TICKER_HEIGHT * show_ticker;
	}

	// season color
	display_color_img( skinverwaltung_t::seasons_icons->get_image_id(wl->get_season()), 2, status_bar_icon_y, 0, false, true );
	if(  tooltip_check  &&  tooltip_xpos<14  ) {
		static char const* const seasons[] = { "q2", "q3", "q4", "q1" };
		tooltip_text = translator::translate(seasons[wl->get_season()]);
		tooltip_check = false;
	}

	scr_coord_val right_border = disp_width-4;

	// shown if timeline game
	if(  wl->use_timeline()  &&  skinverwaltung_t::timelinesymbol  ) {
		right_border -= 14;
		display_color_img( skinverwaltung_t::timelinesymbol->get_image_id(0), right_border, status_bar_icon_y, 0, false, true );
		if(  tooltip_check  &&  tooltip_xpos>=right_border  ) {
			tooltip_text = translator::translate("timeline");
			tooltip_check = false;
		}
	}

	// shown if connected
	if(  env_t::networkmode  &&  skinverwaltung_t::networksymbol  ) {
		right_border -= 14;
		display_color_img( skinverwaltung_t::networksymbol->get_image_id(0), right_border, status_bar_icon_y, 0, false, true );
		if(  tooltip_check  &&  tooltip_xpos>=right_border  ) {
			tooltip_text = translator::translate("Connected with server");
			tooltip_check = false;
		}
	}

	// put pause icon
	if(  wl->is_paused()  &&  skinverwaltung_t::pausesymbol  ) {
		right_border -= 14;
		display_color_img( skinverwaltung_t::pausesymbol->get_image_id(0), right_border, status_bar_icon_y, 0, false, true );
		if(  tooltip_check  &&  tooltip_xpos>=right_border  ) {
			tooltip_text = translator::translate("GAME PAUSED");
			tooltip_check = false;
		}
	}

	// put fast forward icon
	if(  wl->is_fast_forward()  &&  skinverwaltung_t::fastforwardsymbol  ) {
		right_border -= 14;
		display_color_img( skinverwaltung_t::fastforwardsymbol->get_image_id(0), right_border, status_bar_icon_y, 0, false, true );
		if(  tooltip_check  &&  tooltip_xpos>=right_border  ) {
			tooltip_text = translator::translate("Fast forward");
			tooltip_check = false;
		}
	}

	koord3d pos = wl->get_zeiger()->get_pos();

	static cbuffer_t info;
	info.clear();
	if(  pos!=koord3d::invalid  ) {
		info.printf( "(%s)", pos.get_str() );
	}
	if(  skinverwaltung_t::timelinesymbol==NULL  ) {
		info.printf( " %s", translator::translate(wl->use_timeline()?"timeline":"no timeline") );
	}
	if(wl->show_distance!=koord3d::invalid  &&  wl->show_distance!=pos) {
		info.printf("-(%d,%d)", wl->show_distance.x-pos.x, wl->show_distance.y-pos.y );
	}
	if(  !env_t::networkmode  ) {
		// time multiplier text
		if(wl->is_fast_forward()) {
			info.printf(" %s(T~%1.2f)", skinverwaltung_t::fastforwardsymbol?"":">> ", wl->get_simloops()/50.0 );
		}
		else if(!wl->is_paused()) {
			info.printf(" (T=%1.2f)", wl->get_time_multiplier()/16.0 );
		}
		else if(  skinverwaltung_t::pausesymbol==NULL  ) {
			info.printf( " %s", translator::translate("GAME PAUSED") );
		}
	}
#ifdef DEBUG
	if(  env_t::verbose_debug>3  ) {
		if(  haltestelle_t::get_rerouting_status()==RECONNECTING  ) {
			info.append( " +" );
		}
		else if(  haltestelle_t::get_rerouting_status()==REROUTING  ) {
			info.append( " *" );
		}
		if(  skinverwaltung_t::compass_iso == NULL  &&  wl->get_settings().get_rotation()  ) {
			static const char *compass_dir[4] = { "North", "East", "South", "West" };
			info.append( " " );
			info.append( translator::translate( compass_dir[ 4-wl->get_settings().get_rotation() ] ) );
		}
	}
#endif
	display_proportional_rgb(20, status_bar_text_y, time, ALIGN_LEFT, SYSCOL_STATUSBAR_TEXT, true);
	display_proportional_rgb(right_border-4, status_bar_text_y, info, ALIGN_RIGHT, SYSCOL_STATUSBAR_TEXT, true);
	/* Since the visual center (disp_width + ((w_left + 8) & 0xFFF0) - ((w_right + 8) & 0xFFF0)) / 2;
	 * jumps left and right with proportional fonts, we just take the actual center
	 */
	scr_coord_val middle = disp_width / 2;

	if(wl->get_active_player()) {
		char buffer[256];
		display_proportional_rgb( middle-5, status_bar_text_y, wl->get_active_player()->get_name(), ALIGN_RIGHT, PLAYER_FLAG|color_idx_to_rgb(wl->get_active_player()->get_player_color1()+env_t::gui_player_color_dark), true);
		money_to_string(buffer, konto );
		display_proportional_rgb( middle+5, status_bar_text_y, buffer, ALIGN_LEFT, konto >= 0.0?MONEY_PLUS:MONEY_MINUS, true);
	}
}


void win_set_world(karte_t *world)
{
	wl = world;
	// remove all save window positions
	old_win_pos.clear();
}


void win_redraw_world()
{
	if(wl) {
		wl->set_dirty();
	}
}


bool win_change_zoom_factor(bool magnify)
{
	const bool result = magnify ? zoom_factor_up() : zoom_factor_down();

	wl->get_viewport()->metrics_updated();

	return result;
}


void win_load_font(const char *fname, uint8 fontsize)
{
	bool force_reload = fontsize != env_t::fontsize;
	env_t::fontsize = fontsize;

	if (display_load_font(fname, force_reload) ) {
		// successfull
		gui_theme_t::themes_init( env_t::default_theme, false, false );

		event_t *ev = new event_t();
		ev->ev_class = EVENT_SYSTEM;
		ev->ev_code = SYSTEM_THEME_CHANGED;
		queue_event( ev );
	}
	else {
		// restore old font
		display_load_font(env_t::fontname.c_str(), true);
	}
	win_redraw_world();
}


/**
 * Sets the tooltip to display.
 * Has to be called from within gui_frame_t::draw
 * @param owner : owner==NULL disables timing (initial delay and visible duration)
 */
void win_set_tooltip(scr_coord_val xpos, scr_coord_val ypos, const char *text, const void *const owner, const void *const group)
{
	// check whether the right window will set the tooltip
	if(inside_event_handling != tooltip_element  ) {
		return;
	}
	// must be set every time as win_display_flush() will reset them
	tooltip_text = text;

	// update ownership if changed
	if(  owner!=tooltip_owner  ) {
		tooltip_owner = owner;
		// update register time only if owner is valid
		if(  owner  ) {
			const uint32 current_time = dr_time();
			if(  group  &&  group==tooltip_group  ) {
				// case : same group
				const uint32 elapsed_time = current_time - tooltip_register_time;
				const uint32 threshold = env_t::tooltip_delay - (env_t::tooltip_delay>>2); // 3/4 of delay
				if(  elapsed_time>threshold  &&  elapsed_time<=env_t::tooltip_delay+env_t::tooltip_duration  ) {
					// case : threshold was reached and duration not expired -> delay time is reduced to 1/4
					tooltip_register_time = current_time - threshold;
				}
				else {
					// case : either before threshold or duration expired
					tooltip_register_time = current_time;
				}
			}
			else {
				// case : owner has no associated group or group is different -> simply reset to current time
				tooltip_group = group;
				tooltip_register_time = current_time;
			}
		}
		else {
			// no owner to associate with a group even if the group is valid
			tooltip_group = 0;
		}
	}

	if (text) {
		scr_size tt_size = scr_size(proportional_string_width(text), LINESPACE + 2);
		win_clamp_xywh_position(xpos, ypos, tt_size, true);
		ypos += LINESPACE / 2 + 1;
	}

	tooltip_xpos = xpos;
	tooltip_ypos = ypos;
}


/**
 * Sets the tooltip to display.
 */
void win_set_static_tooltip(const char *text)
{
	static_tooltip_text = text ? text : "";
}


// shows a modal dialoge
void modal_dialogue(gui_frame_t* gui, ptrdiff_t magic, karte_t* welt, bool (*quit)())
{
	if (display_get_width() == 0) {
		dbg->error("modal_dialogue", "called without a display driver => nothing will be shown!");
		env_t::quit_simutrans = true;
		// cannot handle this!
		return;
	}

	// switch off autosave
	sint32 old_autosave = env_t::autosave;
	env_t::autosave = 0;

	event_t ev;
	scr_coord_val x = (display_get_width() - gui->get_windowsize().w) / 2;
	scr_coord_val y = (display_get_height() - gui->get_windowsize().h) / 2;
	win_clamp_xywh_position(x, y, gui->get_windowsize(), true);
	create_win(x, y, gui, w_info, magic);

	if (welt) {
		welt->set_pause(false);
		welt->reset_interaction();
		welt->reset_timer();


		const uint32 ms_per_frame = 1000 / env_t::fps;
		const uint32 sync_steps_per_step = 5; // env_t::network_frames_per_step
		uint32 frame_start_time;
		uint32 sync_steps_until_step = sync_steps_per_step;

		while (win_is_open(gui) && !env_t::quit_simutrans && !quit()) {
			frame_start_time = dr_time();

			do {
				DBG_DEBUG4("modal_dialogue", "calling win_poll_event");
				win_poll_event(&ev);

				win_clamp_xywh_position(ev.mx, ev.my, scr_size(1, 1), false);
				win_clamp_xywh_position(ev.mx, ev.my, scr_size(1, 1), false);

				if (ev.ev_class == EVENT_KEYBOARD && ev.ev_code == SIM_KEY_F1) {
					if (gui_frame_t* win = win_get_top()) {
						if (const char* helpfile = win->get_help_filename()) {
							help_frame_t::open_help_on(helpfile);
							continue;
						}
					}
				}

				DBG_DEBUG4("modal_dialogue", "calling check_pos_win");
				check_pos_win(&ev);

				if (ev.ev_class == EVENT_SYSTEM && ev.ev_code == SYSTEM_QUIT) {
					env_t::quit_simutrans = true;
					break;
				}
			} while(  ev.ev_class != EVENT_NONE  &&  dr_time()-frame_start_time < 50  );

			DBG_DEBUG4("modal_dialogue", "calling welt->sync_step");
			welt->sync_step((ms_per_frame * welt->get_time_multiplier()) / 16, true, true);

			if (--sync_steps_until_step == 0) {
				DBG_DEBUG4("modal_dialogue", "calling welt->step");
				intr_disable();
				welt->step();
				intr_enable();
				sync_steps_until_step = sync_steps_per_step;
			}

			const uint32 next_frame_start_time = frame_start_time + ms_per_frame;
			const uint32 now = dr_time();

			if (now < next_frame_start_time) {
				dr_sleep(next_frame_start_time - now);
			}
		}

	}
	else {
		display_show_pointer(true);
		display_show_load_pointer(0);
		display_fillbox_wh_rgb(0, 0, display_get_width(), display_get_height(), color_idx_to_rgb(COL_BLACK), true);
		while (win_is_open(gui) && !env_t::quit_simutrans && !quit()) {
			// do not move, do not close it!

			// check for events again after waiting
			if (quit()) {
				break;
			}
			dr_prepare_flush();
			gui->draw(win_get_pos(gui), gui->get_windowsize());
			dr_flush();

			// some backends (etc. SDL2) tend to flood the queue with events. Hence we need to process them all quickly
			uint32 frame_start_time = dr_time();
			do {
				display_poll_event(&ev);
				if (ev.ev_class == EVENT_SYSTEM) {
					if (ev.ev_code == SYSTEM_RESIZE) {
						// main window resized
						simgraph_resize(ev.new_window_size);
						dr_prepare_flush();
						display_fillbox_wh_rgb(0, 0, ev.new_window_size.w, ev.new_window_size.h, color_idx_to_rgb(COL_BLACK), true);
						gui->draw(win_get_pos(gui), gui->get_windowsize());
						dr_flush();
					}
					else if (ev.ev_code == SYSTEM_QUIT) {
						env_t::quit_simutrans = true;
						break;
					}
				}
				else {
					// other events
					check_pos_win(&ev);
				}
				// however, after 50ms we do a redraw ...
			} while (ev.ev_class != EVENT_NONE &&  dr_time() - frame_start_time < 50);

			// sleep the rest of the time
			uint32 end_time = dr_time();
			if (end_time - frame_start_time < 50) {
				dr_sleep(50 - (end_time - frame_start_time));
			}
		}
		display_show_load_pointer(1);
		dr_prepare_flush();
		display_fillbox_wh_rgb(0, 0, display_get_width(), display_get_height(), color_idx_to_rgb(COL_BLACK), true);
		dr_flush();
	}

	// just trigger not another following window => wait for button release
	if (IS_LEFTCLICK(&ev)) {
		do {
			display_poll_event(&ev);
		} while (!IS_LEFTRELEASE(&ev));
	}

	// restore autosave
	env_t::autosave = old_autosave;
}
