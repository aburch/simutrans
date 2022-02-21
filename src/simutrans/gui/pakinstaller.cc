/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../utils/cbuffer.h"

#include "pakinstaller.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../sys/simsys.h"

#include "../paksetinfo.h"
#include "../simloadingscreen.h"

bool pakinstaller_t::finish_install;

pakinstaller_t::pakinstaller_t() :
	gui_frame_t(translator::translate("Install graphics")),
	paks(gui_scrolled_list_t::listskin),
	obsolete_paks(gui_scrolled_list_t::listskin)
{
	finish_install = false;

	set_table_layout(2, 0);

	new_component_span<gui_label_t>( "Select one or more graphics to install (Ctrl+click):", 2 );

	for (int i = 0; i < 10; i++) {
		paks.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(pakinfo[i*2+1], i<10?SYSCOL_TEXT: SYSCOL_TEXT_SHADOW);
	}
	paks.enable_multiple_selection();
	scr_coord_val paks_h = paks.get_max_size().h;
	add_component(&paks,2);

	new_component_span<gui_label_t>( "The following graphics are unmaintained:", 2 );

	for (int i = 10; i < PAKSET_COUNT; i++) {
		obsolete_paks.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(pakinfo[i*2+1], i<10?SYSCOL_TEXT: SYSCOL_TEXT_SHADOW);
	}
	obsolete_paks.enable_multiple_selection();
	scr_coord_val obsolete_paks_h = obsolete_paks.get_max_size().h;
	add_component(&obsolete_paks,2);

	install.init(button_t::roundbox_state | button_t::flexible, "Install");
	add_component(&install);
	install.add_listener(this);

	cancel.init(button_t::roundbox_state | button_t::flexible, "Cancel");
	cancel.add_listener(this);
	add_component(&cancel);

	reset_min_windowsize();
	set_windowsize(get_min_windowsize()+scr_size(0,paks_h-paks.get_size().h + obsolete_paks_h- obsolete_paks.get_size().h));
}

#ifdef __ANDROID__
// since the script does only work on few devices
#define USE_OWN_PAKINSTALL
#endif

#ifndef USE_OWN_PAKINSTALL
/**
 * This method is called if an action is triggered
 */
bool pakinstaller_t::action_triggered(gui_action_creator_t *comp, value_t)
{
	if (comp == &cancel) {
		finish_install = true;
		return false;
	}

	// now install
	dr_chdir( env_t::data_dir );
	loadingscreen_t ls("Install paks", paks.get_selections().get_count()+obsolete_paks.get_selections().get_count(), true, false);
	int j = 0;
	for(sint32 i : paks.get_selections()) {
		cbuffer_t param;
		ls.set_info(pakinfo[i * 2 + 1]);
#ifdef _WIN32
		param.append( "powershell -ExecutionPolicy ByPass -NoExit .\\get_pak.ps1 \"" );
#else
#ifndef __ANDROID__
		param.append("./get_pak.sh \"");
#else
		param.append( "sh get_pak.sh \"" );
#endif
#endif
		param.append(pakinfo[i*2]);
		param.append("\"");

		const int retval = system( param );
		DBG_DEBUG("pakinstaller_t::action_triggered", "Command '%s' returned %d", param.get_str(), retval);
		ls.set_progress(++j);
	}

	for(sint32 i : obsolete_paks.get_selections()) {
		cbuffer_t param;
		ls.set_info(pakinfo[i * 2 + 21]);
#ifdef _WIN32
		param.append("powershell -ExecutionPolicy ByPass .\\get_pak.ps1 \"");
#else
		param.append( "./get_pak.sh \"" );
#endif
		param.append(pakinfo[i*2+20]);
		param.append("\"");

		const int retval = system( param );
		DBG_DEBUG("pakinstaller_t::action_triggered", "Command '%s' returned %d", param.get_str(), retval);
		ls.set_progress(++j);
	}

	finish_install = true;
	return false;
}
#else

#ifndef USE_EXTERNAL_ZIP_CURL

#include "../utils/simstring.h"

#include "../network/network_file_transfer.h"
extern "C" {
#define STDC
#define voidpc voidpczip
#include "../../external/zip.c"
};


