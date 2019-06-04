/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * The window frame all dialogs are based
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#ifndef gui_gui_frame_h
#define gui_gui_frame_h

#include "../display/scr_coord.h"
#include "../display/simgraph.h"
#include "../simcolor.h"
#include "../dataobj/koord3d.h"
#include "../dataobj/translator.h"
#include "components/gui_container.h"
#include "components/gui_button.h"

#include "gui_theme.h"

// Floating cursor eases to place components on a frame with a fixed width.
// It places components in a horizontal line one by one as long as the frame is 
// wide enough. If a component would exceed the maximum right coordinate, then
// the floating cursor wraps the component into a new "line".
class floating_cursor_t
{
	scr_coord cursor;
	scr_coord_val left;
	scr_coord_val right;
	scr_coord_val row_height;
public:
	/** constructor
	 * @param initial, position for the first component
	 * @param min_left,	the x coordinate where to start new lines
	 * @param max_right, the x coordinate where lines end
	 */
	floating_cursor_t(const scr_coord& initial, scr_coord_val min_left, scr_coord_val max_right);

	/** next_pos() calculates and returns the position of the next component.
	 * @author Bernd Gabriel
	 * @date 19-Jan-2014
	 * @param size, size of the next component
	 */
	scr_coord next_pos(const scr_size& size);

	/** new_line() starts a new line.
	 * If next_pos() has not been called since construction or last new_line(), 
	 * the row_height is still 0 and thus will add a D_V_SPAVE to the y position only.
	 */
	void new_line();

	inline scr_coord_val get_row_height() const { return row_height; }
	inline const scr_coord& get_pos() const { return cursor; }
};					   

class loadsave_t;
class karte_ptr_t;
class player_t;

/**
 * A Class for window with Component.
 * Unlike other Window Classes in Simutrans, this is
 * a true component-oriented window that all actions
 * delegates to its component.
 *
 * @author Hj. Malthaner
 */
class gui_frame_t
{
public:
	/**
	 * Resize modes
	 * @author Markus Weber
	 * @date   11-May-2002
	 */
	enum resize_modes {
		no_resize = 0, vertical_resize = 1, horizonal_resize = 2, diagonal_resize = 3
	};

private:
	gui_container_t container;

	const char *name;
	scr_size size;

	/**
	 * Min. size of the window
	 * @author Markus Weber
	 * @date   11-May-2002
	 */
	scr_size min_windowsize;

	resize_modes resize_mode; // 25-may-02  markus weber added
	const player_t *owner;

	// set true for total redraw
	bool dirty:1;
	bool opaque:1;

	uint8 percent_transparent;
	COLOR_VAL color_transparent;

protected:
	void set_dirty() { dirty=true; }

	void unset_dirty() { dirty=false; }

	/**
	 * resize window in response to a resize event
	 * @author Markus Weber, Hj. Malthaner
	 * @date   11-May-2002
	 */
	virtual void resize(const scr_coord delta);

	void set_owner( const player_t *player ) { owner = player; }

	void set_transparent( uint8 percent, COLOR_VAL col ) { opaque = percent==0; percent_transparent = percent; color_transparent = col; }

	static karte_ptr_t welt;
public:
	/**
	 * @param name, Window title
	 * @param player, owner for color
	 * @author Hj. Malthaner
	 */
	gui_frame_t(const char *name, const player_t *player=NULL);

	virtual ~gui_frame_t() {}

	/**
	 * Adds the component to the window
	 * @author Hj. Malthaner
	 */
	void add_component(gui_component_t *comp) { container.add_component(comp); }

	/**
	 * Removes the component from the container.
	 * @author Hj. Malthaner
	 */
	void remove_component(gui_component_t *comp) { container.remove_component(comp); }

	/**
	 * The name is displayed in the titlebar
	 * @return the non-translated name of the Component
	 * @author Hj. Malthaner
	 */
	const char *get_name() const { return name; }

	/**
	 * sets the Name (Window title)
	 * @author Hj. Malthaner
	 */
	void set_name(const char *name);

