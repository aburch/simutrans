/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../macros.h"
#include "../simdebug.h"
#include "../sys/simsys.h"
#include "../simtypes.h"
#include "../display/simgraph.h" // for unicode stuff
#include "translator.h"
#include "loadsave.h"
#include "environment.h"
#include "../simmem.h"
#include "../utils/cbuffer.h"
#include "../utils/searchfolder.h"
#include "../utils/simstring.h"
#include "../utils/unicode.h"
#include "../tpl/vector_tpl.h"

using std::string;

// allow all kinds of line feeds
static char *fgets_line(char *buffer, int max_len, FILE *file)
{
	char *result = fgets(buffer, max_len, file);
	size_t len = strlen(buffer);
	// remove all trailing junk
	while(  len>1  &&  (buffer[len-1]==13  ||  buffer[len-1]==10)  ) {
		buffer[len-1] = 0;
		len--;
	}
	return result;
}


const char *translator::lang_info::translate(const char *text) const
{
	if(  text    == NULL  ) {
		return "(null)";
	}
	if(  text[0] == '\0'  ) {
		return text;
	}
	const char *trans = texts.get(text);
	return trans != NULL ? trans : text;
}


/* Made to be dynamic, allowing any number of languages to be loaded */
static translator::lang_info langs[40];
static translator::lang_info *current_langinfo = langs;
static stringhashtable_tpl<const char*> compatibility;


translator translator::single_instance;


const translator::lang_info* translator::get_lang()
{
	return current_langinfo;
}


const translator::lang_info* translator::get_langs()
{
	return langs;
}


/* first two file functions needed in connection with utf */

/**
 * checks, if we need a unicode translation
 */
static bool is_unicode_file(FILE* f)
{
	unsigned char str[2];
	int pos = ftell(f);
//	DBG_DEBUG("is_unicode_file()", "checking for unicode");
//	fflush(NULL);
	if (fread( str, 1, 2,  f ) != 2) {
		return false;
	}
//	DBG_DEBUG("is_unicode_file()", "file starts with %x%x",str[0],str[1]);
//	fflush(NULL);
	if (str[0] == 0xC2 && str[1] == 0xA7) {
		// the first line must contain an UTF8 coded paragraph (Latin A7, UTF8 C2 A7), then it is unicode
		DBG_DEBUG("is_unicode_file()", "file is UTF-8");
		return true;
	}
	if(  str[0]==0xEF  &&  str[1]==0xBB   &&  fgetc(f)==0xBF  ) {
		// the first letter is the byte order mark => may need to skip a paragraph (Latin A7, UTF8 C2 A7)
		pos = ftell(f);
		if (fread( str, 1, 2,  f ) != 2) {
			return false;
		}
		if(  str[0] != 0xC2  ||  str[1] == 0xA7  ) {
			fseek(f, pos, SEEK_SET);
			dbg->error( "is_unicode_file()", "file is UTF-8 but has no paragraph" );
		}
		DBG_DEBUG("is_unicode_file()", "file is UTF-8");
		return true;
	}
	fseek(f, pos, SEEK_SET);
	return false;
}



// recodes string to put them into the tables
static char *recode(const char *src, bool translate_from_utf, bool translate_to_utf, bool is_latin2 )
{
	char *base;
	if(  translate_to_utf != translate_from_utf  ) {
		// worst case
		base = MALLOCN(char, strlen(src) * 2 + 2);
	}
	else {
		base = MALLOCN(char, strlen(src) + 2);
	}
	char *dst = base;
	uint8 c = 0;

	do {
		if (*src =='\\') {
			if (*(src + 1) == 0) {
				// backslash at end of line -> corrupted
				break;
			}

			src += 2;
			*dst++ = c = '\n';
		}
		else {
			c = *src;
			if(c>127) {
				if(  translate_from_utf == translate_to_utf  ) {
					// but copy full letters! (or, if ASCII, copy more than one letter, does not matter
					do {
						*dst++ = *src++;
					} while (is_cont_char(*src));
					c = *src;
				}
				else if(  translate_to_utf  ) {
					if(  !is_latin2  ) {
						// make UTF8 from latin1
						dst += c = utf16_to_utf8((unsigned char)*src++, (utf8*)dst);
					}
					else {
						dst += c = utf16_to_utf8( latin2_to_unicode( (uint8)*src++ ), (utf8*)dst );
					}
				}
				else if(  translate_from_utf  ) {
					// make latin from UTF8 (ignore overflows!)
					if(  !is_latin2  ) {
						*dst++ = c = (uint8)utf8_decoder_t::decode((utf8 const *&)src);
					}
					else {
						*dst++ = c = unicode_to_latin2(utf8_decoder_t::decode((utf8 const *&)src));
					}
				}
			}
			else if(c>=13) {
				// just copy
				src ++;
				*dst++ = c;
			}
			else {
				// ignore this character
				src ++;
			}
		}
	} while (c != '\0');
	*dst = 0;
	return base;
}



