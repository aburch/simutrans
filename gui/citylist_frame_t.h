#ifndef citylist_frame_t_h
#define citylist_frame_t_h

#include "../gui/gui_frame.h"

class gui_scrollpane_t;
class citylist_stats_t;
class stadt_t;
class karte_t;


/**
 * City list window
 * @author Hj. Malthaner
 */
class citylist_frame_t : public gui_frame_t
{
 private:

  gui_scrollpane_t *scrolly;
  citylist_stats_t *stats;

 public:

  citylist_frame_t(karte_t * welt);
  ~citylist_frame_t();


  /**
   * resize window in response to a resize event
   * @author Hj. Malthaner
   */
  virtual void resize(const koord delta);
};

#endif // citylist_frame_t_h
