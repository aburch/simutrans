//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  xref_writer.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: xref_writer.h        $       $Author: hajo $
//  $Revision: 1.1 $         $Date: 2002/09/18 19:13:22 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: xref_writer.h,v $
//  Revision 1.1  2002/09/18 19:13:22  hajo
//  Volker: new config system
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __XREF_WRITER_H
#define __XREF_WRITER_H

/////////////////////////////////////////////////////////////////////////////
//
//  includes
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include "obj_writer.h"
#include "../objversion.h"


/////////////////////////////////////////////////////////////////////////////
//
//  forward declarations
//
/////////////////////////////////////////////////////////////////////////////

class cstring_t;
class obj_node_t;


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      xref_writer_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class xref_writer_t : public obj_writer_t {
    static xref_writer_t the_instance;

    xref_writer_t() { register_writer(false); }
public:
    static xref_writer_t *instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_xref; }
    virtual const char *get_type_name() const { return "xref"; }

    void write_obj(FILE *fp, obj_node_t &parent, obj_type type, const char *text, bool fatal);
    void dump_node(FILE *infp, const obj_node_info_t &node);
};


/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
#endif // __XREF_WRITER_H
