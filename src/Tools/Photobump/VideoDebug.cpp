

#include "stdafx.h"
#include "PhotoImage.h"
#include "Video.h"

//////////////////////////////////////////////////////////////////////////
void CVideo::SwapBuffers(HDC pdc)
{

	//DrawWorldOrigin(200,10);

	//////////////////////////////////////////////////////////////////////////
	// BBOXES
	//////////////////////////////////////////////////////////////////////////
	if (!m_lstBBoxes.empty())
	{	

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		//glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glColor4f(1,1,1,1);

		for (AABBVecIt i=m_lstBBoxes.begin();i!=m_lstBBoxes.end();i++)
		{
			CAABBd tAABB=(*i);

			vector3f vMins=tAABB.m_vMins;
			vector3f vMaxs=tAABB.m_vMaxs;

			glBegin(GL_LINE_LOOP);
			glVertex3d(vMins.x,vMins.y,vMins.z); //0
			glVertex3d(vMins.x,vMins.y,vMaxs.z); //1
			glVertex3d(vMaxs.x,vMins.y,vMaxs.z); //2
			glVertex3d(vMaxs.x,vMins.y,vMins.z); //3
			glEnd();

			glBegin(GL_LINE_LOOP);
			glVertex3d(vMins.x,vMins.y,vMins.z); //0
			glVertex3d(vMins.x,vMins.y,vMaxs.z); //1
			glVertex3d(vMins.x,vMaxs.y,vMaxs.z); //6
			glVertex3d(vMins.x,vMaxs.y,vMins.z); //4
			glEnd();

			glBegin(GL_LINE_LOOP);
			glVertex3d(vMins.x,vMaxs.y,vMins.z); //4
			glVertex3d(vMins.x,vMaxs.y,vMaxs.z); //6
			glVertex3d(vMaxs.x,vMaxs.y,vMaxs.z); //7
			glVertex3d(vMaxs.x,vMaxs.y,vMins.z); //5
			glEnd();

			glBegin(GL_LINE_LOOP);
			glVertex3d(vMaxs.x,vMins.y,vMins.z); //3
			glVertex3d(vMaxs.x,vMins.y,vMaxs.z); //2
			glVertex3d(vMaxs.x,vMaxs.y,vMaxs.z); //7
			glVertex3d(vMaxs.x,vMaxs.y,vMins.z); //5
			glEnd();
		
		} //i

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glEnable(GL_TEXTURE_2D);

		m_lstBBoxes.clear();
	}

	if (!m_lstMessages.empty())
	{	
		glDisable(GL_CULL_FACE);
		
		glEnable(GL_TEXTURE_2D);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0.0, m_Width, m_Height, 0.0, -1.0, 1.0); 
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		
		glDisable(GL_DEPTH_TEST);

		for (MessageVecIt i4=m_lstMessages.begin();i4!=m_lstMessages.end();i4++)
		{
			screenMessage tempMessage=(*i4);

			SetCurrentTexture(m_pDefaultFont);			
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);    
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);  

			glColor4f(1,1,1,1); //TODO
			
			PrintFast(tempMessage.x,tempMessage.y,tempMessage.fScale,tempMessage.szMessage);
		}

		
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		m_lstMessages.clear();
	}

	CheckError("EndFrame");

	::SwapBuffers(pdc);	
}

/////////////////////////////////////////////////////////////////
void CVideo::DrawWorldOrigin(ftype fGroundSize,ftype fGridSize)
{

  glDisable(GL_TEXTURE_2D);

	//draw the 3 axis:

	glLineWidth(2.0);
	//X 
	glBegin(GL_LINES);
	glColor3f(1,0,0);glVertex3d(0,0,0);
	glColor3f(1,0,0);glVertex3d(fGroundSize,0,0);
	glEnd();

	//Y
	glBegin(GL_LINES);
	glColor3f(0,1,0);glVertex3d(0,0,0);
	glColor3f(0,1,0);glVertex3d(0,fGroundSize,0);
	glEnd();

	//Z 
	glBegin(GL_LINES);
	glColor3f(0,0,1);glVertex3d(0,0,0);
	glColor3f(0,0,1);glVertex3d(0,0,fGroundSize);
	glEnd();

	glEnable(GL_TEXTURE_2D);	
	
	//return;

	glDisable(GL_TEXTURE_2D);

	glLineWidth(1.0);
	
	glColor4f(1,1,1,1);

	fGroundSize/=2.0;

	// draw the ground plane
	glBegin(GL_LINE_LOOP);
	glVertex3d(-fGroundSize,0,-fGroundSize);
	glVertex3d(fGroundSize,0,-fGroundSize);
	glVertex3d(fGroundSize,0,fGroundSize);
	glVertex3d(-fGroundSize,0,fGroundSize);
	glEnd();

	glBegin(GL_LINES);
	
	ftype fPos=-fGroundSize;
	while (fPos<fGroundSize)	
	{		
		glVertex3d(fPos,0,-fGroundSize);
		glVertex3d(fPos,0,fGroundSize);

		glVertex3d(-fGroundSize,0,fPos);
		glVertex3d(fGroundSize,0,fPos);

		fPos+=fGridSize;
	} //k

	glEnd();

	glEnable(GL_TEXTURE_2D);	
}

