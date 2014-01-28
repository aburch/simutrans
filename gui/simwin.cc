/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

/*
 * Sub-window for Sim
 * keine Klasse, da die funktionen von C-Code aus aufgerufen werden koennen
 *
 * The function implements a WindowManager 'Object'
 * There's only one WindowManager
 *
 * 17.11.97, Hj. Malthaner
 *
 * Window now typified
 * 21.06.98, Hj. Malthaner
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
#include "../simsys.h"
#include "../simticker.h"
#include "simwin.h"
#include "../simintr.h"
#include "../simhalt.h"
#include "../simworld.h"

#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/tabfile.h"

#include "../besch/skin_besch.h"

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
#include "werkzeug_waehler.h"
#include "player_frame_t.h"
#include "money_frame.h"
#include "halt_detail.h"
#include "halt_info.h"
#include "convoi_detail_t.h"
#include "convoi_info_t.h"
#include "fahrplan_gui.h"
#include "line_management_gui.h"
#include "schedule_list.h"
#include "stadt_info.h"
#include "message_frame_t.h"
#include "message_option_t.h"
#include "fabrik_info.h"
#include "themeselector.h"

#include "../simversion.h"

class inthashtable_tpl<ptrdiff_t,scr_coord> old_win_pos;


#define dragger_size 12

// (Mathew Hounsell)
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
	scr_coord pos;              // Window position
	uint32 dauer;           // How long should the window stay open?
	uint8 wt;               // the flags for the window type
	ptrdiff_t magic_number; // either magic number or this pointer (which is unique too)
	gui_frame_t *gui;
	uint16	gadget_state;	// which buttons to hilite
	bool sticky;            // true if window is sticky
	bool rollup;
	bool dirty;

	simwin_gadget_flags_t flags; // (Mathew Hounsell) See Above.

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


// Hajo: tooltip data
static int tooltip_xpos = 0;
static int tooltip_ypos = 0;
static const char * tooltip_text = 0;
static const char * static_tooltip_text = 0;
// Knightly :	For timed tooltip with initial delay and finite visible duration.
//				Valid owners are required for timing. Invalid (NULL) owners disable timing.
static const void * tooltip_owner = 0;	// owner of the registered tooltip
static const void * tooltip_group = 0;	// group to which the owner belongs
static unsigned long tooltip_register_time = 0;	// time at which a tooltip is initially registered

static bool show_ticker=0;

/* Hajo: if we are inside the event handler,
 * the window handler has gui pointer as value,
 * to defer destruction if this window
 */
static void *inside_event_handling = NULL;

// only this gui element can set a tooltip
static void *tooltip_element = NULL;

static void destroy_framed_win(simwin_t *win);

//=========================================================================
// Helper Functions

#define REVERSE_GADGETS (!env_t::window_buttons_right)

/**
 * Display a window gadget
 * @author Hj. Malthaner
 */
static int display_gadget_box(sint8 code,
			      int const x, int const y,
			      int const color,
			      bool const pushed)
{

	// If we have a skin, get gadget image data
	const bild_t *img = NULL;
	if(  skinverwaltung_t::gadget  ) {
		// "x", "?", "=", "«", "»"
		const bild_besch_t *pic = skinverwaltung_t::gadget->get_bild(code);
		if (  pic != NULL  ) {
			img = pic->get_pic();
		}
	}

	if(pushed) {
		display_fillbox_wh_clip(x+1, y+1, D_GADGET_WIDTH-2, D_TITLEBAR_HEIGHT-2, (color & 0xF8) + max(7, (color&0x07)+2), false);
	}

	// Do we have a gadget image?
	if(  img != NULL  ) {

		// Max Kielland: This center the gadget image and compensates for any left/top margins within the image to be backward compatible with older PAK sets.
		display_color_img(img->bild_nr, x-img->x + D_GET_CENTER_ALIGN_OFFSET(img->w,D_GADGET_WIDTH), y, 0, false, false);

	}
	else {
		const char *gadget_text = "#";
		static const char *gadget_texts[5]={ "X", "?", "=", "<", ">" };

		if(  code <= SKIN_BUTTON_NEXT  ) {
			gadget_text = gadget_texts[code];
		}
		else if(  code == SKIN_GADGET_GOTOPOS  ) {
			gadget_text	= "*";
		}
		else if(  code == SKIN_GADGET_NOTPINNED  ) {
			gadget_text	= "s";
		}
		else if(  code == SKIN_GADGET_PINNED  ) {
			gadget_text	= "S";
		}
		display_proportional( x+4, y+4, gadget_text, ALIGN_LEFT, COL_BLACK, false );
	}

	display_vline_wh_clip(x,                 y,   D_TITLEBAR_HEIGHT,   color+1,   false);
	display_vline_wh_clip(x+D_GADGET_WIDTH-1, y+1, D_TITLEBAR_HEIGHT-2, COL_BLACK, false);
	display_vline_wh_clip(x+D_GADGET_WIDTH,   y+1, D_TITLEBAR_HEIGHT-2, color+1,   false);

	// Hajo: return width of gadget
	return D_GADGET_WIDTH;
}


