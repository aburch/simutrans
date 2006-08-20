#ifndef factorylist_frame_t_h
#define factorylist_frame_t_h

#include "../gui/gui_frame.h"
#include "../dataobj/translator.h"


class gui_scrollpane_t;
class factorylist_stats_t;
class factorylist_header_t;
class karte_t;


/**
 * Factory list window
 * @author Hj. Malthaner
 */
class factorylist_frame_t : public gui_frame_t
{
 private:

  gui_scrollpane_t *scrolly;
  factorylist_stats_t *stats;
  factorylist_header_t *head;

 public:

  factorylist_frame_t(karte_t * welt);
  ~factorylist_frame_t();


  /**
   * resize window in response to a resize event
   * @author Hj. Malthaner
   */
  virtual void resize(const koord delta);

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author V. Meyer
     */
    virtual const char * gib_hilfe_datei() const {return "factorylist_filter.txt"; }
};

#endif // factorylist_frame_t_h
