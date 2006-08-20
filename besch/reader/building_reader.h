//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  building_reader.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: building_reader.h    $       $Author: hajo $
//  $Revision: 1.3 $         $Date: 2003/11/22 16:53:50 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: building_reader.h,v $
//  Revision 1.3  2003/11/22 16:53:50  hajo
//  Hajo: integrated Hendriks changes
//
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __BUILDING_READER_H
#define __BUILDING_READER_H

/////////////////////////////////////////////////////////////////////////////
//
//  includes
//
/////////////////////////////////////////////////////////////////////////////

#include "obj_reader.h"


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      tile_reader_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class tile_reader_t : public obj_reader_t {
    static tile_reader_t the_instance;

    tile_reader_t() { register_reader(); }
public:
    static tile_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_tile; }
    virtual const char *get_type_name() const { return "tile"; }

    /* Read a node. Does version check and compatibility transformations.
     * @author Hj. Malthaner
     */
    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);
};


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      building_reader_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class building_reader_t : public obj_reader_t {
    static building_reader_t the_instance;

    building_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
    virtual bool successfully_loaded() const;

public:
    static building_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_building; }
    virtual const char *get_type_name() const { return "building"; }

    /* Read a node. Does version check and compatibility transformations.
     * @author Hj. Malthaner
     */
    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);

};

#endif // __BUILDING_READER_H
