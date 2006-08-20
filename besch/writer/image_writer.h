//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  image_writer.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: image_writer.h       $       $Author: hajo $
//  $Revision: 1.5 $         $Date: 2003/06/21 09:28:09 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: image_writer.h,v $
//  Revision 1.5  2003/06/21 09:28:09  hajo
//  Hajo: sync
//
//  Revision 1.4  2003/06/14 15:52:09  hajo
//  Hajo: preparing 0.82
//
//  Revision 1.3  2003/02/26 09:41:37  hajo
//  Hajo: sync for 0.81.23exp
//
//  Revision 1.2  2003/02/02 10:15:42  hajo
//  Hajo: sync for 0.81.21exp
//
//  Revision 1.1  2002/09/18 19:13:22  hajo
//  Volker: new config system
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __IMAGE_WRITER_H
#define __IMAGE_WRITER_H

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
class obj_node_t;
struct dimension;

/////////////////////////////////////////////////////////////////////////////
//
//  typedefs
//
/////////////////////////////////////////////////////////////////////////////

typedef unsigned int   PIXRGB;
typedef unsigned short PIXVAL;

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      image_writer_t()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class image_writer_t : public obj_writer_t {
    static image_writer_t the_instance;

    static cstring_t last_img_file;
    static unsigned char *block;
    static unsigned width;
    static unsigned height;
    static int img_size;	// default 64

    image_writer_t() { register_writer(false); }
public:
    static void dump_special_histogramm();

    static image_writer_t *instance() { return &the_instance; }

    static void set_img_size(int _img_size) { img_size = _img_size; }

    virtual obj_type get_type() const { return obj_image; }
    virtual const char *get_type_name() const { return "image"; }

    void write_obj(FILE *fp, obj_node_t &parent, cstring_t imagekey);

private:
    bool block_laden(const char *fname);
    static PIXRGB block_getpix(int x, int y)
    {
	return ((block[y * width * 3 + x * 3]     << 16) +
	        (block[y * width * 3 + x * 3 + 1] << 8) +
		(block[y * width * 3 + x * 3 + 2]));
    }


    /**
     * Encodes an image into a sprite data structure, considers
     * special colors.
     *
     * @author Hj. Malthaner
     */
    static PIXVAL* encode_image(int x, int y, dimension *dim, int *len);
};

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
#endif // __IMAGE_WRITER_H
