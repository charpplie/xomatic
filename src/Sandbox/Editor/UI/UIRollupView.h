#ifndef __UIRollupView_H__
#define __UIRollupView_H__

#include "Controls/RollupBar.h"
#include "Controls/RollupCtrl.h"

////////////////////////////////////////////////////////////////////
class CUIRollupControl
{
public:
	CRollupCtrl* Create( CWnd* parent );

	int AddPage( const char* caption, CDialog *pwndTemplate, bool bExpanded = true );
	void RemovePage( int idx );

private:
	CRollupCtrl m_Control;
};

////////////////////////////////////////////////////////////////////
class CUIRollupView
{
public:
	~CUIRollupView();

	CRollupBar* Create( CWnd* parent );
	CUIRollupControl* CreateControl( int id, const char* caption );

	int SelectRollUpBar( int rollupBarId );
	CUIRollupControl* GetControl( int rollupBarId );

private:
	CRollupBar m_rollUpBar;
	typedef std::map<int, CUIRollupControl*> TRollupControls;
	TRollupControls	m_rollUpControls;
};

#endif __UIRollupView_H__