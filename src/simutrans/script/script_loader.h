/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SCRIPT_SCRIPT_LOADER_H
#define SCRIPT_SCRIPT_LOADER_H


class script_vm_t;

struct script_loader_t
{
	/**
	 * Starts vm, loads the scripting interface, loads script/[base_file_name].
	 * @returns vm or NULL in case of error
	 */
	static script_vm_t* start_vm(const char* base_file_name, const char* logfile_name, const char* include_path, bool is_scenario);

	/**
	 * Loads base script files: env_t::base_dir/script/script_base.nut and env_t::base_dir/script/[base].
	 */
	static bool load_base_script(script_vm_t *vm, const char* base);

	/**
	 * loads necessary compatibility scripts
	 */
	static void load_compatibility_script(script_vm_t *script);
};

#endif
