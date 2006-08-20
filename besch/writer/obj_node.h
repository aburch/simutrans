//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  obj_node.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: obj_node.h           $       $Author: hajo $
//  $Revision: 1.2 $         $Date: 2002/09/25 19:31:18 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: obj_node.h,v $
//  Revision 1.2  2002/09/25 19:31:18  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __OBJ_NODE_H
#define __OBJ_NODE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
//
//  includes
//
/////////////////////////////////////////////////////////////////////////////

#include "../obj_node_info.h"
#include <stdio.h>

/////////////////////////////////////////////////////////////////////////////
//
//  forward declarations
//
/////////////////////////////////////////////////////////////////////////////

class  obj_writer_t;

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      obj_node_t()
//
//---------------------------------------------------------------------------
//  Description:
//       ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class obj_node_t {
    static uint32 free_offset;	    // next free offset in file

    obj_node_info_t desc;

    uint32 write_offset;    // Start of node data in file (after node.desc)
    obj_node_t *parent;
    bool adjust;	    // a normal besch structure start with 4 extra bytes
public:
    //
    // set_start_offset() - set offset of first node in file
    // ONLY CALL BEFORE ANY NODES ARE CREATED !!!
    //
    static void set_start_offset(uint32 offset) { free_offset = offset; }

    //
    // construct a new node, parameters:
    //	    writer  object, that writes the node to the file
    //	    size    space needed for node data
    //	    parent  parent node
    //
    obj_node_t(obj_writer_t *writer, int size, obj_node_t *parent, bool adjust);
    //
    // Write the complete node data to the file
    //
    void write_data(FILE *fp, const void *data);
    //
    // Write part of the node data to the file
    // The caller is responsible that all areas are written y multiple calls
    // Throws obj_pak_exception_t
    //
    void write_data_at(FILE *fp, const void *data, int offset, int size);
    //
    // Write the internal node info to the file
    // DO THIS AFTER ALL CHILD NODES ARE WRITTEN !!!
    //
    void write(FILE *fp);
};

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
#endif // __OBJ_NODE_H
