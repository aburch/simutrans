#ifndef gui_scrolled_list_h
#define gui_scrolled_list_h

#include <string.h>

#include "gui_scrollbar.h"
#include "action_listener.h"
#include "../../ifc/gui_action_creator.h"
#include "../../simcolor.h"

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
class gui_scrolled_list_t  : public gui_komponente_action_creator_t, action_listener_t
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
		virtual uint8 gib_color() { return color; }
		virtual void set_color(uint8 col) { color = col; }
		virtual const char *get_text() = 0;
		virtual void set_text(char *) = 0;
		virtual bool is_valid() { return true; }	//  can be used to indicate invalid entries
	};

	// editable text
	class var_text_scrollitem_t : public scrollitem_t {
	private:
		const char *text;
	public:
		var_text_scrollitem_t( const char *t, uint8 col ) : scrollitem_t(col) {
			text = strdup( t );
		}
		virtual ~var_text_scrollitem_t() { delete text; }
		const char *get_text() { return text; }
		virtual void set_text(char *t) {
			delete text;
			text = strdup(t);
		}
	};

	// only uses pointer, non-editable
	class const_text_scrollitem_t : public scrollitem_t {
	private:
		const char *text;
	public:
		const_text_scrollitem_t( const char *t, uint8 col ) : scrollitem_t(col) { text = t; }
		virtual ~const_text_scrollitem_t() {}
		const char *get_text() { return text; }
		virtual void set_text(char *) {}
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
	void setze_highlight_color(int c) { highlight_color = c; }

	void show_selection(int s);

	void setze_selection(int s) { selection = s; }
	int gib_selection() { return selection; }
	int get_count() { return item_list.count(); }

	/*  when rebuilding a list, be sure to call recalculate the slider
	 *  with recalculate_slider() to update the scrollbar properly. */
	void clear_elements();
	void append_element( scrollitem_t *item );
	scrollitem_t *get_element(int i) const { return ((unsigned)i<item_list.count()) ? item_list.at(i) : NULL; }

	// set the first element to be shown in the list
	sint32 get_sb_offset() { return sb.gib_knob_offset(); }
	void set_sb_offset( sint32 off ) { sb.setze_knob_offset( off ); offset = sb.gib_knob_offset(); }

	/**
	 * request other pane-size. returns realized size.
	 * @return value can be in between current and wanted.
	 */
	koord request_groesse(koord request);

	void infowin_event(const event_t *ev);
	void zeichnen(koord pos);

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	bool action_triggered(gui_komponente_t *komp, value_t extra);
};

#endif