//////////////////////////////////////////////////////////////////////////
void CVideo::DrawOrientedArrow(vector3f vPos,vector3f vNormal,ftype fSize,const vector3f &vColor)
{
	vector3f vUp,vRight,vDest;
	MakeNormalVectors(vNormal,vUp,vRight);
	
	vRight*=(fSize); 
	vUp*=(fSize); 

	vNormal=vNormal*fSize;	

	glBegin(GL_LINES);

	glColor3dv(&vColor.x);glVertex3dv(&vPos.x);
	vDest=vPos+vUp-vNormal;
	glColor3dv(&vColor.x);glVertex3dv(&vDest.x);

	glColor3dv(&vColor.x);glVertex3dv(&vPos.x);
	vDest=vPos-vUp-vNormal;
	glColor3dv(&vColor.x);glVertex3dv(&vDest.x);

	glColor3dv(&vColor.x);glVertex3dv(&vPos.x);
	vDest=vPos+vRight-vNormal;
	glColor3dv(&vColor.x);glVertex3dv(&vDest.x);

	glColor3dv(&vColor.x);glVertex3dv(&vPos.x);
	vDest=vPos-vRight-vNormal;
	glColor3dv(&vColor.x);glVertex3dv(&vDest.x);

	glEnd();
}

/////////////////////////////////////////////////////////////////
void CVideo::Draw3DBBox(const vector3f &vMins,const vector3f &vMaxs)
{
	CAABBd tAABB;
	tAABB.m_vMins=vMins;
	tAABB.m_vMaxs=vMaxs;
	m_lstBBoxes.push_back(tAABB);
}

//////////////////////////////////////////////////////////////////////////
void CVideo::DrawBall(const vector3f &vPos,const vector3f &vColor,ftype fRadius)
{
	glDisable(GL_TEXTURE_2D);

	//glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);

	glPushMatrix();		
	glTranslated(vPos.x,vPos.y,vPos.z);
	glColor3d(vColor.x,vColor.y,vColor.z);
	GLUquadricObj * q = gluNewQuadric();
	gluSphere( q, fRadius, 12, 12 );
	gluDeleteQuadric(q);
	glPopMatrix();	

	//SetPolygonMode(m_CurrMode); // restore previous

	glEnable(GL_TEXTURE_2D);
	glColor3f(1,1,1);
}

