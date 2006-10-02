/*
 * translator.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "../simdebug.h"
#include "../simtypes.h"
#include "../simgraph.h"	// for unicode stuff
#include "translator.h"
#include "loadsave.h"
#include "../simmem.h"
#include "../utils/cstring_t.h"
#include "../utils/searchfolder.h"
#include "../utils/simstring.h"	//tstrncpy()
#include "../tpl/slist_mit_gewichten_tpl.h"
#include "../unicode.h"


#ifndef stringhashtable_tpl_h
#include "../tpl/stringhashtable_tpl.h"
#endif

translator * translator::single_instance = new translator();


#ifdef DEBUG
// diagnosis
static void
dump_hashtable(stringhashtable_tpl<const char *> *tbl)
{
	stringhashtable_iterator_tpl<const char *> iter (tbl);
	printf("keys\n====\n");
	tbl->dump_stats();
	printf("entries\n=======\n");
	while( iter.next() ) {
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
static bool
is_unicode_file(FILE *f)
{
	unsigned char	str[2];
	int	pos = ftell(f);
//DBG_DEBUG("is_unicode_file()", "checking for unicode");fflush(NULL);
	fread( str, 1, 2,  f );
//DBG_DEBUG("is_unicode_file()", "file starts with %x%x",str[0],str[1]);fflush(NULL);
	if(  str[0]==0xC2  &&  str[1]==0xA7  ) {
		// the first line must contain an UTF8 coded paragraph (Latin A7, UTF8 C2 A7), then it is unicode
DBG_DEBUG("is_unicode_file()", "file is UTF-8");
		return true;
	}
	fseek( f, pos, SEEK_SET );
	return false;
}



// recodes string to put them into the tables
char *
recode(const char *src,bool translate_from_utf,bool translate_to_utf)
{
	char *base;
	if(  translate_to_utf  ) {
		// worst case
		base = (char *) guarded_malloc(sizeof(char) * (strlen(src)*2+2));
	}
	else {
		base = (char *) guarded_malloc(sizeof(char) * (strlen(src)+2));
	}
	char *dst = base, c;

	do {
		if(*src =='\\') {
			src +=2;
			*dst++ = c = '\n';
		}
		else {
			if(  translate_from_utf==translate_from_utf  ) {
				// both true or false => do noting
				// just copy
				c = *src++;
				*dst++ = c;
			} else if(  translate_to_utf  ) {
				// make UTF8 from latin
				dst += utf16_to_utf8((unsigned char)*src++, (utf8*)dst);
			}
			else if(  translate_from_utf  ) {
				// make latin from UTF8 (ignore overflows!)
				int len=0;
				*dst++ = c = utf8_to_utf16((const utf8*)src, &len);
				src += len;
			}
		}
	} while(c != 0);
	*dst = 0;
	return base;
}



/* needed for loading city names */
static char szenario_path[256];

/* Liste aller Städtenamen
 * @author Hj. Malthaner
 */
static slist_tpl <const char *> namen_liste;

/* since the city names are language dependent, they are now kept here!
 * @date 2.1.2005
 * @author prissi
 */
static const int anz_t1 = 11;
static const char * name_t1[anz_t1] =
{
    "%1_CITY_SYLL", "%2_CITY_SYLL", "%3_CITY_SYLL", "%4_CITY_SYLL", "%5_CITY_SYLL",
    "%6_CITY_SYLL", "%7_CITY_SYLL", "%8_CITY_SYLL", "%9_CITY_SYLL", "%A_CITY_SYLL",
    "%B_CITY_SYLL",

};

static const int anz_t2 = 10;
static const char * name_t2[anz_t2] =
{
    "&1_CITY_SYLL", "&2_CITY_SYLL", "&3_CITY_SYLL", "&4_CITY_SYLL", "&5_CITY_SYLL",
    "&6_CITY_SYLL", "&7_CITY_SYLL", "&8_CITY_SYLL", "&9_CITY_SYLL", "&A_CITY_SYLL",
};



int translator::get_count_city_name(void)
{
  return namen_liste.count();
}



