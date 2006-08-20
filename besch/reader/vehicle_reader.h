//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  vehicle_reader.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: vehicle_reader.h     $       $Author: hajo $
//  $Revision: 1.4 $         $Date: 2004/01/16 21:35:20 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: vehicle_reader.h,v $
//  Revision 1.4  2004/01/16 21:35:20  hajo
//  Hajo: sync with Hendrik
//
//  Revision 1.3  2003/06/29 10:33:38  hajo
//  Hajo: added Volkers changes
//
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __VEHICLE_READER_H
#define __VEHICLE_READER_H

/////////////////////////////////////////////////////////////////////////////
//
//  includes
//
/////////////////////////////////////////////////////////////////////////////

#include "obj_reader.h"

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      vehicle_reader_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class vehicle_reader_t : public obj_reader_t {
    static vehicle_reader_t the_instance;

    vehicle_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
    virtual bool successfully_loaded() const;

public:
    static vehicle_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_vehicle; }
    virtual const char *get_type_name() const { return "vehicle"; }

    /* Read a node. Does version check and compatibility transformations.
     * @author Hj. Malthaner
     */
    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);
};

#endif // __VEHICLE_READER_H
