//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  factory_reader.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: factory_reader.h     $       $Author: hajo $
//  $Revision: 1.3 $         $Date: 2003/10/29 22:00:39 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: factory_reader.h,v $
//  Revision 1.3  2003/10/29 22:00:39  hajo
//  Hajo: sync for Hendrik Siegeln
//
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __FACTORY_READER_H
#define __FACTORY_READER_H

/////////////////////////////////////////////////////////////////////////////
//
//  includes
//
/////////////////////////////////////////////////////////////////////////////

#include "obj_reader.h"

class factory_smoke_reader_t : public obj_reader_t {
    static factory_smoke_reader_t the_instance;

    factory_smoke_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
public:
    static factory_smoke_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_fsmoke; }
    virtual const char *get_type_name() const { return "factory smoke"; }
};

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      factory_supplier_reader_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class factory_supplier_reader_t : public obj_reader_t {
    static factory_supplier_reader_t the_instance;

    factory_supplier_reader_t() { register_reader(); }
public:
    static factory_supplier_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_fsupplier; }
    virtual const char *get_type_name() const { return "factory supplier"; }
};

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      factory_product_reader_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class factory_product_reader_t : public obj_reader_t {
    static factory_product_reader_t the_instance;

    factory_product_reader_t() { register_reader(); }
public:
    static factory_product_reader_t*instance() { return &the_instance; }

    /**
     * Read a factory product node. Does version check and
     * compatibility transformations.
     * @author Hj. Malthaner
     */
    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);

    virtual obj_type get_type() const { return obj_fproduct; }
    virtual const char *get_type_name() const { return "factory product"; }
};

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      factory_reader_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class factory_reader_t : public obj_reader_t {
    static factory_reader_t the_instance;

    factory_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
public:

    static factory_reader_t*instance() { return &the_instance; }

    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);
    virtual obj_type get_type() const { return obj_factory; }
    virtual const char *get_type_name() const { return "factory"; }
};

#endif // __FACTORY_READER_H
