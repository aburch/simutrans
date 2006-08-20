//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  text_writer.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: text_writer.h        $       $Author: hajo $
//  $Revision: 1.1 $         $Date: 2002/09/18 19:13:22 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: text_writer.h,v $
//  Revision 1.1  2002/09/18 19:13:22  hajo
//  Volker: new config system
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __TEXT_WRITER_H
#define __TEXT_WRITER_H

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
//      text_writer_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class text_writer_t : public obj_writer_t {
    static text_writer_t the_instance;

    text_writer_t() { register_writer(false); }
public:
    static text_writer_t *instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_text; }
    virtual const char *get_type_name() const { return "text"; }

    void dump_node(FILE *infp, const obj_node_info_t &node);
    void write_obj(FILE *fp, obj_node_t &parent, const char *text);
};

#endif // __TEXT_WRITER_H
