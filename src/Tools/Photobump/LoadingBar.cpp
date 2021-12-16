#include "StdAfx.h"
#include "loadingbar.h"
#include "Video.h"
#include "PhotoImage.h"

//////////////////////////////////////////////////////////////////////////
CLoadingBar::CLoadingBar(CVideo *pVideo,CPaintDC *dc,const char *szName,const char *szTexture)
{
	m_pVideo=pVideo;
	strcpy(m_szName,szName);
	m_pLoadingBar=m_pVideo->LoadTexture(szTexture,CGTEXT_USE_WORKING_DIR);	
	if (!m_pLoadingBar)
		m_pLoadingBar=m_pVideo->LoadTexture(DEFAULT_TEXTURE_FILE);	
	m_fCurrent=0;
	m_dc=dc;
	Draw();
}

//////////////////////////////////////////////////////////////////////////
CLoadingBar::~CLoadingBar(void)
{
	m_pVideo->RemoveTexture(m_pLoadingBar);
}

//////////////////////////////////////////////////////////////////////////
void CLoadingBar::Set(ftype fPercent)
{
	m_fCurrent=fPercent;
	Draw();
}

//////////////////////////////////////////////////////////////////////////
void CLoadingBar::Tick(ftype fPercent)
{
	m_fCurrent+=fPercent;
	Draw();
}

//////////////////////////////////////////////////////////////////////////
void CLoadingBar::Draw()
{
	int nLen=CHAR_SIZE*((int)strlen(m_szName)+8);
	int nX=30; //800/2-(int)m_pVideo->ScaleCoordX((ftype)(nLen/2));
	int nY=200;

	// clear screen
	m_pVideo->BeginFrame();

	m_pVideo->WriteXY(NULL,nX,nY,1.2,"%s (%0.2f percent)",m_szName,m_fCurrent);

	float fX1=(float)(30);
	float fY1=(float)(nY+64);	
	float fX2=(float)(m_pLoadingBar->m_nGLWidth*m_fCurrent/100.0);
	float fY2=(float)(m_pLoadingBar->m_nGLHeight/2);	

	m_pVideo->SetCurrentTexture(m_pLoadingBar);

	m_pVideo->Draw2dImage(fX1,fY1,fX2,fY2,m_pLoadingBar,0,0,(float)m_fCurrent/100.0f,1.0f);

	m_pVideo->SwapBuffers(m_dc->m_ps.hdc);	
}
