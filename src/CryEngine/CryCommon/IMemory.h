/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2009.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: 
  
 -------------------------------------------------------------------------
  History:
  - 11:5:2009   : Created by Andrey Honich

*************************************************************************/
#include DEVIRTUALIZE_HEADER_FIX(IMemory.h)

#ifndef __IMEMORY_H__
#define __IMEMORY_H__

#if _MSC_VER > 1000
#	pragma once
#endif


typedef _i_reference_target<int> _i_reference_target_t;

struct IMemoryBlock : public _i_reference_target_t
{
  virtual void * GetData() = 0;
  virtual int GetSize() = 0;
};


#endif //__IMEMORY_H__