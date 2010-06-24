/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "../macros.h"
#include "../simdebug.h"
#include "../simtypes.h"
#include "../simgraph.h"	// for unicode stuff
#include "translator.h"
#include "loadsave.h"
#include "umgebung.h"
#include "../simmem.h"
#include "../utils/searchfolder.h"
#include "../utils/simstring.h"
#include "../unicode.h"
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


const char *translator::lang_info::translate(const char* text) const
{
	if (text    == NULL) {
		return "(null)";
	}
	if (text[0] == '\0') {
		return text;
	}
	const char* trans = texts.get(text);
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


#ifdef need_dump_hashtable
// diagnosis
static void dump_hashtable(stringhashtable_tpl<const char*>* tbl)
{
	stringhashtable_iterator_tpl<const char*> iter(tbl);
	printf("keys\n====\n");
	tbl->dump_stats();
	printf("entries\n=======\n");
	while (iter.next()) {
		printf("%s\n",iter.get_current_value());
	}
	fflush(NULL);
}
#endif

/* first two file fuctions needed in connection with utf */

/* checks, if we need a unicode translation (during load only done for identifying strings like "Auflösen")
 * @date 2.1.2005
 * @author prissi
 */
static bool is_unicode_file(FILE* f)
{
	unsigned char	str[2];
	int	pos = ftell(f);
//	DBG_DEBUG("is_unicode_file()", "checking for unicode");
//	fflush(NULL);
	fread( str, 1, 2,  f );
//	DBG_DEBUG("is_unicode_file()", "file starts with %x%x",str[0],str[1]);
//	fflush(NULL);
	if (str[0] == 0xC2 && str[1] == 0xA7) {
		// the first line must contain an UTF8 coded paragraph (Latin A7, UTF8 C2 A7), then it is unicode
		DBG_DEBUG("is_unicode_file()", "file is UTF-8");
		return true;
	}
	fseek(f, pos, SEEK_SET);
	return false;
}



// the bytes in an UTF sequence have always the format 10xxxxxx
static inline int is_cont_char(utf8 c) { return (c & 0xC0) == 0x80; }


// recodes string to put them into the tables
static char* recode(const char* src, bool translate_from_utf, bool translate_to_utf)
{
	char* base;
	if (translate_to_utf!=translate_from_utf) {
		// worst case
		base = MALLOCN(char, strlen(src) * 2 + 2);
	}
	else {
		base = MALLOCN(char, strlen(src) + 2);
	}
	char* dst = base;
	uint8 c = 0;

	do {
		if (*src =='\\') {
			src +=2;
			*dst++ = c = '\n';
		}
		else {
			c = *src;
			if(c>127) {
				if(translate_from_utf == translate_to_utf) {
					// but copy full letters! (or, if ASCII, copy more than one letter, does not matter
					do {
						*dst++ = *src++;
					} while (is_cont_char(*src));
					c = *src;
				} else if (translate_to_utf) {
					// make UTF8 from latin
					dst += c = utf16_to_utf8((unsigned char)*src++, (utf8*)dst);
				} else if (translate_from_utf) {
					// make latin from UTF8 (ignore overflows!)
					size_t len = 0;
					*dst++ = c = (uint8)utf8_to_utf16((const utf8*)src, &len);
					src += len;
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
static char szenario_path[256];

/* Liste aller Städtenamen
 * @author Hj. Malthaner
 */
static vector_tpl<const char*> namen_liste;



int translator::get_count_city_name(void)
{
	return namen_liste.get_count();
}



const char* translator::get_city_name(uint nr)
{
	// fallback for empty list (should never happen)
	if (namen_liste.empty()) {
		return "Simcity";
	}
	return namen_liste[nr % namen_liste.get_count()];
}



/* the city list is now reloaded after the language is changed
 * new cities will get their appropriate names
 * @author hajo, prissi
 */
static void init_city_names(bool is_utf_language)
{
	FILE* file;

	// alle namen aufräumen
	namen_liste.clear();

	// Hajo: init city names. There are two options:
	//
	// 1.) read list from file
	// 2.) create random names

	// try to read list

	// @author prissi: first try in scenario
	// not found => try user location
	string local_file_name(umgebung_t::user_dir);
	local_file_name = local_file_name + "citylist_" + translator::get_lang()->iso + ".txt";
	file = fopen(local_file_name.c_str(), "rb");
	DBG_DEBUG("translator::init_city_names()", "try to read city name list '%s'", local_file_name.c_str());
	if (file==NULL) {
		string local_file_name(umgebung_t::program_dir);
		local_file_name = local_file_name + szenario_path + "text/citylist_" + translator::get_lang()->iso + ".txt";
		DBG_DEBUG("translator::init_city_names()", "try to read city name list '%s'", local_file_name.c_str());
		file = fopen(local_file_name.c_str(), "rb");
		DBG_DEBUG("translator::init_city_names()", "try to read city name list '%s'", local_file_name.c_str());
	}
	// not found => try old location
	if (file==NULL) {
		string local_file_name(umgebung_t::program_dir);
		local_file_name = local_file_name + "text/citylist_" + translator::get_lang()->iso + ".txt";
		DBG_DEBUG("translator::init_city_names()", "try to read city name list '%s'", local_file_name.c_str());
		file = fopen(local_file_name.c_str(), "rb");
		DBG_DEBUG("translator::init_city_names()", "try to read city name list '%s'", local_file_name.c_str());
	}
	fflush(NULL);
	DBG_DEBUG("translator::init_city_names()","file %p",file);

	if (file != NULL) {
		// ok, could open file
		char buf[256];
		bool file_is_utf = is_unicode_file(file);
		while(  !feof(file)  ) {
			if (fgets_line(buf, sizeof(buf), file)) {
				rtrim(buf);
				char *c = recode(buf, file_is_utf, is_utf_language);
				if(  *c!=0  &&  *c!='#'  ) {
					namen_liste.append(c);
				}
			}
		}
		fclose(file);
	}

	if (namen_liste.empty()) {
		DBG_MESSAGE("translator::init_city_names", "reading failed, creating random names.");
		// Hajo: try to read list failed, create random names
		for(  uint i = 0;  i < 16;  i++  ) {
			char name[32];
			sprintf( name, "%%%X_CITY_SYLL", i );
			const char* s1 = translator::translate(name);
			if(s1==name) {
				// name not available ...
				continue;
			}
			// now add all second name extensions ...
			const size_t l1 = strlen(s1);
			for(  uint j = 0;  j < 16;  j++  ) {

				sprintf( name, "&%X_CITY_SYLL", j );
				const char* s2 = translator::translate(name);
				if(s2==name) {
					// name not available ...
					continue;
				}
				const size_t l2 = strlen(s2);
				char* const c = MALLOCN(char, l1 + l2 + 1);
				sprintf(c, "%s%s", s1, s2);
				namen_liste.append(c);
			}
		}
	}
}


/* now on to the translate stuff */


static void load_language_file_body(FILE* file, stringhashtable_tpl<const char*>* table, bool language_is_utf, bool file_is_utf)
{
	char buffer1 [4096];
	char buffer2 [4096];

	bool convert_to_unicode = language_is_utf && !file_is_utf;

	do {
		fgets_line(buffer1, sizeof(buffer1), file);
		if (buffer1[0] == '#') {
			// ignore comments
			continue;
		}
		fgets_line(buffer2, sizeof(buffer2), file);

		if (!feof(file)) {
			// "\n" etc umsetzen
			//buffer1[strlen(buffer1) - 1] = '\0';
			//buffer2[strlen(buffer2) - 1] = '\0';
			table->set(recode(buffer1, file_is_utf, false), recode(buffer2, false, convert_to_unicode));
		}
	} while (!feof(file));
}


void translator::load_language_file(FILE* file)
{
	char buffer1 [256];
	bool file_is_utf = is_unicode_file(file);
	// Read language name
	fgets_line(buffer1, sizeof(buffer1), file);

	langs[single_instance.lang_count].name = strdup(buffer1);
	// if the language file is utf, all language strings are assumed to be unicode
	// @author prissi
	langs[single_instance.lang_count].utf_encoded = file_is_utf;

	//load up translations, putting them into
	//language table of index 'lang'
	load_language_file_body(file, &langs[single_instance.lang_count].texts, file_is_utf, file_is_utf);
}


static translator::lang_info* get_lang_by_iso(const char* iso)
{
	for (translator::lang_info* i = langs; i != langs + translator::get_language_count(); ++i) {
		if (i->iso_base[0] == iso[0] && i->iso_base[1] == iso[1]) {
			return i;
		}
	}
	return NULL;
}


bool translator::load(const string &scenario_path)
{
	chdir( umgebung_t::program_dir );
	tstrncpy(szenario_path, scenario_path.c_str(), lengthof(szenario_path));

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

		FILE* file = NULL;
		file = fopen(fileName.c_str(), "rb");
		if (file != NULL) {
			DBG_MESSAGE("translator::load()", "base file \"%s\" - iso: \"%s\"", fileName.c_str(), iso.c_str());
			load_language_iso(iso);
			load_language_file(file);
			fclose(file);
			single_instance.lang_count++;
			if (single_instance.lang_count == lengthof(langs)) {
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

	// now read the scenario specific text
	// there can be more than one file per language, provided it is name like iso_xyz.tab
	const string folderName(scenario_path + "text/");
	int num_pak_lang_dat = folder.search(folderName, "tab");
	DBG_MESSAGE("translator::load()", "search folder \"%s\" and found %i files", folderName.c_str(), num_pak_lang_dat);
	//read now the basic language infos
	for (searchfolder_t::const_iterator i = folder.begin(), end = folder.end(); i != end; ++i) {
		const string fileName(*i);
		size_t pstart = fileName.rfind('/') + 1;
		const string iso = fileName.substr(pstart, fileName.size() - pstart - 4);

		lang_info* lang = get_lang_by_iso(iso.c_str());
		if (lang != NULL) {
			DBG_MESSAGE("translator::load()", "loading pak translations from %s for language %s", fileName.c_str(), lang->iso_base);
			FILE* file = fopen(fileName.c_str(), "rb");
			if (file != NULL) {
				bool file_is_utf = is_unicode_file(file);
				load_language_file_body(file, &lang->texts, lang->utf_encoded, file_is_utf);
				fclose(file);
			} else {
				dbg->warning("translator::load()", "cannot open '%s'", fileName.c_str());
			}
		} else {
			dbg->warning("translator::load()", "no basic texts for language '%s'", iso.c_str());
		}
	}

	if(  umgebung_t::program_dir!=umgebung_t::user_dir  &&  umgebung_t::default_einstellungen.get_with_private_paks()  ) {
		chdir( umgebung_t::user_dir );
		// now read the scenario specific text
		// there can be more than one file per language, provided it is name like iso_xyz.tab
		const string folderName(scenario_path + "text/");
		folder.search(folderName, "tab");
		//read now the basic language infos
		for (searchfolder_t::const_iterator i = folder.begin(), end = folder.end(); i != end; ++i) {
			const string fileName(*i);
			size_t pstart = fileName.rfind('/') + 1;
			const string iso = fileName.substr(pstart, fileName.size()  - pstart - 4);

			lang_info* lang = get_lang_by_iso(iso.c_str());
			if (lang != NULL) {
				DBG_MESSAGE("translator::load()", "loading pak addon translations from %s for language %s", fileName.c_str(), lang->iso_base);
				FILE* file = fopen(fileName.c_str(), "rb");
				if (file != NULL) {
					bool file_is_utf = is_unicode_file(file);
					load_language_file_body(file, &lang->texts, lang->utf_encoded, file_is_utf);
					fclose(file);
				} else {
					dbg->warning("translator::load()", "cannot open '%s'", fileName.c_str());
				}
			} else {
				dbg->warning("translator::load()", "no addon texts for language '%s'", iso.c_str());
			}
		}
		chdir( umgebung_t::program_dir );
	}

	//if NO languages were loaded then game cannot continue
	if (single_instance.lang_count < 1) {
		return false;
	}

	// now we try to read the compatibility stuff
	FILE* file = fopen((scenario_path + "compat.tab").c_str(), "rb");
	if (file != NULL) {
		load_language_file_body(file, &compatibility, false, false);
		DBG_MESSAGE("translator::load()", "scenario compatibilty texts loaded.");
		fclose(file);
	}
	else {
		DBG_MESSAGE("translator::load()", "no scenario compatibility texts");
	}

	// also addon compatibility ...
	if(  umgebung_t::program_dir!=umgebung_t::user_dir  &&  umgebung_t::default_einstellungen.get_with_private_paks()  ) {
		chdir( umgebung_t::user_dir );
		FILE* file = fopen((scenario_path + "compat.tab").c_str(), "rb");
		if (file != NULL) {
			load_language_file_body(file, &compatibility, false, false);
			DBG_MESSAGE("translator::load()", "scenario addon compatibility texts loaded.");
			fclose(file);
		}
		chdir( umgebung_t::program_dir );
	}

//	dump_hashtable(&compatibility);

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
		base = iso.substr(loc);
	}
	langs[single_instance.lang_count].iso_base = strdup(base.c_str());
}


void translator::set_language(int lang)
{
	if(  0 <= lang  &&  lang < single_instance.lang_count  ) {
		single_instance.current_lang = lang;
		current_langinfo = langs+lang;
		umgebung_t::language_iso = langs[lang].iso;
		umgebung_t::default_einstellungen.set_name_language_iso( langs[lang].iso );
		display_set_unicode(langs[lang].utf_encoded);
		init_city_names(langs[lang].utf_encoded);
		DBG_MESSAGE("translator::set_language()", "%s, unicode %d", langs[lang].name, langs[lang].utf_encoded);
	}
	else {
		dbg->warning("translator::set_language()", "Out of bounds : %d", lang);
	}
}


void translator::set_language(const char* iso)
{
	for(  int i = 0;  i < single_instance.lang_count;  i++  ) {
		const char* iso_base = langs[i].iso_base;
		if(  iso_base[0] == iso[0]  &&  iso_base[1] == iso[1]  ) {
			set_language(i);
			return;
		}
	}
}


const char* translator::translate(const char* str)
{
	return get_lang()->translate(str);
}


const char* translator::translate(const char* str, int lang)
{
	return langs[lang].translate(str);
}


const char* translator::get_month_name(uint16 month)
{
	static const char* const month_names[] = {
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
	assert(month < lengthof(month_names));
	return translate(month_names[month]);
}


/* get a name for a non-matching object */
const char* translator::compatibility_name(const char* str)
{
	if(  str==NULL  ) {
		return "(null)";
	}
	if(  str[0]=='\0'  ) {
		return str;
	}
	const char* trans = compatibility.get(str);
	return trans != NULL ? trans : str;
}
