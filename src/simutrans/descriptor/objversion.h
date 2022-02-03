/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_OBJVERSION_H
#define DESCRIPTOR_OBJVERSION_H


#include "../simtypes.h"

#define COMPILER_VERSION      "0.1.3exp"
#define COMPILER_VERSION_CODE_11 (0 * 1000000 + 1 * 1000 + 1)
#define COMPILER_VERSION_CODE (0 * 1000000 + 1 * 1000 + 3)

/*
 * obj_type value are stored inside the pak-files. Values are choosen to make
 * them somewhat readable (up to 4 uppercase letters describing the type).
 * obj as 4 byte: 3073094 bytes total
 * obj as 2 byte: 3063046 bytes total
 * obj as 1 byte: 3058022 bytes total
 * saves 4 to 1:  15072 bytes = 0,5% not worth it
 */
#define C4ID(a, b ,c ,d) (((uint32)a) | ((uint32)b) << 8 | ((uint32)c) << 16 | ((uint32)d) << 24)


enum obj_type
{
	obj_bridge      = C4ID('B','R','D','G'),
	obj_building    = C4ID('B','U','I','L'),
	obj_citycar     = C4ID('C','C','A','R'),
	obj_crossing    = C4ID('C','R','S','S'),
	obj_cursor      = C4ID('C','U','R','S'),
	obj_factory     = C4ID('F','A','C','T'),
	obj_ffield      = C4ID('F','F','I','E'),
	obj_ffldclass   = C4ID('F','F','C','L'),
	obj_field       = C4ID('F','I','E','L'),
	obj_fproduct    = C4ID('F','P','R','O'),
	obj_fsmoke      = C4ID('F','S','M','O'),
	obj_fsupplier   = C4ID('F','S','U','P'),
	obj_good        = C4ID('G','O','O','D'),
	obj_ground      = C4ID('G','R','N','D'),
	obj_groundobj   = C4ID('G','O','B','J'),
	obj_image       = C4ID('I','M','G', 0 ),
	obj_imagelist   = C4ID('I','M','G','1'),
	obj_imagelist2d = C4ID('I','M','G','2'),
	obj_menu        = C4ID('M','E','N','U'),
	obj_miscimages  = C4ID('M','I','S','C'),
	obj_pedestrian  = C4ID('P','A','S','S'),
	obj_roadsign    = C4ID('S','I','G','N'),
	obj_root        = C4ID('R','O','O','T'),
	obj_smoke       = C4ID('S','M','O','K'),
	obj_sound       = C4ID('S','O','U','N'),
	obj_symbol      = C4ID('S','Y','M','B'),
	obj_text        = C4ID('T','E','X','T'),
	obj_tile        = C4ID('T','I','L','E'),
	obj_tree        = C4ID('T','R','E','E'),
	obj_tunnel      = C4ID('T','U','N','L'),
	obj_vehicle     = C4ID('V','H','C','L'),
	obj_way         = C4ID('W','A','Y', 0 ),
	obj_way_obj     = C4ID('W','Y','O','B'),
	obj_xref        = C4ID('X','R','E','F')
};

#endif