/* needed for loading city names */
static char pakset_path[256];

// List of custom city and streetnames
vector_tpl<char *> translator::city_name_list;
vector_tpl<char *> translator::street_name_list;


// fills a list from a file with the given prefix followed by a language code
void translator::load_custom_list( int lang, vector_tpl<char *>&name_list, const char *fileprefix )
{
	FILE *file;

	// Clean up all names
	for(char* const i : name_list) {
		free(i);
	}
	name_list.clear();

	// first try in pakset
	{
		string local_file_name(env_t::user_dir);
		local_file_name = local_file_name + "addons/" + pakset_path + "text/" + fileprefix + langs[lang].iso_base + ".txt";
		DBG_DEBUG("translator::load_custom_list()", "try to read city name list from '%s'", local_file_name.c_str());
		file = dr_fopen(local_file_name.c_str(), "rb");
	}
	// not found => try user location
	if(  file==NULL  ) {
		string local_file_name(env_t::user_dir);
		local_file_name = local_file_name + fileprefix + langs[lang].iso_base + ".txt";
		file = dr_fopen(local_file_name.c_str(), "rb");
		DBG_DEBUG("translator::load_custom_list()", "try to read city name list from '%s'", local_file_name.c_str());
	}
	// not found => try pak location
	if(  file==NULL  ) {
		string local_file_name(env_t::base_dir);
		local_file_name = local_file_name + pakset_path + "text/" + fileprefix + langs[lang].iso_base + ".txt";
		DBG_DEBUG("translator::load_custom_list()", "try to read city name list from '%s'", local_file_name.c_str());
		file = dr_fopen(local_file_name.c_str(), "rb");
	}
	// not found => try global translations
	if(  file==NULL  ) {
		string local_file_name(env_t::base_dir);
		local_file_name = local_file_name + "text/" + fileprefix + langs[lang].iso_base + ".txt";
		DBG_DEBUG("translator::load_custom_list()", "try to read city name list from '%s'", local_file_name.c_str());
		file = dr_fopen(local_file_name.c_str(), "rb");
	}
	fflush(NULL);

	if (file != NULL) {
		// ok, could open file
		char buf[256];
		bool file_is_utf = is_unicode_file(file);
		while(  !feof(file)  ) {
			if (fgets_line(buf, sizeof(buf), file)) {
				rtrim(buf);
				char *c = recode(buf, file_is_utf, true, langs[lang].is_latin2_based );
				if(  *c!=0  &&  *c!='#'  ) {
					name_list.append(c);
				}
			}
		}
		fclose(file);
		DBG_DEBUG("translator::load_custom_list()","Loaded list %s_%s.txt.", fileprefix, langs[lang].iso_base );
	}
	else {
		DBG_DEBUG("translator::load_custom_list()","No list %s_%s.txt found, using defaults.", fileprefix, langs[lang].iso_base );
	}
}


/**
 * the city list is now reloaded after the language is changed
 * new cities will get their appropriate names
 */
void translator::init_custom_names(int lang)
{
	// init names. There are two options:
	//
	// 1.) read list from file
	// 2.) create random names (only for cities)

	// try to read list
	load_custom_list( lang, city_name_list, "citylist_" );
	load_custom_list( lang, street_name_list, "streetlist_" );

	if (city_name_list.empty()) {
		DBG_MESSAGE("translator::init_city_names", "reading failed, creating random names.");
		// try to read list failed, create random names
		for(  uint i = 0;  i < 36;  i++  ) {
			char name[32];
			sprintf( name, "%%%c_CITY_SYLL", i+(i<10 ? '0' : 'A'-10 ) );
			const char *s1 = translator::translate(name,lang);
			if(s1==name) {
				// name not available ...
				continue;
			}
			// now add all second name extensions ...
			const size_t l1 = strlen(s1);
			for(  uint j = 0;  j < 36;  j++  ) {

				sprintf( name, "&%c_CITY_SYLL", j+(j<10 ? '0' : 'A'-10 ) );
				const char *s2 = translator::translate(name,lang);
				if(s2==name) {
					// name not available ...
					continue;
				}
				const size_t l2 = strlen(s2);
				char *const c = MALLOCN(char, l1 + l2 + 1);
				sprintf(c, "%s%s", s1, s2);
				city_name_list.append(c);
			}
		}
	}
}


