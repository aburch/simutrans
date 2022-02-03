/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "convoy_item.h"
#include "../simconvoi.h"
#include "../tool/simmenu.h"
#include "../world/simworld.h"
#include "../utils/cbuffer.h"


const char* convoy_scrollitem_t::get_text() const
{
	return cnv->get_name();
}


PIXVAL convoy_scrollitem_t::get_color() const
{
	return cnv->get_status_color();
}


void convoy_scrollitem_t::set_text(char const* const t)
{
	if(  t  &&  t[0]  &&  strcmp( t, cnv->get_name() )  ) {
		// text changed => call tool
		cbuffer_t buf;
		buf.printf("c%u,%s", cnv.get_id(), t );
		tool_t *tool = create_tool( TOOL_RENAME | SIMPLE_TOOL );
		tool->set_default_param( buf );
		world()->set_tool( tool, cnv->get_owner() );
		// since init always returns false, it is safe to delete immediately
		delete tool;
	}
}