void extract_pak_from_zip(const char* zipfile)
{

	struct zip_t* zip = zip_open(zipfile, 0, 'r');
	const int n = zip_entries_total(zip);
	bool has_simutrans_folder = false;

	loadingscreen_t ls("Extract files", n, true, false);

	for (int i = 0; i < n && i < 10 && !has_simutrans_folder; ++i) {
		zip_entry_openbyindex(zip, i);
		{
			int isdir = zip_entry_isdir(zip);
			if (isdir) {
				has_simutrans_folder = STRNICMP("simutrans", zip_entry_name(zip), 9) == 0;
			}
		}
		zip_entry_close(zip);
	}
	// now extract
	for (int i = 0; i < n; i++) {
		zip_entry_openbyindex(zip, i);
		{
			int isdir = zip_entry_isdir(zip);
			const char* name = zip_entry_name(zip) + (has_simutrans_folder ? 10 : 0);
			if (isdir) {
				// create directory (may fail if exists ...
				dr_mkdir(name);
			}
			else {
				// or extract file
				DBG_DEBUG("Install pak file", "%s", name);
				zip_entry_fread(zip, name);
			}
		}
		zip_entry_close(zip);
		ls.set_progress(i);
	}
	zip_close(zip);
}

// native download (curl), extract (libzip)
bool pakinstaller_t::action_triggered(gui_action_creator_t* comp, value_t)
{
	if (comp == &cancel) {
		destroy_win(this);
		return true;
	}

	dr_chdir(env_t::data_dir);

	loadingscreen_t ls("Install paks", paks.get_selections().get_count() * 2, true, false);
	char outfilename[FILENAME_MAX];
	int j = 0;
	for (sint32 i : paks.get_selections()) {
		ls.set_info(pakinfo[i * 2 + 1]);
		sprintf(outfilename, "%s.zip", pakinfo[i * 2 + 1]);
		const char* url = pakinfo[i * 2] + 7; // minus http://

		char site_ip[1024];
		tstrncpy(site_ip, url, lengthof(site_ip));

		const char* site_path = strchr(url, '/');
		site_ip[site_path - url] = 0;
		strcat(site_ip, ":80");

		const char* err = NULL;

		err = network_http_get_file(site_ip, site_path, outfilename);
		if (err) {
			dbg->warning("Pakset download failed with", "%s", err);
			j += 2;
			ls.set_progress(j);
			continue;
		}

		ls.set_progress(++j);
		DBG_DEBUG(__FUNCTION__, "pak target %s", pakinfo[i * 2]);

		DBG_DEBUG(__FUNCTION__, "download successful to %s, attempting extract", outfilename);
		extract_pak_from_zip(outfilename);
		dr_remove(outfilename);

		ls.set_progress(++j);
	}
	finish_install = true;
	return false;
}


#else
#include <zip.h>
// linux/android specific, function to create folder makes use of opendir (linux system call) and mkdir (via system)

static bool create_folder_if_required(const char* extracted_path)
{
	char extracted_folder_name[FILENAME_MAX];
	strcpy(extracted_folder_name, extracted_path);
	char* last_occurence = strrchr(extracted_folder_name, '/');
	if (last_occurence != NULL) {
		last_occurence[0] = '\0';
	}
	else {
		DBG_DEBUG(__FUNCTION__, "Error searching for path? %s", extracted_folder_name);
		return false;
	}

	char current_path[PATH_MAX];
	dr_getcwd(current_path, PATH_MAX);
	if (dr_chdir(extracted_folder_name) != 0) {
		dr_mkdir(extracted_folder_name);
		DBG_DEBUG(__FUNCTION__, "- - Created folder %s", extracted_folder_name);
	}
	if (dr_chdir(extracted_folder_name) != 0) {
		DBG_DEBUG(__FUNCTION__, "Failed to create %s", extracted_folder_name);
		return false;
	}
	dr_chdir(current_path);
	return true;
}

