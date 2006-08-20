//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  tree_writer.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: tree_writer.cpp      $       $Author: hajo $
//  $Revision: 1.4 $         $Date: 2004/01/16 21:35:20 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: tree_writer.cc,v $
//  Revision 1.4  2004/01/16 21:35:20  hajo
//  Hajo: sync with Hendrik
//
//  Revision 1.3  2004/01/01 11:34:43  hajo
//  Hajo: merge with Hendriks update
//
//  Revision 1.2  2002/09/25 19:31:18  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"

#include "../baum_besch.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist2d_writer.h"

#include "tree_writer.h"

// Hajodoc: Tree object type
// Hajoval: string ("tree")
// "obj"
//
// Hajodoc: Tree name (must be unique)
// Hajoval: string
// "name"

void tree_writer_t::write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj)
{
    baum_besch_t besch;

    obj_node_t	node(this, sizeof(besch), &parent, true);

    // Hajodoc: Preferred height of this tree type
    // Hajoval: int (useful range: 0-14)
    besch.hoehenlage = obj.get_int("height", 0);

    write_head(fp, node, obj);

    slist_tpl< slist_tpl<cstring_t> > keys;

    for(unsigned int age = 0; ; age++) {
	for(int h = 0; ; h++) {
	    char buf[40];

	    // Hajodoc: Images of the tree
	    // Hajoval: int age, int h (age in 0..4, h usually 0)
	    sprintf(buf, "image[%d][%d]", age, h);

	    cstring_t str = obj.get(buf);
	    if(str.len() == 0) {
		break;
	    }
	    if(h == 0) {
		keys.append( slist_tpl<cstring_t>() );
	    }
	    keys.at(age).append(str);
	}
	if(keys.count() <= age) {
	    break;
	}
    }
    imagelist2d_writer_t::instance()->write_obj(fp, node, keys);

    node.write_data(fp, &besch);
    node.write(fp);
}

/*
Hajoexstart:
Obj=tree
Name=Tree1
Height=0
Image[0][0]=../images/ls-trees01.0.0
Image[1][0]=../images/ls-trees01.0.1
Image[2][0]=../images/ls-trees01.0.2
Image[3][0]=../images/ls-trees01.0.3
Image[4][0]=../images/ls-trees01.0.4
Hajoexend:
*/


/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
