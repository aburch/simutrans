//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  crossing_writer.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: crossing_writer.cpp  $       $Author: hajo $
//  $Revision: 1.2 $         $Date: 2002/09/25 19:31:17 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: crossing_writer.cc,v $
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"

#include "obj_node.h"
#include "obj_pak_exception.h"
#include "../kreuzung_besch.h"
#include "text_writer.h"
#include "image_writer.h"

#include "crossing_writer.h"

#ifdef _MSC_VER
#define STRICMP stricmp
#else
#define STRICMP strcasecmp
#endif

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      crossing_writer_t::write_obj()
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
void crossing_writer_t::write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj)
{
    kreuzung_besch_t besch;

    obj_node_t	node(this, sizeof(besch), &parent, true);

    write_head(fp, node, obj);

    const char *waytype = obj.get("waytype[ns]");

    if(!STRICMP(waytype, "road")) {
	besch.wegtyp_ns = weg_t::strasse;
    } else if(!STRICMP(waytype, "track")) {
	besch.wegtyp_ns = weg_t::schiene;
    } else {
	cstring_t reason;

	reason.printf("invalid waytype %s for crossing %s\n", waytype, obj.get("name"));

	throw new obj_pak_exception_t("crossing_writer_t", reason);
    }
    waytype = obj.get("waytype[ew]");

    if(!STRICMP(waytype, "road")) {
	besch.wegtyp_ow = weg_t::strasse;
    } else if(!STRICMP(waytype, "track")) {
	besch.wegtyp_ow = weg_t::schiene;
    } else {
	cstring_t reason;

	reason.printf("invalid waytype %s for crossing %s\n", waytype, obj.get("name"));

	throw new obj_pak_exception_t("crossing_writer_t", reason);
    }
    image_writer_t::instance()->write_obj(fp, node, obj.get("image"));

    node.write_data(fp, &besch);
    node.write(fp);
}
