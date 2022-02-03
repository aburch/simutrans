/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_GUI_FRAME_H
#define GUI_GUI_FRAME_H


#include "../display/scr_coord.h"
#include "../display/simgraph.h"
#include "../simcolor.h"
#include "../dataobj/koord3d.h"
#include "components/gui_aligned_container.h"
#include "components/gui_button.h"

#include "gui_theme.h"

class loadsave_t;
class karte_ptr_t;
class player_t;

/**
 * A Class for window with Component.
 * Unlike other Window Classes in Simutrans, this is
 * a true component-oriented window that all actions
 * delegates to its component.
 */
class gui_frame_t : protected gui_aligned_container_t
{
public:
	/**
	 * Resize modes
	 */
	enum resize_modes {
		no_resize         = 0,
		vertical_resize   = 1,
		horizontal_resize = 2,
		diagonal_resize   = 3
	};

private:

	const char *name;
	scr_size windowsize; ///< Size of the whole window (possibly with title bar)
	scr_size min_windowsize; ///< min size of the whole window

	resize_modes resize_mode;
	const player_t *owner;

	// set true for total redraw
	bool dirty:1;
	bool opaque:1;

	uint8 percent_transparent;
	PIXVAL color_transparent;

	using gui_aligned_container_t::draw;
	using gui_aligned_container_t::set_size;

protected:
	void set_dirty() { dirty=true; }

	void unset_dirty() { dirty=false; }

	/**
	 * resize window in response to a resize event
	 */
	virtual void resize(const scr_coord delta);

	void set_owner( const player_t *player ) { owner = player; }

	void set_transparent( uint8 percent, PIXVAL col ) { opaque = percent==0; percent_transparent = percent; color_transparent = col; }

	static karte_ptr_t welt;

public:
	/**
	 * @param name Window title
	 * @param player owner for color
	 */
	gui_frame_t(const char *name, const player_t *player=NULL);

	virtual ~gui_frame_t() {}

	/**
	 * The name is displayed in the titlebar
	 * @return the non-translated name of the Component
	 */
	const char *get_name() const { return name; }

	/**
	 * sets the Name (Window title)
	 */
	void set_name(const char *name) { this->name=name; }

	/**
	 * This returns an unique id (different from magic_reserved), if the dialogue can be saved.
	 */
	virtual uint32 get_rdwr_id();

	virtual void rdwr( loadsave_t * ) {}

	/**
	 * get color information for the window title
	 * -borders and -body background
	 */
	virtual FLAGGED_PIXVAL get_titlecolor() const;

	/**
	 * @return gets the window sizes
	 */
	scr_size get_windowsize() const { return windowsize; }

protected:
	/**
	 * Sets the window sizes
	 */
	virtual void set_windowsize(scr_size size);

	/**
	 * Set minimum size of the window
	 */
	void set_min_windowsize(scr_size new_size) { min_windowsize = new_size; }

	/**
	 * Set minimum window size to minimum size of container.
	 */
	void reset_min_windowsize();
public:
	/**
	 * Get minimum size of the window
	 */
	scr_size get_min_windowsize() { return min_windowsize; }

	/**
	 * Max Kielland 2013: Client size auto calculation with title bar and margins.
	 * @return the usable width and height of the window
	*/
	scr_size get_client_windowsize() const {
		return windowsize - scr_size(0, ( has_title()*D_TITLEBAR_HEIGHT ) );
	}

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 */
	virtual const char * get_help_filename() const {return NULL;}

	/**
	 * Does this window need a next button in the title bar?
	 * @return true if such a button is needed
	 */
	virtual bool has_next() const {return false;}

	/**
	 * Does this window need a prev button in the title bar?
	 * @return true if such a button is needed
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
	 */
	void set_resizemode(resize_modes mode) { resize_mode = mode; }

	/**
	 * Get resize mode
	 */
	resize_modes get_resizemode() const { return resize_mode; }

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
	 */
	bool infowin_event(const event_t *ev) OVERRIDE;

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
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
	void set_focus( gui_component_t *c ) { gui_aligned_container_t::set_focus(c); }

	gui_component_t *get_focus() OVERRIDE { return gui_aligned_container_t::get_focus(); }
};

#endif