//-------------------------------------------------------------------------
// (Mathew Hounsell) Created
static int display_gadget_boxes(
	simwin_gadget_flags_t* flags,
	int x, int y,
	int color,
	uint16 gadget_state,
	bool sticky_pushed,
	bool goto_pushed
) {
	int width = 0;
	const int k=(REVERSE_GADGETS?1:-1);

	// Only the close and sticky gadget can be pushed.
	if(  flags->close  ) {
		width += k*display_gadget_box( SKIN_GADGET_CLOSE, x + width, y, color, gadget_state & (1<<SKIN_GADGET_CLOSE) );
	}
	if(  flags->size  ) {
		width += k*display_gadget_box( SKIN_GADGET_MINIMIZE, x + width, y, color, gadget_state & (1<<SKIN_GADGET_MINIMIZE) );
	}
	if(  flags->help  ) {
		width += k*display_gadget_box( SKIN_GADGET_HELP, x + width, y, color, gadget_state & (1<<SKIN_GADGET_HELP) );
	}
	if(  flags->prev  ) {
		width += k*display_gadget_box( SKIN_BUTTON_PREVIOUS, x + width, y, color, gadget_state & (1<<SKIN_BUTTON_PREVIOUS) );
	}
	if(  flags->next  ) {
		width += k*display_gadget_box( SKIN_BUTTON_NEXT, x + width, y, color, gadget_state & (1<<SKIN_BUTTON_NEXT) );
	}
	if(  flags->gotopos  ) {
		width += k*display_gadget_box( SKIN_GADGET_GOTOPOS, x + width, y, color, goto_pushed  ||  (gadget_state & (1<<SKIN_GADGET_GOTOPOS)) );
	}
	if(  flags->sticky  ) {
		width += k*display_gadget_box( sticky_pushed ? SKIN_GADGET_PINNED : SKIN_GADGET_NOTPINNED, x + width, y, color, gadget_state & (1<<SKIN_GADGET_NOTPINNED) );
	}

	return abs( width );
}


