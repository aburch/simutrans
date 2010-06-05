/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */
#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <stdio.h>
#include "../tpl/stringhashtable_tpl.h"


class cstring_t;


/**
 * Central location for loading and translating language text for the
 * UI of Simutrans.
 *
 * The languages are 0 based index, with a valid range of(with lang being
 * required language): <code>0 <= lang < lang_count</code>.
 *
 * @author Hj. Malthaner, Adam Barclay
 */
class translator
{
	private:
		//cannot be instantiated outside translator
		translator() { current_lang = -1; }
		~translator() {}

		int current_lang;
		int lang_count;

		/* The single instance that this class will use to gain access to
		 * the member variables such as language names
		 */
		static translator single_instance;

		/* Methods related to loading a language file into memory */
		static void load_language_file(FILE* file);
		static void load_language_iso(cstring_t& iso);

	public:
		struct lang_info {
			const char* translate(const char* text) const;

			stringhashtable_tpl<const char*> texts;
			const char* name;
			const char* iso;
			const char* iso_base;
			bool utf_encoded;
		};

		static const char* get_city_name(uint nr); ///< return a random city name
		static int get_count_city_name(void);

		/**
		 * Loads up all files of language type from the 'language' directory.
		 * This method must be called for languages to be loaded up, undefined
		 * behaviour may follow if calls to translate message or similar are
		 * called before load has been called
		 */
		static bool load(const cstring_t& scenario_path);

		/**
		 * Get/Set the currently selected language, based on the
		 * index number
		 */
		static int get_language() {
			return single_instance.current_lang;
		}

		/** Get information about the currently selected language */
		static const lang_info* get_lang();

		static const lang_info* get_langs();

		/**
		 * First checks to see whether the language is in bounds, will
		 * then change what language is being used, otherwise prints
		 * an error message, leaving the language as it is
		 */
		static void set_language(int lang);
		static void set_language(const char* iso);

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
};

#endif
