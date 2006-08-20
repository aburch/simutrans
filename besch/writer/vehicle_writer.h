//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  vehicle_writer.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: vehicle_writer.h     $       $Author: hajo $
//  $Revision: 1.2 $         $Date: 2004/01/16 21:35:20 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: vehicle_writer.h,v $
//  Revision 1.2  2004/01/16 21:35:20  hajo
//  Hajo: sync with Hendrik
//
//  Revision 1.1  2002/09/18 19:13:22  hajo
//  Volker: new config system
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __VEHICLE_WRITER_H
#define __VEHICLE_WRITER_H

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
//      vehicle_writer_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class vehicle_writer_t : public obj_writer_t {
    static vehicle_writer_t the_instance;

    vehicle_writer_t() { register_writer(true); }
protected:
    virtual cstring_t get_node_name(FILE *fp) const { return name_from_next_node(fp); }
public:

    /**
     * Writes vehicle node data to file
     * @author Hj. Malthaner
     */
    virtual void write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj);

    virtual obj_type get_type() const { return obj_vehicle; }
    virtual const char *get_type_name() const { return "vehicle"; }
};

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
#endif // __VEHICLE_WRITER_H
