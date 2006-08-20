//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  roadsign_writer_t.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: roadsign_writer_t.cpp      $       $Author: hajo $
//  $Revision: 1.4 $         $Date: 2004/01/16 21:35:20 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"

#include "../roadsign_besch.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist_writer.h"

#include "roadsign_writer.h"
#include "skin_writer.h"

void roadsign_writer_t::write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj)
{
	obj_node_t	node(this, 5, &parent, false);	/* false, because we write this ourselves */

	// Hajodoc: Preferred height of this tree type
	// Hajoval: int (useful range: 0-14)
	roadsign_besch_t besch;
	besch.min_speed = obj.get_int("min_speed", 0);
	besch.flags = (obj.get_int("single_way", 0)>0) + (obj.get_int("free_route", 0)>0)*2;

	// Hajo: temp vars of appropriate size
	uint16 v16;
	uint8 v8;

	// Hajo: write version data
	v16 = 0x8001;
	node.write_data_at(fp, &v16, 0, sizeof(uint16));

	v16 = (uint16) besch.min_speed;
	node.write_data_at(fp, &v16, 2, sizeof(uint8));

	v8 = (uint8)besch.flags;
	node.write_data_at(fp, &v8, 4, sizeof(uint8));

	write_head(fp, node, obj);

	// add the images
	slist_tpl<cstring_t> keys;
	cstring_t str;

	for(int i=0; i<16; i++) {
		char buf[40];

		sprintf(buf, "image[%i]", i);
		str = obj.get(buf);
		// make sure, there are always 4, 8, 12, ... images (for all directions)
		if(str.len()==0  &&  (i%4)==0) {
			break;
		}
		keys.append(str);
	}
	imagelist_writer_t::instance()->write_obj(fp, node, keys);

	// probably add some icons, if defined
	slist_tpl<cstring_t> cursorkeys;

	cstring_t c=cstring_t(obj.get("cursor")), i=cstring_t(obj.get("icon"));
	cursorkeys.append(c);
	cursorkeys.append(i);
	if(c.len()>0  ||  i.len()>0) {
		cursorskin_writer_t::instance()->write_obj(fp, node, obj, cursorkeys);
	}

	node.write(fp);
}

/*
Hajoexstart:
Obj=roadsign
Name=no_entry
min_speed=0
*/
