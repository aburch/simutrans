//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  imagelist_reader.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: imagelist_reader.h   $       $Author: hajo $
//  $Revision: 1.1 $         $Date: 2002/09/18 19:13:21 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: imagelist_reader.h,v $
//  Revision 1.1  2002/09/18 19:13:21  hajo
//  Volker: new config system
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __IMAGELIST_READER_H
#define __IMAGELIST_READER_H

/////////////////////////////////////////////////////////////////////////////
//
//  includes
//
/////////////////////////////////////////////////////////////////////////////

#include "obj_reader.h"

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      imagelist_reader_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class imagelist_reader_t : public obj_reader_t {
    static imagelist_reader_t the_instance;

    imagelist_reader_t() { register_reader(); }
public:
    static imagelist_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_imagelist; }
    virtual const char *get_type_name() const { return "imagelist"; }
};


/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
#endif // __IMAGELIST_READER_H
