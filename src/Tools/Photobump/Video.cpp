#include "StdAfx.h"
#include "video.h"
#include "photoimage.h"
#include <direct.h>

#pragma comment(lib, "OPENGL32.LIB")
#pragma comment(lib, "GLU32.LIB")

//////////////////////////////////////////////////////////////////////////
CVideo::CVideo(CUtils *pUtils)
{
	m_Width=0;
	m_Height=0;		
	m_pUtils=pUtils;	
}

//////////////////////////////////////////////////////////////////////////
CVideo::~CVideo(void)
{
	SAFE_DELETE(m_pDefaultFont);
}

//////////////////////////////////////////////////////////////////////////
void CVideo::InitGL()
{
	m_szSysDir[0]=0;
	_getcwd(m_szSysDir,sizeof(m_szSysDir));		

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

  glEnable(GL_CULL_FACE);	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	//glClearColor(0.2f,0.2f,0.2f,1);
	glClearColor(0,0,0,1);

	//m_pDefaultTexture=LoadTexture(DEFAULT_TEXTURE_FILE,CGTEXT_FLAG_NOMIPMAP|CGTEXT_FLAG_CLAMP);
	m_pDefaultFont=LoadTexture(DEFAULT_FONT_FILE,CGTEXT_FLAG_NOMIPMAP|CGTEXT_FLAG_CLAMP);

	SetPolygonMode(eSolid);	

	//ftype fParams[4];
	//glGetFloatv(GL_CURRENT_RASTER_POSITION,fParams);
	
	//m_RefCamera.m_fSpeed=DEFAULT_CAMERA_SPEED;
}

//////////////////////////////////////////////////////////////////////////
void CVideo::SetPolygonMode(ePolygonMode eMode)
{
	switch(eMode) 
	{
	case eWireFrame:
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
		m_CurrMode=eWireFrame;
		break;
	case eSolid:
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
		m_CurrMode=eSolid;
		break;	
	}
}

//////////////////////////////////////////////////////////////////////////
void CVideo::BeginFrame()
{
	m_nFrameId=m_pUtils->GetFrameNumber();

	// Clear the color and depth buffers 
	//glClearColor(0,0,1,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	//ResetRenderSurfaces();	
}

//////////////////////////////////////////////////////////////////////////
void CVideo::OnSize(int x1,int y1,int x2,int y2)
{
  //
  // Set up the mapping of 3-space to screen space
  //
  //GLdouble gldAspect = (GLdouble) (x2-x1)/ (GLdouble) (y2-y1);
  //glMatrixMode(GL_PROJECTION); //OutputGlError("MatrixMode") ;
  //glLoadIdentity();	
	
	//gluPerspective(cam.GetFov()/(gf_PI/180.0)*cam.GetProjRatio(), 1.0/cam.GetProjRatio(), cam.GetZMin(), cam.GetZMax());    
  //gluPerspective(30.0, gldAspect, 1.0, 10.0);
	
  glViewport(x1, y1, x2, y2);	
	m_Width=x2-x1;
	m_Height=y2-y1;

	// reset the camera
	// NO! must be done from outside otherwise we keep diffrent cameras
	//m_Camera.Init(m_Width,m_Height);	
	//SetCamera(&m_Camera);

	//glMatrixMode(GL_MODELVIEW);
}


//////////////////////////////////////////////////////////////////////////
void CVideo::WriteXY(CPhotoImage *pFont,int x,int y,ftype fScale,const char *szMessage, ...)
{	
	//return;

	va_list	arglist;
	char		szBuf[512];		

	if (!pFont)	
		pFont=m_pDefaultFont;	

	if (!szMessage || (!pFont))
		return; //no messgae or no font set yet
		
	va_start(arglist, szMessage);
	vsprintf(szBuf, szMessage, arglist);
	va_end(arglist);

	screenMessage tempMessage;
	//tempMessage.pFont=pFont;
	tempMessage.x=x;
	tempMessage.y=y;
	strcpy(tempMessage.szMessage,szBuf);
	tempMessage.fScale=fScale;

	tempMessage.b2d=true;

	m_lstMessages.push_back(tempMessage);
}

//////////////////////////////////////////////////////////////////////////
void CVideo::SetCurrentTexture(CPhotoImage *pText)
{
	if (pText)
	{	
		glBindTexture(GL_TEXTURE_2D,pText->m_nID);		
	}
}

