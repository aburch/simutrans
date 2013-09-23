#include "api_obj_desc_base.h"

#include "../../bauer/warenbauer.h"
#include "../../besch/ware_besch.h"
#include "../../squirrel/sq_extensions.h"

using namespace script_api;

implement_besch_param(ware_besch_t, "good_desc_x", (const ware_besch_t* (*)(const char*))(&warenbauer_t::get_info) );


// tag of obj_besch_std_name_t class
static const void *obj_besch_std_name_tag = &obj_besch_std_name_tag;

// tag of obj_besch_std_name_t class
static const void *obj_besch_timelined_tag = &obj_besch_timelined_tag;

const obj_besch_std_name_t* param<const obj_besch_std_name_t*>::get(HSQUIRRELVM vm, SQInteger index)
{
	return (const obj_besch_std_name_t*)get_instanceup(vm, index, tag(), squirrel_type());
}

void* param<const obj_besch_std_name_t*>::tag()
{
	return &obj_besch_std_name_tag;
}


const obj_besch_timelined_t* param<const obj_besch_timelined_t*>::get(HSQUIRRELVM vm, SQInteger index)
{
	return (const obj_besch_timelined_t*)get_instanceup(vm, index, tag(), squirrel_type());
}

void* param<const obj_besch_timelined_t*>::tag()
{
	return &obj_besch_timelined_tag;
}
