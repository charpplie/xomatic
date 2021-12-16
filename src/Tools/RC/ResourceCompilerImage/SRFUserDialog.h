#pragma once

#include "UIBumpmapPanel.h"							// CUIBumpmapPanel

class CSRFCompiler;

class CSRFUserDialog
{
public:

	// constructor
	CSRFUserDialog();

	bool DoModal( CSRFCompiler *inpImageCompiler );



private: // -------------------------------------------------------

	HWND								m_hWindow;						//
	CSRFCompiler *			m_pSRFCompiler;				//
	bool								m_bQuitting;					//

	HWND								m_hTab_SRFOptions;		//
	CUIBumpmapPanel			m_BumpPanel;					//


	void UpdateWindowTitle();

	//!
	static BOOL CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

	void GetDataFromDialog();

	void SetDataToDialog( const bool bInitalUpdate=false );

	void CreateDialogItems();

	void UpdateOccBrightenupStrength();

	void SetPropertyTab( const int iniNo ); 

	void Paint( HDC hdc );
};