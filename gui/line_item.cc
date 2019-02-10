#include <algorithm>

#include "line_item.h"
#include "../simline.h"
#include "../simmenu.h"
#include "../simworld.h"
#include "../utils/cbuffer_t.h"


const char* line_scrollitem_t::get_text() const
{
	return line->get_name();
}


PIXVAL line_scrollitem_t::get_color() const
{
	return line->get_state_color();
}


void line_scrollitem_t::set_text(char const* const t)
{
	if(  t  &&  t[0]  &&  strcmp(t, line->get_name())) {
		// text changed => call tool
		cbuffer_t buf;
		buf.printf( "l%u,%s", line.get_id(), t );
		tool_t *tool = create_tool( TOOL_RENAME | SIMPLE_TOOL );
		tool->set_default_param( buf );

		karte_ptr_t welt;
		welt->set_tool( tool, line->get_owner() );
		// since init always returns false, it is safe to delete immediately
		delete tool;
	}
}


// static helper function for sorting lineintems

line_scrollitem_t::sort_modes_t line_scrollitem_t::sort_mode = line_scrollitem_t::SORT_BY_NAME;


bool line_scrollitem_t::compare(const gui_component_t *aa, const gui_component_t *bb)
{
	const line_scrollitem_t *a = dynamic_cast<const line_scrollitem_t*>(aa);
	const line_scrollitem_t *b = dynamic_cast<const line_scrollitem_t*>(bb);
	// good luck with mixed lists
	assert(a != NULL  &&  b != NULL); (void)(a==b);

	if(  sort_mode != SORT_BY_NAME  ) {
		switch(  sort_mode  ) {
			case SORT_BY_NAME:	// default
				break;
			case SORT_BY_ID:
				return (a->get_line().get_id(),b->get_line().get_id())<0;
			case SORT_BY_PROFIT:
				return (a->get_line()->get_finance_history(1,LINE_PROFIT) - b->get_line()->get_finance_history(1,LINE_PROFIT))<0;
			case SORT_BY_TRANSPORTED:
				return (a->get_line()->get_finance_history(1,LINE_TRANSPORTED_GOODS) - b->get_line()->get_finance_history(1,LINE_TRANSPORTED_GOODS))<0;
			case SORT_BY_CONVOIS:
				return (a->get_line()->get_finance_history(1,LINE_CONVOIS) - b->get_line()->get_finance_history(1,LINE_CONVOIS))<0;
			case SORT_BY_DISTANCE:
				// normalizing to the number of convoys to get the fastest ones ...
				return (a->get_line()->get_finance_history(1,LINE_DISTANCE)/max(1,a->get_line()->get_finance_history(1,LINE_CONVOIS)) -
						b->get_line()->get_finance_history(1,LINE_DISTANCE)/max(1,b->get_line()->get_finance_history(1,LINE_CONVOIS)) )<0;
			default: break;
		}
		// default sorting ...
	}

	// first: try to sort by number
	const char *atxt = a->get_text();
	int aint = 0;
	// isdigit produces with UTF8 assertions ...
	if(  atxt[0]>='0'  &&  atxt[0]<='9'  ) {
		aint = atoi( atxt );
	}
	else if(  atxt[0]=='('  &&  atxt[1]>='0'  &&  atxt[1]<='9'  ) {
		aint = atoi( atxt+1 );
	}
	const char *btxt = b->get_text();
	int bint = 0;
	if(  btxt[0]>='0'  &&  btxt[0]<='9'  ) {
		bint = atoi( btxt );
	}
	else if(  btxt[0]=='('  &&  btxt[1]>='0'  &&  btxt[1]<='9'  ) {
		bint = atoi( btxt+1 );
	}
	if(  aint!=bint  ) {
		return (aint-bint)<0;
	}
	// otherwise: sort by name
	return strcmp(atxt, btxt)<0;
}
