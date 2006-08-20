/*
 *
 *  haus_besch.cpp
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      ...
 *
 */
#include "../simdebug.h"

#include "haus_besch.h"


/*
 *  member function:
 *      haus_tile_besch_t::gib_layout()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Rehcnet aus dem Index das Layout aus, zu dem diese Tile gehört.
 *
 *  Return type:
 *      int
 */
int haus_tile_besch_t::gib_layout() const
{
    koord groesse = gib_besch()->gib_groesse();

    return index / (groesse.x * groesse.y);
}

/*
 *  member function:
 *      haus_tile_besch_t::gib_offset()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Bestimmt die Relativ-Position des Einzelbildes im Gesamtbild des
 *	Gebäudes.
 *
 *  Return type:
 *      koord
 */
koord haus_tile_besch_t::gib_offset() const
{
    const haus_besch_t *besch = gib_besch();
    koord groesse = besch->gib_groesse(gib_layout());	// ggf. gedreht

    return koord( index % groesse.x, (index / groesse.x) % groesse.y );
}


/**
 * Mail generation level
 * @author Hj. Malthaner
 */
int haus_besch_t::gib_post_level() const
{
  switch(gtyp) {
  case gebaeude_t::wohnung:
    return level;
  case gebaeude_t::gewerbe:
    return level << 1;
  case gebaeude_t::industrie:
    return level >> 1;
  default:
    return level;
  }
}



/*
 *  member function:
 *      haus_besch_t::gib_tile()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Abhängig von Position und LAyout ein tile zurückliefern
 *
 *  Return type:
 *      const haus_tile_besch_t *
 *
 *  Argumente:
 *      int layout
 *      int x
 *      int y
 */
const haus_tile_besch_t *haus_besch_t::gib_tile(int layout, int x, int y) const
{
    layout = layout_anpassen(layout);
    koord dims = gib_groesse(layout);

    if(layout < 0 || x < 0 || y < 0 ||layout >= layouts || x >= gib_b(layout) || y >= gib_h(layout)) {
	dbg->fatal("hausbauer_t::gib_tile()",
	           "invalid request for l=%d, x=%d, y=%d on building %s (l=%d, x=%d, y=%d)",
		   layout, x, y, gib_name(), layouts, groesse.x, groesse.y);
    }
    return gib_tile(layout * dims.x * dims.y + y * dims.x + x);
}


/*
 *  member function:
 *      haus_besch_t::layout_anpassen()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Layout normalisieren.
 *
 *  Return type:
 *      int
 *
 *  Argumente:
 *      int layout
 */
int haus_besch_t::layout_anpassen(int layout) const
{
    if(layout >= 2 && layouts <= 2) {
	// Sind Layout C und D nicht definiert, nehemen wir ersatzweise A und B
	layout -= 2;
    }
    if(layout > 0 && layouts <= 1) {
	// Ist Layout B nicht definiert und das Teil quadratisch, nehmen wir ersatzweise A
	if(groesse.x == groesse.y) {
	    layout--;
	}
    }
    return layout;
}
