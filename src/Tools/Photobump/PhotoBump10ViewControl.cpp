

#include "stdafx.h"
#include "PhotoBump10.h"
#include "PhotoBump10View.h"
#include "PhotoBump10Doc.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////
void CPhotoBump10View::OnMouseMove(UINT nFlags, CPoint point) 
{
	CView::OnMouseMove(nFlags, point);

	//CPhotoBump10Doc* pDoc = GetDocument();
	//ASSERT_VALID(pDoc);
	//pDoc->SetActive(this);

	m_ptDesktop = point;
	m_ptLogical = point;

	// Convert the "point" to a value relative to the top/left corner of the 
	// windows desktop
	ClientToScreen(&m_ptDesktop);
	
	// Convert the "point" to a value relative to the top left corner of the 
	// client rect viewport. This is how we know where the mouse pointer is, 
	// even if the window has been scrolled.
	//CString sPtLogical;
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.DPtoLP(&m_ptLogical, 1);

	//theApp.m_pMainFrame->SetPaneText(0, sText, FALSE);
			
	//SetMouseCursor();

	if (m_bTranslateCamera)			
		MouseTranslate(point);										
	else
	if (m_bRotateCamera)
		MouseRotate(point);
	else
	if (m_bForward)
		MouseForward(point);

	m_ptMousePos = point;
	
	if (m_bTranslateCamera || m_bRotateCamera || m_bForward)
	{	
		// Invalidate window so entire client area 
		// is redrawn when UpdateWindow is called.
		Invalidate();   
		//UpdateWindow();
	}
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10View::OnLButtonDown(UINT nFlags,CPoint point)
{
	/*
	if (nFlags & MK_CONTROL)
	{
		m_bTranslateCamera=true;
		m_bRotateCamera=false;
	}
	else
	if (nFlags & MK_SHIFT)
	{		
		m_bTranslateCamera=false;
		m_bRotateCamera=true;
	}
	else	
	{
		m_bTranslateCamera=false;
		m_bRotateCamera=false;
	}
	*/

	m_bLButtonDown=true;

	m_bRotateCamera=false;
	if (nFlags & MK_RBUTTON)
	{
		m_bTranslateCamera=true;		
		m_bForward=false;
	}
	else
	{
		m_bTranslateCamera=false;		
		m_bForward=true;
	}
	
	CView::OnLButtonDown(nFlags,point);
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10View::OnLButtonUp(UINT nFlags,CPoint point)
{	
	m_bLButtonDown=false;
	m_bRotateCamera=m_bTranslateCamera=m_bForward=false;
	CView::OnLButtonUp(nFlags,point);
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10View::OnRButtonDown(UINT nFlags,CPoint point)
{
	m_bForward=false;
	if (nFlags & MK_LBUTTON)
	{
		m_bTranslateCamera=true;
		m_bRotateCamera=false;				
	}
	else
	{
		m_bTranslateCamera=false;
		m_bRotateCamera=true;
	}
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10View::OnRButtonUp(UINT nFlags,CPoint point)
{	
	m_bRotateCamera=m_bTranslateCamera=m_bForward=false;
	CView::OnRButtonUp(nFlags,point);
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10View::MouseTranslate(CPoint point)
{
	vector3f vCurrPos=m_Camera.GetPos();
	ftype fStep=m_Camera.m_fSpeed/1000.0;

	ftype xStep=(m_ptMousePos.x-point.x)*fStep;
	ftype yStep=(m_ptMousePos.y-point.y)*fStep;

	if (m_Camera.m_nPivot)
	{
		vCurrPos.x+=xStep;
		vCurrPos.y-=yStep;
	}
	else
	{	
		vector3f vRight=m_Camera.GetRightVector();	
		//vector3f vForward=m_Camera.GetForwardVector();	
		vector3f vUp=m_Camera.GetUpVector();
		vRight*=xStep;vUp*=yStep;
		vCurrPos=vCurrPos+vRight-vUp;
	}

	m_Camera.SetPos(vCurrPos);
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10View::MouseRotate(CPoint point)
{
	vector3f vCurrAngles=m_Camera.GetAngles();	
	ftype fStep=m_Camera.m_fSpeed/10.0;

	ftype xStep=(m_ptMousePos.y-point.y)*fStep;
	ftype yStep=(m_ptMousePos.x-point.x)*fStep;
	

	vCurrAngles.x+=xStep;
	if (m_Camera.m_nPivot)
		vCurrAngles.z+=yStep;
	else
		vCurrAngles.z-=yStep;
		
	m_Camera.SetAngles(vCurrAngles);	

	//////////////////////////////////////////////////////////////////////////
	// rotate around object
	CPhotoBump10Doc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	vector3f vCenter=(pDoc->m_vMins+pDoc->m_vMaxs)*0.5f;	
	vector3f	vCurrPos=m_Camera.GetPos();
	vector3f	vDir=m_Camera.GetForwardVector();
	ftype			fLen=(vCurrPos-vCenter).Length();
	//vector3f	vPos=-(vDir*fLen);
	vector3f	vPos=vCenter-(vDir*fLen);

	m_Camera.SetPos(vPos);
	//////////////////////////////////////////////////////////////////////////

	/*
	if (!m_Camera.m_nPivot)
	{	
		vector3f vCurrPos=m_Camera.GetPos();
		// do calculations using double precision
		vector3d vCurrPos2(vCurrPos.x,vCurrPos.y,vCurrPos.z);	

		CMatrixf tm;
		tm.Identity();
		// Rotate the camera around the origin	
		
		//tm.RotateMatrix(vector3d(0, 0, 0));	 // free view
		//tm.RotateMatrix(vector3d(-xStep, -yStep, 0));			
		tm.RotateMatrix(vector3d(0, -yStep, 0));			
		vector3d vNewPos=tm.TransformPoint(vCurrPos2);

		vCurrPos.Set((ftype)(vNewPos.x),(ftype)(vNewPos.y),(ftype)(vNewPos.z));

		m_Camera.SetPos(vCurrPos);	
	}
	*/
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10View::MouseForward(CPoint point)
{
	vector3f vCurrPos=m_Camera.GetPos();	
	ftype fStep=(m_ptMousePos.y-point.y)*(m_Camera.m_fSpeed/250.0);

	if (m_Camera.m_nPivot)
		vCurrPos.z-=fStep;	
	else
	{
		vector3f vForward=m_Camera.GetForwardVector();
		vForward*=fStep;
		vCurrPos+=vForward;
	}

	m_Camera.SetPos(vCurrPos);
}

//////////////////////////////////////////////////////////////////////////
BOOL CPhotoBump10View::OnMouseWheel(UINT nFlags, short zDelta,CPoint point)
{	
	vector3f vCurrPos=m_Camera.GetPos();
	ftype fStep=(ftype)(zDelta)*m_Camera.m_fSpeed/4000.0;

	if (m_Camera.m_nPivot)
		vCurrPos.z-=fStep;	
	else
	{
		vector3f vForward=m_Camera.GetForwardVector();
		vForward*=fStep;
		vCurrPos+=vForward;
	}

	m_Camera.SetPos(vCurrPos);

	Invalidate(); 

	return (CView::OnMouseWheel(nFlags,zDelta,point));
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10View::OnKeyDown(UINT nChar,UINT nRepCnt,UINT nFlags)
{
	CView::OnKeyDown(nChar,nRepCnt,nFlags);

	CPhotoBump10Doc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	
	if (nChar>='0' && nChar<='9')
	{
		m_nSelectedCamera=nChar-'0';
		Invalidate(); 
	}
	else
	switch(nChar) 
	{		
		case 'D':			
			m_bDumpError=true; 
		break;
		case 'W':
			SetGLCurrent();
			glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
			wglMakeCurrent(NULL,NULL);	
		break;
		case 'S':
			SetGLCurrent();
			glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
			wglMakeCurrent(NULL,NULL);	
		break;
		case 'L':
			m_bUseGLLight=!m_bUseGLLight;
		break;
		case VK_LEFT:			
		break;

	}
}