/* returns a random city name */
void	translator::get_city_name(char *name, int nr)
{
  if(  namen_liste.count()<=0  ) {
  	// fallback for empty list (should never happen)
  	strcpy( name, "Simcity" );
  	return;
  }
  const char *list_name = namen_liste.at( nr%namen_liste.count() );
  tstrncpy(name, list_name, 63);
  name[63] = 0;
}



/* the city list is now reloaded after the language is changed
 * new cities will get their appropriate names
 * @author hajo, prissi
 */
int  init_city_names(bool is_utf_language)
{
	FILE * file;

	// alle namen aufräumen
	namen_liste.clear();

	// Hajo: init city names. There are two options:
	//
	// 1.) read list from file
	// 2.) create random names

	// try to read list

	// @author prissi: first try in scenario
	cstring_t local_file_name(szenario_path);
	local_file_name = local_file_name+"text/citylist_"+translator::get_language_name_iso(translator::get_language()) + ".txt";
DBG_DEBUG("translator::init_city_names()","try to read city name list '%s'",local_file_name.chars());
	file=fopen(local_file_name, "rb");
DBG_DEBUG("translator::init_city_names()","file %p",file);fflush(NULL);
	// not found => try usual location
	if(file==NULL) {
		cstring_t local_file_name("text/citylist_");
		local_file_name = local_file_name+translator::get_language_name_iso(translator::get_language()) + ".txt";
DBG_DEBUG("translator::init_city_names()","try to read city name list '%s'",local_file_name.chars());
		file = fopen(local_file_name.chars(), "rb");
	}
DBG_DEBUG("translator::init_city_names()","file %p",file);

	if(file!=NULL) {
		// ok, could open file
		char buf[256];
    bool file_is_utf = is_unicode_file(file);
		while(!feof(file)) {

			if(fgets(buf, 128, file)) {
				rtrim(buf);
				char *c= recode( buf, file_is_utf, is_utf_language );
				namen_liste.insert(c);
			}
		}
		fclose(file);

	}

	if(  namen_liste.count()==0  ) {
DBG_MESSAGE("translator::init_city_names", "reading failed, creating random names.");
		// Hajo: try to read list failed, create random names
		for(int i=0; i<anz_t1; i++) {
			const int l1 = strlen(translator::translate(name_t1[i]));
			for(int j=0; j<anz_t2; j++) {
				const int l2 = strlen(translator::translate(name_t2[j]));
				char *c=(char *)guarded_malloc(l1+l2+1);
				sprintf(c, "%s%s", translator::translate(name_t1[i]), translator::translate(name_t2[j]));
				namen_liste.insert(c);
			}
		}
	}
	return namen_liste.count();
}


/* now on to the translate stuff */


void load_language_file_body(FILE *file, stringhashtable_tpl<const char *> * table, bool language_is_utf, bool file_is_utf )
{
    char buffer1 [4096];
    char buffer2 [4096];

	bool	convert_to_unicode = language_is_utf && !file_is_utf;

    do {
        fgets(buffer1, 4095, file);
        if(  buffer1[0]=='#' ) {
        	// ignore comments
        	continue;
        }
        fgets(buffer2, 4095, file);

        buffer1[4095] = 0;
        buffer2[4095] = 0;

        if(!feof( file )) {
            // "\n" etc umsetzen
            buffer1[strlen(buffer1)-1] = 0;
            buffer2[strlen(buffer2)-1] = 0;
            table->set(recode(buffer1,file_is_utf,false), recode(buffer2,false,convert_to_unicode));
        }
    } while(!feof( file ));
}


void translator::load_language_file(FILE *file)
{
    char buffer1 [256];
    bool file_is_utf = is_unicode_file(file);
    // Read language name
    fgets(buffer1, 255, file);
    buffer1[strlen(buffer1)-1] = 0;

    single_instance->languages[single_instance->lang_count] = new stringhashtable_tpl <const char *>;
    single_instance->language_names[single_instance->lang_count] = strdup(buffer1);
    // if the language file is utf, all language strings are assumed to be unicode
    // @author prissi
    single_instance->language_is_utf_encoded[single_instance->lang_count] = file_is_utf;

    //load up translations, putting them into
    //language table of index 'lang'
    load_language_file_body(file,single_instance->languages[single_instance->lang_count],file_is_utf,file_is_utf);
}


