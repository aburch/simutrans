//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  simskin.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: simskin.h            $       $Author: hajo $
//  $Revision: 1.5 $         $Date: 2004/10/30 09:20:48 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: simskin.h,v $
//  Revision 1.5  2004/10/30 09:20:48  hajo
//  sync for Dario
//
//  Revision 1.4  2004/02/03 19:26:11  hajo
//  Hajo: sync for Hendrik
//
//  Revision 1.3  2003/07/23 19:55:52  hajo
//  Hajo: sync for Volker
//
//  Revision 1.2  2003/01/08 19:54:02  hajo
//  Hajo: preparation for delivery to volker
//
//  Revision 1.1  2002/09/18 19:18:18  hajo
//  Volker: new config system
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __SIMSKIN_H
#define __SIMSKIN_H

/////////////////////////////////////////////////////////////////////////////
//
//  includes
//
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
//
//  forward declarations
//
/////////////////////////////////////////////////////////////////////////////

class skin_besch_t;

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      skinverwaltung_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class skinverwaltung_t {
public:
    enum skintyp_t { nothing, menu, cursor, symbol, misc };

    static const skin_besch_t *hauptmenu;
    static const skin_besch_t *schienen_werkzeug;
    static const skin_besch_t *strassen_werkzeug;
    static const skin_besch_t *schiffs_werkzeug;
    static const skin_besch_t *hang_werkzeug;
    static const skin_besch_t *special_werkzeug;
    static const skin_besch_t *listen_werkzeug;
    static const skin_besch_t *farbmenu;
    static const skin_besch_t *edit_werkzeug;

    static const skin_besch_t *logosymbol;
    static const skin_besch_t *neujahrsymbol;
    static const skin_besch_t *neueweltsymbol;
    static const skin_besch_t *flaggensymbol;
    static const skin_besch_t *meldungsymbol;
    static const skin_besch_t *bauigelsymbol;

    // for the stops in the list
    static const skin_besch_t *zughaltsymbol;
    static const skin_besch_t *autohaltsymbol;
    static const skin_besch_t *shiffshaltsymbol;
    static const skin_besch_t *bushaltsymbol;

    static const skin_besch_t *fragezeiger;
    static const skin_besch_t *signalzeiger;
    static const skin_besch_t *downzeiger;
    static const skin_besch_t *upzeiger;
    static const skin_besch_t *belegtzeiger;
    static const skin_besch_t *killzeiger;
    static const skin_besch_t *slopezeiger;
    static const skin_besch_t *fahrplanzeiger;
    static const skin_besch_t *strassentunnelzeiger;
    static const skin_besch_t *schienentunnelzeiger;
    static const skin_besch_t *frachthofzeiger;
    static const skin_besch_t *werftNSzeiger;
    static const skin_besch_t *werftOWzeiger;
    static const skin_besch_t *stadtzeiger;
    static const skin_besch_t *baumzeiger;
    static const skin_besch_t *undoc_zeiger;

    static const skin_besch_t *signale;
    static const skin_besch_t *presignals;
    static const skin_besch_t *baugrube;
    static const skin_besch_t *fussweg;
    static const skin_besch_t *pumpe;
    static const skin_besch_t *senke;

    static const skin_besch_t *oberleitung;
    static const skin_besch_t *electricity;

    /**
     * Window skin images
     * @author Hj. Malthaner
     */
    static const skin_besch_t *window_skin;

    static bool register_besch(skintyp_t type, const skin_besch_t *besch);
    static bool alles_geladen(skintyp_t type);

};

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
#endif // __SIMSKIN_H
