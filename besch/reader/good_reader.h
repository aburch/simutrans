//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  good_reader.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: good_reader.h        $       $Author: hajo $
//  $Revision: 1.3 $         $Date: 2003/10/29 22:00:39 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: good_reader.h,v $
//  Revision 1.3  2003/10/29 22:00:39  hajo
//  Hajo: sync for Hendrik Siegeln
//
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __GOOD_READER_H
#define __GOOD_READER_H

/////////////////////////////////////////////////////////////////////////////
//
//  includes
//
/////////////////////////////////////////////////////////////////////////////

#include "obj_reader.h"


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      good_reader_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class good_reader_t : public obj_reader_t {
    static good_reader_t the_instance;

    good_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
    virtual bool successfully_loaded() const;
public:
    static good_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_good; }
    virtual const char *get_type_name() const { return "good"; }

    /**
     * Read a goods info node. Does version check and
     * compatibility transformations.
     * @author Hj. Malthaner
     */
    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);
};

#endif // __GOOD_READER_H
