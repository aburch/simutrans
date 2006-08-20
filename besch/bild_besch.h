/*
 *
 *  bild_besch.h
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
#ifndef __BILD_BESCH_H
#define __BILD_BESCH_H


#ifdef __cplusplus
extern "C" {
#endif

/*
 *  includes
 */
#ifdef __cplusplus
#include "obj_besch.h"
#else
#include "../simtypes.h"
#endif

/*
 *  class:
 *      bild_besch_t()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Beschreibung eines Bildes. Da wir teilweise C brauchen, gibt es 2
 *	Versionen -> aufpassen: bei virtuellen Methoden krachte es.
 *
 *  Kindknoten:
 *	(keine)
 */
#ifdef __cplusplus
struct bild_besch_t : public obj_besch_t {
public:
    const char *gib_daten() const
    {
	return reinterpret_cast<const char *>(this + 1);
    }
    int gib_nummer() const
    {
	return bild_nr;
    }
#else
struct bild_besch_t {
  void *verboten;
#endif
  uint8   x;	//changed dims to unsigned!
  uint8   w;
  uint8   y;
  uint8   h;
  sint32  len;
  uint16  bild_nr;	// Speichern wir erstmal als Dummy mit, wird von register_image() ersetzt
  uint8   zoomable;   // Hajo: some image may not be zoomed i.e. icons
};

#ifdef __cplusplus
}
#endif

#endif // __BILD_BESCH_H
