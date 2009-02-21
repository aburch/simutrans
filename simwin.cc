/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/* Subfenster fuer Sim
 * keine Klasse, da die funktionen von C-Code aus aufgerufen werden koennen
 *
 * Die Funktionen implementieren ein 'Object' Windowmanager
 * Es gibt nur diesen einen Windowmanager
 *
 * 17.11.97, Hj. Malthaner
 *
 * Fenster jetzt typisiert
 * 21.06.98, Hj. Malthaner
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "simwin.h"
#include "simworld.h"
#include "simmenu.h"
#include "player/simplay.h"
#include "simcolor.h"
#include "simgraph.h"
#include "simskin.h"
#include "simticker.h"
#include "simsys.h"

#include "dataobj/translator.h"
#include "dataobj/umgebung.h"

#include "ifc/gui_fenster.h"
#include "gui/help_frame.h"
#include "gui/werkzeug_waehler.h"

#include "besch/skin_besch.h"

#include "dings/zeiger.h"

#include "tpl/vector_tpl.h"

#include "utils/simstring.h"


#define dragger_size 12

static gui_komponente_t * focus=NULL;

// (Mathew Hounsell)
// I added a button to the map window to fix it's size to the best one.
// This struct is the flow back to the object of the refactoring.
class simwin_gadget_flags
{
public:
   simwin_gadget_flags( void ) : close( false ) , help( false ) , prev( false ), size( false ), next( false ) { }

   bool close;
   bool help;
   bool prev;
   bool size;
   bool next;
};

class simwin
{
public:
	koord pos;         // Fensterposition
	uint32 dauer;        // Wie lange soll das Fenster angezeigt werden ?
	sint16 xoff, yoff;   // Offsets zur Maus beim verschieben
	uint8 wt;	// the flags for the window type
	long magic_number;	// either magic number or this pointer (which is unique too)
	gui_fenster_t *gui;
	bool closing;
	bool rollup;

	simwin_gadget_flags flags; // (Mathew Hounsell) See Above.

	simwin() : flags() {}

	bool operator== (const simwin &) const;
};

bool simwin::operator== (const simwin &other) const { return gui == other.gui; }

// true , if windows need to be redraw "dirty" (including title)
static bool windows_dirty = false;

static vector_tpl<simwin> kill_list(64);

static const int MAX_WIN = 64;          // 64 Fenster sollten reichen

static simwin wins[MAX_WIN+1];
static int ins_win = 0;		        // zeiger auf naechsten freien eintrag

static karte_t* wl = NULL; // Zeiger auf aktuelle Welt, wird in win_set_welt gesetzt



// Hajo: tooltip data
static int tooltip_xpos = 0;
static int tooltip_ypos = 0;
static const char * tooltip_text = 0;
static const char * static_tooltip_text = 0;

static bool show_ticker=0;

// Hajo: if we are inside the event handler, windows may not be
// destroyed immediately
static bool inside_event_handling = false;

static void destroy_framed_win(simwin *wins);

//=========================================================================
// Helper Functions

#define REVERSE_GADGETS (!umgebung_t::window_buttons_right)
// (Mathew Hounsell) A "Gadget Box" is a windows control button.
enum simwin_gadget_et { GADGET_CLOSE, GADGET_HELP, GADGET_SIZE, GADGET_PREV, GADGET_NEXT, COUNT_GADGET };


/**
 * Display a window gadget
 * @author Hj. Malthaner
 */
static int display_gadget_box(simwin_gadget_et const  code,
			      int const x, int const y,
			      int const color,
			      bool const pushed)
{
	display_vline_wh(x,    y,   16, color+1, false);
	display_vline_wh(x+15, y+1, 14, COL_BLACK, false);
	display_vline_wh(x+16, y+1, 14, color+1, false);

	if(pushed) {
		display_fillbox_wh(x+1, y+1, 14, 14, color+1, false);
	}

	// "x", "?", "=", "«", "»"
	const int img = skinverwaltung_t::window_skin->get_bild(code+1)->get_nummer();

	// to prevent day and nightchange
	display_color_img(img, x, y, 0, false, false);

	// Hajo: return width of gadget
	return 16;
}


