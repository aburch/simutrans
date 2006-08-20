//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  raucher.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: raucher.h            $       $Author: hajo $
//  $Revision: 1.4 $         $Date: 2002/09/18 19:11:17 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: raucher.h,v $
//  Revision 1.4  2002/09/18 19:11:17  hajo
//  Volker: new config system
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __RAUCHER_H
#define __RAUCHER_H

/////////////////////////////////////////////////////////////////////////////
//
//  forward declarations
//
/////////////////////////////////////////////////////////////////////////////

class karte_t;
class ding_t;
class rauch_besch_t;
template <class X> class stringhashtable_tpl;


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      raucher_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class raucher_t : public ding_t
{
private:
    static stringhashtable_tpl<const rauch_besch_t *> besch_table;

    static const rauch_besch_t *gib_besch(const char *name);

    const rauch_besch_t *besch;
public:
    static void register_besch(const rauch_besch_t *besch, const char *name);

    raucher_t(karte_t *welt, loadsave_t *file);
    raucher_t(karte_t *welt, koord3d pos, const rauch_besch_t *besch);

    bool step(long delta_t);
    void zeige_info();

    void rdwr(loadsave_t *file);

    enum ding_t::typ gib_typ() const {return raucher;};

};

#endif // __RAUCHER_H