	/* this returns an unique id, if the dialogue can be saved
	 * if this is defined, you better define a matching constructor with karte_t * and loadsave_t *
	 */
	virtual uint32 get_rdwr_id() { return 0; }

	virtual void rdwr( loadsave_t * ) {}

	/**
	 * get color information for the window title
	 * -borders and -body background
	 * @author Hj. Malthaner
	 */
	virtual PLAYER_COLOR_VAL get_titlecolor() const;

	/**
	 * @return gets the window sizes
	 * @author Hj. Malthaner
	 */
	scr_size get_windowsize() const { return size; }

protected:
	/**
	 * Sets the window sizes
	 * @author Hj. Malthaner
	 */
	virtual void set_windowsize(scr_size size);

	/**
	 * Set minimum size of the window
	 * @author Markus Weber
	 * @date   11-May-2002
	 */
	void set_min_windowsize(scr_size size) { min_windowsize = size; }

public:
	/**
	 * Get minimum size of the window
	 * @author Markus Weber
	 * @date   11-May-2002
	 */
	scr_size get_min_windowsize() { return min_windowsize; }

	/**
	 * Max Kielland 2013: Client size auto calculation with title bar and margins.
	 * @return the usable width and height of the window
	 * @author Markus Weber
	 * @date   11-May-2002
 	*/
	scr_size get_client_windowsize() const {
		return size - scr_size(0, ( has_title()*D_TITLEBAR_HEIGHT ) );
	}

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	virtual const char * get_help_filename() const {return NULL;}

	/**
	 * Does this window need a min size button in the title bar?
	 * @return true if such a button is needed
	 * @author Hj. Malthaner
	 */
	virtual bool has_min_sizer() const {return false;}

	/**
	 * Does this window need a next button in the title bar?
	 * @return true if such a button is needed
	 * @author Volker Meyer
	 */
	virtual bool has_next() const {return false;}

	/**
	 * Does this window need a prev button in the title bar?
	 * @return true if such a button is needed
	 * @author Volker Meyer
	 */
	virtual bool has_prev() const {return has_next();}

	/**
	 * Does this window need a sticky in the title bar?
	 * @return true if such a button is needed
	 */
	virtual bool has_sticky() const { return true; }

	/**
	 * Does this window need its title to be shown?
	 * @return if false title and all gadgets will be not drawn
	 */
	virtual bool has_title() const { return true; }

	// position of a connected thing on the map
	// The calling parameter is only true when actually clicking the gadget
	virtual koord3d get_weltpos( bool /*set*/ ) { return koord3d::invalid; }

	// returns true, when the window show the current object already
	virtual bool is_weltpos() { return false; }

	bool is_dirty() const { return dirty; }

	/**
	 * Set resize mode
	 * @author Markus Weber
	 * @date   11-May-2002
	 */
	void set_resizemode(resize_modes mode) { resize_mode = mode; }

	/**
	 * Get resize mode
	 * @author Markus Weber
	 * @date   25-May-2002
	 */
	resize_modes get_resizemode() { return resize_mode; }

	/**
	 * Returns true, if inside window area.
	 */
	virtual bool is_hit(int x, int y)
	{
		scr_size size = get_windowsize();
		return (  x>=0  &&  y>=0  &&  x<size.w  &&  y<size.h  );
	}

	/**
	 * Events are notified to GUI components via this method.
	 * @author Hj. Malthaner
	 */
	virtual bool infowin_event(const event_t *ev);

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
	 * @author Hj. Malthaner
	 */
	virtual void draw(scr_coord pos, scr_size size);

	// called, when the map is rotated
	virtual void map_rotate90( sint16 /*new_ysize*/ ) { }


	/**
	 * Set focus to component.
	 *
	 * @warning Calling this method from anything called from
	 * gui_container_t::infowin_event (e.g. in action_triggered)
	 * will have NO effect.
	 */
	void set_focus( gui_component_t *k ) { container.set_focus(k); }

	virtual gui_component_t *get_focus() { return container.get_focus(); }
};

#endif
