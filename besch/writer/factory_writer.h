//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  factory_writer.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: factory_writer.h     $       $Author: hajo $
//  $Revision: 1.3 $         $Date: 2003/10/29 22:00:39 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: factory_writer.h,v $
//  Revision 1.3  2003/10/29 22:00:39  hajo
//  Hajo: sync for Hendrik Siegeln
//
//  Revision 1.2  2003/02/26 09:41:37  hajo
//  Hajo: sync for 0.81.23exp
//
//  Revision 1.1  2002/09/18 19:13:22  hajo
//  Volker: new config system
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __FACTORY_WRITER_H
#define __FACTORY_WRITER_H

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
//      factory_smoke_writer_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class factory_smoke_writer_t : public obj_writer_t {
    static factory_smoke_writer_t the_instance;

    factory_smoke_writer_t() { register_writer(false); }
public:
    static factory_smoke_writer_t *instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_fsmoke; }
    virtual const char *get_type_name() const { return "factory smoke"; }

    void write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj);
};

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      factory_product_writer_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class factory_product_writer_t : public obj_writer_t {
    static factory_product_writer_t the_instance;

    factory_product_writer_t() { register_writer(false); }
public:
    static factory_product_writer_t *instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_fproduct; }
    virtual const char *get_type_name() const { return "factory product"; }

    void write_obj(FILE *outfp, obj_node_t &parent, int capacity, int factor, const char *warename);
};

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      factory_supplier_writer_t;()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class factory_supplier_writer_t : public obj_writer_t {
    static factory_supplier_writer_t the_instance;

    factory_supplier_writer_t() { register_writer(false); }
public:
    static factory_supplier_writer_t *instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_fsupplier; }
    virtual const char *get_type_name() const { return "factory supplier"; }

    void write_obj(FILE *outfp, obj_node_t &parent, int capacity, int count, int verbrauch, const char *warename);
};

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      factory_writer_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class factory_writer_t : public obj_writer_t {
    static factory_writer_t the_instance;

    factory_writer_t() { register_writer(true); }
protected:
    virtual cstring_t get_node_name(FILE *fp) const;
public:
    virtual void write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj);

    virtual obj_type get_type() const { return obj_factory; }
    virtual const char *get_type_name() const { return "factory"; }

};

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
#endif // __FACTORY_WRITER_H