bool translator::load(const cstring_t & scenario_path)
{
	tstrncpy(szenario_path, scenario_path, sizeof(szenario_path));

	//initialize these values to 0(ie. nothing loaded)
	single_instance->lang_count = single_instance->current_lang = 0;

DBG_MESSAGE("translator::load()", "Loading languages...");
	searchfolder_t folder;
	int num_lang_dat = folder.search("text/", "tab");

DBG_MESSAGE("translator::load()", "%d languages to load", num_lang_dat);
	int loc = MAX_LANG;

	//only allows MAX_LANG number of languages to be loaded
	//read now the basic language infos
	while(num_lang_dat-->0  &&  loc>0) {
		cstring_t fileName(folder.at(num_lang_dat));
		cstring_t testFolderName("text/");
		cstring_t iso = fileName.substr(fileName.find_back('/')+1, fileName.len() - 4);

		FILE *file = NULL;
//		file = fopen(testFolderName + iso + ".tab", "rb");
		file = fopen(fileName.chars(), "rb");
		if(file) {
DBG_MESSAGE("translator::load()","base file \"%s\" - iso: \"%s\"",fileName.chars(),iso.chars());
			load_language_iso(iso);
			load_language_file(file);
			fclose(file);
			single_instance->lang_count++;
			loc --;
		}
	}

	// now read the scenario specific text
	// there can be more than one file per language, provided it is name like iso_xyz.tab
	cstring_t folderName(scenario_path+"text/");
	int num_pak_lang_dat = folder.search(folderName, "tab");
DBG_MESSAGE("translator::load()","search folder \"%s\" and found %i files",folderName.chars(),num_pak_lang_dat);
	//read now the basic language infos
	while(num_pak_lang_dat-->0) {
		cstring_t fileName(folder.at(num_pak_lang_dat));
		cstring_t iso = fileName.substr(fileName.find_back('/')+1, fileName.len()-4);

		int iso_nr=get_language_iso(iso.chars());
		if(iso_nr>=0) {
DBG_MESSAGE("translator::load()","loading pak translations from %s for iso nr %i",fileName.chars(),iso_nr);
			FILE *file = fopen(fileName.chars(), "rb");
			if(file) {
				bool file_is_utf = is_unicode_file(file);
				load_language_file_body(file, single_instance->languages[iso_nr], single_instance->language_is_utf_encoded[iso_nr], file_is_utf );
				fclose(file);
			}
			else {
				dbg->warning("translator::load()", "cannot open '%s'", fileName.chars());
			}
		}
		else {
			dbg->warning("translator::load()", "no basic texts for language '%s'", iso.chars());
		}
	}

  // some languages not loaded
  // let the user know what's happened
  if(num_lang_dat>0) {
      dbg->warning("translator::load()", "%d languages were not loaded, limit reached", num_lang_dat);
      while(num_lang_dat-- > 0) {
          dbg->warning("translator::load()", " %s not loaded", folder.at(num_lang_dat));
      }
  }

  //if NO languages were loaded then game cannot continue
  if(single_instance->lang_count<1) {
      return false;
  }

	// now we try to read the compatibility stuff
	FILE *file = fopen(scenario_path + "compat.tab", "rb");
	single_instance->compatibility = NULL;
	if(file) {
		single_instance->compatibility = new stringhashtable_tpl<const char *>;
		load_language_file_body(file, single_instance->compatibility, false, false );
		DBG_MESSAGE("translator::load()", "scenario compatibilty texts loaded." );
		fclose(file);
//		dump_hashtable(single_instance->compatibility);
	}
	else {
		DBG_MESSAGE("translator::load()", "no scenario compatibilty texts" );
	}

	// Hajo: look for english, use english if available
	int en_iso_nr = get_language_iso( "en" );
	if(en_iso_nr>=0) {
		set_language(en_iso_nr);
	}

  // it's all ok
  return true;
}


