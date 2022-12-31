/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simutrans/simversion.h"

#include <cstdio>


int main()
{
  return puts(VERSION_NUMBER) < 0;
}