/* now on to the translate stuff */


static void load_language_file_body(FILE* file, stringhashtable_tpl<const char*>* table, bool language_is_utf, bool file_is_utf, bool language_is_latin2 )
{
	char buffer1 [4096];
	char buffer2 [4096];

	bool convert_to_unicode = language_is_utf && !file_is_utf;

	do {
		fgets_line(buffer1, sizeof(buffer1), file);
		if(  buffer1[0] == '#'  ) {
			// ignore comments
			continue;
		}
		if(  !feof(file)  ) {
			fgets_line(buffer2, sizeof(buffer2), file);
			if(  strcmp(buffer1,buffer2)  ) {
				// only add line which are actually different
				char *raw = recode(buffer1, file_is_utf, false, language_is_latin2 );
				char *translated = recode(buffer2, false, convert_to_unicode,language_is_latin2);

				if(  cbuffer_t::check_format_strings(raw, translated)  ) {
					table->set( raw, translated );
				}
				else {
					free(raw);
					free(translated);
				}
			}
		}
	} while (!feof(file));
}


void translator::load_language_file(FILE* file)
{
	char buffer1[256];
	bool file_is_utf = is_unicode_file(file);

	// Read language name
	fgets_line(buffer1, sizeof(buffer1), file);

	langs[single_instance.lang_count].name = strdup(buffer1);

	if(  !file_is_utf  ) {
		// find out the font if not unicode (and skip it)
		while(  !feof(file)  ) {
			fgets_line( buffer1, sizeof(buffer1), file );
			if(  buffer1[0] == '#'  ) {
				continue;
			}
			if(  strcmp(buffer1,"PROP_FONT_FILE") == 0  ) {
				fgets_line( buffer1, sizeof(buffer1), file );
				// HACK: so we guess about latin2 from the font name!
				langs[single_instance.lang_count].is_latin2_based = STRNICMP( buffer1+5, "latin2", 6 )==0;
				// we must register now a unicode font
				langs[single_instance.lang_count].texts.set( "PROP_FONT_FILE", langs[single_instance.lang_count].is_latin2_based ? "cyr.bdf" : strdup(buffer1) );
				break;
			}
		}
	}
	else {
		// since it is anyway UTF8
		langs[single_instance.lang_count].is_latin2_based = false;
	}

	//load up translations, putting them into
	//language table of index 'lang'
	load_language_file_body(file, &langs[single_instance.lang_count].texts, true, file_is_utf, langs[single_instance.lang_count].is_latin2_based );
}


static translator::lang_info* get_lang_by_iso(const char *iso)
{
	for(  translator::lang_info* i = langs;  i != langs + translator::get_language_count();  ++i  ) {
		if(  i->iso_base[0] == iso[0]  &&  i->iso_base[1] == iso[1]  ) {
			return i;
		}
	}
	return NULL;
}


static uint32 get_highest_character( const utf8 *str )
{
	size_t len = 0;
	uint32 max_char = 0, symbol;
	do {
		symbol = utf8_decoder_t::decode( str, len );
		str += len;
		if( symbol > max_char ) {
			max_char = symbol;
		}
	} while( symbol > 0 );
	return max_char;
}


uint32 translator::guess_highest_unicode(int n)
{
	const char* T1 = langs[n].texts.get( "Bruecke muss an\neinfachem\nHang beginnen!\n" );
	uint32 max_char = 0xDF;
	if( T1 ) {
		max_char = get_highest_character( (const utf8 *)T1 );
	}
	const char* T2 = langs[n].texts.get( "Start" );
	if( T2 ) {
		uint32 max_char2 = get_highest_character( (const utf8 *)T2 );
		max_char = max( max_char, max_char2 );
	}
	return max_char;
}