//////////////////////////////////////////////////////////////////////////
void CVideo::DrawCamera(const CCamera &cam)
{
	vector3f dir, rgt, org,up,rgt2;

	dir = cam.GetForwardVector(); 
	rgt = cam.GetRightVector(); 
	up=cam.GetUpVector();up*=0.15;
	rgt2=rgt;rgt2*=0.30;
	ftype ang = RAD2DEG(cam.m_fFov)/2.0; 
	org = cam.GetPos(); dir *= 0.3;	
	CMatrixf m,m2;m.Identity();
	vector3f vertex = dir;
	m2=m.GetRotation(rgt, ang);
	vertex = m2.TransformPoint(vertex);
	//CMatrixf mat = m.GetRotation(dir, 60);
	CMatrixf mat = m.GetRotation(dir, 30);
	vector3f tmpvertex;
	int ctr;

	// fill the inside of the camera
	glDisable(GL_TEXTURE_2D);        

	// disable, because if we change culling face order
	// from the menu the camera is not drawn correctly
	glDisable(GL_CULL_FACE); 

	// draw the camera box
	vector3f vCenter;

	//glColor3f(0,0,0);
	//saddle brown  Red:[139] Green:[069] Blue[019]  
	glColor3f(139.0f/255.0f/2.0f,69.0f/255.0f/2.0f,19.0f/255.0f/2.0f);

	glBegin(GL_QUADS);

	vector3f dir2=dir*0.1;
	vector3f dir3=dir*0.5;
	// front face	
	vCenter=org+dir2;
	tmpvertex=vCenter-up+rgt2; glVertex3dv(tmpvertex);
	tmpvertex=vCenter+up+rgt2; glVertex3dv(tmpvertex);
	tmpvertex=vCenter+up-rgt2; glVertex3dv(tmpvertex);
	tmpvertex=vCenter-up-rgt2; glVertex3dv(tmpvertex);	
	// back face	
	vCenter=org-dir3;
	tmpvertex=vCenter+up+rgt2; glVertex3dv(tmpvertex);
	tmpvertex=vCenter-up+rgt2; glVertex3dv(tmpvertex);
	tmpvertex=vCenter-up-rgt2; glVertex3dv(tmpvertex);	
	tmpvertex=vCenter+up-rgt2; glVertex3dv(tmpvertex);	
	// right face		
	vCenter=org+rgt2;
	tmpvertex=vCenter+dir2+up; glVertex3dv(tmpvertex);
	tmpvertex=vCenter+dir2-up; glVertex3dv(tmpvertex);
	tmpvertex=vCenter-dir3-up; glVertex3dv(tmpvertex);	
	tmpvertex=vCenter-dir3+up; glVertex3dv(tmpvertex);	
	// left face		
	vCenter=org-rgt2;
	tmpvertex=vCenter+dir2-up; glVertex3dv(tmpvertex);
	tmpvertex=vCenter+dir2+up; glVertex3dv(tmpvertex);	
	tmpvertex=vCenter-dir3+up; glVertex3dv(tmpvertex);	
	tmpvertex=vCenter-dir3-up; glVertex3dv(tmpvertex);		
	// bottom face		
	vCenter=org-up;
	tmpvertex=vCenter+dir2+rgt2; glVertex3dv(tmpvertex);	
	tmpvertex=vCenter+dir2-rgt2; glVertex3dv(tmpvertex);	
	tmpvertex=vCenter-dir3-rgt2; glVertex3dv(tmpvertex);		
	tmpvertex=vCenter-dir3+rgt2; glVertex3dv(tmpvertex);		
	// top face		
	vCenter=org+up;
	tmpvertex=vCenter+dir2-rgt2; glVertex3dv(tmpvertex);
	tmpvertex=vCenter+dir2+rgt2; glVertex3dv(tmpvertex);	
	tmpvertex=vCenter-dir3+rgt2; glVertex3dv(tmpvertex);	
	tmpvertex=vCenter-dir3-rgt2; glVertex3dv(tmpvertex);		
	glEnd();
		
	
	glColor3f(0,0,0);
	glBegin(GL_TRIANGLE_FAN);
	glVertex3dv(&org[0]);
	glColor3f(1,1,1);
	tmpvertex = org + vertex;
	glVertex3dv(&tmpvertex[0]);
	for (ctr=0; ctr<6*2; ctr++)
	{
		vertex = mat.TransformPoint(vertex);
		vector3f tmpvertex = org + vertex;
		glVertex3dv(&tmpvertex.x);
	}
	glEnd();	

	//draw a line in the center of the camera
	glBegin(GL_LINES);
	glVertex3dv(&org[0]);
	glVertex3dv(&(org + dir*2.5f)[0]);
	glEnd();

	
	glEnable(GL_CULL_FACE); // enable like it was before
	glEnable(GL_TEXTURE_2D);
}

/////////////////////////////////////////////////////////////////
const char* CVideo::ErrorString(int errorCode)
{
   // GL Errors 
   if (errorCode==GL_NO_ERROR) {
      return "no error";
   }
   else if (errorCode==GL_INVALID_VALUE) {
      return "invalid value";
   }
   else if (errorCode==GL_INVALID_ENUM) {
      return "invalid enum";
   }
   else if (errorCode==GL_INVALID_OPERATION) {
      return "invalid operation";
   }
   else if (errorCode==GL_STACK_OVERFLOW) {
      return "stack overflow";
   }
   else if (errorCode==GL_STACK_UNDERFLOW) {
      return "stack underflow";
   }
   else if (errorCode==GL_OUT_OF_MEMORY) {
      return "out of memory";
   }
   return NULL;
}

/////////////////////////////////////////////////////////////////
void CVideo::CheckError(const char *szComment)
{
	int nErrNum=0;
	
  GLenum nErr=glGetError();  
  while (nErr!=GL_NO_ERROR)
  {
    const char *pErrText=ErrorString(nErr);
    if (szComment) 
      MyOutputDebugString("<CVideo> glGetError: %s: %s \n",pErrText ? pErrText : "-", szComment);
    else
      MyOutputDebugString("<CVideo> glGetError: %s: \n",pErrText ? pErrText : "-");
    
    nErr=glGetError();

		//Break();

    if (++nErrNum>10)
      break;
  } //nErr!=GL_NO_ERROR
}



