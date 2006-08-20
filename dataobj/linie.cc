#include "../simdebug.h"

#include "linie.h"
#include "../tpl/slist_tpl.h"



/**
 * Liste aller Linien
 * @author Hj. Malthaner
 */
static slist_tpl<linie_t *> alle_linien;


slist_tpl<linie_t *> & linie_t::gib_alle_linien()
{
  return alle_linien;
}


/**
 * In alten Spielständen hat jedes Fahrzeug eigene Linien-Info.
 * Diese Methode erlaubt es gemeinsame Infos zu identifizieren.
 *
 * @return Zeiger auf die Linieninfo
 * @author Hj. Malthaner
 */

linie_t * linie_t::vereinige(array_tpl<struct linieneintrag_t> * alt)
{

 slist_iterator_tpl<linie_t *> iter (alle_linien);


 while(iter.next()) {
   linie_t * linie = iter.get_current();

   bool equal = true;

   if(linie->eintrag.get_size() == alt->get_size()) {

     for(unsigned int i=0; i<alt->get_size(); i++) {
       equal &=
	 alt->at(i).pos == linie->eintrag.at(i).pos &&
	 alt->at(i).ladegrad == linie->eintrag.at(i).ladegrad;
     }
   }

   if(equal) {
     return linie;
   }
 }

 // gabs noch nicht, neu anlegen


 return new linie_t(alt);
}


linie_t::linie_t(array_tpl<struct linieneintrag_t> *alt) : eintrag(16)
{
  for(unsigned int i=0; i<eintrag.get_size(); i++) {

    eintrag.at(i).pos = alt->at(i).pos;
    eintrag.at(i).ladegrad = alt->at(i).ladegrad;
  }

  alle_linien.insert(this);

  sprintf(name, "Linie %p", this);

  DBG_MESSAGE("linie_t", "List now contains %d lines", alle_linien.count());
}


const char * linie_t::gib_name() const
{
  return name;
}
