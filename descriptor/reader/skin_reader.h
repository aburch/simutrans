/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_SKIN_READER_H
#define DESCRIPTOR_READER_SKIN_READER_H


#include "../../simskin.h"
#include "../../tpl/slist_tpl.h"

#include "obj_reader.h"


class skin_reader_t : public obj_reader_t
{
public:
	/// @copydoc obj_reader_t::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;

protected:
	/// @copydoc obj_reader_t::register_obj
	void register_obj(obj_desc_t *&desc) OVERRIDE;

	/// @copydoc obj_reader_t::successfully_loaded
	bool successfully_loaded() const OVERRIDE;

	/// @returns type of skin this reader is able to read
	virtual skinverwaltung_t::skintyp_t get_skintype() const = 0;
};


class menuskin_reader_t : public skin_reader_t
{
	OBJ_READER_DEF(menuskin_reader_t, obj_menu, "menu");

protected:
	/// @copydoc skin_reader_t::get_skintype
	skinverwaltung_t::skintyp_t get_skintype() const OVERRIDE { return skinverwaltung_t::menu; }
};


class cursorskin_reader_t : public skin_reader_t
{
	OBJ_READER_DEF(cursorskin_reader_t, obj_cursor, "cursor");

protected:
	/// @copydoc skin_reader_t::get_skintype
	skinverwaltung_t::skintyp_t get_skintype() const OVERRIDE { return skinverwaltung_t::cursor; }
};


class symbolskin_reader_t : public skin_reader_t
{
	OBJ_READER_DEF(symbolskin_reader_t, obj_symbol, "symbol");

protected:
	/// @copydoc skin_reader_t::get_skintype
	skinverwaltung_t::skintyp_t get_skintype() const OVERRIDE { return skinverwaltung_t::symbol; }
};


class fieldskin_reader_t : public skin_reader_t
{
	OBJ_READER_DEF(fieldskin_reader_t, obj_field, "field");

protected:
	/// @copydoc skin_reader_t::get_skintype
	skinverwaltung_t::skintyp_t get_skintype() const OVERRIDE { return skinverwaltung_t::nothing; }
};


class smoke_reader_t : public skin_reader_t
{
	OBJ_READER_DEF(smoke_reader_t, obj_smoke, "smoke");

protected:
	/// @copydoc skin_reader_t::get_skintype
	skinverwaltung_t::skintyp_t get_skintype() const OVERRIDE { return skinverwaltung_t::nothing; }
};


class miscimages_reader_t : public skin_reader_t
{
	OBJ_READER_DEF(miscimages_reader_t, obj_miscimages, "misc");

protected:
	/// @copydoc skin_reader_t::get_skintype
	skinverwaltung_t::skintyp_t get_skintype() const OVERRIDE { return skinverwaltung_t::misc; }
};


#endif
