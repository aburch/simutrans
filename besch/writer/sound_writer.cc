//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  ground_writer.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: ground_writer.cpp    $       $Author: hajo $
//  $Revision: 1.6 $         $Date: 2003/07/23 19:55:53 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: ground_writer.cc,v $
//  Revision 1.6  2003/07/23 19:55:53  hajo
//  Hajo: sync for Volker
//
//  Revision 1.5  2003/03/26 20:59:24  hajo
//  Hajo: sync with other developers
//
//  Revision 1.4  2003/02/26 09:41:37  hajo
//  Hajo: sync for 0.81.23exp
//
//  Revision 1.3  2003/02/02 10:15:42  hajo
//  Hajo: sync for 0.81.21exp
//
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include "../../dataobj/tabfile.h"

#include "obj_node.h"
#include "../sound_besch.h"
#include "text_writer.h"

#include "sound_writer.h"

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      sound_writer_t::write_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      FILE *fp
//      obj_node_t &parent
//      tabfileobj_t &obj
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void sound_writer_t::write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj)
{
    obj_node_t	node(this, 4, &parent, true);

    write_head(fp, node, obj);

    // Hajo: version number
    // Hajo: Version needs high bit set as trigger -> this is required
    //       as marker because formerly nodes were unversionend
    uint16 uv16 = 0x8001;
    node.write_data_at(fp, &uv16, 0, sizeof(uint16));

    uv16 = obj.get_int("sound_nr", NO_SOUND);	// for compatibility reasons; the old nr of a sound
    node.write_data_at(fp, &uv16, 2, sizeof(uint16));

    node.write(fp);
}