static sint8 decode_gadget_boxes(
               simwin_gadget_flags_t const * const flags,
               int const x,
               int const px
) {
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

//-------------------------------------------------------------------------
// (Mathew Hounsell) Re-factored
static void win_draw_window_title(const scr_coord pos, const scr_size size,
		const PLAYER_COLOR_VAL titel_farbe,
		const char * const text,
		const PLAYER_COLOR_VAL text_farbe,
		const koord3d welt_pos,
		const uint16 gadget_state,
		const bool sticky,
		const bool goto_pushed,
		simwin_gadget_flags_t &flags )
{
	PUSH_CLIP(pos.x, pos.y, size.w, size.h);
	display_fillbox_wh_clip(pos.x, pos.y, size.w, 1, titel_farbe+1, false);
	display_fillbox_wh_clip(pos.x, pos.y+1, size.w, D_TITLEBAR_HEIGHT-2, titel_farbe, false);
	display_fillbox_wh_clip(pos.x, pos.y+D_TITLEBAR_HEIGHT-1, size.w, 1, COL_BLACK, false);
	display_vline_wh_clip(pos.x+size.w-1, pos.y,   D_TITLEBAR_HEIGHT-1, COL_BLACK, false);

	// Draw the gadgets and then move left and draw text.
	flags.gotopos = (welt_pos != koord3d::invalid);
	int width = display_gadget_boxes( &flags, pos.x+(REVERSE_GADGETS?0:size.w-D_GADGET_WIDTH-4), pos.y, titel_farbe, gadget_state, sticky, goto_pushed );
	int titlewidth = display_proportional_clip( pos.x + (REVERSE_GADGETS?width+4:4), pos.y+(D_TITLEBAR_HEIGHT-LINEASCENT)/2, text, ALIGN_LEFT, text_farbe, false );
	if(  flags.gotopos  ) {
		display_proportional_clip( pos.x + (REVERSE_GADGETS?width+4:4)+titlewidth+8, pos.y+(D_TITLEBAR_HEIGHT-LINEASCENT)/2, welt_pos.get_2d().get_fullstr(), ALIGN_LEFT, text_farbe, false );
	}
	POP_CLIP();
}


//-------------------------------------------------------------------------

/**
 * Draw dragger widget
 * @author Hj. Malthaner
 */
static void win_draw_window_dragger(scr_coord pos, scr_size size)
{
	pos += size;
	if(  skinverwaltung_t::gadget  &&  skinverwaltung_t::gadget->get_bild_nr(SKIN_WINDOW_RESIZE)!=IMG_LEER  ) {
		const bild_besch_t *dragger = skinverwaltung_t::gadget->get_bild(SKIN_WINDOW_RESIZE);
		display_color_img( dragger->get_nummer(), pos.x-dragger->get_pic()->w, pos.y-dragger->get_pic()->h, 0, false, false);
	}
	else {
		for(  int x=0;  x<dragger_size;  x++  ) {
			display_fillbox_wh( pos.x-x, pos.y-dragger_size+x, x, 1, (x & 1) ? COL_BLACK : MN_GREY4, true);
		}
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


// returns the window on this positions
gui_frame_t *win_get_oncoord( const scr_coord pt )
{
	for(  int i=wins.get_count()-1;  i>=0;  i--  ) {
		if(  wins[i].gui->getroffen( pt.x-wins[i].pos.x, pt.y-wins[i].pos.y )  ) {
			return wins[i].gui;
		}
	}
	return NULL;
}


/**
 * Returns top window
 * @author prissi
 */
gui_frame_t *win_get_top()
{
	return wins.empty() ? 0 : wins.back().gui;
}


/**
 * returns the focused component of the top window
 * @author Knightly
 */
gui_komponente_t *win_get_focus()
{
	return wins.empty() ? 0 : wins.back().gui->get_focus();
}


int win_get_open_count()
{
	return wins.get_count();
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
 * @author Hj. Malthaner
 */
bool win_is_top(const gui_frame_t *ig)
{
	return !wins.empty() && wins.back().gui == ig;
}


// window functions


// save/restore all dialogues
void rdwr_all_win(loadsave_t *file)
{
	if(  file->get_version()>102003  ) {
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
				switch(id) {

					// end of dialogues
					case magic_none: return;

					// actual dialogues to restore
					case magic_convoi_info:    w = new convoi_info_t(); break;
					case magic_convoi_detail:  w = new convoi_detail_t(); break;
					case magic_themes:         w = new themeselector_t(); break;
					case magic_halt_info:      w = new halt_info_t(); break;
					case magic_halt_detail:    w = new halt_detail_t(); break;
					case magic_reliefmap:      w = new map_frame_t(); break;
					case magic_ki_kontroll_t:  w = new ki_kontroll_t(); break;
					case magic_schedule_rdwr_dummy: w = new fahrplan_gui_t(); break;
					case magic_line_schedule_rdwr_dummy: w = new line_management_gui_t(); break;
					case magic_city_info_t:    w = new stadt_info_t(); break;
					case magic_messageframe:   w = new message_frame_t(); break;
					case magic_message_options: w = new message_option_t(); break;
					case magic_factory_info:   w = new fabrik_info_t(); break;

					default:
						if(  id>=magic_finances_t  &&  id<magic_finances_t+MAX_PLAYER_COUNT  ) {
							w = new money_frame_t( wl->get_spieler(id-magic_finances_t) );
						}
						else if(  id>=magic_line_management_t  &&  id<magic_line_management_t+MAX_PLAYER_COUNT  ) {
							w = new schedule_list_gui_t( wl->get_spieler(id-magic_line_management_t) );
						}
						else if(  id>=magic_toolbar  &&  id<magic_toolbar+256  ) {
							werkzeug_t::toolbar_tool[id-magic_toolbar]->update(wl->get_active_player());
							w = werkzeug_t::toolbar_tool[id-magic_toolbar]->get_werkzeug_waehler();
						}
						else {
							dbg->fatal( "rdwr_all_win()", "No idea how to restore magic 0x%X", id );
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
				create_win( p.x, p.y, w, (wintype)win_type, id );
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


int create_win(int x, int y, gui_frame_t* const gui, wintype const wt, ptrdiff_t const magic)
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

		sint16 const menu_height = env_t::iconsize.h;

		// (Mathew Hounsell) Make Sure Closes Aren't Forgotten.
		// Must Reset as the entries and thus flags are reused
		win.flags.close = true;
		win.flags.title = gui->has_title();
		win.flags.help = ( gui->get_hilfe_datei() != NULL );
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

		// Hajo: Notify window to be shown
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

		scr_size size = gui->get_windowsize();

		if(x == -1) {
			// try to keep the toolbar below all other toolbars
			y = menu_height;
			if(wt & w_no_overlap) {
				for( uint32 i=0;  i<wins.get_count()-1;  i++  ) {
					if(wins[i].wt & w_no_overlap) {
						if(wins[i].pos.y>=y) {
							sint16 lower_y = wins[i].pos.y + wins[i].gui->get_windowsize().h;
							if(lower_y >= y) {
								y = lower_y;
							}
						}
					}
				}
				// right aligned
//				x = max( 0, display_get_width()-size.w );
				// but we go for left
				x = 0;
				y = min( y, display_get_height()-size.h );
			}
			else {
				x = min(get_maus_x() - size.w/2, display_get_width()-size.w);
				y = min(get_maus_y() - size.h-env_t::iconsize.h, display_get_height()-size.h);
			}
		}
		if(x<0) {
			x = 0;
		}
		if(y<menu_height) {
			y = menu_height;
		}
		win.pos = scr_coord(x,y);
		win.dirty = true;
		return wins.get_count();
	}
	else {
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
		wins.remove(i);
		destroy_framed_win(&i);
	}
	kill_list.clear();
}


/**
 * Destroy a framed window
 * @author Hj. Malthaner
 */
static void destroy_framed_win(simwin_t *wins)
{
	// mark dirty
	const scr_size size = wins->gui->get_windowsize();
	mark_rect_dirty_wc( wins->pos.x - 1, wins->pos.y - 1, wins->pos.x + size.w + 2, wins->pos.y + size.h + 2 ); // -1, +2 for env_t::window_frame_active

	if(wins->gui) {
		event_t ev;

		ev.ev_class = INFOWIN;
		ev.ev_code = WIN_CLOSE;
		ev.mx = 0;
		ev.my = 0;
		ev.cx = 0;
		ev.cy = 0;
		ev.button_state = 0;

		void *old = inside_event_handling;
		inside_event_handling = wins->gui;
		wins->gui->infowin_event(&ev);
		inside_event_handling = old;
	}

	if(  (wins->wt&w_do_not_delete)==0  ) {

		// save pointer to gui window:
		// could be modified if wins points to value in kill_list and kill_list is modified! nasty surprise
		gui_frame_t* gui = wins->gui;
		// remove from kill list first
		// otherwise delete will be called again on that window
		for(  uint j = 0;  j < kill_list.get_count();  j++  ) {
			if(  kill_list[j].gui == gui  ) {
				kill_list.remove_at(j);
				break;
			}
		}
		delete gui;
	}
	// set dirty flag to refill background
	if(wl) {
		wl->set_background_dirty();
	}
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
			break;
		}
	}
	return false;
}



void destroy_all_win(bool destroy_sticky)
{
	for ( int curWin=0 ; curWin < (int)wins.get_count() ; curWin++ ) {
		if(  destroy_sticky  || !wins[curWin].sticky  ) {
			if(  inside_event_handling==wins[curWin].gui  ) {
				// only add this, if not already added
				kill_list.append_unique(wins[curWin]);
			}
			else {
				destroy_framed_win(&wins[curWin]);
			}
			// compact the window list
			wins.remove_at(curWin);
			curWin--;
		}
	}
}


int top_win(int win, bool keep_state )
{
	if(  (uint32)win==wins.get_count()-1  ) {
		return win;
	} // already topped

	// mark old dirty
	scr_size size = wins.back().gui->get_windowsize();
	wins.back().dirty = true;

	simwin_t tmp = wins[win];
	wins.remove_at(win);
	if(  !keep_state  ) {
		tmp.rollup = false;	// make visible when topping
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
	gui_frame_t *komp = wins[win].gui;
	scr_size size = komp->get_windowsize();
	scr_coord pos = wins[win].pos;
	PLAYER_COLOR_VAL title_color = (komp->get_titelcolor()&0xF8)+env_t::front_window_bar_color;
	PLAYER_COLOR_VAL text_color = +env_t::front_window_text_color;
	if(  (unsigned)win!=wins.get_count()-1  ) {
		// not top => maximum brightness
		title_color = (title_color&0xF8)+env_t::bottom_window_bar_color;
		text_color = env_t::bottom_window_text_color;
	}
	bool need_dragger = komp->get_resizemode() != gui_frame_t::no_resize;

	// %HACK (Mathew Hounsell) So draw will know if gadget is needed.
	wins[win].flags.help = ( komp->get_hilfe_datei() != NULL );
	if(  wins[win].flags.title  ) {
		win_draw_window_title(wins[win].pos,
				size,
				title_color,
				komp->get_name(),
				text_color,
				komp->get_weltpos(false),
				wins[win].gadget_state,
				wins[win].sticky,
				komp->is_weltpos(),
				wins[win].flags );
	}
	if(  wins[win].dirty  ) {
		mark_rect_dirty_wc( wins[win].pos.x, wins[win].pos.y, wins[win].pos.x+size.w+1, wins[win].pos.y+2 );
		wins[win].dirty = false;
	}
	// mark top window, if requested
	if(env_t::window_frame_active  &&  (unsigned)win==wins.get_count()-1) {
		const int y_off = wins[win].flags.title ? 0 : D_TITLEBAR_HEIGHT;
		if(!wins[win].rollup) {
			display_ddd_box( wins[win].pos.x-1, wins[win].pos.y-1 + y_off, size.w+2, size.h+2 - y_off, title_color, title_color+1, wins[win].gui->is_dirty() );
		}
		else {
			display_ddd_box( wins[win].pos.x-1, wins[win].pos.y-1 + y_off, size.w+2, D_TITLEBAR_HEIGHT + 2 - y_off, title_color, title_color+1, wins[win].gui->is_dirty() );
		}
	}
	if(!wins[win].rollup) {
		komp->draw(wins[win].pos, size);

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
	const sint16 x = get_maus_x();
	const sint16 y = get_maus_y();
	tooltip_element = NULL;
	for(  uint32 i = wins.get_count(); i-- != 0;  ) {
		if(  (!wins[i].rollup  &&  wins[i].gui->getroffen(x-wins[i].pos.x,y-wins[i].pos.y))  ||
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


static inline void snap_check_distance( sint16 *r, const sint16 a, const sint16 b )
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
			other_pos.x = 0;
			other_pos.y = env_t::iconsize.h;
			other_size.x = display_get_width();
			other_size.y = display_get_height()-16-other_pos.y; // 16 = bottom ticker height?
			if(  show_ticker  ) {
				other_size.y -= 16;
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
		if(  from_pos.y+from_size.y>=other_pos.y  &&  from_pos.y<=other_pos.y+other_size.y  ) {
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
	to_pos.x = CLIP( to_pos.x, 8-to_size.x, display_get_width()-16 );
	to_pos.y = CLIP( to_pos.y, env_t::iconsize.h, display_get_height()-24 );

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


/* main window event handler
 * renovated may 2005 by prissi to take care of irregularly shaped windows
 * also remove some unnecessary calls
 */
bool check_pos_win(event_t *ev)
{
	static int is_resizing = -1;
	static int is_moving = -1;

	bool swallowed = false;

	const int x = ev->ev_class==EVENT_MOVE ? ev->mx : ev->cx;
	const int y = ev->ev_class==EVENT_MOVE ? ev->my : ev->cy;

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
			// Knightly :	should not proceed, otherwise the left release event will be fed to other components;
			//				return true (i.e. event swallowed) to prevent propagation back to the main view
			return true;
		}
	}

	// Knightly : disable any active tooltip upon mouse click by forcing expiration of tooltip duration
	if(  ev->ev_class==EVENT_CLICK  ) {
		tooltip_register_time = 0;
	}

	// click in main menu?
	if (!werkzeug_t::toolbar_tool.empty()  &&
			werkzeug_t::toolbar_tool[0]->get_werkzeug_waehler()  &&
			env_t::iconsize.h > y  &&
			ev->ev_class != EVENT_KEYBOARD) {
		event_t wev = *ev;
		inside_event_handling = werkzeug_t::toolbar_tool[0];
		werkzeug_t::toolbar_tool[0]->get_werkzeug_waehler()->infowin_event( &wev );
		inside_event_handling = NULL;
		// swallow event
		return true;
	}

	// cursor event only go to top window (but not if rolled up)
	if(  ev->ev_class == EVENT_KEYBOARD  &&  !wins.empty()  ) {
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
	if(  ev->ev_class != EVENT_KEYBOARD  &&  y > display_get_height()-16  ) {
		// swallow event
		return true;
	}

	// swallow all other events in ticker (if there)
	if(  ev->ev_class != EVENT_KEYBOARD  &&  show_ticker  &&  y > display_get_height()-32  ) {
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

		if(  wins[i].gui->getroffen( x-wins[i].pos.x, y-wins[i].pos.y )  ) {

			// all events in window are swallowed
			swallowed = true;

			inside_event_handling = wins[i].gui;

			// Top window first
			if(  (int)wins.get_count()-1>i  &&  IS_LEFTCLICK(ev)  &&  (!wins[i].rollup  ||  ev->cy<wins[i].pos.y+D_TITLEBAR_HEIGHT)  ) {
				i = top_win(i,false);
			}

			// Hajo: if within title bar && window needs decoration
			// Max Kielland: Use title height
			if(  y<wins[i].pos.y+D_TITLEBAR_HEIGHT  &&  wins[i].flags.title  ) {
				// no more moving
				is_moving = -1;
				wins[i].dirty = true;

				// %HACK (Mathew Hounsell) So decode will know if gadget is needed.
				wins[i].flags.help = ( wins[i].gui->get_hilfe_datei() != NULL );

				// Where Was It ?
				sint8 code = decode_gadget_boxes( ( & wins[i].flags ), wins[i].pos.x + (REVERSE_GADGETS?0:wins[i].gui->get_windowsize().w-D_GADGET_WIDTH-4), x );

				if(  code < SKIN_GADGET_COUNT  ) {
					if(  IS_LEFTCLICK(ev)  ) {
						wins[i].gadget_state |= (1 << code);
					}
					else if(  IS_LEFTRELEASE(ev)  ) {
						wins[i].gadget_state &= ~(1 << code);
						if(  ev->my >= wins[i].pos.y  &&  ev->my < wins[i].pos.y+D_TITLEBAR_HEIGHT  &&  decode_gadget_boxes( ( & wins[i].flags ), wins[i].pos.x + (REVERSE_GADGETS?0:wins[i].gui->get_windowsize().w-D_GADGET_WIDTH-4), ev->mx )==code  ) {
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
									help_frame_t::open_help_on( wins[i].gui->get_hilfe_datei() );
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
									{	// change position on map (or follow)
										koord3d k = wins[i].gui->get_weltpos(true);
										if(  k!=koord3d::invalid  ) {
											spieler_t::get_welt()->get_viewport()->change_world_position( k );
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
					if(IS_RIGHTCLICK(ev)) {
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
												(ev->cx > wins[i].pos.x + size.w - dragger_size  &&
												 ev->cy > wins[i].pos.y + size.h - dragger_size);

					if((IS_LEFTCLICK(ev)  ||  IS_LEFTDRAG(ev)  ||  IS_LEFTREPEAT(ev))  &&  canresize  &&  wins[i].gui->get_resizemode()!=gui_frame_t::no_resize) {
						resize_win( i, ev );
						is_resizing = i;
					}
					else {
						is_resizing = -1;
						// click in Window
						event_t wev = *ev;
						translate_event(&wev, -wins[i].pos.x, -wins[i].pos.y);
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


void win_get_event(event_t* const ev)
{
	display_get_event(ev);
}


void win_poll_event(event_t* const ev)
{
	display_poll_event(ev);
	// main window resized
	if(  ev->ev_class==EVENT_SYSTEM  &&  ev->ev_code==SYSTEM_RESIZE  ) {
		// main window resized
		simgraph_resize( ev->mx, ev->my );
		ticker::redraw_ticker();
		wl->set_dirty();
		wl->get_viewport()->metrics_updated();
		ev->ev_class = EVENT_NONE;
	}
	// save and reload all windows (currently only used when a new theme is applied)
	if(  ev->ev_class==EVENT_SYSTEM  &&  ev->ev_code==SYSTEM_RELOAD_WINDOWS  ) {
		chdir( env_t::user_dir );
		loadsave_t dlg;
		if(  dlg.wr_open( "dlgpos.xml", loadsave_t::xml_zipped, "temp", SERVER_SAVEGAME_VER_NR )  ) {
			// save all
			rdwr_all_win( &dlg );
			dlg.close();
			destroy_all_win( true );
			if(  dlg.rd_open( "dlgpos.xml" )  ) {
				// and reload them ...
				rdwr_all_win( &dlg );
			}
		}
		ev->ev_class = EVENT_NONE;
	}
}


// finally updates the display
void win_display_flush(double konto)
{
	const sint16 disp_width = display_get_width();
	const sint16 disp_height = display_get_height();
	const sint16 menu_height = env_t::iconsize.h;

	// display main menu
	werkzeug_waehler_t *main_menu = werkzeug_t::toolbar_tool[0]->get_werkzeug_waehler();
	display_set_clip_wh( 0, 0, disp_width, menu_height+1 );
	display_fillbox_wh( 0, 0, disp_width, menu_height, MN_GREY2, false );
	// .. extra logic to enable tooltips
	tooltip_element = menu_height > get_maus_y() ? main_menu : NULL;
	void *old_inside_event_handling = inside_event_handling;
	inside_event_handling = main_menu;
	main_menu->draw( scr_coord(0,-D_TITLEBAR_HEIGHT), scr_size(disp_width,menu_height) );
	inside_event_handling = old_inside_event_handling;

	display_set_clip_wh( 0, menu_height, disp_width, disp_height-menu_height+1 );

	show_ticker = false;
	if (!ticker::empty()) {
		ticker::draw();
		if (ticker::empty()) {
			// set dirty background for removing ticker
			if(wl) {
				wl->set_background_dirty();
			}
		}
		else {
			show_ticker = true;
			// need to adapt tooltip_y coordinates
			tooltip_ypos = min(tooltip_ypos, disp_height-15-10-16);
		}
	}

	// ok, we want to clip the height for everything!
	// unfortunately, the easiest way is by manipulating the global high
	{
		sint16 oldh = display_get_height();
		display_set_height( oldh-(wl?16:0)-16*show_ticker );

		display_all_win();
		remove_old_win();

		if(env_t::show_tooltips) {
			// Hajo: check if there is a tooltip to display
			if(  tooltip_text  &&  *tooltip_text  ) {
				// Knightly : display tooltip when current owner is invalid or when it is within visible duration
				unsigned long elapsed_time;
				if(  !tooltip_owner  ||  ((elapsed_time=dr_time()-tooltip_register_time)>env_t::tooltip_delay  &&  elapsed_time<=env_t::tooltip_delay+env_t::tooltip_duration)  ) {
					const sint16 width = proportional_string_width(tooltip_text)+7;
					display_ddd_proportional_clip(min(tooltip_xpos,disp_width-width), max(menu_height+7,tooltip_ypos), width, 0, env_t::tooltip_color, env_t::tooltip_textcolor, tooltip_text, true);
					if(wl) {
						wl->set_background_dirty();
					}
				}
			}
			else if(static_tooltip_text!=NULL  &&  *static_tooltip_text) {
				const sint16 width = proportional_string_width(static_tooltip_text)+7;
				display_ddd_proportional_clip(min(get_maus_x()+16,disp_width-width), max(menu_height+7,get_maus_y()-16), width, 0, env_t::tooltip_color, env_t::tooltip_textcolor, static_tooltip_text, true);
				if(wl) {
					wl->set_background_dirty();
				}
			}
			// Knightly : reset owner and group if no tooltip has been registered
			if(  !tooltip_text  ) {
				tooltip_owner = 0;
				tooltip_group = 0;
			}
			// Hajo : clear tooltip text to avoid sticky tooltips
			tooltip_text = 0;
		}

		display_set_height( oldh );

		if(!wl) {
			// no infos during loading etc
			return;
		}
	}

	char const *time = tick_to_string( wl->get_zeit_ms(), true );

	// bottom text background
	display_set_clip_wh( 0, 0, disp_width, disp_height );
	display_fillbox_wh(0, disp_height-16, disp_width, 1, MN_GREY4, false);
	display_fillbox_wh(0, disp_height-15, disp_width, 15, MN_GREY1, false);

	bool tooltip_check = get_maus_y()>disp_height-15;
	if(  tooltip_check  ) {
		tooltip_xpos = get_maus_x();
		tooltip_ypos = disp_height-15-10-16*show_ticker;
	}

	// season color
	display_color_img( skinverwaltung_t::seasons_icons->get_bild_nr(wl->get_season()), 2, disp_height-15, 0, false, true );
	if(  tooltip_check  &&  tooltip_xpos<14  ) {
		static char const* const seasons[] = { "q2", "q3", "q4", "q1" };
		tooltip_text = translator::translate(seasons[wl->get_season()]);
		tooltip_check = false;
	}

	scr_coord_val right_border = disp_width-4;

	// shown if timeline game
	if(  wl->use_timeline()  &&  skinverwaltung_t::timelinesymbol  ) {
		right_border -= 14;
		display_color_img( skinverwaltung_t::timelinesymbol->get_bild_nr(0), right_border, disp_height-15, 0, false, true );
		if(  tooltip_check  &&  tooltip_xpos>=right_border  ) {
			tooltip_text = translator::translate("timeline");
			tooltip_check = false;
		}
	}

	// shown if connected
	if(  env_t::networkmode  &&  skinverwaltung_t::networksymbol  ) {
		right_border -= 14;
		display_color_img( skinverwaltung_t::networksymbol->get_bild_nr(0), right_border, disp_height-15, 0, false, true );
		if(  tooltip_check  &&  tooltip_xpos>=right_border  ) {
			tooltip_text = translator::translate("Connected with server");
			tooltip_check = false;
		}
	}

	// put pause icon
	if(  wl->is_paused()  &&  skinverwaltung_t::pausesymbol  ) {
		right_border -= 14;
		display_color_img( skinverwaltung_t::pausesymbol->get_bild_nr(0), right_border, disp_height-15, 0, false, true );
		if(  tooltip_check  &&  tooltip_xpos>=right_border  ) {
			tooltip_text = translator::translate("GAME PAUSED");
			tooltip_check = false;
		}
	}

	// put fast forward icon
	if(  wl->is_fast_forward()  &&  skinverwaltung_t::fastforwardsymbol  ) {
		right_border -= 14;
		display_color_img( skinverwaltung_t::fastforwardsymbol->get_bild_nr(0), right_border, disp_height-15, 0, false, true );
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
	}
#endif

	scr_coord_val w_left = 20+display_proportional(20, disp_height-12, time, ALIGN_LEFT, COL_BLACK, true);
	scr_coord_val w_right  = display_proportional(right_border-4, disp_height-12, info, ALIGN_RIGHT, COL_BLACK, true);
	scr_coord_val middle = (disp_width+((w_left+8)&0xFFF0)-((w_right+8)&0xFFF0))/2;

	if(wl->get_active_player()) {
		char buffer[256];
		display_proportional( middle-5, disp_height-12, wl->get_active_player()->get_name(), ALIGN_RIGHT, PLAYER_FLAG|(wl->get_active_player()->get_player_color1()+0), true);
		money_to_string(buffer, konto);
		display_proportional( middle+5, disp_height-12, buffer, ALIGN_LEFT, konto >= 0.0?MONEY_PLUS:MONEY_MINUS, true);
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


/**
 * Sets the tooltip to display.
 * Has to be called from within gui_frame_t::draw
 * @param owner : owner==NULL disables timing (initial delay and visible duration)
 * @author Hj. Malthaner, Knightly
 */
void win_set_tooltip(int xpos, int ypos, const char *text, const void *const owner, const void *const group)
{
	// check whether the right window will set the tooltip
	if (inside_event_handling != tooltip_element) {
		return;
	}
	// must be set every time as win_display_flush() will reset them
	tooltip_text = text;

	// update ownership if changed
	if(  owner!=tooltip_owner  ) {
		tooltip_owner = owner;
		// update register time only if owner is valid
		if(  owner  ) {
			const unsigned long current_time = dr_time();
			if(  group  &&  group==tooltip_group  ) {
				// case : same group
				const unsigned long elapsed_time = current_time - tooltip_register_time;
				const unsigned long threshold = env_t::tooltip_delay - (env_t::tooltip_delay>>2);	// 3/4 of delay
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

	tooltip_xpos = xpos;
	tooltip_ypos = ypos;
}



/**
 * Sets the tooltip to display.
 * @author Hj. Malthaner
 */
void win_set_static_tooltip(const char *text)
{
	static_tooltip_text = text;
}
