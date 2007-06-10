#ifndef freight_list_sorter_h
#define freight_list_sorter_h

#include "utils/cbuffer_t.h"
#include "halthandle_t.h"
#include "simware.h"

// same sorting for stations and vehicle/convoi freight ...

class ware_t;

class freight_list_sorter_t
{
public:
	/*
	 * struct hold travel details for wares that travel
	 * @author hsiegeln
	 */
	typedef struct {
		ware_t ware;
		halthandle_t destination;
		halthandle_t via_destination;
	} travel_details;

	enum sort_mode_t { by_name=0, by_via=1, by_via_sum=2, by_amount=3};

	static void sort_freight(const vector_tpl<ware_t>* warray, cbuffer_t& buf, sort_mode_t sort_mode, const slist_tpl<ware_t>* full_list, const char* what_doing);

private:
	static sort_mode_t sortby;

	static int compare_ware(const void *td1, const void *td2);

	static void add_ware_heading( cbuffer_t &buf, uint32 sum, uint32 max, const ware_t *ware, const char *what_doing );
};


#endif