static void extract_pak_from_zip(const char* outfilename)
{
	zip_t* zip_archive;
	int err;

	if ((zip_archive = zip_open(outfilename, ZIP_RDONLY, &err)) == NULL) {
		zip_error_t error;
		zip_error_init_with_code(&error, err);
		DBG_DEBUG(__FUNCTION__, "cannot open zip archive \"%s\": %s : %d", outfilename, zip_error_strerror(&error), error);
		zip_error_fini(&error);
	}

	DBG_DEBUG(__FUNCTION__, "zip archive opened");

	zip_uint64_t nentry = (zip_uint64_t)zip_get_num_entries(zip_archive, 0);
	for (zip_uint64_t idx = 0; idx < nentry; idx++) {
		struct zip_stat st;
		if (zip_stat_index(zip_archive, idx, 0, &st) == -1) {
			DBG_DEBUG(__FUNCTION__, "cannot read file stat %d in zip archive: %s", idx, zip_strerror(zip_archive));
			continue;
		}

		zip_file_t* zip_file;
		if ((zip_file = zip_fopen_index(zip_archive, idx, 0)) == NULL) {
			DBG_DEBUG(__FUNCTION__, "cannot open file %s (index %d) in zip_archive: %s", st.name, idx, zip_strerror(zip_archive));
			continue;
		}

		char* contents = new char[st.size];
		zip_int64_t read_size;
		if ((read_size = zip_fread(zip_file, contents, st.size)) == -1) {
			DBG_DEBUG(__FUNCTION__, "failed to read content in file %s (index %d) of zip_archive: %s", st.name, idx, zip_file_strerror(zip_file));
		}
		else {
			if (read_size != (zip_int64_t)st.size) {
				DBG_DEBUG(__FUNCTION__, "unexpected content size in file %s (index %d) of zip_archive; is %d, should be %d: %s", st.name, idx, read_size, st.size);
			}
			else {
				// path may start with simutrans/, in which case it can be removed safely
				bool start_with_simutrans = strncmp(st.name, "simutrans", 9) == 0;
				const char* target_filename = start_with_simutrans ? st.name + 10 : st.name;

				char extracted_path[FILENAME_MAX];
				sprintf(extracted_path, "%s%s", env_t::data_dir, target_filename);
				DBG_DEBUG(__FUNCTION__, "- %s: %d", extracted_path, st.size);

				if (!create_folder_if_required(extracted_path)) {
					continue;
				}

				if (st.size > 0) {
					if (FILE* f = dr_fopen(extracted_path, "wb")) {
						fwrite(contents, st.size, 1, f);
						fclose(f);
					}
					else {
						DBG_DEBUG(__FUNCTION__, "Error writing file");
					}
				}
			}
		}
		zip_fclose(zip_file);
	}

	if (zip_close(zip_archive) == -1) {
		DBG_DEBUG(__FUNCTION__, "cannot close zip archive: %s", zip_strerror(zip_archive));
	}
	dr_chdir(env_t::data_dir);
	dr_remove(outfilename);
}


#include <curl/curl.h>

static size_t curl_write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written;
}

static CURLcode curl_download_file(CURL* curl, const char* target_file, const char* url) {
	FILE* fp = fopen(target_file, "wb");
	CURLcode res;
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	char cabundle_path[PATH_MAX];
	sprintf(cabundle_path, "%s%s", env_t::data_dir, "cacert.pem");
	FILE* cabundle_file;
	if ((cabundle_file = fopen(cabundle_path, "r"))) {
		fclose(cabundle_file);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl, CURLOPT_CAINFO, cabundle_path);
	}
	else {
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		dbg->warning(__FUNCTION__, "ssl certificate authority bundle not found at %s; https calls will not be validated; this may be a security concern", cabundle_path);
	}
	curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	res = curl_easy_perform(curl);
	fclose(fp);
	return res;
}


// native download (curl), extract (libzip)
bool pakinstaller_t::action_triggered(gui_action_creator_t*, value_t)
{
	CURL* curl = curl_easy_init(); // can only be called once during program lifecycle
	if (curl) {
		dr_chdir(env_t::data_dir);
		DBG_DEBUG(__FUNCTION__, "libcurl initialized");

		char outfilename[FILENAME_MAX];
		loadingscreen_t ls("Install paks", paks.get_selections().get_count() * 2, true, false);
		int j = 0;
		for (sint32 i : paks.get_selections()) {
			ls.set_info(pakinfo[i * 2 + 1]);
			sprintf(outfilename, "%s.zip", pakinfo[i * 2 + 1]);

			CURLcode res = curl_download_file(curl, outfilename, pakinfo[i * 2]);
			ls.set_progress(++j);
			DBG_DEBUG(__FUNCTION__, "pak target %s", pakinfo[i * 2]);

			if (res == 0) {
				DBG_DEBUG(__FUNCTION__, "download successful to %s, attempting extract", outfilename);
				read_zip(outfilename);
			}
			else {
				DBG_DEBUG(__FUNCTION__, "download failed with error code %s, check curl errors; skipping", curl_easy_strerror(res));
			}
			ls.set_progress(++j);
		}
		curl_easy_cleanup(curl);
	}
	else {
		DBG_DEBUG(__FUNCTION__, "libcurl failed to initialize, pakset not downloaded");
	}
	finish_install = true;
	return false;
}
#endif
#endif