void translator::load_files_from_folder(const char *folder_name, const char *what)
{
	searchfolder_t folder;
	const int num_pak_lang_dat = folder.search(folder_name, "tab");
	DBG_MESSAGE("translator::load_files_from_folder()", "search folder \"%s\" and found %i files", folder_name, num_pak_lang_dat); (void)num_pak_lang_dat;

	//read now the basic language infos
	for(const char* const& filename : folder) {
		lang_info* lang = NULL;
		const char* langcode = strrchr(filename,'.');

		if(  langcode  &&  (langcode-filename)>2  ) {
			// try before the point
			lang = get_lang_by_iso(langcode-2);
			if (lang == NULL) {
				// try instead the start of the string
				lang = get_lang_by_iso(filename);
			}
		}

		if (lang != NULL) {
			DBG_MESSAGE("translator::load_files_from_folder()", "loading %s translations from %s for language %s", what, filename, lang->iso_base);
			if (FILE* const file = dr_fopen(filename, "rb")) {
				bool file_is_utf = is_unicode_file(file);
				load_language_file_body(file, &lang->texts, true, file_is_utf, lang->is_latin2_based );
				fclose(file);
			}
			else {
				dbg->warning("translator::load_files_from_folder()", "cannot open '%s'", filename);
			}
		}
		else {
			dbg->warning("translator::load_files_from_folder()", "%s no language '%s'", filename, what );
		}
	}
}


bool translator::load(const string &path_to_pakset)
{
	dr_chdir( env_t::base_dir );
	tstrncpy(pakset_path, path_to_pakset.c_str(), lengthof(pakset_path));

	//initialize these values to 0(ie. nothing loaded)
	single_instance.current_lang = -1;
	single_instance.lang_count = 0;

	DBG_MESSAGE("translator::load()", "Loading languages...");
	searchfolder_t folder;
	folder.search("text/", "tab");

	//read now the basic language infos
	for (searchfolder_t::const_iterator i = folder.begin(), end = folder.end(); i != end; ++i) {
		const string fileName(*i);
		size_t pstart = fileName.rfind('/') + 1;
		const string iso = fileName.substr(pstart, fileName.size() - pstart - 4);

		if (FILE* const file = dr_fopen(fileName.c_str(), "rb")) {
			DBG_MESSAGE("translator::load()", "base file \"%s\" - iso: \"%s\"", fileName.c_str(), iso.c_str());
			load_language_iso(iso);
			load_language_file(file);
			fclose(file);
			langs[single_instance.lang_count].highest_character = guess_highest_unicode( single_instance.lang_count );
			single_instance.lang_count++;
			if (single_instance.lang_count == (int)lengthof(langs)) {
				if (++i != end) {
					// some languages were not loaded, let the user know what happened
					dbg->warning("translator::load()", "some languages were not loaded, limit reached");
					for (; i != end; ++i) {
						dbg->warning("translator::load()", " %s not loaded", *i);
					}
				}
				break;
			}
		}
	}

	// now read the pakset specific text
	// there can be more than one file per language, provided it is name like iso_xyz.tab
	const string folderName(path_to_pakset + "text/");
	load_files_from_folder(folderName.c_str(), "pak");

	if(  env_t::default_settings.get_with_private_paks()  ) {
		dr_chdir( env_t::user_dir );
		// now read the pakset specific text
		// there can be more than one file per language, provided it is name like iso_xyz.tab
		const string folderName("addons/" + path_to_pakset + "text/");
		load_files_from_folder(folderName.c_str(), "pak addons");
		dr_chdir( env_t::base_dir );
	}

	//if NO languages were loaded then game cannot continue
	if (single_instance.lang_count < 1) {
		return false;
	}

	// now we try to read the compatibility stuff
	if (FILE* const file = dr_fopen((path_to_pakset + "compat.tab").c_str(), "rb")) {
		load_language_file_body(file, &compatibility, false, false, false );
		DBG_MESSAGE("translator::load()", "pakset compatibility texts loaded.");
		fclose(file);
	}
	else {
		DBG_MESSAGE("translator::load()", "no pakset compatibility texts");
	}

	// also addon compatibility ...
	if(  env_t::default_settings.get_with_private_paks()  ) {
		dr_chdir( env_t::user_dir );
		if (FILE* const file = dr_fopen(string("addons/"+path_to_pakset + "compat.tab").c_str(), "rb")) {
			load_language_file_body(file, &compatibility, false, false, false );
			DBG_MESSAGE("translator::load()", "pakset addon compatibility texts loaded.");
			fclose(file);
		}
		dr_chdir( env_t::base_dir );
	}

	// use english if available
	current_langinfo = get_lang_by_iso("en");

	// it's all ok
	return true;
}