//-------------------------------------------------------------------------
// (Mathew Hounsell) Created
static int display_gadget_boxes(
               simwin_gadget_flags const * const flags,
               int const x, int const y,
               int const color,
               bool const pushed
) {
    int width = 0;
    const int w=(REVERSE_GADGETS?16:-16);

	// Only the close gadget can be pushed.
	if( flags->close ) {
	    display_gadget_box( GADGET_CLOSE, x +w*width, y, color, pushed );
	    width ++;
	}
	if( flags->size ) {
	    display_gadget_box( GADGET_SIZE, x + w*width, y, color, false );
	    width++;
	}
	if( flags->help ) {
	    display_gadget_box( GADGET_HELP, x + w*width, y, color, false );
	    width++;
	}
	if( flags->prev) {
	    display_gadget_box( GADGET_PREV, x + w*width, y, color, false );
	    width++;
	}
	if( flags->next) {
	    display_gadget_box( GADGET_NEXT, x + w*width, y, color, false );
	    width++;
	}

    return abs( w*width );
}

static simwin_gadget_et decode_gadget_boxes(
               simwin_gadget_flags const * const flags,
               int const x,
               int const px
) {
	int offset = px-x;
	const int w=(REVERSE_GADGETS?-16:16);

//DBG_MESSAGE("simwin_gadget_et decode_gadget_boxes()","offset=%i, w=%i",offset, w );

	// Only the close gadget can be pushed.
	if( flags->close ) {
		if( offset >= 0  &&  offset<16  ) {
//DBG_MESSAGE("simwin_gadget_et decode_gadget_boxes()","close" );
			return GADGET_CLOSE;
		}
		offset += w;
	}
	if( flags->size ) {
		if( offset >= 0  &&  offset<16  ) {
//DBG_MESSAGE("simwin_gadget_et decode_gadget_boxes()","size" );
			return GADGET_SIZE ;
		}
		offset += w;
	}
	if( flags->help ) {
		if( offset >= 0  &&  offset<16  ) {
//DBG_MESSAGE("simwin_gadget_et decode_gadget_boxes()","help" );
			return GADGET_HELP ;
		}
		offset += w;
	}
	if( flags->prev ) {
		if( offset >= 0  &&  offset<16  ) {
			return GADGET_PREV ;
		}
		offset += w;
	}
	if( flags->next ) {
		if( offset >= 0  &&  offset<16  ) {
			return GADGET_NEXT ;
		}
		offset += w;
	}
	return COUNT_GADGET;
}

//-------------------------------------------------------------------------
// (Mathew Hounsell) Refactored
static void win_draw_window_title(const koord pos, const koord gr,
		const PLAYER_COLOR_VAL titel_farbe,
		const char * const text,
		const bool closing,
		const simwin_gadget_flags * const flags )
{
	PUSH_CLIP(pos.x, pos.y, gr.x, gr.y);
	display_fillbox_wh_clip(pos.x, pos.y, gr.x, 1, titel_farbe+1, false);
	display_fillbox_wh_clip(pos.x, pos.y+1, gr.x, 14, titel_farbe, false);
	display_fillbox_wh_clip(pos.x, pos.y+15, gr.x, 1, COL_BLACK, false);
	display_vline_wh_clip(pos.x+gr.x-1, pos.y,   15, COL_BLACK, false);

	// Draw the gadgets and then move left and draw text.
	int width = display_gadget_boxes( flags, pos.x+(REVERSE_GADGETS?0:gr.x-20), pos.y, titel_farbe, closing );
	display_proportional_clip( pos.x + (REVERSE_GADGETS?width+4:4), pos.y+(16-large_font_height)/2, text, ALIGN_LEFT, COL_WHITE, false );
	POP_CLIP();
}


//-------------------------------------------------------------------------

/**
 * Draw dragger widget
 * @author Hj. Malthaner
 */
static void win_draw_window_dragger(koord pos, koord gr)
{
  pos += gr;

  for(int x=0; x<dragger_size; x++) {
    display_fillbox_wh(pos.x-x,
		       pos.y-dragger_size+x,
		       x, 1, (x & 1) ? COL_BLACK : MN_GREY4, true);
  }
}


//=========================================================================

/**
 * redirect keyboard input into UI windows
 *
 * @return true if focus granted
 * @author Hj. Malthaner
 */
bool request_focus(gui_komponente_t *req_focus)
{
	if(focus  &&  req_focus!=focus) {
		// someone has already requested the focus
		dbg->warning("bool request_focus()","Focus was already granted");
	}
	focus = req_focus;
	return true;
}


/**
 * redirect keyboard input into game engine
 *
 * @author Hj. Malthaner
 */
void release_focus(gui_komponente_t *this_focus)
{
	if(focus  &&  focus==this_focus) {
		focus = NULL;
	} else {
		dbg->warning("void release_focus()","Focus was already released");
	}
}



/**
 * our?
 *
 * @author Hj. Malthaner
 */
bool has_focus(const gui_komponente_t *req_focus)
{
	return focus==req_focus;
}



