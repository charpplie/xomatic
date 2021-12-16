#include "StdAfx.h"
#include "LensFlareView.h"
#include "LensFlareItem.h"
#include "LensFlareEditor.h"
#include "Objects/EntityObject.h"
#include "Material/MaterialManager.h"

BEGIN_MESSAGE_MAP(CLensFlareView, CPreviewModelCtrl)
	//{{AFX_MSG_MAP(CLensFlareView)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CLensFlareView::CLensFlareView()
{
	CLensFlareEditor::GetLensFlareEditor()->RegisterLensFlareItemChangeListener(this);
}

CLensFlareView::~CLensFlareView()
{
	CLensFlareEditor::GetLensFlareEditor()->UnregisterLensFlareItemChangeListener(this);
}

int CLensFlareView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (__super::OnCreate(lpCreateStruct) == -1)
		return -1;

	SetClearColor(ColorF(0,0,0,0));
	SetGrid(false);
	SetAxis(false);
	EnableUpdate(true);
	m_pLensFlareMaterial = GetIEditor()->GetMaterialManager()->LoadMaterial(CEntityObject::s_LensFlareMaterialName);
	m_InitCameraPos = m_camera.GetPosition();

	return 0;
}

void CLensFlareView::RenderObject( IMaterial* pMaterial, SRenderingPassInfo& passInfo )
{
	if( m_pLensFlareItem )
	{
		IOpticsElementBasePtr pOptics = m_pLensFlareItem->GetOptics();
		if( pOptics && m_pLensFlareMaterial )
		{
			IMaterial* pMaterial = m_pLensFlareMaterial->GetMatInfo();
			if( pMaterial )
			{
				m_pRenderer->SetViewport(1,1,m_pRenderer->GetWidth(),m_pRenderer->GetHeight());
				SShaderItem& shaderItem = pMaterial->GetShaderItem();
				SLensFlareRenderParam param;
				param.pCamera = &m_camera;
				param.pShader = shaderItem.m_pShader;
				pOptics->Render(&param, Vec3(0,0,0));
			}
		}
	}
}

void CLensFlareView::OnLensFlareChangeItem( CLensFlareItem* pLensFlareItem )
{
	if( m_pLensFlareItem != pLensFlareItem )
	{
		m_pLensFlareItem = pLensFlareItem;
		Update(true);
		m_bHaveAnythingToRender = m_pLensFlareItem != NULL;
	}
}

void CLensFlareView::OnLensFlareDeleteItem( CLensFlareItem* pLensFlareItem )
{
	if( m_pLensFlareItem == pLensFlareItem )
	{
		m_pLensFlareItem = NULL;
		Update();
		m_bHaveAnythingToRender = false;
	}
}

void CLensFlareView::OnMouseMove(UINT nFlags, CPoint point)
{
	if( nFlags & MK_MBUTTON )
	{
		float dx = (float)(point.x-m_prevPoint.x)*0.1f;
		float dy = (float)(point.y-m_prevPoint.y)*0.1f;

		if( fabs(dx) > 0.001f || fabs(dy) > 0.001f )
		{
			Vec3 ydir = m_camera.GetMatrix().GetColumn2().GetNormalized();
			Vec3 xdir = m_camera.GetMatrix().GetColumn0().GetNormalized();
			Vec3 currentPos = m_camera.GetPosition();
			m_camera.SetPosition(currentPos + xdir*dx - ydir*dy);
			Update();
		}

		m_prevPoint = point;
	}
}

void CLensFlareView::OnMButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
	m_prevPoint = point;
}

void CLensFlareView::OnMButtonUp(UINT nFlags, CPoint point)
{
	::ReleaseCapture();
}

void CLensFlareView::OnRButtonDown(UINT nFlags, CPoint point)
{
}

void CLensFlareView::OnRButtonUp(UINT nFlags, CPoint point)
{
	m_camera.SetPosition(m_InitCameraPos);
	Update();
}

void CLensFlareView::OnInternalVariableChange( IVariable* pVar )
{
	Update();
}

void CLensFlareView::OnLensFlareChangeElement( CLensFlareElement* pLensFlareElement )
{
	Update();
}

void CLensFlareView::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch (event)
	{
	case eNotify_OnCloseScene:
		ReleaseObject();
		break;
	}
}

void CLensFlareView::OnSize(UINT nType, int cx, int cy)
{	
	__super::OnSize(nType,cx,cy);
	Update();
}