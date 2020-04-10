/****************************************************************************/
//
// Dyncreate.cpp
//
// Support for polymorphic dynamic object creation under RTTI
//
// Copyright (C) 1997-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

/*----------------------------------------------------------------------------
// INCLUDE FILES
-----------------------------------------------------------------------------*/
#include "stdafx.h"

using namespace JTI_Util;

/*----------------------------------------------------------------------------
//	GLOBALS
-----------------------------------------------------------------------------*/
IRttiFactory* IRttiFactory::headPtr_ = NULL;