// returns the window (if open) otherwise zero
gui_fenster_t *
win_get_magic(long magic)
{
	if(magic!=-1  &&  magic!=0) {
		// es kann nur ein fenster fuer jede pos. magic number geben
		for(int i=0; i<ins_win; i++) {
			if(wins[i].magic_number == magic) {
				// if 'special' magic number, return it
				return wins[i].gui;
			}
		}
	}
	return NULL;
}



/**
 * Returns top window
 * @author prissi
 */
const gui_fenster_t *
win_get_top()
{
	return (ins_win - 1>=0) ? wins[ins_win-1].gui : NULL;
}



int win_get_open_count()
{
	return ins_win;
}


// brings a window to front, if open
bool
top_win(const gui_fenster_t *gui)
{
	int i;
	for(i=ins_win-1; i>=0; i--) {
		if(wins[i].gui==gui) {
			if(ins_win-1!=i) {
				top_win(i);
			}
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
bool win_is_top(const gui_fenster_t *ig)
{
  const int i = ins_win - 1;
  return i>=0 ? wins[i].gui == ig : false;
}


// window functions

int
create_win(gui_fenster_t *gui, uint8 wt, long magic)
{
	return create_win( -1, -1, gui, wt, magic);
}

int
create_win(int x, int y, gui_fenster_t *gui, uint8 wt, long magic)
{
DBG_DEBUG("create_win()","ins_win=%d", ins_win);
	assert(ins_win >= 0);

	if(ins_win < MAX_WIN) {

		// (Mathew Hounsell) Make Sure Closes Aren't Forgotten.
		// Must Reset as the entries and thus flags are reused
		wins[ins_win].flags.close = true;
		wins[ins_win].flags.help = ( gui->get_hilfe_datei() != NULL );
		wins[ins_win].flags.prev = gui->has_prev();
		wins[ins_win].flags.next = gui->has_next();
		wins[ins_win].flags.size = gui->has_min_sizer();

		if(magic != -1) {
			// es kann nur ein fenster fuer jede pos. magic number geben

			for(int i=0; i<ins_win; i++) {
				if(wins[i].wt==wt  &&  wins[i].magic_number==magic) {
					// gibts schon, wir machen kein neues
					// aber wir machen es sichtbar, falls verdeckt
DBG_DEBUG("create_win()","magic=%d already there, bring to front", magic);
					focus = NULL;	// free focus
					top_win(i);
					return -1;
				}
			}
		}

		// Hajo: Notify window to be shown
		if(gui) {
			focus = NULL;	// free focus
			event_t ev;

			ev.ev_class = INFOWIN;
			ev.ev_code = WIN_OPEN;
			ev.mx = 0;
			ev.my = 0;
			ev.cx = 0;
			ev.cy = 0;
			ev.button_state = 0;

			gui->infowin_event(&ev);
		}

		// this window already open?
		// prissi: (why do we have these magic cookies???)
		for (int i=0; i<ins_win; i++) {
			if (wins[i].gui == gui) {
				top_win(i);
				return -1;
			}
		}

		// take care of time delete windows ...
		wins[ins_win].gui = gui;
		wins[ins_win].wt = (wt&w_time_delete) ? (uint8)w_info : wt;
		wins[ins_win].dauer = (wt&w_time_delete) ? MESG_WAIT : -1;
		wins[ins_win].magic_number = magic;
		wins[ins_win].closing = false;
		wins[ins_win].rollup = false;

		koord gr;

		if(gui != NULL) {
			gr = gui->get_fenstergroesse();
		}
		else {
			gr = koord(192, 92);
		}

		if(x == -1) {
			// try to keep the toolbar below all other toolbars
			y = 32;
			if(wt & w_no_overlap) {
				for(int i=0; i<ins_win; i++) {
					if(wins[i].wt & w_no_overlap) {
						if(wins[i].pos.y>=y) {
							sint16 lower_y = wins[i].pos.y + wins[i].gui->get_fenstergroesse().y;
							if(lower_y >= y) {
								y = lower_y;
							}
						}
					}
				}
				// right aligned
//				x = max( 0, display_get_width()-gr.x );
				// but we go for left
				x = 0;
				y = min( y, display_get_height()-gr.y );
			}
			else {
				x = min(get_maus_x() - gr.x/2, display_get_width()-gr.x);
				y = min(get_maus_y() - gr.y-32, display_get_height()-gr.y);
			}
		}
		if(x<0) {
			x = 0;
		}
		if(y<32) {
			y = 32;
		}
		wins[ins_win].pos = koord(x,y);

		mark_rect_dirty_wc( x, y, x+gr.x, y+gr.y );

DBG_DEBUG("create_win()","new ins_win=%d", ins_win+1);
		return ins_win ++;
	}
	else {
		return -1;
	}
}

/**
 * Destroy a framed window
 * @author Hj. Malthaner
 */
static void destroy_framed_win(simwin *wins)
{
	// mark dirty
	koord gr = wins->gui->get_fenstergroesse();
	mark_rect_dirty_wc( wins->pos.x, wins->pos.y, wins->pos.x+gr.x, wins->pos.y+gr.y );

	if(wins->gui) {

		event_t ev;

		ev.ev_class = INFOWIN;
		ev.ev_code = WIN_CLOSE;
		ev.mx = 0;
		ev.my = 0;
		ev.cx = 0;
		ev.cy = 0;
		ev.button_state = 0;
		wins->gui->infowin_event(&ev);
	}

	if(  (wins->wt&w_do_not_delete)==0  ) {
		delete wins->gui;
	}
	windows_dirty = true;
}



void
destroy_win(const long magic)
{
	const gui_fenster_t *gui = win_get_magic(magic);
	if(gui) {
		destroy_win( gui );
	}
}



void
destroy_win(const gui_fenster_t *gui)
{
	int i;
	for(i=ins_win-1; i>=0; i--) {
		if(wins[i].gui == gui) {
			if(inside_event_handling) {
				// only add this, if not already added
				if (!kill_list.is_contained(wins[i])) {
					kill_list.append(wins[i]);
				}
			}
			else {
				destroy_framed_win(&wins[i]);
			}
			// compact the window list
			if(i < ins_win-1) {
				memmove(&wins[i], &wins[i+1], sizeof(simwin) * (ins_win - i - 1));
			}
			ins_win--;
			break;
		}
	}
}



void destroy_all_win()
{
	for(int i=ins_win-1; i>=0; i--) {
		if(inside_event_handling) {
			// only add this, if not already added
			if (!kill_list.is_contained(wins[i])) {
				kill_list.append(wins[i]);
			}
		}
		else {
			destroy_framed_win(wins+i);
		}
		// compact the window list
		if(i < ins_win-1) {
			memmove(&wins[i], &wins[i+1], sizeof(simwin) * (ins_win - i - 1));
		}
		ins_win--;
	}
}


int top_win(int win)
{
DBG_MESSAGE("top_win()","win=%i ins_win=%i",win,ins_win);
	if (win==ins_win-1) {
		return win;
	} // already topped

	simwin tmp = wins[win];
	memmove(&wins[win], &wins[win+1], sizeof(simwin) * (ins_win - win - 1));
	wins[ins_win-1] = tmp;

	// mark dirty
	koord gr = wins[win].gui->get_fenstergroesse();
	mark_rect_dirty_wc( wins[win].pos.x, wins[win].pos.y, wins[win].pos.x+gr.x, wins[win].pos.y+gr.y );

	event_t ev;

	ev.ev_class = INFOWIN;
	ev.ev_code = WIN_TOP;
	ev.mx = 0;
	ev.my = 0;
	ev.cx = 0;
	ev.cy = 0;
	ev.button_state = 0;

	tmp.gui->infowin_event(&ev);

	return ins_win-1;
}


void display_win(int win)
{
	// ok, now process it
	gui_fenster_t *komp = wins[win].gui;
	koord gr = komp->get_fenstergroesse();
	koord pos = wins[win].pos;
	int titel_farbe = komp->get_titelcolor();
	bool need_dragger = komp->get_resizemode() != gui_fenster_t::no_resize;

	// %HACK (Mathew Hounsell) So draw will know if gadget is needed.
	wins[win].flags.help = ( komp->get_hilfe_datei() != NULL );
	win_draw_window_title(wins[win].pos,
			gr,
			titel_farbe,
			translator::translate(komp->get_name()),
			wins[win].closing,
			( & wins[win].flags ) );
	// mark top window, if requested
	if(umgebung_t::window_frame_active  &&  win==ins_win-1) {
		if(!wins[win].rollup) {
			display_ddd_box( wins[win].pos.x-1, wins[win].pos.y-1, gr.x+2, gr.y+2 , titel_farbe, titel_farbe+1 );
		}
		else {
			display_ddd_box( wins[win].pos.x-1, wins[win].pos.y-1, gr.x+2, 18, titel_farbe, titel_farbe+1 );
		}
	}
	if(!wins[win].rollup) {
		komp->zeichnen(wins[win].pos, gr);

		// dragger zeichnen
		if(need_dragger) {
			win_draw_window_dragger( pos, gr);
		}
	}
}


void
display_all_win()
{
	const char *current_tooltip = tooltip_text;
	for(int i=0; i<ins_win; i++) {
		display_win(i);
		// prissi: tooltips are only allowed for the uppermost window and main menu
		if(i<ins_win-1) {
			tooltip_text = current_tooltip;
		}
	}
}



void win_rotate90( sint16 new_ysize )
{
	for(int i=0; i<ins_win; i++) {
		wins[i].gui->map_rotate90( new_ysize );
	}
}



static void remove_old_win()
{
	// alte fenster entfernen, falls dauer abgelaufen
	for(int i=ins_win-1; i>=0; i--) {
		if(wins[i].dauer > 0) {
			wins[i].dauer --;
			if(wins[i].dauer == 0) {
				destroy_win( wins[i].gui );
			}
		}
	}
}


void move_win(int win, event_t *ev)
{
	koord gr = wins[win].gui->get_fenstergroesse();

	// need to mark all old position dirty
	mark_rect_dirty_wc( wins[win].pos.x, wins[win].pos.y, wins[win].pos.x+gr.x, wins[win].pos.y+gr.y );

	int xfrom = ev->cx;
	int yfrom = ev->cy;
	int xto = ev->mx;
	int yto = ev->my;
	int x,y, xdelta, ydelta;

	// CLIP(wert,min,max)
	x = CLIP(wins[win].pos.x + (xto-xfrom), 8-gr.x, display_get_width()-16);
	y = CLIP(wins[win].pos.y + (yto-yfrom), 32, display_get_height()-24);

	// delta is actual window movement.
	xdelta = x - wins[win].pos.x;
	ydelta = y - wins[win].pos.y;

	wins[win].pos.x += xdelta;
	wins[win].pos.y += ydelta;

	// and to mark new position also dirty ...
	mark_rect_dirty_wc( wins[win].pos.x, wins[win].pos.y, wins[win].pos.x+gr.x, wins[win].pos.y+gr.y );

	change_drag_start(xdelta, ydelta);
}


int win_get_posx(gui_fenster_t *gui)
{
	for(int i=ins_win-1; i>=0; i--) {
		if(wins[i].gui == gui) {
			return wins[i].pos.x;
		}
	}
	return -1;
}


int win_get_posy(gui_fenster_t *gui)
{
	for(int i=ins_win-1; i>=0; i--) {
		if(wins[i].gui == gui) {
			return wins[i].pos.y;
		}
	}
	return -1;
}


void win_set_pos(gui_fenster_t *gui, int x, int y)
{
	for(int i=ins_win-1; i>=0; i--) {
		if(wins[i].gui == gui) {
			wins[i].pos.x = x;
			wins[i].pos.y = y;
			const koord gr = wins[i].gui->get_fenstergroesse();
			mark_rect_dirty_wc( x, y, x+gr.x, y+gr.y );
			return;
		}
	}
}


static void process_kill_list()
{
	for (uint i = 0; i < kill_list.get_count(); i++) {
		destroy_framed_win(&kill_list[i]);
	}
	kill_list.clear();
}


/* main window event handler
 * renovated may 2005 by prissi to take care of irregularly shaped windows
 * also remove some unneccessary calls
 */
bool check_pos_win(event_t *ev)
{
	static bool is_resizing = false;

	int i;
	bool swallowed = false;

	const int x = ev->mx;
	const int y = ev->my;

	inside_event_handling = true;

	// for the moment, no none events
	if (ev->ev_class == EVENT_NONE) {
		inside_event_handling = false;
		process_kill_list();
		return false;
	}

	// we stop resizing once the user releases the button
	if(is_resizing && IS_LEFTRELEASE(ev)) {
		is_resizing = false;
		// printf("Leave resizing mode\n");
	}

	// swallow all events in the infobar
	if(y>display_get_height()-32) {
		// goto infowin koordinate, if ticker is active
		if(show_ticker  &&    y<=display_get_height()-16  &&   IS_LEFTRELEASE(ev)) {
			koord p = ticker::get_welt_pos();
			if(wl->ist_in_kartengrenzen(p)) {
				wl->change_world_position(koord3d(p,wl->min_hgt(p)));
			}
			inside_event_handling = false;
			return true;
		}
	}
	else if(werkzeug_t::toolbar_tool.get_count()>0  &&  y<werkzeug_t::toolbar_tool[0]->iconsize.y  &&  ev->ev_class!=EVENT_KEYBOARD) {
		// click in main menu
		event_t wev = *ev;
		werkzeug_t::toolbar_tool[0]->get_werkzeug_waehler()->infowin_event( &wev );
		// swallow event
		inside_event_handling = false;
		return true;
	}


	for(i=ins_win-1; i>=0  &&  !swallowed; i--) {

		// check click inside window
		if(  wins[i].gui->getroffen( ev->cx-wins[i].pos.x, ev->cy-wins[i].pos.y )  ) {

			// all events in window are swallowed
			swallowed = true;

			// Top window first
			if(ins_win-1>i  &&  IS_LEFTCLICK(ev)  &&  (!wins[i].rollup  ||  ( ev->cy < wins[i].pos.y+16 ))) {
				i = top_win(i);
			}

			// Hajo: if within title bar && window needs decoration
			if( ev->cy < wins[i].pos.y+16 ) {

				// %HACK (Mathew Hounsell) So decode will know if gadget is needed.
				wins[i].flags.help = ( wins[i].gui->get_hilfe_datei() != NULL );

				// Where Was It ?
				simwin_gadget_et code = decode_gadget_boxes( ( & wins[i].flags ), wins[i].pos.x + (REVERSE_GADGETS?0:wins[i].gui->get_fenstergroesse().x-20), ev->cx );

				switch( code ) {
					case GADGET_CLOSE :
						if (IS_LEFTCLICK(ev)) {
							wins[i].closing = true;
						} else if  (IS_LEFTRELEASE(ev)) {
							if (y>=wins[i].pos.y  &&  y<wins[i].pos.y+16  &&  decode_gadget_boxes( ( & wins[i].flags ), wins[i].pos.x + (REVERSE_GADGETS?0:wins[i].gui->get_fenstergroesse().x-20), x )==GADGET_CLOSE) {
								destroy_win(wins[i].gui);
							} else {
								wins[i].closing = false;
							}
						}
						break;
					case GADGET_SIZE: // (Mathew Hounsell)
						if (IS_LEFTCLICK(ev)) {
							ev->ev_class = WINDOW_MAKE_MIN_SIZE;
							ev->ev_code = 0;
							wins[i].gui->infowin_event( ev );
						}
						break;
					case GADGET_HELP :
						if (IS_LEFTCLICK(ev)) {
							create_win(new help_frame_t(wins[i].gui->get_hilfe_datei()), w_info, (long)(wins[i].gui->get_hilfe_datei()) );
							inside_event_handling = false;
						}
						break;
					case GADGET_PREV:
						if (IS_LEFTCLICK(ev)) {
							ev->ev_class = WINDOW_CHOOSE_NEXT;
							ev->ev_code = PREV_WINDOW;  // backward
							wins[i].gui->infowin_event( ev );
						}
						break;
					case GADGET_NEXT:
						if (IS_LEFTCLICK(ev)) {
							ev->ev_class = WINDOW_CHOOSE_NEXT;
							ev->ev_code = NEXT_WINDOW;  // forward
							wins[i].gui->infowin_event( ev );
						}
						break;
					default : // Title
						if (IS_LEFTDRAG(ev)) {
							i = top_win(i);
							move_win(i, ev);
						}
						if(IS_RIGHTCLICK(ev)) {
							wins[i].rollup ^= 1;
							gui_fenster_t *gui = wins[i].gui;
							koord gr = gui->get_fenstergroesse();
							mark_rect_dirty_wc( wins[i].pos.x, wins[i].pos.y, wins[i].pos.x+gr.x, wins[i].pos.y+gr.y );
						}

				}

				// It has been handled so stop checking.
				break;

			}
			else {
				if(!wins[i].rollup) {
					// click in Window / Resize?
					//11-May-02   markus weber added

					gui_fenster_t *gui = wins[i].gui;
					koord gr = gui->get_fenstergroesse();

					// resizer hit ?
					const bool canresize = is_resizing ||
														(ev->mx > wins[i].pos.x + gr.x - dragger_size &&
														ev->my > wins[i].pos.y + gr.y - dragger_size);

					if((IS_LEFTCLICK(ev) || IS_LEFTDRAG(ev)) && canresize && gui->get_resizemode() != gui_fenster_t::no_resize) {
						// Hajo: go into resize mode
						is_resizing = true;

						// printf("Enter resizing mode\n");
						ev->ev_class = WINDOW_RESIZE;
						ev->ev_code = 0;
						event_t wev = *ev;
						// since we may be smaller afterwards
						mark_rect_dirty_wc( wins[i].pos.x, wins[i].pos.y, wins[i].pos.x+gr.x, wins[i].pos.y+gr.y );
						translate_event(&wev, -wins[i].pos.x, -wins[i].pos.y);
						gui->infowin_event( &wev );
					}
					else {
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
		}
	}

	// if no focused, we do not deliver keyboard input
	if(focus==NULL && ev->ev_class == EVENT_KEYBOARD) {
		swallowed = false;
	}

	inside_event_handling = false;
	process_kill_list();

	return swallowed;
}


void win_get_event(struct event_t *ev)
{
  display_get_event(ev);
}


void win_poll_event(struct event_t *ev)
{
	display_poll_event(ev);
	// main window resized
	if(ev->ev_class==EVENT_SYSTEM  &&  ev->ev_code==SYSTEM_RESIZE) {
		// main window resized
		simgraph_resize( ev->mx, ev->my );
		ticker::redraw_ticker();
		ev->ev_class = EVENT_NONE;
	}
}


// since seaons 0 is always summer for backward compatibility
static const char * seasons[] =
{
    "q2", "q3", "q4", "q1"
};


// finally updates the display
void win_display_flush(double konto)
{
	const sint16 disp_width = display_get_width();
	const sint16 disp_height = display_get_height();
	const sint16 menu_height = werkzeug_t::toolbar_tool[0]->iconsize.y;

	werkzeug_waehler_t *main_menu = werkzeug_t::toolbar_tool[0]->get_werkzeug_waehler();
	display_set_clip_wh( 0, 0, disp_width, menu_height+1 );
	display_fillbox_wh(0, 0, disp_width, menu_height, MN_GREY2, false);
	main_menu->zeichnen(koord(0,-16), koord(disp_width,menu_height) );
	// redraw all?
	if(windows_dirty) {
		mark_rect_dirty_wc( 0, 0, disp_width, disp_height );
		windows_dirty = false;
	}
	display_set_clip_wh( 0, menu_height, disp_width, disp_height-menu_height+1 );

	show_ticker = false;
	if (!ticker::empty()) {
		ticker::zeichnen();
		if (ticker::empty()) {
			// set dirty background for removing ticker
			wl->set_dirty();
		}
		else {
			show_ticker = true;
		}
	}

	// ok, we want to clip the height for everything!
	// unfourtunately, the easiest way is by manipulating the global high
	{
		sint16 oldh = display_get_height();
		display_set_height( oldh-(wl?16:0)-16*show_ticker );

		display_all_win();
		remove_old_win();

		if(umgebung_t::show_tooltips) {
			// Hajo: check if there is a tooltip to display
			if(tooltip_text!=NULL  &&  *tooltip_text) {
				const sint16 width = proportional_string_width(tooltip_text)+7;
				display_ddd_proportional(min(tooltip_xpos,disp_width-width), max(menu_height+7,tooltip_ypos), width, 0, umgebung_t::tooltip_color, umgebung_t::tooltip_textcolor, tooltip_text, true);
				// Hajo: clear tooltip to avoid sticky tooltips
				tooltip_text = 0;
			}
			else if(static_tooltip_text!=NULL  &&  *static_tooltip_text) {
				const sint16 width = proportional_string_width(static_tooltip_text)+7;
				display_ddd_proportional(min(get_maus_x()+16,disp_width-width), max(menu_height+7,get_maus_y()-16), width, 0, umgebung_t::tooltip_color, umgebung_t::tooltip_textcolor, static_tooltip_text, true);
			}
		}

		display_set_height( oldh );

		if(!wl) {
			// no infos during loading etc
			return;
		}
	}

	koord3d pos;
	uint32 ticks=1, month=0, year=0;

	const ding_t *dt = wl->get_zeiger();
	pos = dt->get_pos();
	month = wl->get_last_month();
	year = wl->get_last_year();
	ticks = wl->get_zeit_ms();

	// calculate also days if desired
	const uint32 ticks_this_month = ticks % wl->ticks_per_tag;
	uint32 tage, stunden, minuten;
	if(umgebung_t::show_month>1) {
		static sint32 tage_per_month[12]={31,28,31,30,31,30,31,31,30,31,30,31};
		tage = (((sint64)ticks_this_month*tage_per_month[month]) >> wl->ticks_bits_per_tag) + 1;
		stunden = (((sint64)ticks_this_month*tage_per_month[month]) >> (wl->ticks_bits_per_tag-16));
		minuten = (((stunden*3) % 8192)*60)/8192;
		stunden = ((stunden*3) / 8192)%24;
	}
	else {
		tage = 0;
		stunden = (ticks_this_month * 24) >> wl->ticks_bits_per_tag;
		minuten = ((ticks_this_month * 24 * 60) >> wl->ticks_bits_per_tag)%60;
	}

	char time [128];
	char info [256];
	char stretch_text[256];
	char delta_pos[64];

//DBG_MESSAGE("umgebung_t::show_month","%d",umgebung_t::show_month);
	// @author hsiegeln - updated to show month
	// @author prissi - also show date if desired
	switch(umgebung_t::show_month) {
		// german style
		case 4:	sprintf(time, "%s, %d. %s %d %d:%02dh",
						translator::translate(seasons[wl->get_jahreszeit()]),
						tage,
						translator::get_month_name(month%12),
						year,
						stunden,
						minuten
						);
					break;
		// us style
		case 3:	sprintf(time, "%s, %s %d %d %2d:%02d%s",
						translator::translate(seasons[wl->get_jahreszeit()]),
						translator::get_month_name(month%12),
						tage,
						year,
						stunden%12,
						minuten,
						stunden<12 ? "am":"pm"
						);
					break;
		// japanese style
		case 2:	sprintf(time, "%s, %d/%s/%d %2d:%02dh",
						translator::translate(seasons[wl->get_jahreszeit()]),
						year,
						translator::get_month_name(month%12),
						tage,
						stunden,
						minuten
						);
					break;
		// just month
		case 1:	sprintf(time, "%s, %s %d %2d:%02dh",
						translator::get_month_name(month%12),
						translator::translate(seasons[wl->get_jahreszeit()]),
						year,
						stunden,
						minuten
						);
					break;
		// just only season
		default:	sprintf(time, "%s %d",
						translator::translate(seasons[wl->get_jahreszeit()]),
						year);
					break;
	}

	// time multiplier text
	if(wl->is_fast_forward()) {
		sprintf(stretch_text, ">> (T~%1.2f)", wl->get_simloops()/50.0 );
	}
	else if(wl->is_paused()) {
		strcpy( stretch_text, translator::translate("GAME PAUSED") );
	}
	else {
		sprintf(stretch_text, "(T=%1.2f)", wl->get_time_multiplier()/16.0 );
	}

	if(wl->show_distance!=koord3d::invalid  &&  wl->show_distance!=pos) {
		sprintf(delta_pos,"-(%d,%d) ", wl->show_distance.x-pos.x, wl->show_distance.y-pos.y );
	}
	else {
		delta_pos[0] = 0;
	}
	sprintf(info,"(%d,%d,%d)%s %s  %s", pos.x, pos.y, pos.z/Z_TILE_STEP, delta_pos, stretch_text, translator::translate(wl->use_timeline()?"timeline":"no timeline") );

	// bottom text line
	display_set_clip_wh( 0, 0, disp_width, disp_height );
	display_fillbox_wh(0, disp_height-16, disp_width, 1, MN_GREY4, false);
	display_fillbox_wh(0, disp_height-15, disp_width, 15, MN_GREY1, false);

	display_color_img( skinverwaltung_t::seasons_icons->get_bild_nr(wl->get_jahreszeit()), 2, disp_height-15, 0, false, true );

	int w_left = 20+display_proportional(20, disp_height-12, time, ALIGN_LEFT, COL_BLACK, true);
	int w_right = 10+display_proportional(disp_width-10, disp_height-12, info, ALIGN_RIGHT, COL_BLACK, true);
	int middle = (disp_width+((w_left+8)&0xFFF0)-((w_right+8)&0xFFF0))/2;

	if(wl->get_active_player()) {
		char buffer[256];
		display_proportional( middle-5, disp_height-12, wl->get_active_player()->get_name(), ALIGN_RIGHT, PLAYER_FLAG|(wl->get_active_player()->get_player_color1()+0), true);
		money_to_string(buffer, konto);
		display_proportional( middle+5, disp_height-12, buffer, ALIGN_LEFT, konto >= 0.0?MONEY_PLUS:MONEY_MINUS, true);
	}
}


void win_set_welt(karte_t *welt)
{
	wl = welt;
}


bool win_change_zoom_factor(bool magnify)
{
	bool ok = magnify ? zoom_factor_up() : zoom_factor_down();
	if(ok) {
		event_t ev;

		ev.ev_class = WINDOW_REZOOM;
		ev.ev_code = get_tile_raster_width();
		ev.mx = 0;
		ev.my = 0;
		ev.cx = 0;
		ev.cy = 0;
		ev.button_state = 0;

		for(int i=0; i<ins_win; i++) {
			wins[i].gui->infowin_event(&ev);
		}
	}
	return ok;
}


/**
 * Sets the tooltip to display.
 * @author Hj. Malthaner
 */
void win_set_tooltip(int xpos, int ypos, const char *text)
{
	tooltip_xpos = xpos;
	tooltip_ypos = max(32+7,ypos);
	tooltip_text = text;
}



/**
 * Sets the tooltip to display.
 * @author Hj. Malthaner
 */
void win_set_static_tooltip(const char *text)
{
	static_tooltip_text = text;
}
