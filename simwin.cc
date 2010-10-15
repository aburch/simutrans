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

#include "simcolor.h"
#include "simevent.h"
#include "simgraph.h"
#include "simmenu.h"
#include "simskin.h"
#include "simsys.h"
#include "simticker.h"
#include "simwin.h"
#include "simhalt.h"
#include "simworld.h"

#include "dataobj/translator.h"
#include "dataobj/umgebung.h"
#include "dataobj/loadsave.h"

#include "besch/skin_besch.h"

#include "dings/zeiger.h"

#include "gui/map_frame.h"
#include "gui/help_frame.h"
#include "gui/messagebox.h"
#include "gui/gui_frame.h"

#include "player/simplay.h"
#include "tpl/vector_tpl.h"
#include "utils/simstring.h"
#include "utils/cbuffer_t.h"

// needed to restore/save them
#include "gui/werkzeug_waehler.h"
#include "gui/player_frame_t.h"
#include "gui/money_frame.h"



#define dragger_size 12

// (Mathew Hounsell)
// I added a button to the map window to fix it's size to the best one.
// This struct is the flow back to the object of the refactoring.
class simwin_gadget_flags_t
{
public:
	simwin_gadget_flags_t( void ) : close( false ) , help( false ) , prev( false ), size( false ), next( false ), sticky( false ) { }

	bool close:1;
	bool help:1;
	bool prev:1;
	bool size:1;
	bool next:1;
	bool sticky:1;
};

class simwin_t
{
public:
	koord pos;         // Fensterposition
	uint32 dauer;        // Wie lange soll das Fenster angezeigt werden ?
	uint8 wt;	// the flags for the window type
	long magic_number;	// either magic number or this pointer (which is unique too)
	gui_frame_t *gui;
	bool closing;
	bool sticky;	// true if window is sticky
	bool rollup;

	simwin_gadget_flags_t flags; // (Mathew Hounsell) See Above.

	simwin_t() : flags() {}

	bool operator== (const simwin_t &) const;
};

bool simwin_t::operator== (const simwin_t &other) const { return gui == other.gui; }

// true , if windows need to be redraw "dirty" (including title)
static bool windows_dirty = false;

#define MAX_WIN (64)
static vector_tpl<simwin_t> wins(MAX_WIN);
static vector_tpl<simwin_t> kill_list(MAX_WIN);

static karte_t* wl = NULL; // Zeiger auf aktuelle Welt, wird in win_set_welt gesetzt



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

static void destroy_framed_win(simwin_t *win);

//=========================================================================
// Helper Functions

#define REVERSE_GADGETS (!umgebung_t::window_buttons_right)
// (Mathew Hounsell) A "Gadget Box" is a windows control button.
enum simwin_gadget_et { GADGET_CLOSE, GADGET_HELP, GADGET_SIZE, GADGET_PREV, GADGET_NEXT, GADGET_STICKY=21, GADGET_STICKY_PUSHED, COUNT_GADGET=255 };


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

	if(  skinverwaltung_t::window_skin  ) {
		// "x", "?", "=", "«", "»"
		const int img = skinverwaltung_t::window_skin->get_bild_nr(code+1);

		// to prevent day and nightchange
		display_color_img(img, x, y, 0, false, false);
	}
	else {
		static const char *gadget_text[6]={ "X", "?", "=", "<", ">", "S" };
		display_proportional( x+4, y+4, code<lengthof(gadget_text) ? gadget_text[code] :  "#", ALIGN_LEFT, COL_BLACK, false );
	}

	// Hajo: return width of gadget
	return 16;
}


