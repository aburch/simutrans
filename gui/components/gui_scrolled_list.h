#ifndef gui_scrolled_list_h
#define gui_scrolled_list_h

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

	struct item {
		const char *text;
		uint8 color;
	};

	slist_tpl<item> item_list;
	int total_vertical_size() const;

public:
	gui_scrolled_list_t(enum type);

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
	// clear list of elements
	void clear_elements();
	void insert_element(const char *string, const int pos=0, const uint8 color=COL_BLACK);
	void append_element(const char *string, const uint8 color=COL_BLACK);
	const char *get_element(int i) const { return ((unsigned)i<item_list.count()) ? item_list.at(i).text : ""; }
	void remove_element(const int pos);

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
