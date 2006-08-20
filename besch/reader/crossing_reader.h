//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  crossing_reader.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: crossing_reader.h    $       $Author: hajo $
//  $Revision: 1.2 $         $Date: 2002/09/25 19:31:17 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: crossing_reader.h,v $
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __CROSSING_READER_H
#define __CROSSING_READER_H

/////////////////////////////////////////////////////////////////////////////
//
//  includes
//
/////////////////////////////////////////////////////////////////////////////

#include "obj_reader.h"


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      crossing_reader_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class crossing_reader_t : public obj_reader_t {
    static crossing_reader_t the_instance;

    crossing_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
    virtual bool successfully_loaded() const;
public:
    static crossing_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_crossing; }
    virtual const char *get_type_name() const { return "crossing"; }
};

#endif // __CROSSING_READER_H
