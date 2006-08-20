//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  obj_reader.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: obj_reader.h         $       $Author: hajo $
//  $Revision: 1.5 $         $Date: 2004/01/16 21:35:20 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: obj_reader.h,v $
//  Revision 1.5  2004/01/16 21:35:20  hajo
//  Hajo: sync with Hendrik
//
//  Revision 1.4  2003/11/22 16:53:50  hajo
//  Hajo: integrated Hendriks changes
//
//  Revision 1.3  2003/10/29 22:00:39  hajo
//  Hajo: sync for Hendrik Siegeln
//
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __OBJ_READER_H
#define __OBJ_READER_H

/////////////////////////////////////////////////////////////////////////////
//
//  includes
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include "../obj_besch.h"
#include "../objversion.h"

/////////////////////////////////////////////////////////////////////////////
//
//  forward declarations
//
/////////////////////////////////////////////////////////////////////////////

struct obj_node_info_t;
template<class key_t, class value_t> class inthashtable_tpl;
template<class value_t> class stringhashtable_tpl;
template<class key_t, class value_t> class ptrhashtable_tpl;
template<class T> class slist_tpl;


// Hajo: some decoding helper functions

/**
 * Reads uint8 from memory area. Advances pointer by 1 byte.
 * @author Hj. Malthaner
 */
extern uint8 decode_uint8(char * &data);


/**
 * Reads sint8 from memory area. Advances pointer by 1 byte.
 * @author Hj. Malthaner
 */
extern sint8 decode_sint8(char * &data);


/**
 * Reads uint16 from memory area. Advances pointer by 2 bytes.
 * @author Hj. Malthaner
 */
extern uint16 decode_uint16(char * &data);


/**
 * Reads sint16 from memory area. Advances pointer by 2 bytes.
 * @author Hj. Malthaner
 */
extern sint16 decode_sint16(char * &data);


/**
 * Reads uint32 from memory area. Advances pointer by 4 bytes.
 * @author Hj. Malthaner
 */
extern uint32 decode_uint32(char * &data);



//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      obj_reader_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class obj_reader_t {
    //
    // table of registered obj readers sorted by id
    //
    static inthashtable_tpl<obj_type, obj_reader_t *> *obj_reader;
    //
    // object addresses needed for resolving xrefs later
    // - stored in a hashhash table with type and name
    //
    static inthashtable_tpl<obj_type, stringhashtable_tpl<obj_besch_t *> > loaded;
    static inthashtable_tpl<obj_type, stringhashtable_tpl< slist_tpl<obj_besch_t **> > > unresolved;
    static ptrhashtable_tpl<obj_besch_t **, int>  fatals;

    static void read_file(const char *name);
    static void read_nodes(FILE *fp, obj_besch_t *parent, obj_besch_t *&data);
    static void skip_nodes(FILE *fp);

protected:
    static void delete_node(obj_besch_t *node);

    obj_reader_t() { /* Beware: Cannot register here! */}

    static void obj_for_xref(obj_type type, const char *name, obj_besch_t *data);
    static void xref_to_resolve(obj_type type, const char *name, obj_besch_t **dest, bool fatal);
    static void resolve_xrefs();

    /**
     * Hajo 11-Oct-03:
     * this method reads a node. I made this virtual to
     * allow subclasses to define their own strategies how to
     * read nodes.
     */
    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);
    virtual void register_obj(obj_besch_t *&/*data*/) {}
    virtual bool successfully_loaded() const { return true; }

    void register_reader();
public:
    virtual obj_type get_type() const = 0;
    virtual const char *get_type_name() const = 0;

    static bool init(const char *liste);
};

#endif // __OBJ_READER_H
