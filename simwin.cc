/*
 * simwin.cc
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
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
#include "simplay.h"

#include "simtime.h"
#include "dings/zeiger.h"

#include "simcolor.h"

#include "ifc/gui_fenster.h"
#include "gui/help_frame.h"
#include "dataobj/translator.h"
#include "dataobj/umgebung.h"

#include "simgraph.h"
#include "simdisplay.h"

#include "simskin.h"
#include "besch/skin_besch.h"

#include "simticker.h"

#include "tpl/microvec_tpl.h"

#define dragger_size 12


static microvec_tpl <int> kill_list (200);


// (Mathew Hounsell)
// I added a button to the map window to fix it's size to the best one.
// This struct is the flow back to the object of the refactoring.
struct simwin_gadget_flags {
   simwin_gadget_flags( void ) : close( false ) , help( false ) , prev( false ), size( false ), next( false ) { }

   bool close;
   bool help;
   bool prev;
   bool size;
   bool next;
};

struct simwin {
     koord pos;         // Fensterposition
     int dauer;        // Wie lange soll das Fenster angezeigt werden ?
     int xoff, yoff;   // Offsets zur Maus beim verschieben
     enum wintype wt;
     int magic_number;
     gui_fenster_t *gui;
     bool closing;

     simwin_gadget_flags flags; // (Mathew Hounsell) See Above.
};

static const int MAX_WIN = 64;          // 64 Fenster sollten reichen

static struct simwin wins[MAX_WIN+1];
static int ins_win = 0;		        // zeiger auf naechsten freien eintrag

static karte_t *wl=NULL;		// Zeiger auf aktuelle Welt, wird in win_get_event gesetzt

static bool focus_granted = false;      // default: keyboard input into game engine


// Hajo: tooltip data
static int tooltip_xpos = 0;
static int tooltip_ypos = 0;
static const char * tooltip_text = 0;

static bool show_ticker=0;

// Hajo: if we are inside the event handler, windows may not be
// destroyed immediately
static bool inside_event_handling = false;

static void destroy_framed_win(int win);

//=========================================================================
// Helper Functions

#define REVERSE_GADGETS (umgebung_t::window_buttons_right==false)
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
    const int img = skinverwaltung_t::window_skin->gib_bild(code+1)->gib_nummer();

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
                                  const int titel_farbe,
				  const char * const text,
				  const bool closing,
				  const simwin_gadget_flags * const flags )
{
	PUSH_CLIP(pos.x, pos.y, gr.x, gr.y);
    display_fillbox_wh_clip(pos.x, pos.y, gr.x, 1, titel_farbe+1, true);
    display_fillbox_wh_clip(pos.x, pos.y+1, gr.x, 14, titel_farbe, true);
    display_fillbox_wh_clip(pos.x, pos.y+15, gr.x, 1, COL_BLACK, true);
    display_vline_wh_clip(pos.x+gr.x-1, pos.y,   15, COL_BLACK, true);

    // Draw the gadgets and then move left and draw text.
    int width = display_gadget_boxes( flags, pos.x+(REVERSE_GADGETS?0:gr.x-20), pos.y, titel_farbe, closing );
    display_proportional_clip( pos.x + (REVERSE_GADGETS?width+4:4), pos.y+(16-large_font_height)/2, text, ALIGN_LEFT, COL_WHITE, true );
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
bool request_focus()
{
    if(focus_granted) {
	// someone has already requested the focus

	dbg->warning("bool request_focus()",
		     "Focus was already granted");

	return false;
    } else {
	focus_granted = true;

	return true;
    }
}


/**
 * redirect keyboard input into game engine
 *
 * @author Hj. Malthaner
 */
void release_focus()
{
    if(focus_granted) {
	focus_granted = false;
    } else {
	dbg->warning("void release_focus()",
		     "Focus was already released");
    }
}



