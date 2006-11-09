#ifndef linie_t_h
#define linie_t_h

#include "linieneintrag.h"
#include "../tpl/array_tpl.h"


template <class T> class slist_tpl;

/**
 * Eine Linie ist eine Reihe von Haltepunkten, und einer Menge zugeprdneter
 * fahrzeuge die dieser Liinie zugeordnet sind
 *
 * @author Hj. Malthaner
 */
class linie_t
{
 private:

  char name[64];

 public:

  static slist_tpl<linie_t *> & gib_alle_linien();


  /**
   * In alten Spielständen hat jedes Fahrzeug eigene Linien-Info.
   * Diese Methode erlaubt es gemeinsame Infos zu identifizieren.
   *
   * @return Zeiger auf die Linieninfo
   * @author Hj. Malthaner
   */

  static linie_t * vereinige(array_tpl<struct linieneintrag_t> * alt);


  /**
   * Einträge für diese Linie
   * @author Hj. Malthaner
   */
  array_tpl<struct linieneintrag_t> eintrag;



  linie_t(array_tpl<struct linieneintrag_t> *alt);


  const char * gib_name() const;

};

#endif
