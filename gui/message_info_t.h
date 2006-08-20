/*
 * stadt_info.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef message_info_h
#define message_info_h


#include "../simimg.h"
#include "infowin.h"
#include "gui_frame.h"

class karte_t;


/**
 * Display a message
 * @author prissi
 */
class message_info_t : public infowin_t
{
private:
	karte_t *welt;
	int	bild;
	int color;
	koord k;
	const char *text;

public:
  message_info_t::message_info_t(karte_t *welt,int color,char *text,koord k,int bild);
  ~message_info_t();

    /**
     * gibt farbinformationen fuer Fenstertitel, -ränder und -körper
     * zurück
     * @author Hj. Malthaner
     */
    fensterfarben gib_fensterfarben() const;

    /**
     * Jedes Objekt braucht ein Bild.
     *
     * @author Hj. Malthaner
     * @return Die Nummer des aktuellen Bildes für das Objekt.
     */
    int gib_bild() const;

    /**
     * @return Die aktuelle Planquadrat-Koordinate des Objekts
     *
     * @author Hj. Malthaner
     */
    koord3d gib_pos() const {return koord3d(k,1);};

    /**
     * in top-level fenstern wird der Name in der Titelzeile dargestellt
     * @return den nicht uebersetzten Namen der Komponente
     * @author Hj. Malthaner
     */
    const char *gib_name() const;

    /**
     * Zeichnet die Komponente
     * @author Hj. Malthaner
     */
    void info(cbuffer_t & buf) const;
};

#endif //message_frame_h
