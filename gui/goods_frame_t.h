/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef goods_frame_t_h
#define goods_frame_t_h

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_scrollpane.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "goods_stats_t.h"

class karte_t;

/**
 * Shows statistics. Only goods so far.
 * @author Hj. Malthaner
 */
class goods_frame_t : public gui_frame_t, private action_listener_t
{
private:
	enum sort_mode_t { unsortiert=0, nach_name=1, nach_gewinn=2, nach_bonus=3, nach_catg=4, SORT_MODES=5 };
	static const char *sort_text[SORT_MODES];

	// static, so we remember the last settings
	static int relative_speed_change;
	static bool sortreverse;
	static sort_mode_t sortby;

	karte_t * welt;
	char	speed_bonus[6];
	char	speed_message[256];
	uint16 good_list[256];

	gui_label_t sort_label;
	button_t	sortedby;
	button_t	sorteddir;
	gui_label_t change_speed_label;
	button_t	speed_up;
	button_t	speed_down;

	goods_stats_t goods_stats;
	gui_scrollpane_t scrolly;

	// creates the list and pass it to the child finction good_stats, which does the display stuff ...
	static int compare_goods(const void *p1, const void *p2);
	void sort_list();

public:
  goods_frame_t(karte_t *wl);

  /**
   * resize window in response to a resize event
   * @author Hj. Malthaner
   * @date   16-Oct-2003
   */
  void resize(const koord delta);

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author V. Meyer
     */
    const char * get_hilfe_datei() const {return "goods_filter.txt"; }

    /**
     * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
     * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
     * in dem die Komponente dargestellt wird.
     * @author Hj. Malthaner
     */
    void zeichnen(koord pos, koord gr);

    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     *
     * Returns true, if action is done and no more
     * components should be triggered.
     * V.Meyer
     */
    bool action_triggered( gui_action_creator_t *komp, value_t extra);
};

#endif
