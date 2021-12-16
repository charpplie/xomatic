////////////////////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// ----------------------------------------------------------------------------------------
//  File name:   GameBind
//  Description: 
//
//////////////////////////////////////////////////////////////////////////////////////////// 
#include "StdAfx.h"
#include "GameBind.h"

#include <IEditor.h>
#include <IGame.h>
#include <IGameFramework.h>
#include <Descriptor/IFramework.h>
#include <Descriptor/ITypeLibrary.h>
#include <Descriptor/IDatabase.h>

GameBind* GameBind::s_pThis = NULL;

GameBind::GameBind()
	: m_pDescriptorLibrary(NULL)
	, m_pDatabase(NULL)
	, m_pEditor(NULL)
{
	assert(s_pThis == NULL);
	s_pThis = this;
}

GameBind::~GameBind()
{
	assert(s_pThis != NULL);
	s_pThis = NULL;
}

bool GameBind::Init( IEditor* pEditor )
{
	CRY_ASSERT(pEditor);

	m_pEditor = pEditor;

	SSystemGlobalEnvironment* pGlobalEnvironment = pEditor->GetSystem()->GetGlobalEnvironment();
	if(pGlobalEnvironment != NULL)
	{
		Descriptor::IFramework* pFramework = pGlobalEnvironment->pGame->GetIGameFramework()->QueryExtension<Descriptor::IFramework>();
		if(pFramework != NULL)
		{
			m_pDescriptorLibrary = &pFramework->GetTypeLibrary();
			m_pDatabase = pFramework->GetDatabase().GetEditorInterface();
		}
	}

	return (m_pDatabase != NULL) && (m_pDescriptorLibrary != NULL);
}

ISourceControl* GameBind::GetSourceControl() const
{
	return m_pEditor->GetSourceControl();
}