#ifndef gui_scrolled_list_h
#define gui_scrolled_list_h

#include "gui_scrollbar.h"
#include "../ifc/scrollbar_listener.h"
#include "../../ifc/gui_action_creator.h"

class karte_t;
template <class T> class slist_tpl;

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
class gui_scrolled_list_t  : public gui_komponente_action_creator_t, public scrollbar_listener_t
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

  scrollbar_t *sb;

  slist_tpl<const char *> * item_list;
  int total_vertical_size() const;

public:

  gui_scrolled_list_t(enum type);
  virtual ~gui_scrolled_list_t();


  /**
   * Sets the color of selected entry
   * @author Hj. Malthaner
   */
  void setze_highlight_color(int c) {
    highlight_color = c;
  };


	void show_selection(int s);

  void setze_selection(int s) { selection = s; }
  int gib_selection() { return selection; }
  int get_count() { return item_list->count(); }

  void scrollbar_moved(class scrollbar_t *sb, int range, int value);

  /*  when rebuilding a list, be sure to call recalculate the slider
   *  with recalculate_slider() to update the scrollbar properly. */
  // clear list of elements
  void clear_elements();
  void insert_element(const char *string, const int pos = 0);
  void append_element(const char *string);
  const char *get_element(int i) const { return ((unsigned)i<item_list->count()) ? item_list->at(i) : ""; };
  void remove_element(const int pos);

  /**
   * request other pane-size. returns realized size.
   * @return value can be in between current and wanted.
   */
  koord request_groesse(koord request);

  void infowin_event(const event_t *ev);
  void zeichnen(koord pos) const;
};

#endif
