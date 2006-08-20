/* fundament.cc
 *
 * Untergrund für Gebäude in Simutrans.
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include "fundament.h"
#include "../simhalt.h"
#include "../mm/mempool.h"


mempool_t * fundament_t::mempool = new mempool_t(sizeof(fundament_t) );


fundament_t::fundament_t(karte_t *welt, loadsave_t *file) : grund_t(welt)
{
  rdwr(file);
}

fundament_t::fundament_t(karte_t *welt, koord3d pos) : grund_t(welt, pos)
{
  setze_bild( -1 );
}


/**
 * Auffforderung, ein Infofenster zu öffnen.
 * @author Hj. Malthaner
 */
void fundament_t::zeige_info()
{
  if(gib_halt().is_bound()) {
    gib_halt()->zeige_info();
  }
}


void
fundament_t::calc_bild()
{
  grund_t::calc_bild();
  setze_bild( -1 );
}


void * fundament_t::operator new(size_t /*s*/)
{
    return mempool->alloc();
}


void fundament_t::operator delete(void *p)
{
    mempool->free( p );
}
