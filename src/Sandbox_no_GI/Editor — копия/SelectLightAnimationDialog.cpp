//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File Name        : SelectLightAnimationDialog.cpp
//  Author           : Jaewon Jung
//  Time of creation : 10/27/2011   15:35
//  Compilers        : VS2010
//  Description      : Used in a property item to select a light animation
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "SelectLightAnimationDialog.h"
#include <IMovieSystem.h>

IMPLEMENT_DYNAMIC(CSelectLightAnimationDialog, CGenericSelectItemDialog)

//////////////////////////////////////////////////////////////////////////
CSelectLightAnimationDialog::CSelectLightAnimationDialog(CWnd* pParent) :	CGenericSelectItemDialog(pParent)
{
	m_dialogID = "Dialogs\\SelectLightAnimation";
}

//////////////////////////////////////////////////////////////////////////
BOOL CSelectLightAnimationDialog::OnInitDialog()
{
	SetTitle(_T("Select Light Animation"));
	SetMode(eMODE_LIST);
	return __super::OnInitDialog();
}

//////////////////////////////////////////////////////////////////////////
void CSelectLightAnimationDialog::GetItems(std::vector<SItem>& outItems)
{
	IMovieSystem *pMovieSystem = GetIEditor()->GetMovieSystem();
	for(int i=0; i<pMovieSystem->GetNumSequences(); ++i)
	{
		IAnimSequence *pSequence = pMovieSystem->GetSequence(i);
		if((pSequence->GetFlags() & IAnimSequence::LIGHT_ANIMATION_SET) == 0)
			continue;

		for(int k=0; k<pSequence->GetNodeCount(); ++k)
		{
			assert(pSequence->GetNode(k)->GetType() == ANODE_LIGHT);
			if(pSequence->GetNode(k)->GetType() != ANODE_LIGHT)
				continue;
			SItem item;
			item.name = pSequence->GetNode(k)->GetName();
			outItems.push_back(item);
		}
		return;
	}
}
