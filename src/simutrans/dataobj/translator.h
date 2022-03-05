/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_TRANSLATOR_H
#define DATAOBJ_TRANSLATOR_H


#include <stdio.h>
#include <string>
#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/vector_tpl.h"


/**
 * Central location for loading and translating language text for the
 * UI of Simutrans.
 *
 * The languages are 0 based index, with a valid range of(with lang being
 * required language): <code>0 <= lang < lang_count</code>.
 */
class translator
{
private:
	//cannot be instantiated outside translator
	translator() { current_lang = -1; }

	int current_lang;
	int lang_count;

	/* The single instance that this class will use to gain access to
	 * the member variables such as language names
	 */
	static translator single_instance;

	static uint32 guess_highest_unicode(int lang);

	/* Methods related to loading a language file into memory */
	static void load_language_file(FILE* file);
	static void load_language_iso(const std::string &iso);

	static vector_tpl<char*> city_name_list;
	static vector_tpl<char*> street_name_list;

	static void load_custom_list( int lang, vector_tpl<char*> &name_list, const char *fileprefix );

public:
	struct lang_info {
		const char* translate(const char* text) const;

		stringhashtable_tpl<const char*> texts;
		const char *name;
		const char *iso;
		const char *iso_base;
		bool is_latin2_based;
		uint32 highest_character;
		uint8 ellipsis_width;
	};

	static void init_custom_names(int lang);

	static const vector_tpl<char*> &get_city_name_list() { return city_name_list; }
	static const vector_tpl<char*> &get_street_name_list() { return street_name_list; }

	/**
	 * Loads up all files of language type from the 'language' directory.
	 * This method must be called for languages to be loaded up, undefined
	 * behaviour may follow if calls to translate message or similar are
	 * called before load has been called
	 */
	static bool load();

	/**
	 * Loads all language file in folder folder_name
	 * folder_name is relative to current dir (set by chdir)
	 */
	static void load_files_from_folder(const char* folder_name, const char* what);

	/**
	 * Get/Set the currently selected language, based on the
	 * index number
	 */
	static int get_language() {
		return single_instance.current_lang;
	}

	// returns the id for this language or -1 if not there
	static int get_language(const char* iso);

	/** Get information about the currently selected language */
	static const lang_info* get_lang();

	static const lang_info* get_langs();

	/**
	 * First checks to see whether the language is in bounds, will
	 * then change what language is being used, otherwise prints
	 * an error message, leaving the language as it is
	 */
	static void set_language(int lang);
	static bool set_language(const char* iso);

	/**
	 * Returns the number of loaded languages.
	 */
	static int get_language_count() { return single_instance.lang_count; }

	/**
	 * Translates a given string(key) to its locale
	 * specific counterpart, using the current language
	 * table.
	 * the second variant just uses the language with the index
	 * @return translated string, (null) if string is null,
	 * or the string if the translation is not found
	 */
	static const char *translate(const char* str);
	static const char *translate(const char* str, int lang);

	/**
	 * @return replacement info for almost any object within the game
	 */
	static const char *compatibility_name(const char* str);

	// return the name of the month
	static const char *get_month_name(uint16 month);
	// return the short name of the month
	static const char *get_short_month_name(uint16 month);
	// return date in selected format
	static const char *get_date(uint16 year, uint16 month);
	static const char *get_date(uint16 year, uint16 month, uint16 day, char const* season);
	static const char* get_short_date(uint16 year, uint16 month);
	static const char* get_month_date(uint16 month, uint16 day);
	static const char* get_day_date( uint16 day );
};

#endif
