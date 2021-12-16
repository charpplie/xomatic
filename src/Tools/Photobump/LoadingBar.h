
#pragma once

//////////////////////////////////////////////////////////////////////////
#define LOADING_BAR_TEXTURE	"2d\\LoadingBar.bmp"

//////////////////////////////////////////////////////////////////////////
class CLoadingBar
{
public:
	CLoadingBar(CVideo *pVideo,CPaintDC *dc,const char *szName,const char *szTexture=LOADING_BAR_TEXTURE);
	virtual ~CLoadingBar(void);
	
	void	Tick(ftype fPercent);
	void	Set(ftype fPercent);

private:

	void	Draw();		
	CPhotoImage	*m_pLoadingBar;
	CVideo			*m_pVideo;
	char				m_szName[512];
	ftype				m_fCurrent;
	CPaintDC		*m_dc;
};