//////////////////////////////////////////////////////////////////////////
void CVideo::PrintFast(int x, int y, ftype fScale, const char *szBuf)
{

  x=(int)(ScaleCoordX((ftype)(x)));
  y=(int)(ScaleCoordY((ftype)(y)));

  int xsize=(int)(CHAR_SIZE*fScale*(ftype)(m_Width)/800.0)/2;
  int ysize=(int)(CHAR_SIZE*fScale*(ftype)(m_Height)/600.0);
	
	glBegin(GL_QUADS); 
	//MyOutputDebugString("GLBEGIN2\n");
	//for (int i = (int)strlen(szBuf); i; i--, x += CHAR_SIZE/2, szBuf++)
	for (int i = (int)strlen(szBuf); i; i--, x += xsize, szBuf++)
	{
		// keep in range 0-1 for rendering...
		ftype xot = (ftype)((*szBuf & 15)*CHAR_SIZE)/(ftype)FONT_SIZE;     
		ftype yot = (ftype)((*szBuf >> 4)*CHAR_SIZE)/(ftype)FONT_SIZE;     

		/*
		glTexCoord2f(xot, yot);
		glVertex2i  (x,y+ysize);
		glTexCoord2f(xot+CHAR_INC, yot);		
		glVertex2i(x+xsize, y+ysize);
		glTexCoord2f(xot+CHAR_INC, yot+CHAR_INC);		
		glVertex2i(x+xsize, y);		
		glTexCoord2f(xot, yot+CHAR_INC);    		
		glVertex2i(x, y);		
		*/
		
		glTexCoord2d(xot, yot);
		glVertex2i(x, y);
		glTexCoord2d(xot+CHAR_INC, yot);		
		glVertex2i(x+xsize, y);
		glTexCoord2d(xot+CHAR_INC, yot+CHAR_INC);		
		glVertex2i(x+xsize, y+ysize);
		glTexCoord2d(xot, yot+CHAR_INC);    		
		glVertex2i  (x,y+ysize);
		
	}
	glEnd();             
	//MyOutputDebugString("GLEND2\n");
}

//////////////////////////////////////////////////////////////////////////
void CVideo::Draw2dImage(float xpos,float ypos,float w,float h,CPhotoImage *pImage,float s0,float t0,float s1,float t1)
{ 
	if (!pImage)
		return;

  xpos=(float)ScaleCoordX((ftype)(xpos));
  ypos=(float)ScaleCoordY(ypos)-1.0f;
	w=(float)ScaleCoordX(w)+1.0f;
	h=((float)ScaleCoordY(h))+2.0f;

	GLboolean bRestoreZTest=GL_FALSE;
	glGetBooleanv(GL_DEPTH_TEST,&bRestoreZTest);
  
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();	
  glOrtho(0.0, m_Width, m_Height, 0.0, -9999.0, 9999.0);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  //glRotatef(angle,0,0,1);
    
	SetCurrentTexture(pImage);  

  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);  

  glBegin(GL_QUADS);
  
	float r=1,g=1,b=1,a=1;

  glColor4f(r,g,b,a);
  glTexCoord2f(s0, t0);  
  glVertex2f((xpos), (ypos));

  glColor4f(r,g,b,a);
  glTexCoord2f(s1, t0);  
  glVertex2f((xpos+w), (ypos));

  glColor4f(r,g,b,a);
  glTexCoord2f(s1, t1);  
  glVertex2f((xpos+w),(ypos+h));

  glColor4f(r,g,b,a);
  glTexCoord2f(s0, t1);  
  glVertex2f((xpos), (ypos+h));

  glEnd();  
  

  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  glEnable(GL_CULL_FACE);
	if (bRestoreZTest)
		glEnable(GL_DEPTH_TEST);
 
  glDisable(GL_BLEND);

  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL); 
}

/////////////////////////////////////////////////////////////////
CPhotoImage *CVideo::LoadTexture(const char *szFilename,unsigned int dwFlags,const char *szExportName)
{
	TextureMapIt i=m_lstTextures.find(szFilename);

	if (i!=m_lstTextures.end())
	{
		return (i->second);
	}	

	CPhotoImage *pText=new CPhotoImage(this);

	if (pText->Load(szFilename,dwFlags,szExportName))
	{
		m_lstTextures[pText->GetName()]=pText;
		return (pText);
	}

	delete pText;

	return (NULL);
}

//////////////////////////////////////////////////////////////////////////
void CVideo::RemoveTexture(CPhotoImage *pText)
{
	if (!pText)
		return;

	TextureMapIt i=m_lstTextures.find(pText->GetName());

	if (i!=m_lstTextures.end())
	{
		m_lstTextures.erase(i);
	}	
		
}
