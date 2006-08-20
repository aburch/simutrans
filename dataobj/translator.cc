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

#include "../simdebug.h"
#include "../simtypes.h"
#include "translator.h"
#include "loadsave.h"
#include "../simmem.h"
#include "../utils/cstring_t.h"
#include "../utils/searchfolder.h"


#ifndef stringhashtable_tpl_h
#include "../tpl/stringhashtable_tpl.h"
#endif



translator * translator::single_instance = new translator();


static char * recode(const char *src)
{
    char *base = (char *) guarded_malloc(sizeof(char) * (strlen(src)+2));
    char *dst = base;

    do {
        if(*src =='\\') {
            src +=2;
            *dst++ = '\n';
        } else {
            *dst++ = *src++;
        }
    } while(*src != 0);
    *dst = 0;

    return base;
}


static void load_language_file_body(FILE *file,
				    stringhashtable_tpl<const char *> * table)
{
    char buffer1 [4096];
    char buffer2 [4096];

    do {
        fgets(buffer1, 4095, file);
        fgets(buffer2, 4095, file);

        buffer1[4095] = 0;
        buffer2[4095] = 0;

        if(!feof( file )) {
            // "\n" etc umsetzen
            buffer1[strlen(buffer1)-1] = 0;
            buffer2[strlen(buffer2)-1] = 0;
            table->put(recode(buffer1), recode(buffer2));
        }
    } while(!feof( file ));
}


void translator::load_language_file(FILE *file)
{
    char buffer1 [256];

    // Read language name
    fgets(buffer1, 255, file);
    buffer1[strlen(buffer1)-1] = 0;

    single_instance->languages[single_instance->lang_count] = new stringhashtable_tpl <const char *>;
    single_instance->language_names[single_instance->lang_count] = strdup(buffer1);

    //load up translations, putting them into
    //language table of index 'lang'
    load_language_file_body(file,
			    single_instance->languages[single_instance->lang_count]);
}


bool translator::load(const cstring_t & scenario_path)
{
    //initialize these values to 0(ie. nothing loaded)
    single_instance->lang_count = single_instance->current_lang = 0;

    dbg->message("translator::load()", "Loading languages...");
    searchfolder_t folder;
    int i = folder.search("text/", "tab");

    dbg->message("translator::load()", " %d languages to load", i);
    int loc = MAX_LANG;

    //only allows MAX_LANG number of languages to be loaded
    //this will be changed over to vector of unlimited languages
    for(;i-- > 0 && loc-- > 0;)
    {
        cstring_t fileName(folder.at(i));
        cstring_t folderName("text/");
        cstring_t extension(".tab");
        cstring_t iso = fileName.substr(5, fileName.len() - 4);

        FILE *file = NULL;
        file = fopen(folderName + iso + extension, "rb");
        if(file) {
            load_language_iso(iso);
            load_language_file(file);
            fclose(file);

	    // Hajo: read scenario specific texts
	    file = fopen(scenario_path + "/text" + iso + extension, "rb");
	    if(file) {
	      load_language_file_body(file,
				      single_instance->languages[single_instance->lang_count]);
	      fclose(file);
	    } else {
	      dbg->warning("translator::load()", "no scenario texts for language '%s'", iso.chars());
	    }

            single_instance->lang_count++;
        } else {
            dbg->warning("translator::load()", "reading language file %s failed", fileName.chars());
        }
    }

    // some languages not loaded
    // let the user know what's happened
    if(i > 1)
    {
        dbg->message("translator::load()", "%d languages were not loaded, limit reached", i);
        for(;i-- > 0;)
        {
            dbg->warning("translator::load()", " %s not loaded", folder.at(i).chars());
        }
    }

    //if NO languages were loaded then game cannot continue
    if(single_instance->lang_count < 1)
    {
        return false;
    }


    // Hajo: look for english, use english if available
    for(int i=0; i<single_instance->lang_count; i++) {
      const char *iso_base = get_language_name_iso_base(i);

      // dbg->message("translator::load()", "%d: iso_base=%s", i, iso_base);

      if(iso_base[1] == 'e' && iso_base[2] == 'n') {
	set_language(i);
	break;
      }
    }

    // it's all ok
    return true;
}


void translator::load_language_iso(cstring_t & iso)
{
    cstring_t base(iso);
    single_instance->language_names_iso[single_instance->lang_count] = strdup(iso.chars()+1);
    int loc = iso.find('_');
    if(loc != -1)
    {
        base = iso.left(loc);
    }
    single_instance->language_names_iso_base[single_instance->lang_count] = strdup(base.chars());
}


void translator::set_language(int lang)
{
    if(is_in_bounds(lang))
    {
        single_instance->current_lang = lang;
    } else {
        dbg->warning("translator::set_language()" , "Out of bounds : %d", lang);
    }
}


const char * translator::translate(const char *str)
{
    if(str == NULL)
    {
        return "(null)";
    } else if(!check(str))
    {
        return str;
    } else {
        return (const char *)single_instance->languages[get_language()]->get(str);
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
const char * translator::get_language_name(int lang)
{
    if(is_in_bounds(lang))
    {
        return single_instance->language_names[lang];
    } else {
        dbg->warning("translator::get_language_name()","Out of bounds : %d", lang);
        return "Error";
    }
}


const char * translator::get_language_name_iso(int lang)
{
    if(is_in_bounds(lang))
    {
        return single_instance->language_names_iso[lang];
    } else {
        dbg->warning("translator::get_language_name_iso()","Out of bounds : %d", lang);
        return "Error";
    }
}

const char * translator::get_language_name_iso_base(int lang)
{
    if(is_in_bounds(lang))
    {
        return single_instance->language_names_iso_base[lang];
    } else {
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