// returns the window (if open) otherwise zero
gui_fenster_t *
win_get_magic(int magic)
{
	if(magic!= -1) {
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
const gui_fenster_t *win_get_top()
{
	return (ins_win - 1>=0) ? wins[ins_win-1].gui : NULL;
}


/**
 * Checks if a window is a top level window
 * @author Hj. Malthaner
 */
bool win_is_top(const gui_fenster_t *ig)
{
  const int i = ins_win - 1;
  return wins[i].gui == ig;
}


// window functions

int
create_win(gui_fenster_t *gui, enum wintype wt, int magic)
{
    return create_win(-1, -1, -1, gui, wt, magic);
}

int
create_win(int x, int y, gui_fenster_t *gui, enum wintype wt)
{
    return create_win(x, y, -1, gui, wt);
}

int
create_win(int x, int y, int dauer, gui_fenster_t *gui,
           enum wintype wt, int magic)
{
DBG_DEBUG("create_win()","ins_win=%d", ins_win);
	assert(ins_win >= 0);

	if(ins_win < MAX_WIN) {

		// (Mathew Hounsell) Make Sure Closes Aren't Forgotten.
		// Must Reset as the entries and thus flags are reused
		wins[ins_win].flags.close = true;
		wins[ins_win].flags.help = ( gui->gib_hilfe_datei() != NULL );
		wins[ins_win].flags.prev = gui->has_prev();
		wins[ins_win].flags.next = gui->has_next();
		wins[ins_win].flags.size = gui->has_min_sizer();

		if(magic != -1) {
			// es kann nur ein fenster fuer jede pos. magic number geben

			for(int i=0; i<ins_win; i++) {
				if(wins[i].magic_number == magic) {
					// gibts schon, wir machen kein neues
					// aber wir machen es sichtbar, falls verdeckt
DBG_DEBUG("create_win()","magic=%d already there, bring to fornt", magic);
					top_win(i);

					// if 'special' magic number, delete 'new'-ed object
					if (magic >= magic_sprachengui_t) {
						delete gui;
					}
					return -1;
				}
			}
		}

		// Hajo: Notify window to be shown
		if(gui) {
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

		wins[ins_win].gui = gui;
		wins[ins_win].wt = wt;
		wins[ins_win].dauer = dauer;
		wins[ins_win].magic_number = magic;
		wins[ins_win].closing = false;

		koord gr;

		if(gui != NULL) {
			gr = gui->gib_fenstergroesse();
		}
		else {
			gr = koord(192, 92);
		}

		if(x == -1) {
			x = min(gib_maus_x() - gr.x/2, display_get_width()-gr.x);
			y = min(gib_maus_y() - gr.y-32, display_get_height()-gr.y);
		}
		wins[ins_win].pos.x = MAX(x,0);
		wins[ins_win].pos.y = MAX(32, y);

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
static void destroy_framed_win(int win)
{
//printf("destroy_framed_win(): win=%d of %d, gui=%p\n", win, ins_win, wins[win].gui);

	if(win>=ins_win) {
dbg->error("destroy_framed_win()","win=%i >= ins_win=%i",win,ins_win);
	}

	assert(win >= 0);
	assert(win < MAX_WIN);
	assert(win<ins_win);

// printf("destroy_framed_win(): win=%d of %d, gui=%p\n", win, ins_win, wins[win].gui);

	// Hajo: do not destroy frameless windows
	if(wins[win].wt==w_frameless) {
//printf("destroy_framed_win(): win=%d of %d is frameless\n", win, ins_win, wins[win].gui);
		return;
	}

	gui_fenster_t *tmp = NULL;

	if(wins[win].gui) {
		event_t ev;

		ev.ev_class = INFOWIN;
		ev.ev_code = WIN_CLOSE;
		ev.mx = 0;
		ev.my = 0;
		ev.cx = 0;
		ev.cy = 0;
		ev.button_state = 0;
		wins[win].gui->infowin_event(&ev);
	}

	if(wins[win].wt == w_autodelete) {
		tmp = wins[win].gui;
	}

	// reset fields to 'safe' values
	wins[win].pos = koord(0,0);
	wins[win].dauer = -1;
	wins[win].xoff = 0;
	wins[win].yoff = 0;
	wins[win].wt = w_info;
	wins[win].magic_number = magic_none;
	wins[win].gui = NULL;
	wins[win].closing = false;

	// if there was an autodelete object, delete it
	delete tmp;

// printf("destroy_framed_win(): destroyed\n");

	// if we removed not the last window, we need
	// to compact the list
	if(win < ins_win-1) {
		memmove(&wins[win], &wins[win+1], sizeof(struct simwin) * (ins_win - win - 1));
	}

	ins_win--;
//printf("destroy_framed_win() ins_win=%i\n",ins_win);fflush(NULL);
}

void
destroy_win(const gui_fenster_t *gui)
{
	int i;
	for(i=ins_win-1; i>=0; i--) {
		if(wins[i].gui == gui) {
			if(inside_event_handling) {
				// only add this, if not already added
				int j;
				for( j=0;  j<kill_list.get_count();  j++  ) {
					if(kill_list.get(j)==i) {
						break;
					}
				}
				if(j==kill_list.get_count()) {
					kill_list.append(i);
				}
			}
			else {
				destroy_framed_win(i);
			}
			break;
		}
	}
}

void destroy_all_win()
{
	for(int i=ins_win-1; i>=0; i--) {
		if(inside_event_handling) {
			// only add this, if not already added
			int j;
			for( j=0;  j<kill_list.get_count();  j++  ) {
				if(kill_list.get(j)==i) {
					break;
				}
			}
			if(j==kill_list.get_count()) {
				kill_list.append(i);
			}
		}
		else {
			destroy_framed_win(i);
		}
	}
}


int top_win(int win)
{
DBG_MESSAGE("top_win()","win=%i ins_win=%i",win,ins_win);
	if (win==ins_win-1) {
		return win;
	} // already topped

	simwin tmp = wins[win];
	memmove(&wins[win], &wins[win+1], sizeof(struct simwin) * (ins_win - win - 1));
	wins[ins_win-1] = tmp;
	return ins_win-1;
}


void display_win(int win)
{
    gui_fenster_t *komp = wins[win].gui;
    koord gr = komp->gib_fenstergroesse();
    int titel_farbe = komp->gib_fensterfarben().titel;

    // %HACK (Mathew Hounsell) So draw will know if gadget is needed.
    wins[win].flags.help = ( komp->gib_hilfe_datei() != NULL );

    // titelleiste zeichnen wenn noetig
    if(wins[win].wt != w_frameless) {
      win_draw_window_title(wins[win].pos,
			    gr,
			    titel_farbe,
			    translator::translate(komp->gib_name()),
			    wins[win].closing,
			    ( & wins[win].flags ) );
    }

   komp->zeichnen(wins[win].pos, gr);

    // dragger zeichnen
    if(komp->get_resizemode() != gui_fenster_t::no_resize) {
      win_draw_window_dragger(wins[win].pos, gr);
    }
}


void
display_all_win()
{
	for(int i=0; i<ins_win; i++) {
		display_win(i);
		// prissi: tooltips are only allowed for the uppermost window
		if(i<ins_win-1) {
			tooltip_text = NULL;
		}
	}
}


static void remove_old_win()
{
	// alte fenster entfernen, falls dauer abgelaufen
	for(int i=ins_win-1; i>=0; i--) {
		if(wins[i].dauer > 0) {
			wins[i].dauer --;
			if(wins[i].dauer == 0) {
				destroy_framed_win(i);
			}
		}
	}
}

void
move_win(int win, event_t *ev)
{
  koord gr = wins[win].gui->gib_fenstergroesse();
  int xfrom = ev->cx;
  int yfrom = ev->cy;
  int xto = ev->mx;
  int yto = ev->my;
  int x,y, xdelta, ydelta;

  // CLIP(wert,min,max)
  x = CLIP(wins[win].pos.x + (xto-xfrom),
	   8-gr.x, display_get_width()-16);
  y = CLIP(wins[win].pos.y + (yto-yfrom),
	   32, display_get_height()-24);

  // delta is actual window movement.
  xdelta = x - wins[win].pos.x;
  ydelta = y - wins[win].pos.y;

  wins[win].pos.x += xdelta;
  wins[win].pos.y += ydelta;
  change_drag_start(xdelta, ydelta);

}


int win_get_posx(gui_fenster_t *gui)
{
    int i;
    for(i=ins_win-1; i>=0; i--) {
	if(wins[i].gui == gui) {
	    return wins[i].pos.x;
	}
    }
    return -1;
}


int win_get_posy(gui_fenster_t *gui)
{
    int i;
    for(i=ins_win-1; i>=0; i--) {
	if(wins[i].gui == gui) {
	    return wins[i].pos.y;
	}
    }
    return -1;
}


void win_set_pos(gui_fenster_t *gui, int x, int y)
{
    int i;
    for(i=ins_win-1; i>=0; i--) {
	if(wins[i].gui == gui) {
	    wins[i].pos.x = x;
	    wins[i].pos.y = y;
	    return;
	}
    }
}


static void process_kill_list()
{
  for(int i=0; i<kill_list.get_count(); i++) {
    destroy_framed_win(kill_list.get(i));
  }
  kill_list.clear();
}


/* main window event handler
 * renovated may 2005 by prissi to take care of irregularly shaped windows
 * also remove some unneccessary calls
 */
bool
check_pos_win(event_t *ev)
{
	static bool is_resizing = false;

	int i;
	bool swallowed = false;

	const int x = ev->mx;
	const int y = ev->my;

	inside_event_handling = true;

	// for the moment, no none events
	if (ev->ev_class == EVENT_NONE) {
		process_kill_list();
		inside_event_handling = false;
		return false;
	}

	// we stop resizing once the user releases the button
	if(is_resizing && IS_LEFTRELEASE(ev)) {
		is_resizing = false;
		// printf("Leave resizing mode\n");
	}

	// swallow all events in the infobar
	if(ev->cy>display_get_height()-32) {
		// goto infowin koordinate, if ticker is active
		if(show_ticker  &&    ev->cy<=display_get_height()-16  &&   IS_LEFTRELEASE(ev)) {
			koord p = ticker_t::get_instance()->get_welt_pos(ev->cx,ev->cy);
			if(wl->ist_in_kartengrenzen(p)) {
				wl->zentriere_auf(p);
			}
			return true;
		}
	}


	for(i=ins_win-1; i>=0; i--) {

		// check click inside window
		if((ev->ev_class != EVENT_NONE && wins[i].gui->getroffen( ev->cx-wins[i].pos.x, ev->cy-wins[i].pos.y ))
			||  (is_resizing && i==ins_win-1)) {

			// Top first
			if (IS_LEFTCLICK(ev)) {
				i = top_win(i);
			}

			// all events in window are swallowed
			swallowed = true;

			// Hajo: if within title bar && window needs decoration
			if( ev->cy < wins[i].pos.y+16 &&  wins[i].wt != w_frameless) {

				// %HACK (Mathew Hounsell) So decode will know if gadget is needed.
				wins[i].flags.help = ( wins[i].gui->gib_hilfe_datei() != NULL );

				// Where Was It ?
				simwin_gadget_et code = decode_gadget_boxes( ( & wins[i].flags ), wins[i].pos.x + (REVERSE_GADGETS?0:wins[i].gui->gib_fenstergroesse().x-20), ev->cx );

				switch( code ) {
					case GADGET_CLOSE :
						if (IS_LEFTCLICK(ev)) {
							wins[i].closing = true;
						} else if  (IS_LEFTRELEASE(ev)) {
							if (y>=wins[i].pos.y  &&  y<wins[i].pos.y+16  &&  decode_gadget_boxes( ( & wins[i].flags ), wins[i].pos.x + (REVERSE_GADGETS?0:wins[i].gui->gib_fenstergroesse().x-20), x )==GADGET_CLOSE) {
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
							create_win(new help_frame_t(wins[i].gui->gib_hilfe_datei()), w_autodelete, magic_none);
							process_kill_list();
							inside_event_handling = false;
							return swallowed;
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

				}

				// It has been handled so stop checking.
				break;

			}
			else {
				// click in Window / Resize?
				//11-May-02   markus weber added

				gui_fenster_t *gui = wins[i].gui;
				koord gr = gui->gib_fenstergroesse();

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
					translate_event(&wev, -wins[i].pos.x, -wins[i].pos.y);
					gui->infowin_event( &wev );
				}
				else if(wins[i].gui->getroffen( ev->cx-wins[i].pos.x, ev->cy-wins[i].pos.y )) {
					// click in Window
					event_t wev = *ev;
					translate_event(&wev, -wins[i].pos.x, -wins[i].pos.y);
					wins[i].gui->infowin_event( &wev );
					break;
				}
				break;
			}
			continue;
		}
	}

	// if no focused, we do not deliver keyboard input
	if(!focus_granted && ev->ev_class == EVENT_KEYBOARD) {
		swallowed = false;
	}

	process_kill_list();
	inside_event_handling = false;

	return swallowed;
}


/*
 * warning! this is a real HACK, this following routine.
 * problem:  when user presses button which opens new window,
 *           release event cannot reach window, and button will not
 *           unpress.
 * solution: callback function, which sets variable to 'false'
 *           and swallows release event.
 */
static bool *snre_pointer;
void swallow_next_release_event(bool *set_to_false)
{
  snre_pointer = set_to_false;
}
void
win_get_event(struct event_t *ev)
{
  display_get_event(ev);
  if (snre_pointer && IS_LEFTRELEASE(ev)) {
    *snre_pointer = false; snre_pointer = 0;
    ev->ev_class = EVENT_NONE;
  }
}

void
win_poll_event(struct event_t *ev)
{
	display_poll_event(ev);
	// swallow pointer event (prissi: why?)
	if (snre_pointer && IS_LEFTRELEASE(ev)) {
		*snre_pointer = false;
		snre_pointer = 0;
		ev->ev_class = EVENT_NONE;
	}
	// main window resized
	if(ev->ev_class==EVENT_SYSTEM  &&  ev->ev_code==SYSTEM_RESIZE) {
		// main window resized
		simgraph_resize( ev->mx, ev->my );
		win_display_menu();
		ev->ev_class = EVENT_NONE;
	}
}

static const char*  tooltips[22] =
{
    "Einstellungsfenster",
    "Reliefkarte",
    "Abfrage",
    "SLOPETOOLS",
    "RAILTOOLS",
    "MONORAILTOOLS",
    "TRAMTOOLS",
    "ROADTOOLS",
    "SHIPTOOLS",
    "AIRTOOLS",
    "SPECIALTOOLS",
    "Abriss",
    "",
    "Line Management",
    "LISTTOOLS",
    "Mailbox",
    "Finanzen",
    "",
    "Screenshot",
    "Pause",
    "Fast forward",
    "Help"
};



// menu tooltips
static void win_display_tooltips()
{
	if(gib_maus_y() <= 32 && gib_maus_x() < 704) {
		if(*tooltips[gib_maus_x() / 32] != 0) {

			tooltip_text = translator::translate(tooltips[gib_maus_x() / 32]);
			tooltip_xpos = ((gib_maus_x() & 0xFFE0)+16);
			tooltip_ypos = 39;
		}
	}
}

static const int hours2night[] =
{
    4,4,4,4,4,4,4,4,

    4,4,4,4,3,2,1,0,

    0,0,0,0,0,0,0,0,

    0,0,0,0,0,0,0,0,

    0,0,0,0,0,0,0,1,

    2,3,4,4,4,4,4,4
};

// since seaons 0 is always summer for backward compatibility
static const char * seasons[] =
{
    "q2", "q3", "q4", "q1"
};



// draw the menu
void win_display_menu()
{
	const int start_y=display_get_height()-32;
	const int width = display_get_width();

	display_setze_clip_wh( 0, 0, width, 33 );
	display_icon_leiste(-1, skinverwaltung_t::hauptmenu->gib_bild(0)->bild_nr);
	display_setze_clip_wh( 0, 32, width, start_y+32 );
	if(ticker_t::get_instance()->count()>0) {
		// maybe something is omitted of the message
		display_fillbox_wh(0, start_y, width, 1, COL_BLACK, true);
		display_fillbox_wh(0, start_y+1, width, 15, MN_GREY2, true);
	}
}



void
win_display_flush(int , int color, double konto)
{
#ifdef USE_SOFTPOINTER
	display_setze_clip_wh( 0, 0, display_get_width(), display_get_height()+1 );
	display_icon_leiste(color, skinverwaltung_t::hauptmenu->gib_bild(0)->bild_nr);
#else
	display_setze_clip_wh( 0, 32, display_get_width(), display_get_height()+1 );
#endif

	show_ticker = false;
	if(ticker_t::get_instance()->count()>0) {
		ticker_t::get_instance()->zeichnen();
		if(ticker_t::get_instance()->count()==0) {
			// set dirty background for removing ticker
			wl->setze_dirty();
		}
		else {
			show_ticker = true;
		}
	}

	// ok, we want to clip the height for everything!
	// prissi: but we have to use this HACK!
	// unfourtunately, the easiest way is by manipulating the global high
	{
		extern int disp_height;
		const int diff_y = 15+16*show_ticker;
		disp_height -= diff_y;

		display_all_win();
		remove_old_win();

		win_display_tooltips();

		// Hajo: check if there is a tooltip to display
		if(tooltip_text!=NULL) {
			const int width = proportional_string_width(tooltip_text)+7;

			display_ddd_proportional(tooltip_xpos,
													MAX(39,tooltip_ypos),
													width,
													0,
													2,
													COL_BLACK,
													tooltip_text,
													true);
			// Hajo: clear tooltip to avoid sticky tooltips
			tooltip_text = 0;
		}

		disp_height += diff_y;
	}

    koord3d pos;
    uint32 ticks=1, month=0, year=0;

	if(wl) {
		const ding_t *dt = wl->gib_zeiger();
		pos = dt->gib_pos();
		month = wl->get_last_month();
		year = wl->get_last_year();
		ticks = wl->gib_zeit_ms();
	}

	// calculate also days if desired
	const uint32 ticks_this_month = ticks & (karte_t::ticks_per_tag-1);
	uint32 tage, stunden4;
	if(umgebung_t::show_month>1) {
		static sint32 tage_per_month[12]={31,28,31,30,31,30,31,31,30,31,30,31};
		tage = ((ticks_this_month*tage_per_month[month]) >> karte_t::ticks_bits_per_tag) + 1;
		stunden4 = ((ticks_this_month*tage_per_month[month]*96) >> karte_t::ticks_bits_per_tag)%96;
//DBG_MESSAGE("xxx","ticks=%i, ticks_this_month=%i, tage=%i,  karte_t::ticks_bits_per_tag=%i",ticks,ticks_this_month,tage, karte_t::ticks_bits_per_tag);
	}
	else {
		tage = 0;
		stunden4 = (ticks_this_month * 96) >> karte_t::ticks_bits_per_tag;
	}

	// change to night mode?
	// images will be recalculated only, when there has been a change, so we set always
	if(umgebung_t::night_shift) {
		display_day_night_shift(hours2night[stunden4>>1]);
	}
	else {
		display_day_night_shift(0);
	}

    char time [128];
    char info [256];
    char stretch_text[32];
    char delta_pos[64];

//DBG_MESSAGE("umgebung_t::show_month","%d",umgebung_t::show_month);
	// @author hsiegeln - updated to show month
	// @author prissi - also show date if desired
	switch(umgebung_t::show_month) {
		// german style
		case 4:	sprintf(time, "%s, %d. %s %d",
						translator::translate(seasons[wl->gib_jahreszeit()]),
						tage,
						translator::translate(month_names[month%12]),
						year
						);
					break;
		// us style
		case 3:	sprintf(time, "%s, %s %d %d",
						translator::translate(seasons[wl->gib_jahreszeit()]),
						translator::translate(month_names[month%12]),
						tage,
						year
						);
					break;
		// japanese style
		case 2:	sprintf(time, "%s, %d/%s/%d",
						translator::translate(seasons[wl->gib_jahreszeit()]),
						year,
						translator::translate(month_names[month%12]),
						tage
						);
					break;
		// just month
		case 1:	sprintf(time, "%s, %s %d",
						translator::translate(month_names[month%12]),
						translator::translate(seasons[wl->gib_jahreszeit()]),
						year);
					break;
		// just only season
		default:	sprintf(time, "%s %d",
						translator::translate(seasons[wl->gib_jahreszeit()]),
						year);
					break;
	}

	sprintf(stretch_text, wl->is_fast_forward()?">>":"(T=%1.2f)", get_time_multi()/16.0 );

	extern koord3d wkz_wegebau_start;
	if(wkz_wegebau_start!=koord3d::invalid  &&  wkz_wegebau_start!=pos) {
		sprintf(delta_pos,"(%d,%d,%d) -> ",wkz_wegebau_start.x,wkz_wegebau_start.y,wkz_wegebau_start.z/16);
	}
	else {
		delta_pos[0] = 0;
	}
	sprintf(info,"%s(%d,%d,%d) %s  %s", delta_pos, pos.x, pos.y, pos.z / 16, stretch_text, translator::translate(wl->use_timeline()?"timeline":"no timeline") );

	const char *active_player_name = wl->get_active_player()->get_player_nr()==0 ? "" : wl->get_active_player()->gib_name();
	image_id season_img = skinverwaltung_t::seasons_icons ? skinverwaltung_t::seasons_icons->gib_bild_nr(wl->gib_jahreszeit()) : IMG_LEER;
	display_flush(season_img,stunden4, color, konto, time, info, active_player_name, wl->get_active_player()->get_player_color() );
	// season icon
}

void win_setze_welt(karte_t *welt)
{
    wl = welt;
}

void win_set_zoom_factor(int rw)
{
    if(rw != get_zoom_factor()) {
	set_zoom_factor(rw);

	event_t ev;

	ev.ev_class = WINDOW_REZOOM;
	ev.ev_code = rw;
	ev.mx = 0;
	ev.my = 0;
	ev.cx = 0;
	ev.cy = 0;
  	ev.button_state = 0;

	for(int i=0; i<ins_win; i++) {
	    wins[i].gui->infowin_event(&ev);
	}
    }
}


/**
 * Sets the tooltip to display.
 * @author Hj. Malthaner
 */
void win_set_tooltip(int xpos, int ypos, const char *text)
{
  tooltip_xpos = xpos;
  tooltip_ypos = MAX(32+7,ypos);
  tooltip_text = text;
}
