// ParticleEffectPanel.cpp : implementation file
//

#include "StdAfx.h"
#include "ParticleEffectPanel.h"
#include "Objects\ParticleEffectObject.h"

CParticleEffectPanel::CParticleEffectPanel(CWnd* pParent /*=NULL*/)
: CXTResizeDialog(CParticleEffectPanel::IDD, pParent)
{
}


void CParticleEffectPanel::DoDataExchange(CDataExchange* pDX)
{
	CXTResizeDialog::DoDataExchange(pDX);

	//DDX_Control(pDX, IDC_REMOVE, m_removeButton);
}


BEGIN_MESSAGE_MAP(CParticleEffectPanel, CXTResizeDialog)
	ON_BN_CLICKED(IDC_GOTODATABASE, &CParticleEffectPanel::OnBnClickedGotodatabase)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
BOOL CParticleEffectPanel::OnInitDialog() 
{
	CXTResizeDialog::OnInitDialog();

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CParticleEffectPanel message handlers
void CParticleEffectPanel::SetParticleEffectEntity( CParticleEffectObject *entity )
{
	assert( entity );
	m_pEntity = entity;
}

void CParticleEffectPanel::OnBnClickedGotodatabase()
{
	if( m_pEntity )
		m_pEntity->OnMenuGoToDatabase();	
}