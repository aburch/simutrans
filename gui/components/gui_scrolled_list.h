#ifndef gui_scrolled_list_h
#define gui_scrolled_list_h

#include "gui_scrollbar.h"
#include "action_listener.h"
#include "gui_action_creator.h"
#include "../../simcolor.h"
#include "../../utils/plainstring.h"

/**
 * Scrollable list.
 * Displays list, scrollbuttons up/down, dragbar.
 * Has a min and a max size, and can be displayed with any size in between.
 * Does ONLY cater for vertical offset (yet).
 * two possible types:
 * -list.      simply lists some items.
 * -selection. is a list, but additionaly, one item can be selected.
 * @author Niels Roest, additions by Hj. Malthaner
 */
class gui_scrolled_list_t :
	public gui_action_creator_t,
	public action_listener_t,
	public gui_komponente_t
{
public:
	enum type { list, select };

	/**
	 * Container for list entries - consisting of text and color
	 */
	class scrollitem_t {
	private:
		COLOR_VAL color;
	public:
		scrollitem_t( COLOR_VAL col ) { color = col; }
		virtual ~scrollitem_t() {}
		virtual uint8 get_color() { return color; }
		virtual void set_color(uint8 col) { color = col; }
		virtual char const* get_text() const = 0;
		virtual void set_text(char const*) = 0;
		virtual bool is_valid() { return true; }	//  can be used to indicate invalid entries
		virtual bool is_editable() { return false; }
	};

	// editable text
	class var_text_scrollitem_t : public scrollitem_t {
	private:
		plainstring text;

	public:
		var_text_scrollitem_t(char const* const t, uint8 const col) : scrollitem_t(col), text(t) {}

		char const* get_text() const OVERRIDE { return text; }

		void set_text(char const *t) OVERRIDE { text = t; }

		bool is_editable() { return true; }
	};

	// only uses pointer, non-editable
	class const_text_scrollitem_t : public scrollitem_t {
	private:
		const char *text;
	public:
		const_text_scrollitem_t( const char *t, uint8 col ) : scrollitem_t(col) { text = t; }
		char const* get_text() const OVERRIDE { return text; }
		void set_text(char const *) OVERRIDE {}
	};

private:
	enum type type;
	int selection; // only used when type is 'select'.
	int border; // must be substracted from groesse.y to get netto size
	int offset; // vertical offset of top left position.

	/**
	 * color of selected entry
	 * @author Hj. Malthaner
	 */
	int highlight_color;

	scrollbar_t sb;

	slist_tpl<gui_scrolled_list_t::scrollitem_t *> item_list;
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
	scrollitem_t *get_element(sint32 i) const { return ((uint32)i<item_list.get_count()) ? item_list.at(i) : NULL; }

	// set the first element to be shown in the list
	sint32 get_sb_offset() { return sb.get_knob_offset(); }
	void set_sb_offset( sint32 off ) { sb.set_knob_offset( off ); offset = sb.get_knob_offset(); }

	// resizes scrollbar
	void adjust_scrollbar();
	/**
	 * request other pane-size. returns realized size.
	 * use this for flexible sized lists
	 * for fixed sized used only set_groesse()
	 * @return value can be in between full-size wanted.
	 */
	koord request_groesse(koord request);

	void set_groesse(koord groesse) OVERRIDE;

	bool infowin_event(event_t const*) OVERRIDE;

	void zeichnen(koord pos);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
