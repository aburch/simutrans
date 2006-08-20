//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  building_writer.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: building_writer.h    $       $Author: hajo $
//  $Revision: 1.1 $         $Date: 2002/09/18 19:13:22 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: building_writer.h,v $
//  Revision 1.1  2002/09/18 19:13:22  hajo
//  Volker: new config system
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __BUILDING_WRITER_H
#define __BUILDING_WRITER_H


/////////////////////////////////////////////////////////////////////////////
//
//  includes
//
/////////////////////////////////////////////////////////////////////////////

#include "obj_writer.h"
#include "../objversion.h"

/////////////////////////////////////////////////////////////////////////////
//
//  forward declarations
//
/////////////////////////////////////////////////////////////////////////////

//class cstring_t;
//class obj_node_t;
template<class T> class slist_tpl;

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      tile_writer_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class tile_writer_t : public obj_writer_t {
    static tile_writer_t the_instance;

    tile_writer_t() { register_writer(false); }
public:
    static tile_writer_t *instance() { return &the_instance; }

    virtual void write_obj(FILE *fp, obj_node_t &parent, int index,
	const slist_tpl< slist_tpl<cstring_t> > &backkeys,
	const slist_tpl< slist_tpl<cstring_t> > &frontkeys);
protected:
    virtual obj_type get_type() const { return obj_tile; }
    virtual const char *get_type_name() const { return "tile"; }
};

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      building_writer_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class building_writer_t : public obj_writer_t {
    static building_writer_t the_instance;

    building_writer_t() { register_writer(true); }
public:
    virtual cstring_t get_node_name(FILE *fp) const { return name_from_next_node(fp); }

    static building_writer_t *instance() { return &the_instance; }

    virtual void write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj);

    virtual obj_type get_type() const { return obj_building; }
    virtual const char *get_type_name() const { return "building"; }

};

#endif // __BUILDING_WRITER_H