void translator::load_language_iso(cstring_t & iso)
{
	cstring_t base(iso);
	single_instance->language_names_iso[single_instance->lang_count] = strdup(iso.chars());
	int loc = iso.find('_');
	if(loc != -1) {
		base = iso.left(loc);
	}
	single_instance->language_names_iso_base[single_instance->lang_count] = strdup(base.chars());
}


void translator::set_language(int lang)
{
    if(is_in_bounds(lang)) {
        single_instance->current_lang = lang;
        display_set_unicode( single_instance->language_is_utf_encoded[lang] );
        init_city_names( single_instance->language_is_utf_encoded[lang] );
DBG_MESSAGE("translator::set_language()","%s, unicode %d", single_instance->language_names[lang], single_instance->language_is_utf_encoded[lang] );
    } else {
        dbg->warning("translator::set_language()" , "Out of bounds : %d", lang);
    }
}


int translator::get_language_iso(const char *iso)
{
	for(int i=0; i<single_instance->lang_count; i++) {
		const char *iso_base = get_language_name_iso_base(i);
		if(iso_base[0]==iso[0]  &&  iso_base[1]==iso[1]) {
			return i;
		}
	}
	return -1;
}


void translator::set_language(char *iso)
{
	for(int i=0; i<single_instance->lang_count; i++) {
		const char *iso_base = get_language_name_iso_base(i);
		if(iso_base[0] == iso[0] && iso_base[0] == iso[1]) {
			set_language(i);
			return;
		}
	}
}


const char * translator::translate(const char *str)
{
    if(str == NULL) {
        return "(null)";
    }
    else if(*str==0) {
        return str;
    }
    else {
        const char *trans=(const char *)single_instance->languages[get_language()]->get(str);
        return trans ? trans : str;
    }
}


const char * translator::translate_from_lang(const int lang,const char *str)
{
	if(str == NULL){
		return "(null)";
	}
    else if(*str==0  ||  !is_in_bounds(lang)) {
        return str;
    }
    else {
        const char *trans=(const char *)single_instance->languages[lang]->get(str);
        return trans ? trans : str;
    }
}


/* get a name for a non-matching object */
const char * translator::compatibility_name(const char *str)
{
    if(str == NULL) {
        return "(null)";
    }
    else if(*str==0  ||  single_instance->compatibility==NULL) {
        return str;
    }
    else {
        const char *trans=(const char *)single_instance->compatibility->get(str);
        return trans ? trans : str;
    }
}


/**
 * Checks if the given string is in the translation table
 * @author Hj. Malthaner
 */
bool translator::check(const char *str)
{
    const char * trans = (const char *)single_instance->languages[get_language()]->get(str);
    return trans != 0;
}


/** Returns the language name of the specified index */
const char *
translator::get_language_name(int lang)
{
	if(is_in_bounds(lang)) {
		return single_instance->language_names[lang];
	}
	else {
		dbg->warning("translator::get_language_name()","Out of bounds : %d", lang);
		return "Error";
	}
}


const char *
translator::get_language_name_iso(int lang)
{
	if(is_in_bounds(lang)) {
		return single_instance->language_names_iso[lang];
	}
	else {
		dbg->warning("translator::get_language_name_iso()","Out of bounds : %d", lang);
		return "Error";
	}
}

const char * translator::get_language_name_iso_base(int lang)
{
	if(is_in_bounds(lang)) {
		return single_instance->language_names_iso_base[lang];
	}
	else {
		dbg->warning("translator::get_language_name_iso_base()","Out of bounds : %d", lang);
		return "Error";
	}
}


void translator::rdwr(loadsave_t *file)
{
	int actual_lang;

	if(file->is_saving()) {
		actual_lang = single_instance->current_lang;
	}
	file->rdwr_enum(actual_lang, "\n");

	if(file->is_loading()) {
		set_language(actual_lang);
	}
}



