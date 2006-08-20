/*
 * goods_stats_t.h
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef good_stats_t_h
#define good_stats_t_h

#include "../ifc/gui_komponente.h"

/**
 * Display information about each configured good
 * as a list like display
 * @author Hj. Malthaner
 */
class goods_stats_t : public gui_komponente_t
{

 public:


  goods_stats_t();


  /**
   * Zeichnet die Komponente
   * @author Hj. Malthaner
   */
  void zeichnen(koord offset) const;

};

#endif // good_stats_t_h
