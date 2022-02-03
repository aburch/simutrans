/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string.h>
#include "fetchopt.h"

Fetchopt_t::Fetchopt_t(int argc, char **argv, const char *optstring) {
	optarg = NULL;
	optind = 1;
	optstr = optstring;
	ac = argc;
	av = argv;
	pos = 1;
}

char *Fetchopt_t::get_optarg() {
	return optarg;
}

int Fetchopt_t::get_optind() {
	return optind;
}

int Fetchopt_t::next() {
	optarg = NULL;
	if (optind >= ac || av[optind][0] != '-') {
		return -1;
	}
	int optchar = av[optind][pos];
	const char *offset = strchr(optstr, optchar);
	if (offset == NULL || optchar == ':') {
		// Invalid option
		return '?';
	}
	if (*(offset+1) == ':') {
		// Option with argument
		if (av[optind][pos+1] == '\0') {
			// Use next argument for option's argument
			if (ac < optind+2) {
				// Missing argument
				return '?';
			} else {
				optarg = av[optind+1];
				optind += 2;
			}
		} else {
			// Use rest of current argument for option's argument
			optarg = av[optind]+pos+1;
			optind++;
		}
		pos = 1;
		return optchar;
	} else {
		// Simple option
		pos++;
		if (av[optind][pos] == '\0') {
			// Next argument
			pos = 1;
			optind++;
		}
		return optchar;
	}
}
