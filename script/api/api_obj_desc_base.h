#ifndef _API_OBJ_DESC_BASE_H_
#define _API_OBJ_DESC_BASE_H_

/** @file api_obj_desc.h templates for transfer of besch-pointers */

#include "../api_param.h"
#include "../api_class.h"

class obj_besch_std_name_t;
class obj_besch_timelined_t;
class ware_besch_t;

namespace script_api {

#define declare_besch_param(T, sqtype) \
	template<> \
	struct param<const T*> { \
		\
		typedef const T* (*GETFUNC)(const char*);\
		\
		static GETFUNC getfunc(); \
		\
		declare_types("t|x|y", sqtype); \
		\
		declare_create_slot(const T*); \
		\
		static void* tag() \
		{ \
			return (void*)(getfunc()); \
		} \
		static const T* get(HSQUIRRELVM vm, SQInteger index); \
		\
		static SQInteger push(HSQUIRRELVM vm, const T* b); \
	}; \
	template<> \
	struct param<T*> { \
		\
		declare_types("t|x|y", sqtype); \
	};

#define implement_besch_param(T, sqtype, func) \
	param<const T*>::GETFUNC param<const T*>::getfunc() \
	{ \
		return func; \
	} \
	const T* param<const T*>::get(HSQUIRRELVM vm, SQInteger index) \
	{ \
		return (const T*)get_instanceup(vm, index, tag(), sqtype); \
	} \
	SQInteger param<const T*>::push(HSQUIRRELVM vm, const T* b) \
	{ \
		return push_instance(vm, sqtype, b->get_name()); \
	}

	declare_specialized_param(const obj_besch_std_name_t*, "t|x|y", "obj_desc_x");
	declare_param_mask(obj_besch_std_name_t*, "t|x|y", "obj_desc_x");

	declare_specialized_param(const obj_besch_timelined_t*, "t|x|y", "obj_desc_time_x");
	declare_param_mask(obj_besch_timelined_t*, "t|x|y", "obj_desc_time_x");

	declare_besch_param(ware_besch_t, "good_desc_x");
};

#endif
