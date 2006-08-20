//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  factory_writer.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: factory_writer.cpp   $       $Author: hajo $
//  $Revision: 1.4 $         $Date: 2003/10/29 22:00:39 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: factory_writer.cc,v $
//  Revision 1.4  2003/10/29 22:00:39  hajo
//  Hajo: sync for Hendrik Siegeln
//
//  Revision 1.3  2003/02/26 09:41:37  hajo
//  Hajo: sync for 0.81.23exp
//
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include "../../dataobj/tabfile.h"

#include "obj_node.h"
#include "../skin_besch.h"
#include "../fabrik_besch.h"
#include "text_writer.h"
#include "building_writer.h"
#include "factory_writer.h"
#include "xref_writer.h"

#ifdef _MSC_VER
#define STRICMP stricmp
#else
#define STRICMP strcasecmp
#endif


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      factory_smoke_writer_t::write_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      FILE *outfp
//      obj_node_t &parent
//      tabfileobj_t &obj
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void factory_smoke_writer_t::write_obj(FILE *outfp, obj_node_t &parent, tabfileobj_t &obj)
{
    rauch_besch_t besch;

    memset(&besch, 0, sizeof(besch));

    obj_node_t	node(this, sizeof(besch), &parent, true);

    xref_writer_t::instance()->write_obj(outfp, node, obj_smoke, obj.get("smoke"), true);
    besch.pos_off   = obj.get_koord(	"smoketile",	koord(0, 0));
    besch.xy_off    = obj.get_koord(	"smokeoffset",	koord(0, 0));
    besch.zeitmaske = obj.get_int(	"smokespeed",	0);

    node.write_data(outfp, &besch);
    node.write(outfp);
}

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      factory_product_writer_t::write_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      FILE *outfp
//      obj_node_t &parent
//      int capacity
//      const char *warename
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void factory_product_writer_t::write_obj(FILE *outfp,
					 obj_node_t &parent,
					 int capacity,
					 int factor,
					 const char *warename)
{
    obj_node_t	node(this, sizeof(uint16)*3, &parent, false);

    xref_writer_t::instance()->write_obj(outfp, node, obj_good, warename, true);

    // Hajo: Version needs high bit set as trigger -> this is required
    //       as marker because formerly nodes were unversionend
    uint16 data = 0x8001;
    node.write_data_at(outfp, &data, 0, sizeof(uint16));

    data = capacity;
    node.write_data_at(outfp, &data, 2, sizeof(uint16));

    data = factor;
    node.write_data_at(outfp, &data, 4, sizeof(uint16));

    node.write(outfp);
}

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      factory_supplier_writer_t::write_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      FILE *outfp
//      obj_node_t &parent
//      int capacity
//      int count
//      const char *warename
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void factory_supplier_writer_t::write_obj(FILE *outfp, obj_node_t &parent, int capacity, int count, int verbrauch, const char *warename)
{
    fabrik_lieferant_besch_t besch;

    obj_node_t	node(this, sizeof(besch), &parent, true);

    besch.anzahl = count;
    besch.kapazitaet = capacity;
    besch.verbrauch = verbrauch;

    xref_writer_t::instance()->write_obj(outfp, node, obj_good, warename, true);

    node.write_data(outfp, &besch);
    node.write(outfp);
}

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      factory_writer_t::write_obj()
//
//---------------------------------------------------------------------------
//  beschription:
//      ...
//
//  Arguments:
//      FILE *fp
//      obj_node_t &parent
//      tabfileobj_t &obj
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void factory_writer_t::write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj)
{
    fabrik_besch_t besch;

    const char *placing = obj.get("location");

    if(!STRICMP(placing, "land")) {
	besch.platzierung = fabrik_besch_t::Land;
    } else if(!STRICMP(placing, "water")) {
	besch.platzierung = fabrik_besch_t::Wasser;
    } else if(!STRICMP(placing, "city")) {
	besch.platzierung = fabrik_besch_t::Stadt;
    }
    besch.produktivitaet = obj.get_int("productivity", 10);
    besch.bereich = obj.get_int("range", 10);
    besch.gewichtung = obj.get_int("distributionweight", 1);
    besch.kennfarbe = obj.get_int("mapcolor", 168/*???*/);

    obj_node_t	node(this, sizeof(besch), &parent, true);

    obj.put("type", "fac");

    // name/copyright is in building - write_head(fp, node, obj);
    building_writer_t::instance()->write_obj(fp, node, obj);
    if(*obj.get("smoke")) {
	factory_smoke_writer_t::instance()->write_obj(fp, node, obj);
    }
    else {
	xref_writer_t::instance()->write_obj(fp, node, obj_smoke, "", false);
    }
    for(besch.lieferanten = 0; ; besch.lieferanten++) {
	char buf[40];

	sprintf(buf, "inputgood[%d]", besch.lieferanten);
	const char *good = obj.get(buf);

	if(!good || !*good) {
	    break;
	}
	sprintf(buf, "inputsupplier[%d]", besch.lieferanten);
	int supp = obj.get_int(buf, 1);
	sprintf(buf, "inputcapacity[%d]", besch.lieferanten);
	int cap = obj.get_int(buf, 0);
	sprintf(buf, "inputfactor[%d]", besch.lieferanten);
	int verbrauch = (obj.get_int(buf, 100) * 256) / 100;

	factory_supplier_writer_t::instance()->write_obj(fp, node, cap, supp, verbrauch, good);
    }
    for(besch.produkte = 0; ; besch.produkte++) {
	char buf[40];

	sprintf(buf, "outputgood[%d]", besch.produkte);
	const char *good = obj.get(buf);

	if(!good || !*good) {
	    break;
	}
	sprintf(buf, "outputcapacity[%d]", besch.produkte);
	int cap = obj.get_int(buf, 0);

	sprintf(buf, "outputfactor[%d]", besch.produkte);
	int fac = (obj.get_int(buf, 100) * 256) / 100;

	factory_product_writer_t::instance()->write_obj(fp, node, cap, fac, good);
    }
    node.write_data(fp, &besch);
    node.write(fp);
}


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      factory_writer_t::get_node_name()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Return type:
//      cstring_t
//
//  Arguments:
//      (FILE *fp
/////////////////////////////////////////////////////////////////////////////
//@EDOC
cstring_t factory_writer_t::get_node_name(FILE *fp) const
{
    obj_node_info_t node;   // Gebäude - wehe nicht

    fread(&node, sizeof(node), 1, fp);
    if(node.type != obj_building)
	return "";  //???

    fseek(fp, node.size, SEEK_CUR);

    return building_writer_t::instance()->get_node_name(fp);
}

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
