#include "StdAfx.h"

#include "LinkTool.h"

IMPLEMENT_DYNAMIC(CLinkTool,CEditTool)

//////////////////////////////////////////////////////////////////////////
CLinkTool::CLinkTool()
{
	m_pChild=NULL;
	SetStatusText("Click on object and drag a link to a new parent");

	m_hLinkCursor = AfxGetApp()->LoadCursor( IDC_POINTER_LINK );
	m_hLinkNowCursor = AfxGetApp()->LoadCursor( IDC_POINTER_LINKNOW );
	m_hCurrCursor = m_hLinkCursor;
}

//////////////////////////////////////////////////////////////////////////
CLinkTool::~CLinkTool()
{
}

//////////////////////////////////////////////////////////////////////////
void CLinkTool::LinkObject( CBaseObject * pChild, CBaseObject * pParent ) 
{
	if (pChild == NULL)
		return;

	if (ChildIsValid(pParent, pChild))
	{
		CUndo undo( "Link Object" );
		pParent->AttachChild(pChild);
		CString str;
		str.Format("%s attached to %s", pChild->GetName(), pParent->GetName());
		SetStatusText(str);
	}
	else
	{
		SetStatusText("Error: Cyclic linking or already linked.");
	}
}

//////////////////////////////////////////////////////////////////////////
void CLinkTool::LinkSelectedToParent(CBaseObject * pParent)
{
	if (pParent)
	{
		if (IsRelevant(pParent))
		{
			CSelectionGroup * pSel = GetIEditor()->GetSelection();
			if(!pSel->GetCount())
				return;
			CUndo undo( "Link Object(s)" );
			for(int i=0; i<pSel->GetCount(); i++)
			{
				CBaseObject * pChild = pSel->GetObject(i);
				LinkObject(pChild, pParent);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CLinkTool::MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags )
{
	m_hCurrCursor = m_hLinkCursor;
	if (event == eMouseLDown)
	{
		HitContext hitInfo;
		view->HitTest( point,hitInfo );
		CBaseObject *obj = hitInfo.object;
		if (obj)
		{
			if (IsRelevant(obj))
			{
				m_StartDrag = obj->GetWorldPos();
				m_pChild=obj;
			}
		}
	}
	else if (event == eMouseLUp)
	{
		HitContext hitInfo;
		view->HitTest( point,hitInfo );
		CBaseObject *obj = hitInfo.object;
		if (obj)
		{
			if (IsRelevant(obj))
			{
				LinkObject(m_pChild, obj);
			}
		}
		m_pChild=NULL;
	}
	else if (event == eMouseMove)
	{
		m_EndDrag = view->ViewToWorld(point);
		
		HitContext  hitInfo;
		view->HitTest( point,hitInfo );
		CBaseObject *obj = hitInfo.object;
		if (obj)
		{
			if (IsRelevant(obj))
			{
				// Set Cursors.
				view->SetCurrentCursor( STD_CURSOR_HIT,obj->GetName() );
				if (m_pChild)
				{
					if (ChildIsValid(obj, m_pChild))
					{
						m_hCurrCursor = m_hLinkNowCursor;
					}
				}
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CLinkTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{ 
	if (nChar == VK_ESCAPE)
	{
		// Cancel selection.
		GetIEditor()->SetEditTool(0);
	}
	return false; 
}

//////////////////////////////////////////////////////////////////////////
void CLinkTool::Display( DisplayContext &dc )
{
	if (m_pChild)
		dc.DrawLine(m_StartDrag, m_EndDrag, ColorF(1, 0, 0), ColorF(1, 1, 1));
}

//////////////////////////////////////////////////////////////////////////
bool CLinkTool::ChildIsValid(CBaseObject *pParent, CBaseObject *pChild, int nDir)
{
	if (!pParent)
		return false;
	if (!pChild)
		return false;
	if (pParent==pChild)
		return false;
	CBaseObject *pObj;
	if (nDir & 1)
	{
		if (pObj=pChild->GetParent())
		{
			if (!ChildIsValid(pParent, pObj, 1))
			{
				return false;
			}
		}
	}
	if (nDir & 2)
	{
		for (int i=0;i<pChild->GetChildCount();i++)
		{
			if (pObj=pChild->GetChild(i))
			{
				if (!ChildIsValid(pParent, pObj, 2))
				{
					return false;
				}
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CLinkTool::OnSetCursor( CViewport *vp )
{
	SetCursor( m_hCurrCursor );
	return true;
}