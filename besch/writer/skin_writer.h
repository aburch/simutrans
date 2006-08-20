//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  skin_writer.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: skin_writer.h        $       $Author: hajo $
//  $Revision: 1.2 $         $Date: 2002/09/25 19:31:18 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: skin_writer.h,v $
//  Revision 1.2  2002/09/25 19:31:18  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __SKIN_WRITER_H
#define __SKIN_WRITER_H

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
//      skin_writer_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class skin_writer_t : public obj_writer_t {
protected:
    virtual cstring_t get_node_name(FILE *fp) const { return name_from_next_node(fp); }
public:
    virtual void write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj);
    void write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj,
	const slist_tpl<cstring_t> &imagekeys);

    virtual obj_type get_type() const = 0;
    virtual const char *get_type_name() const = 0;
};


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      menuskin_writer_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class menuskin_writer_t : public skin_writer_t {
    static menuskin_writer_t the_instance;

    menuskin_writer_t() { register_writer(true); }
public:
    static menuskin_writer_t *instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_menu; }
    virtual const char *get_type_name() const { return "menu"; }
};


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      cursorskin_writer_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class cursorskin_writer_t : public skin_writer_t {
    static cursorskin_writer_t the_instance;

    cursorskin_writer_t() { register_writer(true); }
public:
    static cursorskin_writer_t *instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_cursor; }
    virtual const char *get_type_name() const { return "cursor"; }
};


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      symbolskin_writer_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class symbolskin_writer_t : public skin_writer_t {
    static symbolskin_writer_t the_instance;

    symbolskin_writer_t() { register_writer(true); }
public:
    static symbolskin_writer_t *instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_symbol; }
    virtual const char *get_type_name() const { return "symbol"; }
};



//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      smoke_writer_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class smoke_writer_t : public skin_writer_t {
    static smoke_writer_t the_instance;

    smoke_writer_t() { register_writer(true); }
public:
    static smoke_writer_t *instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_smoke; }
    virtual const char *get_type_name() const { return "smoke"; }
};

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      miscimages_writer_t()
//
//---------------------------------------------------------------------------
//  Description:
//      Used for images needed by the game but not yet integrated as real
//      objects.
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class miscimages_writer_t : public skin_writer_t {
    static miscimages_writer_t the_instance;

    miscimages_writer_t() { register_writer(true); }
public:
    static miscimages_writer_t *instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_miscimages; }
    virtual const char *get_type_name() const { return "misc"; }
};

#endif // __SKIN_WRITER_H
