//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  registered_obj_writer.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: registered_obj_writer$       $Author: hajo $
//  $Revision: 1.2 $         $Date: 2002/09/25 19:31:18 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: obj_writer.h,v $
//  Revision 1.2  2002/09/25 19:31:18  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __OBJ_WRITER
#define __OBJ_WRITER

/////////////////////////////////////////////////////////////////////////////
//
//  includes
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "../objversion.h"
#include "../../utils/cstring_t.h"

/////////////////////////////////////////////////////////////////////////////
//
//  forward declarations
//
/////////////////////////////////////////////////////////////////////////////

class obj_node_t;
struct obj_node_info_t;
class tabfileobj_t;
template<class X> class stringhashtable_tpl;
template<class key, class X> class inthashtable_tpl;

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      obj_writer_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC

class obj_writer_t {
    static stringhashtable_tpl<obj_writer_t *> *writer_by_name;
    static inthashtable_tpl<obj_type, obj_writer_t *> *writer_by_type;

protected:
    obj_writer_t() { /* Beware: Cannot register here! */}

    void register_writer(bool main_obj);
    void dump_nodes(FILE *infp, int level);
    void list_nodes(FILE *infp);
    void skip_nodes(FILE *fp);
    void show_capabilites();

    virtual cstring_t get_node_name(FILE * /*fp*/) const { return ""; }
    cstring_t name_from_next_node(FILE *fp)const;

    virtual void dump_node(FILE *infp, const obj_node_info_t &node);
    virtual void write_obj(FILE * /*fp*/, obj_node_t &/*parent*/, tabfileobj_t &/*obj*/) {}
    void write_head(FILE *fp, obj_node_t &node, tabfileobj_t &obj);
public:
    virtual obj_type get_type() const = 0;
    virtual const char *get_type_name() const = 0;

    static void write(FILE *fp, obj_node_t &parent, tabfileobj_t &obj);
};

#endif // __OBJ_WRITER
