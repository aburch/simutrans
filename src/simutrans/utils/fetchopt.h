/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef UTILS_FETCHOPT_H
#define UTILS_FETCHOPT_H


/**
This class can be used to parse unix-style single character
command line options. It is intended to provide a similar interface to
the POSIX getopt() function.

The constructor takes the number of arguments (typically the argc value),
the array of arguments and an options string describing the options that
should be searched for.

The value of the options string is constructed from the single-character
options flags you wish to search for. A colon ':' following the option
indicates that the option should take an argument. It is an error if an
option is indicated as having an argument and no argument is found.

E.g. the options string "a:bc" indicates that three flags should be
searched for: -a (which takes an argument), -b and -c (which do not).

The next() method will return the character representing the next option
found. When no further options are found it will return -1. If an error
condition is encountered the character '?' is returned.

The get_optarg() method returns the argument value of the current option
(or NULL if the current option does not take an argument).

When all option arguments have been parsed the value of get_optind()
will be the index into the argv array of the next non-option argument.

A return value of '?' indicates an error, program usage ought to be
printed if this is encountered.

Example:

// Parsing the following command line:
//   mycommand -a override -bc somecommand

Fetchopt_t fetchopt(argc, argv, "a:bc");

const char default_arg_a[] = "default";
char *arg_a = default_arg_a;
bool flag_b = true;
bool flag_c = false;

int ch;
while ((ch = fetchopt.next()) != -1) {
        switch (ch) {
                case 'a':
                        arg_a = fetchopt.get_optarg();
                        break;
                case 'b':
                        flag_b = false;
                        break;
                case 'c':
                        flag_c = true;
                        break;
                case '?':
                case 'h':
                default:
                        usage();
        }
}

// Values will be:
//   arg_a                       = "override"
//   flag_b                      = false
//   flag_c i                    = true
//   fetchopt.get_optind()       = 4
//   argv[fetchopt.get_optind()] = "somecommand"

*/
class Fetchopt_t {
	const char *optstr; // Options definition string
	int ac;             // argc
	char **av;          // argv
	char *optarg;       // Current option's argument string
	int optind;         // Index of current option in argv
	int pos;            // Position of option within current argument

public:
	Fetchopt_t(int, char **, const char *optstring);
	char *get_optarg();
	int get_optind();
	int next();
};

#endif
