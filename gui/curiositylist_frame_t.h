#ifndef curiositylist_frame_t_h
#define curiositylist_frame_t_h

#include "../gui/gui_frame.h"
#include "../gui/curiositylist_stats_t.h"
#include "../dataobj/translator.h"


class gui_scrollpane_t;
class curiositylist_header_t;
class karte_t;


/**
 * Curiosity list window
 * @author Hj. Malthaner
 */
class curiositylist_frame_t : public gui_frame_t
{
 private:

  	gui_scrollpane_t *scrolly;
  	curiositylist_stats_t *stats;
	curiositylist_header_t *header;

 public:

  curiositylist_frame_t(karte_t * welt);
  ~curiositylist_frame_t();

  // update list
  void update() { stats->get_unique_attractions(); }

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
    virtual const char * gib_hilfe_datei() const {return "curiositylist_filter.txt"; }
};

#endif // curiositylist_frame_t_h
