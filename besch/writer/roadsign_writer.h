//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  roadsign_writer.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: roadsign_writer.h        $       $Author: hajo $
//  $Revision: 1.1 $         $Date: 2002/09/18 19:13:22 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: roadsign_writer.h,v $
//  Revision 1.1  2002/09/18 19:13:22  hajo
//  Volker: new config system
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __ROADSIGN_WRITER_H
#define __ROADSIGN_WRITER_H

/////////////////////////////////////////////////////////////////////////////
//
//  includes
//
/////////////////////////////////////////////////////////////////////////////

#include "obj_writer.h"
#include "../objversion.h"

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      roadsign_writer_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class roadsign_writer_t : public obj_writer_t {
    static roadsign_writer_t the_instance;

    roadsign_writer_t() { register_writer(true); }
protected:
    virtual cstring_t get_node_name(FILE *fp) const { return name_from_next_node(fp); }
public:
    virtual void write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj);

    virtual obj_type get_type() const { return obj_roadsign; }
    virtual const char *get_type_name() const { return "roadsign"; }
};

#endif // __ROADSIGN_WRITER_H
