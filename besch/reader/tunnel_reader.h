//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  tunnel_reader.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: tunnel_reader.h      $       $Author: hajo $
//  $Revision: 1.3 $         $Date: 2002/12/05 20:33:55 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: tunnel_reader.h,v $
//  Revision 1.3  2002/12/05 20:33:55  hajo
//  Hajo: checking for sync with Volker, 0.81.10exp
//
//  Revision 1.1  2002/09/18 19:13:22  hajo
//  Volker: new config system
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __TUNNEL_READER_H
#define __TUNNEL_READER_H

/////////////////////////////////////////////////////////////////////////////
//
//  includes
//
/////////////////////////////////////////////////////////////////////////////

#include "obj_reader.h"


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      tunnel_reader_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class tunnel_reader_t : public obj_reader_t {
    static tunnel_reader_t the_instance;

    tunnel_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);

    bool successfully_loaded() const;
public:
    static tunnel_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_tunnel; }
    virtual const char *get_type_name() const { return "tunnel"; }
};


/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
#endif // __TUNNEL_READER_H