//-------------------------------------------------------------------------
// (Mathew Hounsell) Created
static int display_gadget_boxes(
	simwin_gadget_flags_t const * const flags,
	int const x, int const y,
	int const color,
	bool const close_pushed,
	bool const sticky_pushed
) {
    int width = 0;
    const int w=(REVERSE_GADGETS?16:-16);

	// Only the close and sticky gadget can be pushed.
	if(  flags->close  ) {
	    display_gadget_box( GADGET_CLOSE, x +w*width, y, color, close_pushed );
	    width ++;
	}
	if(  flags->size  ) {
	    display_gadget_box( GADGET_SIZE, x + w*width, y, color, false );
	    width++;
	}
	if(  flags->help  ) {
	    display_gadget_box( GADGET_HELP, x + w*width, y, color, false );
	    width++;
	}
	if(  flags->prev  ) {
	    display_gadget_box( GADGET_PREV, x + w*width, y, color, false );
	    width++;
	}
	if(  flags->next  ) {
	    display_gadget_box( GADGET_NEXT, x + w*width, y, color, false );
	    width++;
	}
	if(  flags->sticky  ) {
		display_gadget_box( sticky_pushed ? GADGET_STICKY_PUSHED : GADGET_STICKY, x + w*width, y, color, sticky_pushed );
	    width++;
	}


    return abs( w*width );
}


