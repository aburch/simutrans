/*
 * koord3d.cc
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdlib.h>
#include <stdio.h>

#include "../simtypes.h"
#include "koord3d.h"
#include "../mm/mempool.h"
#include "../dataobj/loadsave.h"

mempool_t * koord3d::mempool = new mempool_t(sizeof(koord3d));

const koord3d koord3d::invalid(-1, -1, -1);


void *
koord3d::operator new(size_t /*s*/)
{
//    printf("new koord\n");

    return mempool->alloc();
}


void
koord3d::operator delete(void *p)
{
//    printf("delete koord\n");

    mempool->free( p );
}


void
koord3d::rdwr(loadsave_t *file)
{
  short v;

  v = x;
  file->rdwr_short(v, " ");
  x = v;

  v = y;
  file->rdwr_short(v, " ");
  y = v;

  v = z;
  file->rdwr_short(v, "\n");
  z = v;
}

koord3d::koord3d(loadsave_t *file)
{
    rdwr(file);
}