void translator::load_language_iso(const string &iso)
{
	string base(iso);
	langs[single_instance.lang_count].iso = strdup(iso.c_str());
	int loc = iso.find('_');
	if (loc != -1) {
		base = iso.substr(0, loc);
	}
	langs[single_instance.lang_count].iso_base = strdup(base.c_str());
}


void translator::set_language(int lang)
{
	if(  0 <= lang  &&  lang < single_instance.lang_count  ) {
		single_instance.current_lang = lang;
		current_langinfo = langs+lang;
		env_t::language_iso = langs[lang].iso;
		env_t::default_settings.set_name_language_iso( langs[lang].iso );
		init_custom_names(lang);
		current_langinfo->ellipsis_width = proportional_string_width( translate("...") );
		DBG_MESSAGE("translator::set_language()", "%s, unicode %d", langs[lang].name, true);
	}
	else {
		dbg->warning("translator::set_language()", "Out of bounds : %d", lang);
	}
}


// returns the id for this language or -1 if not there
int translator::get_language(const char *iso)
{
	for(  int i = 0;  i < single_instance.lang_count;  i++  ) {
		const char *iso_base = langs[i].iso_base;
		if(  iso_base[0] == iso[0]  &&  iso_base[1] == iso[1]  ) {
			return i;
		}
	}
	return -1;
}


bool translator::set_language(const char *iso)
{
	for(  int i = 0;  i < single_instance.lang_count;  i++  ) {
		const char *iso_base = langs[i].iso_base;
		if(  iso_base[0] == iso[0]  &&  iso_base[1] == iso[1]  ) {
			set_language(i);
			return true;
		}
	}

	// if the request language does not exist
	if( single_instance.current_lang == -1 ) {
		for( int i = 0; i < single_instance.lang_count; i++ ) {
			const char* iso_base = langs[i].iso_base;
			if( iso_base[0] == 'e'  &&  iso_base[1] == 'n' ) {
				set_language( i );
				return false;
			}
		}
		// not even english found ...
		set_language(0);
	}

	return false;
}


const char *translator::translate(const char *str)
{
	return get_lang()->translate(str);
}


const char *translator::translate(const char *str, int lang)
{
	return langs[lang].translate(str);
}


const char *translator::get_month_name(uint16 month)
{
	static const char *const month_names[] = {
		"January",
		"February",
		"March",
		"April",
		"May",
		"June",
		"July",
		"August",
		"September",
		"Oktober",
		"November",
		"December"
	};
	return translate(month_names[month % lengthof(month_names)]);
}

const char *translator::get_short_month_name(uint16 month)
{
	static const char *const short_month_names[] = {
		"Jan.",
		"Feb.",
		"Mar.",
		"Apr.",
		"May",
		"June",
		"July",
		"Aug.",
		"Sept.",
		"Oct.",
		"Nov.",
		"Dec."
	};
	return translate(short_month_names[month % lengthof(short_month_names)]);
}

const char *translator::get_date(uint16 year, uint16 month)
{
	char const* const month_ = get_month_name(month);
	char const* const year_sym = strcmp("YEAR_SYMBOL", translate("YEAR_SYMBOL")) ? translate("YEAR_SYMBOL") : "";
	static char sdate[256];
	switch (env_t::show_month) {
		case env_t::DATE_FMT_JAPANESE:
		case env_t::DATE_FMT_JAPANESE_NO_SEASON:
			sprintf(sdate, "%4d%s %s", year, year_sym, month_);
			break;
		case env_t::DATE_FMT_GERMAN:
		case env_t::DATE_FMT_GERMAN_NO_SEASON:
		case env_t::DATE_FMT_US:
		case env_t::DATE_FMT_US_NO_SEASON:
		default:
			sprintf(sdate, "%s %4d%s", month_, year, year_sym);
			break;
	}
	return sdate;
}

