#include "stdafx.h"
#include "EnvironmentProbePanel.h"
#include "Objects/EnvironmentProbeObject.h"
#include "Util/CubemapUtils.h"
/*

  CEnvironmentProbePanel

*/

IMPLEMENT_DYNAMIC(CEnvironmentProbePanel, CDialog)

BEGIN_MESSAGE_MAP(CEnvironmentProbePanel, CDialog)
	ON_BN_CLICKED(IDC_GENERATECUBEMAPS_BUTTON, OnGenerateCubemap)
	ON_BN_CLICKED(IDC_GENERATE_ALL_CUBEMAPS_BUTTON, OnGenerateAllCubemaps)
END_MESSAGE_MAP()

//

BOOL CEnvironmentProbePanel::OnInitDialog()
{
	__super::OnInitDialog();
	return true;
}

void CEnvironmentProbePanel::SetEntity(CEnvironementProbeObject *pEntity)
{
	m_pEntity = pEntity;
}

void CEnvironmentProbePanel::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_GENERATECUBEMAPS_BUTTON,	m_GenerateCubemapsButton);
	DDX_Control(pDX, IDC_GENERATE_ALL_CUBEMAPS_BUTTON,	m_GenerateAllCubemapsButton);
}

//function will generate the cubemap for this probes position
void CEnvironmentProbePanel::OnGenerateCubemap()
{
	m_pEntity->GenerateCubemap();
}

//function will recurse all probes and generate a cubemap for each
void CEnvironmentProbePanel::OnGenerateAllCubemaps()
{
	CubemapUtils::RegenerateAllEnvironmentProbeCubemaps();
}