static simwin_gadget_et decode_gadget_boxes(
               simwin_gadget_flags_t const * const flags,
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
			return GADGET_SIZE;
		}
		offset += w;
	}
	if( flags->help ) {
		if( offset >= 0  &&  offset<16  ) {
//DBG_MESSAGE("simwin_gadget_et decode_gadget_boxes()","help" );
			return GADGET_HELP;
		}
		offset += w;
	}
	if( flags->prev ) {
		if( offset >= 0  &&  offset<16  ) {
			return GADGET_PREV;
		}
		offset += w;
	}
	if( flags->next ) {
		if( offset >= 0  &&  offset<16  ) {
			return GADGET_NEXT;
		}
		offset += w;
	}
	if( flags->sticky ) {
		if( offset >= 0  &&  offset<16  ) {
			return GADGET_STICKY;
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
		const PLAYER_COLOR_VAL text_farbe,
		const bool closing,
		const bool sticky,
		const simwin_gadget_flags_t * const flags )
{
	PUSH_CLIP(pos.x, pos.y, gr.x, gr.y);
	display_fillbox_wh_clip(pos.x, pos.y, gr.x, 1, titel_farbe+1, false);
	display_fillbox_wh_clip(pos.x, pos.y+1, gr.x, 14, titel_farbe, false);
	display_fillbox_wh_clip(pos.x, pos.y+15, gr.x, 1, COL_BLACK, false);
	display_vline_wh_clip(pos.x+gr.x-1, pos.y,   15, COL_BLACK, false);

	// Draw the gadgets and then move left and draw text.
	int width = display_gadget_boxes( flags, pos.x+(REVERSE_GADGETS?0:gr.x-20), pos.y, titel_farbe, closing, sticky );
	display_proportional_clip( pos.x + (REVERSE_GADGETS?width+4:4), pos.y+(16-large_font_height)/2, text, ALIGN_LEFT, text_farbe, false );
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
	if(  skinverwaltung_t::window_skin  &&  skinverwaltung_t::window_skin->get_bild_nr(36)!=IMG_LEER  ) {
		const bild_besch_t *dragger = skinverwaltung_t::window_skin->get_bild(36);
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
gui_frame_t *win_get_magic(long magic)
{
	if(magic!=-1  &&  magic!=0) {
		// es kann nur ein fenster fuer jede pos. magic number geben
		for(  uint i=0;  i<wins.get_count();  i++  ) {
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
bool top_win(const gui_frame_t *gui)
{
	for(  uint i=0;  i<wins.get_count()-1;  i++  ) {
		if(wins[i].gui==gui) {
			top_win(i);
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
void rwdr_all_win(loadsave_t *file)
{
	if(  file->get_version()>102003  ) {
		if(  file->is_saving()  ) {
			for ( uint32 i=0;  i < wins.get_count();  i++ ) {
				uint32 id = wins[i].gui->get_rdwr_id();
				if(  id!=magic_reserved  ) {
					file->rdwr_long( id );
					wins[i].pos.rdwr( file );
					file->rdwr_bool( wins[i].sticky );
					file->rdwr_bool( wins[i].rollup );
					wins[i].gui->rdwr( file );
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
					case magic_reliefmap:      w = new map_frame_t(wl); break;
					case magic_ki_kontroll_t:  w = new ki_kontroll_t(wl); break;

					default:
						if(  id>=magic_finances_t  &&  id<magic_finances_t+MAX_PLAYER_COUNT  ) {
							w = new money_frame_t( wl->get_spieler(id-magic_finances_t) );
						}
						else if(  id>=magic_toolbar  &&  id<magic_toolbar+256  ) {
							werkzeug_t::toolbar_tool[id-magic_toolbar]->update(wl,wl->get_active_player());
							w = werkzeug_t::toolbar_tool[id-magic_toolbar]->get_werkzeug_waehler();
						}
						else {
							dbg->fatal( "rwdr_all_win()", "No idea how to restore magic $%Xlu", id );
						}
				}
				/* sequece is now the same for all dialogues
				 * restore coordinates
				 * create window
				 * restore state
				 * restore content
				 */
				koord p;
				p.rdwr(file);
				create_win( p.x, p.y, w, w_info, id );
				file->rdwr_bool( wins.back().sticky );
				file->rdwr_bool( wins.back().rollup );
				w->rdwr( file );
			}
		}
	}
}



int create_win(gui_frame_t* const gui, wintype const wt, long const magic)
{
	return create_win( -1, -1, gui, wt, magic);
}


int create_win(int x, int y, gui_frame_t* const gui, wintype const wt, long const magic)
{
	assert(gui!=NULL  &&  magic!=0);

	if(  magic!=magic_none  &&  win_get_magic(magic)  ) {
		top_win( win_get_magic(magic) );
		return -1;
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

		if(  wins.get_count()>0  ) {
			// mark old title dirty
			mark_rect_dirty_wc( wins.back().pos.x, wins.back().pos.y, wins.back().pos.x+wins.back().gui->get_fenstergroesse().x, wins.back().pos.y+16 );
		}

		wins.append( simwin_t() );
		simwin_t& win = wins.back();

		// (Mathew Hounsell) Make Sure Closes Aren't Forgotten.
		// Must Reset as the entries and thus flags are reused
		win.flags.close = true;
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
		win.closing = false;
		win.rollup = false;
		win.sticky = false;

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

		koord gr = gui->get_fenstergroesse();

		if(x == -1) {
			// try to keep the toolbar below all other toolbars
			y = 32;
			if(wt & w_no_overlap) {
				for( uint32 i=0;  i<wins.get_count()-1;  i++  ) {
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
		win.pos = koord(x,y);
		mark_rect_dirty_wc( x, y, x+gr.x, y+gr.y );
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
	for(uint i = 0; i < kill_list.get_count(); i++) {
		wins.remove(kill_list[i]);
		destroy_framed_win(&kill_list[i]);
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

		void *old = inside_event_handling;
		inside_event_handling = wins->gui;
		wins->gui->infowin_event(&ev);
		inside_event_handling = old;
	}

	if(  (wins->wt&w_do_not_delete)==0  ) {
		delete wins->gui;
	}
	windows_dirty = true;
}



void destroy_win(const long magic)
{
	const gui_frame_t *gui = win_get_magic(magic);
	if(gui) {
		destroy_win( gui );
	}
}



void destroy_win(const gui_frame_t *gui)
{
	for(  uint i=0;  i<wins.get_count();  i++  ) {
		if(wins[i].gui == gui) {
			if(inside_event_handling==wins[i].gui) {
				kill_list.append_unique(wins[i]);
			}
			else {
				destroy_framed_win(&wins[i]);
				wins.remove_at(i);
			}
			break;
		}
	}
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


int top_win(int win)
{
	if(  (uint32)win==wins.get_count()-1  ) {
		return win;
	} // already topped

	// mark old title dirty
	mark_rect_dirty_wc( wins.back().pos.x, wins.back().pos.y, wins.back().pos.x+wins.back().gui->get_fenstergroesse().x, wins.back().pos.y+16 );

	simwin_t tmp = wins[win];
	wins.remove_at(win);
	wins.append(tmp);

	 // mark new dirty
	koord gr = wins.back().gui->get_fenstergroesse();
	mark_rect_dirty_wc( wins.back().pos.x, wins.back().pos.y, wins.back().pos.x+gr.x, wins.back().pos.y+gr.y );

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
	koord gr = komp->get_fenstergroesse();
	koord pos = wins[win].pos;
	PLAYER_COLOR_VAL title_color = (komp->get_titelcolor()&0xF8)+umgebung_t::front_window_bar_color;
	PLAYER_COLOR_VAL text_color = +umgebung_t::front_window_text_color;
	if(  (unsigned)win!=wins.get_count()-1  ) {
		// not top => maximum brightness
		title_color = (title_color&0xF8)+umgebung_t::bottom_window_bar_color;
		text_color = umgebung_t::bottom_window_text_color;
	}
	bool need_dragger = komp->get_resizemode() != gui_frame_t::no_resize;

	// %HACK (Mathew Hounsell) So draw will know if gadget is needed.
	wins[win].flags.help = ( komp->get_hilfe_datei() != NULL );
	win_draw_window_title(wins[win].pos,
			gr,
			title_color,
			translator::translate(komp->get_name()),
			text_color,
			wins[win].closing,
			wins[win].sticky,
			( & wins[win].flags ) );
	// mark top window, if requested
	if(umgebung_t::window_frame_active  &&  (unsigned)win==wins.get_count()-1) {
		if(!wins[win].rollup) {
			display_ddd_box( wins[win].pos.x-1, wins[win].pos.y-1, gr.x+2, gr.y+2 , title_color, title_color+1 );
		}
		else {
			display_ddd_box( wins[win].pos.x-1, wins[win].pos.y-1, gr.x+2, 18, title_color, title_color+1 );
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


void display_all_win()
{
	// first: empty kill list
	process_kill_list();
	// then display windows
	const char *current_tooltip = tooltip_text;
	const sint16 x = get_maus_x();
	const sint16 y = get_maus_y();
	bool getroffen = false;
	for(  uint i=0;  i<wins.get_count();  i++  ) {
		tooltip_text = NULL;
		void *old_gui = inside_event_handling;
		inside_event_handling = wins[i].gui;
		display_win(i);
		if(  !getroffen  &&  tooltip_text!=NULL  ) {
			current_tooltip = tooltip_text;
		}
		if(  (!wins[i].rollup  &&  wins[i].gui->getroffen(x-wins[i].pos.x,y-wins[i].pos.y))  ||
		     (wins[i].rollup  &&  x>=wins[i].pos.x  &&  x<wins[i].pos.x+wins[i].gui->get_fenstergroesse().x  &&  y>=wins[i].pos.y  &&  y<wins[i].pos.y+16)
		) {
			// prissi: tooltips are only allowed for non overlapping windows
			current_tooltip = tooltip_text;
		}
		inside_event_handling = old_gui;
	}
	tooltip_text = current_tooltip;
}



void win_rotate90( sint16 new_ysize )
{
	for(  uint i=0;  i<wins.get_count();  i++  ) {
		wins[i].gui->map_rotate90( new_ysize );
	}
}



static void remove_old_win()
{
	// alte fenster entfernen, falls dauer abgelaufen
	for(  int i=wins.get_count()-1;  i>=0;  i=min(i,wins.get_count())-1  ) {
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


void resize_win(int i, event_t *ev)
{
	event_t wev = *ev;
	wev.ev_class = WINDOW_RESIZE;
	wev.ev_code = 0;

	// since we may be smaller afterwards
	koord gr = wins[i].gui->get_fenstergroesse();
	mark_rect_dirty_wc( wins[i].pos.x, wins[i].pos.y, wins[i].pos.x+gr.x, wins[i].pos.y+gr.y );
	translate_event(&wev, -wins[i].pos.x, -wins[i].pos.y);
	wins[i].gui->infowin_event( &wev );
}


int win_get_posx(gui_frame_t *gui)
{
	for(  int i=wins.get_count()-1;  i>=0;  i--  ) {
		if(wins[i].gui == gui) {
			return wins[i].pos.x;
		}
	}
	return -1;
}


int win_get_posy(gui_frame_t *gui)
{
	for(  int i=wins.get_count()-1;  i>=0;  i--  ) {
		if(wins[i].gui == gui) {
			return wins[i].pos.y;
		}
	}
	return -1;
}


void win_set_pos(gui_frame_t *gui, int x, int y)
{
	for(  int i=wins.get_count()-1;  i>=0;  i--  ) {
		if(wins[i].gui == gui) {
			wins[i].pos.x = x;
			wins[i].pos.y = y;
			const koord gr = wins[i].gui->get_fenstergroesse();
			mark_rect_dirty_wc( x, y, x+gr.x, y+gr.y );
			return;
		}
	}
}


/* main window event handler
 * renovated may 2005 by prissi to take care of irregularly shaped windows
 * also remove some unneccessary calls
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

	// swallow all events in the infobar
	if(  y>display_get_height()-32  ) {
		// goto infowin koordinate, if ticker is active
		if(  show_ticker  &&  y<=display_get_height()-16  &&  IS_LEFTCLICK(ev)  ) {
			koord p = ticker::get_welt_pos();
			if(wl->ist_in_kartengrenzen(p)) {
				wl->change_world_position(koord3d(p,wl->min_hgt(p)));
			}
			return true;
		}
	}
	else if(  werkzeug_t::toolbar_tool.get_count()>0  &&  werkzeug_t::toolbar_tool[0]->get_werkzeug_waehler()  &&  y<werkzeug_t::toolbar_tool[0]->iconsize.y  &&  ev->ev_class!=EVENT_KEYBOARD  ) {
		// click in main menu
		event_t wev = *ev;
		inside_event_handling = werkzeug_t::toolbar_tool[0];
		werkzeug_t::toolbar_tool[0]->get_werkzeug_waehler()->infowin_event( &wev );
		inside_event_handling = NULL;
		// swallow event
		return true;
	}

	// cursor event only go to top window
	if(  ev->ev_class == EVENT_KEYBOARD  &&  !wins.empty()  ) {
		simwin_t&               win  = wins.back();
		inside_event_handling = win.gui;
		swallowed = win.gui->infowin_event(ev);
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

	// handle all the other events
	for(  int i=wins.get_count()-1;  i>=0  &&  !swallowed;  i=min(i,wins.get_count())-1  ) {

		if(  wins[i].gui->getroffen( x-wins[i].pos.x, y-wins[i].pos.y )  ) {

			// all events in window are swallowed
			swallowed = true;

			inside_event_handling = wins[i].gui;

			// Top window first
			if(  (int)wins.get_count()-1>i  &&  IS_LEFTCLICK(ev)  &&  (!wins[i].rollup  ||  ev->cy<wins[i].pos.y+16)  ) {
				i = top_win(i);
			}

			// Hajo: if within title bar && window needs decoration
			if(  y<wins[i].pos.y+16  ) {
				// no more moving
				is_moving = -1;

				// %HACK (Mathew Hounsell) So decode will know if gadget is needed.
				wins[i].flags.help = ( wins[i].gui->get_hilfe_datei() != NULL );

				// Where Was It ?
				simwin_gadget_et code = decode_gadget_boxes( ( & wins[i].flags ), wins[i].pos.x + (REVERSE_GADGETS?0:wins[i].gui->get_fenstergroesse().x-20), x );

				switch( code ) {
					case GADGET_CLOSE :
						if (IS_LEFTCLICK(ev)) {
							wins[i].closing = true;
						} else if  (IS_LEFTRELEASE(ev)) {
							if (  ev->my>=wins[i].pos.y  &&  ev->my<wins[i].pos.y+16  &&  decode_gadget_boxes( ( & wins[i].flags ), wins[i].pos.x + (REVERSE_GADGETS?0:wins[i].gui->get_fenstergroesse().x-20), ev->mx )==GADGET_CLOSE) {
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
					case GADGET_STICKY:
						if (IS_LEFTCLICK(ev)) {
							wins[i].sticky = !wins[i].sticky;
							// mark title bar dirty
							mark_rect_dirty_wc( wins[i].pos.x, wins[i].pos.y, wins[i].pos.x+wins[i].gui->get_fenstergroesse().x, wins[i].pos.y+16 );
						}
						break;
					default : // Title
						if (IS_LEFTDRAG(ev)) {
							i = top_win(i);
							move_win(i, ev);
							is_moving = i;
						}
						if(IS_RIGHTCLICK(ev)) {
							wins[i].rollup ^= 1;
							gui_frame_t *gui = wins[i].gui;
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

					koord gr = wins[i].gui->get_fenstergroesse();

					// resizer hit ?
					const bool canresize = is_resizing>=0  ||
												(ev->cx > wins[i].pos.x + gr.x - dragger_size  &&
												 ev->cy > wins[i].pos.y + gr.y - dragger_size);

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
			if(  tooltip_text  &&  *tooltip_text  ) {
				// Knightly : display tooltip when current owner is invalid or when it is within visible duration
				unsigned long elapsed_time;
				if(  !tooltip_owner  ||  ((elapsed_time=dr_time()-tooltip_register_time)>umgebung_t::tooltip_delay  &&  elapsed_time<=umgebung_t::tooltip_delay+umgebung_t::tooltip_duration)  ) {
					const sint16 width = proportional_string_width(tooltip_text)+7;
					display_ddd_proportional(min(tooltip_xpos,disp_width-width), max(menu_height+7,tooltip_ypos), width, 0, umgebung_t::tooltip_color, umgebung_t::tooltip_textcolor, tooltip_text, true);
				}
			}
			else if(static_tooltip_text!=NULL  &&  *static_tooltip_text) {
				const sint16 width = proportional_string_width(static_tooltip_text)+7;
				display_ddd_proportional(min(get_maus_x()+16,disp_width-width), max(menu_height+7,get_maus_y()-16), width, 0, umgebung_t::tooltip_color, umgebung_t::tooltip_textcolor, static_tooltip_text, true);
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

	koord3d pos;
	uint32 ticks=1, month=0, year=0;

	const ding_t *dt = wl->get_zeiger();
	pos = dt->get_pos();
	month = wl->get_last_month();
	year = wl->get_last_year();
	ticks = wl->get_zeit_ms();

	// calculate also days if desired
	const uint32 ticks_this_month = ticks % wl->ticks_per_world_month;
	uint32 tage, stunden, minuten;
	if (umgebung_t::show_month > umgebung_t::DATE_FMT_MONTH) {
		static sint32 tage_per_month[12]={31,28,31,30,31,30,31,31,30,31,30,31};
		tage = (((sint64)ticks_this_month*tage_per_month[month]) >> wl->ticks_per_world_month_shift) + 1;
		stunden = (((sint64)ticks_this_month*tage_per_month[month]) >> (wl->ticks_per_world_month_shift-16));
		minuten = (((stunden*3) % 8192)*60)/8192;
		stunden = ((stunden*3) / 8192)%24;
	}
	else {
		tage = 0;
		stunden = (ticks_this_month * 24) >> wl->ticks_per_world_month_shift;
		minuten = ((ticks_this_month * 24 * 60) >> wl->ticks_per_world_month_shift)%60;
	}

	char time [128];

//DBG_MESSAGE("umgebung_t::show_month","%d",umgebung_t::show_month);
	// @author hsiegeln - updated to show month
	// @author prissi - also show date if desired
	// since seaons 0 is always summer for backward compatibility
	static char const* const seasons[] = { "q2", "q3", "q4", "q1" };
	char const* const season = translator::translate(seasons[wl->get_jahreszeit()]);
	char const* const month_ = translator::get_month_name(month % 12);
	switch (umgebung_t::show_month) {
		case umgebung_t::DATE_FMT_GERMAN_NO_SEASON:
			sprintf(time, "%d. %s %d %d:%02dh", tage, month_, year, stunden, minuten);
			break;

		case umgebung_t::DATE_FMT_US_NO_SEASON: {
			uint32 hours_ = stunden % 12;
			if (hours_ == 0) hours_ = 12;
			sprintf(time, "%s %d %d %2d:%02d%s", month_, tage, year, hours_, minuten, stunden < 12 ? "am" : "pm");
			break;
		}

		case umgebung_t::DATE_FMT_JAPANESE_NO_SEASON:
			sprintf(time, "%d/%s/%d %2d:%02dh", year, month_, tage, stunden, minuten);
			break;

		case umgebung_t::DATE_FMT_GERMAN:
			sprintf(time, "%s, %d. %s %d %d:%02dh", season, tage, month_, year, stunden, minuten);
			break;

		case umgebung_t::DATE_FMT_US: {
			uint32 hours_ = stunden % 12;
			if (hours_ == 0) hours_ = 12;
			sprintf(time, "%s, %s %d %d %2d:%02d%s", season, month_, tage, year, hours_, minuten, stunden < 12 ? "am" : "pm");
			break;
		}

		case umgebung_t::DATE_FMT_JAPANESE:
			sprintf(time, "%s, %d/%s/%d %2d:%02dh", season, year, month_, tage, stunden, minuten);
			break;

		case umgebung_t::DATE_FMT_MONTH:
			sprintf(time, "%s, %s %d %2d:%02dh", month_, season, year, stunden, minuten);
			break;

		case umgebung_t::DATE_FMT_SEASON:
			sprintf(time, "%s %d", season, year);
			break;
	}

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
	display_color_img( skinverwaltung_t::seasons_icons->get_bild_nr(wl->get_jahreszeit()), 2, disp_height-15, 0, false, true );
	if(  tooltip_check  &&  tooltip_xpos<14  ) {
		tooltip_text = translator::translate(seasons[wl->get_jahreszeit()]);
		tooltip_check = false;
	}

	KOORD_VAL right_border = disp_width-4;

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
	if(  umgebung_t::networkmode  &&  skinverwaltung_t::networksymbol  ) {
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


	static cbuffer_t info(256);
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
	if(  !umgebung_t::networkmode  ) {
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
	if(  umgebung_t::verbose_debug>3  ) {
		if(  haltestelle_t::get_rerouting_status()==RESCHEDULING  ) {
			info.append( " +" );
		}
		else if(  haltestelle_t::get_rerouting_status()==REROUTING  ) {
			info.append( " *" );
		}
	}
#endif

	KOORD_VAL w_left = 20+display_proportional(20, disp_height-12, time, ALIGN_LEFT, COL_BLACK, true);
	KOORD_VAL w_right  = display_proportional(right_border-4, disp_height-12, info, ALIGN_RIGHT, COL_BLACK, true);
	KOORD_VAL middle = (disp_width+((w_left+8)&0xFFF0)-((w_right+8)&0xFFF0))/2;

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

		for(  sint32 i=wins.get_count()-1;  i>=0;  i=min(i,wins.get_count())-1  ) {
			wins[i].gui->infowin_event(&ev);
		}
	}
	return ok;
}


/**
 * Sets the tooltip to display.
 * @param owner : owner==NULL disables timing (initial delay and visible duration)
 * @author Hj. Malthaner, Knightly
 */
void win_set_tooltip(int xpos, int ypos, const char *text, const void *const owner, const void *const group)
{
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
				const unsigned long threshold = umgebung_t::tooltip_delay - (umgebung_t::tooltip_delay>>2);	// 3/4 of delay
				if(  elapsed_time>threshold  &&  elapsed_time<=umgebung_t::tooltip_delay+umgebung_t::tooltip_duration  ) {
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