const char *translator::get_date(uint16 year, uint16 month, uint16 day, char const* season)
{
	char const* const month_ = get_month_name(month);
	char const* const year_sym = strcmp("YEAR_SYMBOL", translate("YEAR_SYMBOL")) ? translate("YEAR_SYMBOL") : "";
	char const* const day_sym = strcmp("DAY_SYMBOL", translate("DAY_SYMBOL")) ? translate("DAY_SYMBOL") : "";
	static char date[256];
	switch(env_t::show_month) {
		case env_t::DATE_FMT_GERMAN:
			sprintf(date, "%s %d. %s %d%s ", season, day, month_, year, year_sym);
			break;
		case env_t::DATE_FMT_GERMAN_NO_SEASON:
			sprintf(date, "%d. %s %d%s ", day, month_, year, year_sym);
			break;
		case env_t::DATE_FMT_US:
			sprintf(date, "%s %s %d %d%s ", season, month_, day, year, year_sym);
			break;
		case env_t::DATE_FMT_US_NO_SEASON:
			sprintf(date, "%s %d %d%s ", month_, day, year, year_sym);
			break;
		case env_t::DATE_FMT_JAPANESE:
			sprintf(date, "%s %d%s %s %d%s ", season, year, year_sym, month_, day, day_sym);
			break;
		case env_t::DATE_FMT_JAPANESE_NO_SEASON:
			sprintf(date, "%d%s %s %d%s ", year, year_sym, month_, day, day_sym);
			break;
		case env_t::DATE_FMT_MONTH:
			sprintf(date, "%s, %s %d%s ", month_, season, year, year_sym);
			break;
		case env_t::DATE_FMT_SEASON:
			sprintf(date, "%s %d%s ", season, year, year_sym);
			break;
	}
	return date;
}

const char *translator::get_short_date(uint16 year, uint16 month)
{
	char const* const month_ = get_short_month_name(month);
	char const* const year_sym = strcmp("YEAR_SYMBOL", translate("YEAR_SYMBOL")) ? translate("YEAR_SYMBOL") : "";
	static char sdate[256];
	switch (env_t::show_month) {
		case env_t::DATE_FMT_JAPANESE:
		case env_t::DATE_FMT_JAPANESE_NO_SEASON:
			sprintf(sdate, "%4d%s %s", year, year_sym , month_);
			break;
		case env_t::DATE_FMT_GERMAN:
		case env_t::DATE_FMT_GERMAN_NO_SEASON:
		case env_t::DATE_FMT_US:
		case env_t::DATE_FMT_US_NO_SEASON:
		default:
			sprintf(sdate, "%s %4d%s", month_, year, year_sym);
			break;
	}
	return sdate;
}


const char* translator::get_month_date( uint16 month, uint16 day )
{
	char const* const month_ = get_month_name( month );
	char const* const day_sym = strcmp( "DAY_SYMBOL", translate( "DAY_SYMBOL" ) )?translate( "DAY_SYMBOL" ):"";
	static char date[256];
	switch( env_t::show_month ) {
	case env_t::DATE_FMT_GERMAN:
	case env_t::DATE_FMT_GERMAN_NO_SEASON:
		sprintf( date, "%d. %s ", day, month_ );
		break;
	case env_t::DATE_FMT_US:
	case env_t::DATE_FMT_US_NO_SEASON:
		sprintf( date, "%s %d ", month_, day );
		break;
	case env_t::DATE_FMT_JAPANESE:
	case env_t::DATE_FMT_JAPANESE_NO_SEASON:
		sprintf( date, "%s %d%s", month_, day, day_sym );
		break;
	case env_t::DATE_FMT_SEASON:
	case env_t::DATE_FMT_MONTH:
		sprintf( date, "%s, ", month_ );
		break;
	}
	return date;
}


const char* translator::get_day_date(uint16 day)
{
	char const* const day_sym = strcmp("ORDINAL_DAR_SYMBOL", translate("ORDINAL_DAR_SYMBOL")) ? translate("ORDINAL_DAR_SYMBOL") : "th ";
	static char date[256];
	switch( env_t::show_month ) {
	case env_t::DATE_FMT_GERMAN:
	case env_t::DATE_FMT_GERMAN_NO_SEASON:
		sprintf( date, "%d%s", day, day_sym );
		return translator::translate( date );

	case env_t::DATE_FMT_US:
	case env_t::DATE_FMT_US_NO_SEASON:
		sprintf( date, "%d%s", day, day_sym );
		return translator::translate( date );

	case env_t::DATE_FMT_JAPANESE:
	case env_t::DATE_FMT_JAPANESE_NO_SEASON:
		sprintf( date, "%d%s", day, day_sym );
		return translator::translate( date );

	case env_t::DATE_FMT_SEASON:
	case env_t::DATE_FMT_MONTH:
		break;
	}
	return "";
}

/* get a name for a non-matching object */
const char *translator::compatibility_name(const char *str)
{
	if(  str==NULL  ) {
		return "(null)";
	}
	if(  str[0]=='\0'  ) {
		return str;
	}
	const char *trans = compatibility.get(str);
	return trans != NULL ? trans : str;
}
