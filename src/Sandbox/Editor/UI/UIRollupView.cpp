#include "StdAfx.h"
#include "UIRollupView.h"


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
CRollupCtrl* CUIRollupControl::Create( CWnd* parent )
{
	assert(!m_Control.m_hWnd);

	m_Control.Create(WS_VISIBLE | WS_CHILD, CRect(4, 4, 187, 362), parent, NULL);
	return &m_Control;
}

////////////////////////////////////////////////////////////////////
int CUIRollupControl::AddPage( const char* caption, CDialog *pwndTemplate, bool bExpanded /*= true */)
{
	assert(m_Control.m_hWnd);

	return m_Control.InsertPage(caption, pwndTemplate, true, -1, bExpanded);
}

////////////////////////////////////////////////////////////////////
void CUIRollupControl::RemovePage( int idx )
{
	assert(m_Control.m_hWnd);

	m_Control.RemovePage(idx);
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
CUIRollupView::~CUIRollupView()
{
	for (TRollupControls::iterator it = m_rollUpControls.begin(); it != m_rollUpControls.end(); ++it)
		delete it->second;
}

////////////////////////////////////////////////////////////////////
CRollupBar* CUIRollupView::Create( CWnd* parent )
{
	assert(!m_rollUpBar.m_hWnd);

	m_rollUpBar.Create( NULL,NULL,WS_CHILD|WS_VISIBLE,CRect(0,0,1,1),parent,0 );
	return &m_rollUpBar;
}

////////////////////////////////////////////////////////////////////
CUIRollupControl* CUIRollupView::CreateControl( int id, const char* caption )
{
	assert(GetControl(id) == NULL);
	assert(m_rollUpBar.m_hWnd);

	CUIRollupControl* page = new CUIRollupControl();
	m_rollUpBar.SetRollUpCtrl( id, page->Create(&m_rollUpBar), caption );
	m_rollUpControls[id] = page;
	return page;
}

////////////////////////////////////////////////////////////////////
int CUIRollupView::SelectRollUpBar( int rollupBarId )
{
	assert(GetControl(rollupBarId) != NULL);
	assert(m_rollUpBar.m_hWnd);

	m_rollUpBar.Select( rollupBarId );
	return rollupBarId;
}

////////////////////////////////////////////////////////////////////
CUIRollupControl* CUIRollupView::GetControl( int rollupBarId )
{
	assert(m_rollUpBar.m_hWnd);

	TRollupControls::iterator it = m_rollUpControls.find(rollupBarId);
	return it != m_rollUpControls.end() ? it->second : NULL;
}
