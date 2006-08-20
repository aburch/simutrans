//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  bridge_reader.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: bridge_reader.h      $       $Author: hajo $
//  $Revision: 1.3 $         $Date: 2004/10/30 09:20:49 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: bridge_reader.h,v $
//  Revision 1.3  2004/10/30 09:20:49  hajo
//  sync for Dario
//
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __BRIDGE_READER_H
#define __BRIDGE_READER_H

/////////////////////////////////////////////////////////////////////////////
//
//  includes
//
/////////////////////////////////////////////////////////////////////////////

#include "obj_reader.h"

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      bridge_reader_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class bridge_reader_t : public obj_reader_t {
    static bridge_reader_t the_instance;

    bridge_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
    virtual bool successfully_loaded() const;

public:
    static bridge_reader_t*instance() { return &the_instance; }

    /**
     * Read a goods info node. Does version check and
     * compatibility transformations.
     * @author Hj. Malthaner
     */
    obj_besch_t * read_node(FILE *fp, obj_node_info_t &node);

    virtual obj_type get_type() const { return obj_bridge; }
    virtual const char *get_type_name() const { return "bridge"; }
};

#endif // __BRIDGE_READER_H
