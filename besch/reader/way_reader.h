//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  way_reader.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: way_reader.h         $       $Author: hajo $
//  $Revision: 1.3 $         $Date: 2004/10/30 09:20:49 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: way_reader.h,v $
//  Revision 1.3  2004/10/30 09:20:49  hajo
//  sync for Dario
//
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __WAY_READER_H
#define __WAY_READER_H

/////////////////////////////////////////////////////////////////////////////
//
//  includes
//
/////////////////////////////////////////////////////////////////////////////

#include "obj_reader.h"


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      way_reader_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class way_reader_t : public obj_reader_t {
    static way_reader_t the_instance;

    way_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);

    virtual bool successfully_loaded() const;
public:
    static way_reader_t*instance() { return &the_instance; }

    /**
     * Read a way info node. Does version check and
     * compatibility transformations.
     * @author Hj. Malthaner
     */
    virtual obj_besch_t * read_node(FILE *fp, obj_node_info_t &node);

    virtual obj_type get_type() const { return obj_way; }
    virtual const char *get_type_name() const { return "way"; }
};

#endif // __WAY_READER_H
