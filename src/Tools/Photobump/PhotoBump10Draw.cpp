
#include "stdafx.h"
#include "PhotoBump10.h"

#include "PhotoBump10Doc.h"
#include "PhotoBump10View.h"
#include "Video.h"
#include "PhotoFrame.h"
#include "PhotoImage.h"
#include "PVertex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//bool bDebug=true;

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10View::DrawScene()
{
	if (m_bDrawing)
		return;

	m_bDrawing=true;

	m_pUtils->UpdateTimer(false);

	CPaintDC dc(this); // device context for painting
	//wglMakeCurrent(this->GetWindowDC()->GetSafeHdc(),GetRC());
	SetGLCurrent();

	m_pVideo->CheckError("BeginFrame");
	m_pVideo->BeginFrame();

	//m_pVideo->SetPolygonMode(m_nPolygonMode);
		
	CPhotoBump10Doc	*pDoc=GetDocument();
	ASSERT_VALID(pDoc);

	// set cameras the first frame
	if (m_bDumpError)
	{	
		for (lstFramesIt i=pDoc->m_lstFrames.begin();i!=pDoc->m_lstFrames.end();i++)
			pDoc->DumpError((*i));
		m_bDumpError=false;
		goto End;
	}

	if (pDoc->m_bLoadData)
	{	
		pDoc->m_bLoadData=false;

		// clear screen
		m_pVideo->BeginFrame();
		m_pVideo->SwapBuffers(dc.m_ps.hdc);	
		
#ifdef FORWARD_MOTION
		if (!pDoc->LoadCalibrationData2(pDoc->m_szDataLoad,&dc))
#else
		if (!pDoc->LoadCalibrationData(pDoc->m_szDataLoad))
#endif
		{
			if (!theApp.m_bStandalone)
				AfxMessageBox("Calibration data not loaded.");				
			else
#ifdef _DEBUG
				PostQuitMessage(0);
#else
				exit(0);
#endif				
		}
		goto End;
	}

#ifdef FORWARD_MOTION	
	pDoc->Reconstruct5(&dc);
#else
	pDoc->Reconstruct4();	
#endif

	if ((m_nSelectedCamera!=0) && (m_nSelectedCamera-1)<(int)(pDoc->m_lstFrames.size()))
	{	
		CCamera *pCam=&pDoc->m_lstFrames[m_nSelectedCamera-1]->m_Camera;
		m_pVideo->SetCamera(pCam,false,true);
		//m_pVideo->SetCamera(pCam,true,true);		
		//m_pVideo->WriteXY(0,30,40,1.2f,"Frame=%d,Camera %d selected,FOV=%f",m_pUtils->GetFrameNumber(),m_nSelectedCamera,RAD2DEG(pCam->m_fFov));
		m_pVideo->WriteXY(0,30,40,1.2f,"Camera %d selected,FOV=%f",m_nSelectedCamera,RAD2DEG(pCam->m_fFov));
		m_pVideo->WriteXY(0,30,40+20,1.2f,"Pos=%0.2f,%0.2f,%0.2f",pCam->GetPos().x,pCam->GetPos().y,pCam->GetPos().z);		
		m_pVideo->WriteXY(0,30,40+40,1.2f,"Angles=%0.2f,%0.2f,%0.2f",pCam->GetAngles().x,pCam->GetAngles().y,pCam->GetAngles().z);
		//m_pVideo->WriteXY(0,30,40+60,1.2f,"Scale=%d,rays left=",pDoc->GetCurrScale(),pDoc->GetInfo());
		m_pVideo->WriteXY(0,30,40+60,1.2f,"Estimate completion=%d(%d)",pDoc->m_bDone?100:pDoc->GetInfo(),pDoc->GetInfo());
		m_Camera.SetPos(pCam->GetPos());
		m_Camera.SetAngles(pCam->GetAngles());
	}
	else
	{
		m_pVideo->SetCamera(&m_Camera,true,true);
		//m_pVideo->WriteXY(0,30,40,1.2f,"Frame=%d,No Camera selected,FOV=%f",m_pUtils->GetFrameNumber(),RAD2DEG(m_Camera.m_fFov));
		m_pVideo->WriteXY(0,30,40,1.2f,"No Camera selected,FOV=%f",RAD2DEG(m_Camera.m_fFov));
		m_pVideo->WriteXY(0,30,40+20,1.2f,"Pos=%0.2f,%0.2f,%0.2f",m_Camera.GetPos().x,m_Camera.GetPos().y,m_Camera.GetPos().z);		
		//m_pVideo->WriteXY(0,30,40,1.2f,"Faces=%d",GetDocument()->m_pDaVinciView->m_lstFaces.size());
		m_pVideo->WriteXY(0,30,40+40,1.2f,"Angles=%0.2f,%0.2f,%0.2f",m_Camera.GetAngles().x,m_Camera.GetAngles().y,m_Camera.GetAngles().z);
		//m_pVideo->WriteXY(0,30,40+60,1.2f,"Scale=%d,rays left=%d",pDoc->GetCurrScale(),pDoc->GetInfo());
		m_pVideo->WriteXY(0,30,40+60,1.2f,"Estimate completion=%d(%d)",pDoc->m_bDone?100:pDoc->GetInfo(),pDoc->GetInfo());
	}
	
	m_pVideo->Draw3DBBox(pDoc->m_vMins,pDoc->m_vMaxs);

	//m_pVideo->WriteXY(0,30,40+20+20,1.2f,"FPS=%0.2f,FrameId=%d",m_pUtils->GetDeltaFrameTime(),m_pVideo->GetFrameId());
	//m_pVideo->WriteXY(0,30,40+20+20,1.2f,"FPS=%0.2f,FrameId=%d",m_pUtils->GetDeltaFrameTime(),nFrame++);	
		
	int k=1;
	for (lstFramesIt i=pDoc->m_lstFrames.begin();i!=pDoc->m_lstFrames.end();i++,k++)
	{
		CPhotoFrame *pFrame=(*i);
		if (m_nSelectedCamera!=k)
			m_pVideo->DrawCamera(pFrame->m_Camera);
	} //i		

	//glEnable(GL_POINT_SMOOTH);
	glPointSize(3.0);		
	//if (pDoc->m_lstVertices.empty())
	//	return;

	for (lstPhotoVerticesIt i2=pDoc->m_lstVertices.begin();i2!=pDoc->m_lstVertices.end();i2++)
	{
		CPVertex *pVert=(*i2);						
		//CPhotoVertex *pVert=pDoc->m_lstVertices[pDoc->m_lstVertices.size()-10];
		
		/*
		glBegin(GL_LINES);		
		glColor3dv(&pVert->m_vColor.x);
		//for (int k=0;k<pVert->m_nViews;k++)
		for (int k=0;k<2;k++)
		{		
			glVertex3dv(&pVert->m_v1[k].x);
			glVertex3dv(&pVert->m_v2[k].x);			
		}
		glEnd();						
		*/
		 
#ifdef FORWARD_MOTION
		
		if (pDoc->m_bShowFeaturesOnly)
		{		
			// 3d points				
			glBegin(GL_POINTS);
			glColor3dv(&pVert->m_vColor.x);		
			//glColor3f(0,1,0);
			glVertex3dv(&pVert->m_vPos.x);		
			glEnd();				
		}			

		/*
		CCamera *pCam=&pDoc->m_lstFrames[KEY_FRAME]->m_Camera;
		glColor3f(1,1,1);
		glBegin(GL_LINES);		
		glVertex3dv(&pCam->m_vOrigin.x);
		glVertex3dv(&pVert->m_vPos.x);			
		glEnd();						
		*/

		/*
		for (lstFramesIt i=pDoc->m_lstFrames.begin();i!=pDoc->m_lstFrames.end();i++,k++)
		{
			CPhotoFrame *pFrame=(*i);
			if (pFrame->m_nFrameNum!=KEY_FRAME)
				continue;
			//CCamera *pCam=&pFrame->m_Camera;
			CCamera *pCam=&pDoc->m_lstFrames[KEY_FRAME]->m_Camera;

			vector3f vPos1,vPos2;
			int nFrame2=(pFrame->m_nFrameNum+1)%(int)(pDoc->m_lstFrames.size());
			m_pVideo->UnProjectFromScreen(vector3f(pVert->m_v2DPos[pFrame->m_nFrameNum].x,pVert->m_v2DPos[pFrame->m_nFrameNum].y,0.98),vPos1,pCam);
			m_pVideo->UnProjectFromScreen(vector3f(pVert->m_v2DPos[nFrame2].x,pVert->m_v2DPos[nFrame2].y,0.98),vPos2,pCam);

			glBegin(GL_POINTS);
			//glColor3dv(&pVert->m_vColor.x);		
			glColor3f(0,1,0);
			glVertex3dv(&vPos1.x);		
			glColor3f(0,0,1);
			glVertex3dv(&vPos2.x);		
			glEnd();				
						
			glBegin(GL_LINES);
			//glVertex3dv(&pCam->m_vOrigin.x);
			glColor3f(0,1,0);
			glVertex3dv(&vPos1.x);			
			glColor3f(0,0,1);
			glVertex3dv(&vPos2.x);			
			glEnd();						
		} //i		
		*/

#endif
	} //i	


	//////////////////////////////////////////////////////////////////////////	
	if (pDoc->m_nNumIndexes>0)
	{
#ifdef USE_GL_LIGHT
		glEnable(GL_LIGHT0);
		glEnable(GL_LIGHTING);

		if (m_bUseGLLight)
		{		
			glDisable(GL_TEXTURE_2D);
			//float vLightPos[4]={0,-1,0,0};
			//glLightfv(0,GL_POSITION,vLightPos);
		}
#endif		
		
		//glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
		m_pVideo->SetCurrentTexture(pDoc->m_lstFrames[KEY_FRAME]->m_pImage);
		
		//glDisable(GL_CULL_FACE);
		glColor3f(1,1,1);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#ifdef USE_GL_LIGHT		
		//if (m_bUseGLLight)
		glEnableClientState(GL_NORMAL_ARRAY);
#endif

		// start=lowest index used -> 0
		// end=highest vertex used -> GRID_SIZE_SUB2
		// GL_TRIANGLE_STRIP 0x0005
		// GL_UNSIGNED_SHORT	0x1403
		// GL_UNSIGNED_INT		0x1405		
#ifdef USE_GL_LIGHT		
		//if (m_bUseGLLight)
		{		
			glVertexPointer(3, GL_FLOAT, 8 * sizeof(float), pDoc->m_Vertices);
			glTexCoordPointer(2, GL_FLOAT, 8 * sizeof(float),pDoc->m_Vertices+3);			
			glNormalPointer(GL_FLOAT, 8 * sizeof(float),pDoc->m_Vertices+3+2);			
		}
		/*
		else
		{
			glVertexPointer(3, GL_FLOAT, 8 * sizeof(float), pDoc->m_Vertices);
			glTexCoordPointer(2, GL_FLOAT, 8 * sizeof(float),pDoc->m_Vertices+3);					
		}
		*/
#else
		glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), pDoc->m_Vertices);
		glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float),pDoc->m_Vertices+3);					
#endif

		glDrawElements(GL_TRIANGLES,pDoc->m_nNumIndexes,GL_UNSIGNED_INT,pDoc->m_Indices);		

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

#ifdef USE_GL_LIGHT
		//if (m_bUseGLLight)
		{		
			glDisableClientState(GL_NORMAL_ARRAY);
			glEnable(GL_TEXTURE_2D);
			glDisable(GL_LIGHT0);
			glDisable(GL_LIGHTING);
		}
#endif

	}

End:
	//////////////////////////////////////////////////////////////////////////	
	//Swap Buffers	
	m_pVideo->SwapBuffers(dc.m_ps.hdc);	
	m_pVideo->CheckError("EndFrame");

	wglMakeCurrent(NULL,NULL);	

	m_bDrawing=false;
}