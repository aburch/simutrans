//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  obj_pak_exception.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: obj_pak_exception.h  $       $Author: hajo $
//  $Revision: 1.1 $         $Date: 2002/09/18 19:13:22 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: obj_pak_exception.h,v $
//  Revision 1.1  2002/09/18 19:13:22  hajo
//  Volker: new config system
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __OBJ_PAK_EXCEPTION_H
#define __OBJ_PAK_EXCEPTION_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      obj_pak_exception_t()
//
//---------------------------------------------------------------------------
//  Description:
//       ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class obj_pak_exception_t {
    cstring_t   classname;
    cstring_t   problem;
public:
    obj_pak_exception_t(const char *clsname, const char *prob)
    {
	classname = clsname;
	problem = prob;
    }
    const cstring_t &get_class()
    {
	return classname;
    }
    const cstring_t &get_info()
    {
	return problem;
    }
};

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
#endif // __OBJ_PAK_EXCEPTION_H
