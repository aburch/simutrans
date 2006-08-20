/*
 * warenbauer.h
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef warenbauer_t_h
#define warenbauer_t_h


#include "../tpl/slist_tpl.h"
#include "../tpl/ptrhashtable_tpl.h"
#include "../tpl/stringhashtable_tpl.h"

class ware_besch_t;

/**
 * Factory-Klasse fuer Waren.
 *
 * @author Hj. Malthaner
 */
class warenbauer_t
{
 private:

  static stringhashtable_tpl<const ware_besch_t *> besch_names;

  static slist_tpl<const ware_besch_t *> waren;

 public:

  static const ware_besch_t *passagiere;
  static const ware_besch_t *post;
  static const ware_besch_t *nichts;

  static bool alles_geladen();
  static bool register_besch(const ware_besch_t *besch);

  /**
   * Sucht information zur ware 'name' und gibt die
   * Beschreibung davon zurück. Gibt NULL zurück wenn die
   * Ware nicht bekannt ist.
   *
   * @param name der nicht-übersetzte Warenname
   * @author Hj. Malthaner/V. Meyer
   */
  static const ware_besch_t *gib_info(const char* name);

  static const ware_besch_t *gib_info(unsigned int idx) {
    return waren.at(idx);
  }

  static bool ist_fabrik_ware(const ware_besch_t *ware)
  {
    return ware != post && ware != passagiere && ware != nichts;
  }

  static int gib_index(const ware_besch_t *ware)
  {
      return waren.index_of(ware);
  }

  static unsigned int gib_waren_anzahl()
  {
    return waren.count();
  }
};

#endif // warenbauer_t_h
