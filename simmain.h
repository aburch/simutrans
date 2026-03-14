/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMMAIN_H
#define SIMMAIN_H

 // search for this in all possible pakset locations for the directory chkdir and take the first match
bool set_pakdir(const char* chkdir);

int simu_main(int argc, char** argv);

#endif
