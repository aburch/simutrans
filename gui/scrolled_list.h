#ifndef gui_scrolled_list_h
#define gui_scrolled_list_h

#include "../ifc/gui_komponente.h"

#include "scrollbar.h"
#include "ifc/scrollbar_listener.h"
#include "ifc/selection_listener.h"

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
class scrolled_list_gui_t
: public gui_komponente_t, public scrollbar_listener_t
{
public:

  enum type { list, select };

private:

  /**
   * The list of listeners which want to be informed about selection events.
   * @author Hj. Malthaner
   */
  slist_tpl<selection_listener_t *> * listeners;

  /**
   * Informs (calls) all listeners about a new selection
   * @author Hj. Malthaner
   */
  void call_listeners();


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

  scrolled_list_gui_t(enum type);
  virtual ~scrolled_list_gui_t();


  /**
   * Sets the color of selected entry
   * @author Hj. Malthaner
   */
  void setze_highlight_color(int c) {
    highlight_color = c;
  };


  /**
   * Adds a selection_listener_t to this component
   * @author Hj. Malthaner
   */
  void add_listener(selection_listener_t *c);


  /**
   * Removes a selection_listener_t from this component
   * @author Hj. Malthaner
   */
  void remove_listener(selection_listener_t *c);


  void setze_selection(int s) { selection = s; }
  int gib_selection() { return selection; }

  void scrollbar_moved(class scrollbar_t *sb, int range, int value);

  /*  when rebuilding a list, be sure to call recalculate the slider
   *  with recalculate_slider() to update the scrollbar properly. */
  // clear list of elements
  void clear_elements();
  void insert_element(const char *string, const int pos = 0);
  void append_element(const char *string);
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
