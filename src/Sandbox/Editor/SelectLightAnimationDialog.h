//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File Name        : SelectLightAnimationDialog.h
//  Author           : Jaewon Jung
//  Time of creation : 10/27/2011   15:35
//  Compilers        : VS2010
//  Description      : Used in a property item to select a light animation
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
#ifndef __SELECTLIGHTANIMATIONDIALOG_H__
#define __SELECTLIGHTANIMATIONDIALOG_H__
#pragma once

#include "GenericSelectItemDialog.h"

class CSelectLightAnimationDialog : public CGenericSelectItemDialog
{
	DECLARE_DYNAMIC(CSelectLightAnimationDialog)
	CSelectLightAnimationDialog(CWnd* pParent = NULL);

protected:
	virtual BOOL OnInitDialog();

	virtual void GetItems(std::vector<SItem>& outItems);
};

#endif // __SELECTLIGHTANIMATIONDIALOG_H__
