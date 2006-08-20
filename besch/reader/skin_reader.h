//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  skin_reader.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: skin_reader.h        $       $Author: hajo $
//  $Revision: 1.2 $         $Date: 2002/09/25 19:31:17 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: skin_reader.h,v $
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __SKIN_READER_H
#define __SKIN_READER_H

/////////////////////////////////////////////////////////////////////////////
//
//  includes
//
/////////////////////////////////////////////////////////////////////////////

#include "obj_reader.h"


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      skin_reader_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class skin_reader_t : public obj_reader_t {

protected:
    virtual void register_obj(obj_besch_t *&data);
    virtual bool successfully_loaded() const;

    virtual skinverwaltung_t::skintyp_t get_skintype() const = 0;
public:
    virtual obj_type get_type() const = 0;
    virtual const char *get_type_name() const = 0;
};


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      menuskin_reader_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class menuskin_reader_t : public skin_reader_t {
    static menuskin_reader_t the_instance;

    menuskin_reader_t() { register_reader(); }
protected:
    virtual skinverwaltung_t::skintyp_t get_skintype() const { return skinverwaltung_t::menu; }
public:
    static menuskin_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_menu; }
    virtual const char *get_type_name() const { return "menu"; }
};


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      cursorskin_reader_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class cursorskin_reader_t : public skin_reader_t {
    static cursorskin_reader_t the_instance;

    cursorskin_reader_t() { register_reader(); }
protected:
    virtual skinverwaltung_t::skintyp_t get_skintype() const { return skinverwaltung_t::cursor; }
public:
    static cursorskin_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_cursor; }
    virtual const char *get_type_name() const { return "cursor"; }

};


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      symbolskin_reader_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class symbolskin_reader_t : public skin_reader_t {
    static symbolskin_reader_t the_instance;

    symbolskin_reader_t() { register_reader(); }
protected:
    virtual skinverwaltung_t::skintyp_t get_skintype() const { return skinverwaltung_t::symbol; }
public:
    static symbolskin_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_symbol; }
    virtual const char *get_type_name() const { return "symbol"; }

};

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      smoke_reader_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class smoke_reader_t : public skin_reader_t {
    static smoke_reader_t the_instance;

    smoke_reader_t() { register_reader(); }
protected:
    virtual skinverwaltung_t::skintyp_t get_skintype() const {
	return skinverwaltung_t::nothing;
    }
public:
    static smoke_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_smoke; }
    virtual const char *get_type_name() const { return "smoke"; }

};


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      smoke_reader_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class miscimages_reader_t : public skin_reader_t {
    static miscimages_reader_t the_instance;

    miscimages_reader_t() { register_reader(); }
protected:
    virtual skinverwaltung_t::skintyp_t get_skintype() const { return skinverwaltung_t::misc; }
public:
    static miscimages_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_miscimages; }
    virtual const char *get_type_name() const { return "misc"; }
};

#endif // __SKIN_READER_H
