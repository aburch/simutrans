//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  good_writer.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: good_writer.cpp      $       $Author: hajo $
//  $Revision: 1.6 $         $Date: 2004/10/30 09:20:49 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: good_writer.cc,v $
//  Revision 1.6  2004/10/30 09:20:49  hajo
//  sync for Dario
//
//  Revision 1.5  2004/01/16 21:35:20  hajo
//  Hajo: sync with Hendrik
//
//  Revision 1.4  2003/10/29 22:00:39  hajo
//  Hajo: sync for Hendrik Siegeln
//
//  Revision 1.3  2002/12/05 20:33:55  hajo
//  Hajo: checking for sync with Volker, 0.81.10exp
//
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"

#include "../ware_besch.h"
#include "obj_node.h"
#include "text_writer.h"

#include "good_writer.h"


/**
 * Writes goods node data to file
 * @author Hj. Malthaner
 */
void good_writer_t::write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj)
{
    obj_node_t	node(this, sizeof(uint16)*5, &parent, false);

    write_head(fp, node, obj);
    text_writer_t::instance()->write_obj(fp, node, obj.get("metric"));


    uint16 value;

    // Hajo: version number
    // Hajo: Version needs high bit set as trigger -> this is required
    //       as marker because formerly nodes were unversionend
    value = 0x8002;
    node.write_data_at(fp, &value, 0, sizeof(uint16));

    value = obj.get_int("value", 0);
    node.write_data_at(fp, &value, 2, sizeof(uint16));

    value = obj.get_int("catg", 0);
    node.write_data_at(fp, &value, 4, sizeof(uint16));

    value = obj.get_int("speed_bonus", 0);
    node.write_data_at(fp, &value, 6, sizeof(uint16));

    value = obj.get_int("weight_per_unit", 100);
    node.write_data_at(fp, &value, 8, sizeof(uint16));


    node.write(fp);
}


/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
