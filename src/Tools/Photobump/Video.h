#pragma once

//////////////////////////////////////////////////////////////////////////	
#define	FONT_SIZE 256
#define CHAR_SIZE 16
#define CHAR_INC ((ftype)CHAR_SIZE/(ftype)FONT_SIZE)
#define DEFAULT_FONT_FILE				"2d\\default2.tga" 
#define DEFAULT_TEXTURE_FILE		"2d\\Default.tga" 

//////////////////////////////////////////////////////////////////////////	
#define CGTEXT_FLAG_KEEP				1	//	keep in system memory the texture loaded 
#define CGTEXT_FLAG_DONOTUPLOAD	2	//	do not upload the texture in video memory
#define CGTEXT_FLAG_CLAMP				4 //	clamp the texture to 0-1, by default the texture wraps
#define CGTEXT_FLAG_NOMIPMAP		8 //	do not create mipmaps for this texture (fonts, hud)
#define CGTEXT_FLAG_FLIPX				16
#define CGTEXT_FLAG_FLIPY				32
#define CGTEXT_USE_WORKING_DIR	64

//////////////////////////////////////////////////////////////////////////
class CPhotoImage;

//////////////////////////////////////////////////////////////////////////
typedef struct s_Message
{
	char	szMessage[256];
	CPhotoImage	*pFont;
	int		x,y;
	vector3f	vPos;
	bool	b2d;
	ftype fScale;
} screenMessage;

//////////////////////////////////////////////////////////////////////////
typedef std::vector<screenMessage> MessageVec;
typedef std::vector<screenMessage>::iterator MessageVecIt;

//////////////////////////////////////////////////////////////////////////
typedef std::map<std::string,CPhotoImage*,string_nocase_lt> TextureMap;
typedef TextureMap::iterator TextureMapIt;

class CUtils;
class CPhotoImage;

//////////////////////////////////////////////////////////////////////////
typedef enum 
{
	eWireFrame=0,
	eSolid=1
}ePolygonMode;

//////////////////////////////////////////////////////////////////////////
class CVideo
{
public:
	CVideo(CUtils *pUtils);
	virtual ~CVideo(void);

	void		InitGL();
	void		OnSize(int x1,int y1,int x2,int y2);
	
	ftype		ScaleCoordX(ftype value) { value*=ftype(m_Width)/800.0;return (value); }		
	ftype		ScaleCoordY(ftype value) { value*=ftype(m_Height)/600.0;return (value); }

	void		SetCamera(CCamera *pCam,bool bUseGLMatrix,bool bUseGLViewport);	
	void		BeginFrame();
	void		SwapBuffers(HDC pdc);
	void		Reset();
	void		PrintFast(int x, int y, ftype fScale, const char *szBuf);
	void		WriteXY(CPhotoImage *pFont,int x,int y,ftype fScale,const char *szMessage, ...);	

	void		SetPolygonMode(ePolygonMode eMode);
	CPhotoImage	*LoadTexture(const char *szFilename,unsigned int dwFlags=0,const char *szExportName=NULL);
	void		SetCurrentTexture(CPhotoImage *pText);
	void		RemoveTexture(CPhotoImage *pText);

	void		ProjectToScreen(const vector3f &vSource,vector3f &vDest,CCamera *pCam,bool bCache=true);
	void		UnProjectFromScreen(const vector3f &vSource,vector3f &vDest,CCamera *pCam,bool bCache=true);
	const		char *GetWorkingDir() { return(m_szSysDir); }

	//////////////////////////////////////////////////////////////////////////
	// debug
	void	Draw2dImage(float xpos,float ypos,float w,float h,CPhotoImage *pImage,float s0,float t0,float s1,float t1);
	void	DrawOrientedArrow(vector3f vPos,vector3f vNormal,ftype fSize,const vector3f &vColor);
	void	DrawWorldOrigin(ftype fGroundSize,ftype fGridSize);
	void	Draw3DBBox(const vector3f &vMins,const vector3f &vMaxs);
	void	CheckError(const char *szComment);
	void	DrawCamera(const CCamera &cam);
	void	DrawBall(const vector3f &vPos,const vector3f &vColor,ftype fRadius);

	//////////////////////////////////////////////////////////////////////////
	
	int	m_Width;
	int	m_Height;

private:

	const char *ErrorString(int errorCode);	
	void				MyPerspective(double fovx, double aspect, double zNear, double zFar);
	/*
	int					MyProject(double objx, double objy, double objz,
	   const double model[16], const double proj[16],const int viewport[4],
	   double * winx, double * winy, double * winz);

	int	MyUnproject(double winx, double winy, double winz,
	     const double model[16], const double proj[16],
	     const int viewport[4],
			 double nearZ, double farZ,
	     double * objx, double * objy, double * objz);
	*/

	int					MyProject(double objx, double objy, double objz,
	   CCamera	*pCam,
	   double * winx, double * winy, double * winz,bool bCache);

	int	MyUnproject(double winx, double winy, double winz,
		CCamera	*pCam,
		double * objx, double * objy, double * objz,bool bCache);

	CPhotoImage	*m_pDefaultFont;

	MessageVec	m_lstMessages;
	TextureMap	m_lstTextures;
	CCamera			m_Camera;	
	AABBVec			m_lstBBoxes;	
	int					m_nViewport[4];
	int					m_nFrameId;
	CUtils				*m_pUtils;
	ePolygonMode	m_CurrMode;

	char				m_szSysDir[512];

};
