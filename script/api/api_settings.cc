#include "api.h"

/** @file api_settings.cc exports game settings functions. */

#include "../api_class.h"
#include "../api_function.h"
#include "../../dataobj/einstellungen.h"

using namespace script_api;

#define begin_class(c,p) push_class(vm, c);
#define end_class() sq_pop(vm,1);


void export_settings(HSQUIRRELVM vm)
{
	/**
	 * Table with methods to access game settings.
	 */
	begin_class("settings", 0);

	/**
	 * New industries will be spawned if cities grow to over 2^n times this setting.
	 * @returns number
	 */
	register_method(vm, &settings_t::get_industry_increase_every, "get_industry_increase_every");

	/**
	 * New industries will be spawned if cities grow to over 2^n times @p count.
	 * Set to zero to prevent new industries to be spawned.
	 * @param count
	 */
	register_method(vm, &settings_t::set_industry_increase_every, "set_industry_increase_every");

	end_class();
}
