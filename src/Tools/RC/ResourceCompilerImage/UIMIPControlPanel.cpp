#include "StdAfx.h"
#include <windowsx.h>									// ComboBox_GetCurSel
#include <assert.h>										// assert()
#include "UIMIPControlPanel.h"				// CUIMIPControlPanel
#include "resource.h"
#include "ImageProperties.h"					// CBumpProperties
#include <Commdlg.h>									// OPENFILENAME
#include <commctrl.h>									// TBM_SETRANGEMAX, TBM_SETRANGEMIN

const int CUIMIPControlPanel::sliderIDs[NUM_CONTROLLED_MIP_MAPS] = {
	IDC_SLIDER_MIPALPHA0,
	IDC_SLIDER_MIPALPHA1,
	IDC_SLIDER_MIPALPHA2,
	IDC_SLIDER_MIPALPHA3,
	IDC_SLIDER_MIPALPHA4,
	IDC_SLIDER_MIPALPHA5
};

const int CUIMIPControlPanel::editIDs[NUM_CONTROLLED_MIP_MAPS] = {
	IDC_EDIT_MIPALPHA0,
	IDC_EDIT_MIPALPHA1,
	IDC_EDIT_MIPALPHA2,
	IDC_EDIT_MIPALPHA3,
	IDC_EDIT_MIPALPHA4,
	IDC_EDIT_MIPALPHA5
};

CUIMIPControlPanel::CUIMIPControlPanel() :m_hTab_Normalmapgen(0)
{
}

void CUIMIPControlPanel::InitDialog( HWND hWndParent, const bool bShowBumpmapFileName )
{
	// Initialise the values.
	for (int i = 0; i < NUM_CONTROLLED_MIP_MAPS; ++i)
	{
		m_MIPAlphaControl[i].SetRange(0.0f, 100.0f);
		m_MIPAlphaControl[i].SetValue(50.0f);
	}

	m_MIPSharpenControl.SetRange(0.0f, 100.0f);
	m_MIPSharpenControl.SetValue(0.0f);

	HWND hwnd=GetDlgItem(hWndParent,IDC_PROPTAB);assert(hwnd);
	RECT wnp;

	GetWindowRect(hwnd,&wnp);

	m_hTab_Normalmapgen=CreateDialogParam(g_hInst,MAKEINTRESOURCE(IDD_TAB_MIPMAPCONTROL),hWndParent,WndProc,reinterpret_cast<LPARAM>(this));assert(m_hTab_Normalmapgen);
	SetWindowPos(m_hTab_Normalmapgen, HWND_TOP, wnp.left, wnp.top, 0,0,SWP_NOSIZE); 

	{
		HWND hwnd = GetDlgItem(m_hTab_Normalmapgen, IDC_VALUESPARENT); assert(hwnd);
		m_valuesDlg.Create(hwnd);
		RECT rect;
		GetClientRect(hwnd, &rect);
		m_valuesDlg.MoveWindow(&rect);
	}

	// Subclass the sliders and edit controls.
	for (int i = 0; i < NUM_CONTROLLED_MIP_MAPS; ++i)
	{
		HWND hwndEdit = m_valuesDlg.GetDlgItem(CUIMIPControlPanel::editIDs[i]); assert(hwndEdit);
		HWND hwndSlider = m_valuesDlg.GetDlgItem(CUIMIPControlPanel::sliderIDs[i]); assert(hwndSlider);
		m_MIPAlphaControl[i].SubclassWindows(hwndSlider, hwndEdit);
	}

	{
		HWND hwndEdit = m_valuesDlg.GetDlgItem(IDC_EDIT_MIPSHARPEN); assert(hwndEdit);
		HWND hwndSlider = m_valuesDlg.GetDlgItem(IDC_SLIDER_MIPSHARPEN); assert(hwndSlider);
		m_MIPSharpenControl.SubclassWindows(hwndSlider,hwndEdit);

		// temporary disabled - algorithm still need some adjustment
//		EnableWindow(hwndEdit,false);
//		EnableWindow(hwndSlider,false);
	}
}

HWND CUIMIPControlPanel::GetHWND() const
{
	return m_hTab_Normalmapgen;
}

void CUIMIPControlPanel::GetDataFromDialog( CImageProperties &rProp )
{
	for (int i = 0; i < NUM_CONTROLLED_MIP_MAPS; ++i)
		rProp.SetMIPAlpha(i, (int)(this->m_MIPAlphaControl[i].GetValue()+0.5f));

	rProp.SetMipSharpen((int)(this->m_MIPSharpenControl.GetValue()+0.5f));
}

void CUIMIPControlPanel::SetDataToDialog( const CImageProperties &rProp, const bool bInitalUpdate )
{
	int iMIPAlpha[NUM_CONTROLLED_MIP_MAPS];

	rProp.GetMIPAlphaArray(iMIPAlpha);

	for (int i = 0; i < NUM_CONTROLLED_MIP_MAPS; ++i)
		this->m_MIPAlphaControl[i].SetValue((float)iMIPAlpha[i]);

	this->m_MIPSharpenControl.SetValue((float)rProp.GetMipSharpen());
}

BOOL CALLBACK CUIMIPControlPanel::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Get the instance pointer.
	CUIMIPControlPanel* pThis;
	if (uMsg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hWnd, GWL_USERDATA, lParam);
		pThis = reinterpret_cast<CUIMIPControlPanel*>(static_cast<LONG_PTR>(lParam));
	}
	else
	{
		pThis = reinterpret_cast<CUIMIPControlPanel*>(static_cast<LONG_PTR>(GetWindowLongPtr(hWnd, GWL_USERDATA)));
	}

	HWND hParent=GetParent(hWnd);

	if(hParent)
		return SendMessage(hParent,uMsg,wParam,lParam)!=0?TRUE:FALSE;

	//	return DefWindowProc( hWnd, uMsg, wParam, lParam );
	return FALSE;
}
