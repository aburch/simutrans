//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  root_writer.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: root_writer.h        $       $Author: hajo $
//  $Revision: 1.1 $         $Date: 2002/09/18 19:13:22 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: root_writer.h,v $
//  Revision 1.1  2002/09/18 19:13:22  hajo
//  Volker: new config system
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __ROOT_WRITER_H
#define __ROOT_WRITER_H

/////////////////////////////////////////////////////////////////////////////
//
//  includes
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include "obj_writer.h"
#include "../objversion.h"

/////////////////////////////////////////////////////////////////////////////
//
//  forward declarations
//
/////////////////////////////////////////////////////////////////////////////

class cstring_t;
struct obj_node_info_t;

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      root_writer_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class root_writer_t : public obj_writer_t {
    static root_writer_t the_instance;

    static cstring_t inpath;

    root_writer_t() { register_writer(false); }

    void copy_nodes(FILE *outfp, FILE *infp, obj_node_info_t &info);
    void write_header(FILE *fp);
public:
    void capabilites();
    static root_writer_t *instance() { return &the_instance; }
    virtual obj_type get_type() const { return obj_root; }
    virtual const char *get_type_name() const { return "root"; }

    void write(const char *name, int argc, char *argv[]);
    void dump(int argc, char *argv[]);
    void list(int argc, char *argv[]);
    void copy(const char *name, int argc, char *argv[]);

    static const cstring_t &get_inpath() { return inpath; }
};

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
#endif // __ROOT_WRITER_H
