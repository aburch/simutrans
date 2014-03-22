/*
 * Scrollable list.
 * Displays list, scrollbuttons up/down, dragbar.
 * Has a min and a max size, and can be displayed with any size in between.
 * Does ONLY cater for vertical offset (yet).
 * two possible types:
 * -list.      simply lists some items.
 * -selection. is a list, but additionally, one item can be selected.
 * @author Niels Roest, additions by Hj. Malthaner
 */

#ifndef gui_scrolled_list_h
#define gui_scrolled_list_h

#include "gui_scrollbar.h"
#include "action_listener.h"
#include "gui_action_creator.h"
#include "../../simcolor.h"
#include "../../display/simgraph.h"
#include "../../utils/plainstring.h"
#include "../../tpl/vector_tpl.h"

class gui_scrolled_list_t :
	public gui_action_creator_t,
	public action_listener_t,
	public gui_komponente_t
{
public:
	enum type { windowskin, listskin };

	/**
	 * Container for list entries - consisting of text and color
	 */
	class scrollitem_t {
	public:
		virtual ~scrollitem_t() {}
		virtual scr_coord_val get_h() const = 0;	// largest object in this list
		virtual scr_coord_val draw( scr_coord pos, scr_coord_val width, bool is_selected, bool has_focus ) = 0;
		virtual char const* get_text() const = 0;
		virtual bool is_valid() { return true; }	//  can be used to indicate invalid entries
		virtual bool is_editable() { return false; }
		virtual bool sort( vector_tpl<scrollitem_t *> &, int, void * ) const { return false; } // not sorted, leave vector as before
	};

	// only uses pointer, non-editable
	class const_text_scrollitem_t : public scrollitem_t {
	protected:
		const char *consttext;
		COLOR_VAL color;
		static bool compare( scrollitem_t *a, scrollitem_t *b );
	public:
		const_text_scrollitem_t(char const* const t, uint8 const col) : consttext(t), color(col) {}

		virtual scr_coord_val draw( scr_coord pos, scr_coord_val width, bool is_selected, bool has_focus );
		virtual scr_coord_val get_h() const { return LINESPACE; }

		virtual uint8 get_color() { return color; }
		virtual void set_color(uint8 col) { color = col; }

		virtual char const* get_text() const { return consttext; }
		virtual void set_text(char const *) {}

		virtual bool sort( vector_tpl<scrollitem_t *>&v, int, void * ) const OVERRIDE;
	};

	// editable text
	class var_text_scrollitem_t : public const_text_scrollitem_t {
	private:
		plainstring text;

	public:
		var_text_scrollitem_t(char const* const t, uint8 const col) : const_text_scrollitem_t(t,col), text(t) {}

		virtual void set_text(char const *t) OVERRIDE { text = t; }

		virtual bool is_editable() { return true; }
	};

private:
	enum type type;
	sint32 selection; // only used when type is 'select'.
	int border; // must be subtracted from size.h to get net size
	int offset; // vertical offset of top left position.

	/**
	 * color of selected entry
	 * @author Hj. Malthaner
	 */
	int highlight_color;

	scrollbar_t sb;

	vector_tpl<gui_scrolled_list_t::scrollitem_t *> item_list;
	int total_vertical_size() const;

public:
	gui_scrolled_list_t(enum type);

	~gui_scrolled_list_t() { clear_elements(); }

	/**
	* Sets the color of selected entry
	* @author Hj. Malthaner
	*/
	void set_highlight_color(int c) { highlight_color = c; }

	void show_selection(int s);

	void set_selection(int s) { selection = s; }
	sint32 get_selection() const { return selection; }
	sint32 get_count() const { return item_list.get_count(); }

	/*  when rebuilding a list, be sure to call recalculate the slider
	 *  with recalculate_slider() to update the scrollbar properly. */
	void clear_elements();
	void append_element( scrollitem_t *item );
	void insert_element( scrollitem_t *item );
	scrollitem_t *get_element(sint32 i) const { return ((uint32)i<item_list.get_count()) ? item_list[i] : NULL; }

	/**
	 * Sorts the list.
	 * Calls the virtual method scrollitem_t::sort of element at position @p offset.
	 * Adjusts scrollbar.
	 * @param offset sort list from element offset to end
	 * @param sort_param will be used in call to scrollitem_t::sort
	 */
	void sort( int offset, void *sort_param );

	// set the first element to be shown in the list
	sint32 get_sb_offset() { return sb.get_knob_offset(); }
	void set_sb_offset( sint32 off ) { sb.set_knob_offset( off ); offset = sb.get_knob_offset(); }

	// resizes scrollbar
	void adjust_scrollbar();
	/**
	 * request other pane-size. returns realized size.
	 * use this for flexible sized lists
	 * for fixed sized used only set_size()
	 * @return value can be in between full-size wanted.
	 */
	scr_size request_size(scr_size size);

	void set_size(scr_size size) OVERRIDE;

	bool infowin_event(event_t const*) OVERRIDE;

	void draw(scr_coord pos);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
