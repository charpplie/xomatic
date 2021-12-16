#include "StdAfx.h"
#include <assert.h>									// assert()
#include "resource.h"								// IDD_SRFUSERDIALOG
#include "SRFUserDialog.h"					// CSRFUserDialog
#include "tools.h"									// CenterWindow()
#include <commctrl.h>								// TCITEM
#include "SRFCompiler.h"						// CSRFCompiler
#include <windowsx.h>								// ComboBox_GetCurSel


CSRFUserDialog::CSRFUserDialog() :m_pSRFCompiler(0), m_hWindow(0), m_bQuitting(false)
{
	m_hTab_SRFOptions=0;
}

bool CSRFUserDialog::DoModal( CSRFCompiler *pSRFCompiler )
{
	m_pSRFCompiler=pSRFCompiler;

	m_hWindow=CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_SRFUSERDIALOG),0,CSRFUserDialog::WndProc);

	if( !m_hWindow ) 
	{
		DWORD h=GetLastError();
		m_pSRFCompiler=0;
		return false;
	}

	// store this pointer	
	SetWindowLongPtr(m_hWindow,DWL_USER,(LONG)(LONG_PTR)this);

	//	CenterWindow(m_hWindow);			// here I can't see my tab items

	CreateDialogItems();

	CenterWindow(m_hWindow);				// here it's moving

	SetDataToDialog(true);

	UpdateWindowTitle();

	ShowWindow(m_hWindow,SW_SHOW);

	// message loop
	{
		MSG Msg;

		while(GetMessage(&Msg, NULL, 0, WM_USER)) 
		{
			if(m_bQuitting)
				break;

			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}

	DestroyWindow(m_hWindow);
	m_hWindow=0;
	m_pSRFCompiler=0;

	return true;
}




void CSRFUserDialog::GetDataFromDialog()
{
	HWND hwnd;

	hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_UNOCCBRIGHTENUP);assert(hwnd);
	{
		char str[256];

		GetWindowText(hwnd,str,256);

		m_pSRFCompiler->m_Props.m_OccmapBrightenup=(float)atof(str);

		if(m_pSRFCompiler->m_Props.m_OccmapBrightenup<0.00f)
		{ m_pSRFCompiler->m_Props.m_OccmapBrightenup=0.00f;UpdateOccBrightenupStrength(); }
		if(m_pSRFCompiler->m_Props.m_OccmapBrightenup>100.0f)
		{ m_pSRFCompiler->m_Props.m_OccmapBrightenup=100.0f;UpdateOccBrightenupStrength(); }
	}

	hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_NORMALOUTPUTTYPE);assert(hwnd);
	m_pSRFCompiler->m_Props.m_eNormalOutputType=(ENormalOutputType)ComboBox_GetCurSel(hwnd);

	hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_NORMALOUTPUTSPACE);assert(hwnd);
	m_pSRFCompiler->m_Props.m_eUsedSpaceType=(EUsedSpaceType)ComboBox_GetCurSel(hwnd);

	hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_DISPLACEOUTPUTTYPE);assert(hwnd);
	m_pSRFCompiler->m_Props.m_eDisplaceOutputType=(EDisplaceOutputType)ComboBox_GetCurSel(hwnd);

	hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_ACCESS);assert(hwnd);
	m_pSRFCompiler->m_Props.m_bOutputAccess=Button_GetCheck(hwnd)!=0;

	hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_HIGHPOLYMATID);assert(hwnd);
	m_pSRFCompiler->m_Props.m_bOutputCombo=Button_GetCheck(hwnd)!=0;

	hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_UNOCCDIR);assert(hwnd);
	m_pSRFCompiler->m_Props.m_bOutputUnoccAreaDir=Button_GetCheck(hwnd)!=0;

	hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_OUTPUTHORIZON);assert(hwnd);
	m_pSRFCompiler->m_Props.m_bOutputHorizonMap=Button_GetCheck(hwnd)!=0;

	hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_REDUCERES);assert(hwnd);
	m_pSRFCompiler->m_Props.SetReduceResolutionFile(ComboBox_GetCurSel(hwnd)-2);

	hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_EXPANDBORDER);assert(hwnd);
	switch(ComboBox_GetCurSel(hwnd))
	{
		case 0: m_pSRFCompiler->m_Props.SetExpandBorder(0);break;
		case 1: m_pSRFCompiler->m_Props.SetExpandBorder(1);break;
		case 2: m_pSRFCompiler->m_Props.SetExpandBorder(4);break;
		case 3: m_pSRFCompiler->m_Props.SetExpandBorder(8);break;
		case 4: m_pSRFCompiler->m_Props.SetExpandBorder(16);break;
		default: assert(0);
	}

	m_BumpPanel.GetDataFromDialog(m_pSRFCompiler->m_Props);
}


void CSRFUserDialog::UpdateOccBrightenupStrength()
{
	HWND hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_UNOCCBRIGHTENUP);assert(hwnd);

	char str[80];
	sprintf(str,"%.2f",m_pSRFCompiler->m_Props.m_OccmapBrightenup);

	SetWindowText(hwnd,str);
}


void CSRFUserDialog::SetDataToDialog( const bool bInitalUpdate )
{
	HWND hwnd;

	hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_NORMALOUTPUTTYPE);assert(hwnd);
	SendMessage(hwnd,CB_SETCURSEL,m_pSRFCompiler->m_Props.m_eNormalOutputType,0);	

	hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_NORMALOUTPUTSPACE);assert(hwnd);
	SendMessage(hwnd,CB_SETCURSEL,m_pSRFCompiler->m_Props.m_eUsedSpaceType,0);	

	hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_DISPLACEOUTPUTTYPE);assert(hwnd);
	SendMessage(hwnd,CB_SETCURSEL,m_pSRFCompiler->m_Props.m_eDisplaceOutputType,0);	

	hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_ACCESS);assert(hwnd);
	Button_SetCheck(hwnd,m_pSRFCompiler->m_Props.m_bOutputAccess?1:0);

	hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_HIGHPOLYMATID);assert(hwnd);
	Button_SetCheck(hwnd,m_pSRFCompiler->m_Props.m_bOutputCombo?1:0);

	hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_UNOCCDIR);assert(hwnd);
	Button_SetCheck(hwnd,m_pSRFCompiler->m_Props.m_bOutputUnoccAreaDir?1:0);

	hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_OUTPUTHORIZON);assert(hwnd);
	Button_SetCheck(hwnd,m_pSRFCompiler->m_Props.m_bOutputHorizonMap?1:0);

	hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_REDUCERES);assert(hwnd);
	SendMessage(hwnd,CB_SETCURSEL,m_pSRFCompiler->m_Props.GetReduceResolutionFile()+2,0);

	hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_EXPANDBORDER);assert(hwnd);
	switch(m_pSRFCompiler->m_Props.GetExpandBorder())
	{
		case 0: SendMessage(hwnd,CB_SETCURSEL,0,0);break;
		case 1: SendMessage(hwnd,CB_SETCURSEL,1,0);break;
		case 4: SendMessage(hwnd,CB_SETCURSEL,2,0);break;
		case 8: SendMessage(hwnd,CB_SETCURSEL,3,0);break;
		case 16: SendMessage(hwnd,CB_SETCURSEL,4,0);break;
		default: assert(0);SendMessage(hwnd,CB_SETCURSEL,0,0);break;
	}

	m_BumpPanel.SetDataToDialog(m_pSRFCompiler->m_Props,true,bInitalUpdate);

	UpdateOccBrightenupStrength();
}


void CSRFUserDialog::CreateDialogItems()
{
	// tab control
	{
		HWND hwnd=GetDlgItem(m_hWindow,IDC_PROPTAB);assert(hwnd);
		TCITEM tie; 
		RECT wnp;

		tie.mask = TCIF_TEXT | TCIF_IMAGE; 
		tie.iImage = -1; 

		tie.pszText = "SRF Additional Bump"; 
		TabCtrl_InsertItem(hwnd,0,&tie);

		tie.pszText = "SRF Options"; 
		TabCtrl_InsertItem(hwnd,0,&tie);

		GetWindowRect(hwnd,&wnp);

		// sub windows ------------------

		m_BumpPanel.InitDialog(m_hWindow,true);

		{
			m_hTab_SRFOptions=CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_TAB_SRFOPTIONS),m_hWindow,WndProcRedirect);assert(m_hTab_SRFOptions);
			SetWindowPos(m_hTab_SRFOptions, HWND_TOP, wnp.left, wnp.top, 0,0,SWP_NOSIZE); 
		}

/*
		{
			tie.pszText = "File Output"; 
			TabCtrl_InsertItem(hwnd,1,&tie);
		}
*/
		// -------------------------

		// make dialog visible and set right tab
		SetPropertyTab(0);
	}
		
	{
		HWND hwnd;

		// SRF Info ---------------------

		hwnd=GetDlgItem(m_hWindow,IDC_SRFFILEINFO);assert(hwnd);

		string sAllLines;

		for(uint32 dwI=0;;++dwI)
		{
			char *szLine = m_pSRFCompiler->m_InputSRFFile.GetInfoString(dwI);

			if(szLine)
			{
				sAllLines += szLine;
				sAllLines += "\r\n";
			}
			else
				break;
		}

		SetWindowText(hwnd,sAllLines.c_str());
	}


	// normal output type
	{
		HWND hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_NORMALOUTPUTTYPE);assert(hwnd);

		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"R8G8B8 (typical)");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"R8G8");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"R16G16");
	}

	// expand border
	{
		HWND hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_EXPANDBORDER);assert(hwnd);

		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"none");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"1 pixel");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"4 pixel");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"8 pixel");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"16 pixel");
	}

	// normal output type
	{
		HWND hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_NORMALOUTPUTSPACE);assert(hwnd);

		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"object space");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"tangent space A (Possible sheared)");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"tangent space B (Orthogonalized)");
	}

	// displace output type
	{
		HWND hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_DISPLACEOUTPUTTYPE);assert(hwnd);

		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"none");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"linear signed");
	}

	// accessibility
	{
		HWND hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_ACCESS);assert(hwnd);

		EnableWindow(hwnd, m_pSRFCompiler->IsHorizonInformationAvailable());
	}

	// combo
	{
		HWND hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_HIGHPOLYMATID);assert(hwnd);

		EnableWindow(hwnd, m_pSRFCompiler->m_InputSRFFile.IsThereHighPolyMaterialId());
	}

	// unoccluded area direction
	{
		HWND hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_UNOCCDIR);assert(hwnd);

		EnableWindow(hwnd, m_pSRFCompiler->IsHorizonInformationAvailable());
	}
/* not yet working so UI disabled
	// horizonmap
	{
		HWND hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_OUTPUTHORIZON);assert(hwnd);

		EnableWindow(hwnd, m_pSRFCompiler->IsHorizonInformationAvailable());
	}
*/

	// horizon brighten up
	{
		HWND hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_UNOCCBRIGHTENUP);assert(hwnd);

		EnableWindow(hwnd, m_pSRFCompiler->IsHorizonInformationAvailable());
	}

	// reduce resolution
	{
		HWND hwnd=GetDlgItem(m_hTab_SRFOptions,IDC_SRFO_REDUCERES);assert(hwnd);

		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"-2");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"-1");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)" 0 (no change)");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)" 1 (1/4 memory)");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)" 2 (1/16 memory)");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)" 3 (1/64 memory)");
	}
}


void CSRFUserDialog::SetPropertyTab( const int iniNo )
{
	assert(iniNo>=0 && iniNo<=1);
	assert(m_hTab_SRFOptions);
	assert(m_BumpPanel.GetHWND());

	HWND hwnd=GetDlgItem(m_hWindow,IDC_PROPTAB);assert(hwnd);

	TabCtrl_SetCurSel(hwnd,iniNo);

	ShowWindow(m_hTab_SRFOptions,iniNo==0 ? SW_SHOW:SW_HIDE);
	ShowWindow(m_BumpPanel.GetHWND(),iniNo==1 ? SW_SHOW:SW_HIDE);
}


// WndProc vom Treiber auswahl Dialog
BOOL CALLBACK CSRFUserDialog::WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	CSRFUserDialog *This=(CSRFUserDialog *)(LONG_PTR)GetWindowLongPtr(hWnd,DWL_USER);

	switch(uMsg)
	{
		case WM_NOTIFY:
		{
			WORD wID=LOWORD(wParam);

			switch(wID)
			{
				case IDC_PROPTAB:
				{
					HWND hwnd=GetDlgItem(hWnd,IDC_PROPTAB);						assert(hwnd);
					int iPage = TabCtrl_GetCurSel(hwnd); 

					This->SetPropertyTab(iPage);
				}
				return(TRUE);
			}
		}
		return(FALSE);

		case WM_PAINT:											//
			{
				PAINTSTRUCT paint;
				HDC hdc = BeginPaint(hWnd, &paint);
				This->Paint(hdc);
				EndPaint(hWnd, &paint);
				break;
			}

		case WM_COMMAND:											//
			{
				WORD wID=LOWORD(wParam);

				switch(wID)
				{
//					case IDCREATEOUTPUT:
//						This->m_pSRFCompiler->RunWithProperties();
//						MessageBox(hWnd,"Output was created","Crytek Resource Compiler",MB_OK);
//						return TRUE;

					case IDC_SRFO_ADDBUMP_CHOOSE:
						This->m_BumpPanel.ChooseAddBump(hWnd);
						return TRUE;

					case IDOK:
						This->GetDataFromDialog();
						// first update the settings in the file
						if(!This->m_pSRFCompiler->UpdateAndSaveConfig())
							MessageBox(0,"Error while saving the settings (write protected?)","ResourceCompiler Image Error",MB_OK);
						else
						{
							// then compile with the settings (as this might change the input data (e.g. expand border)
							This->m_pSRFCompiler->RunWithProperties();
							This->m_bQuitting=true;
						}
						return TRUE;

					case IDCANCEL:
						This->m_bQuitting=true;
						return TRUE;

					case IDC_SRFFILEINFO:
					case IDC_SRFO_ACCESS:
					case IDC_SRFO_UNOCCDIR:
					case IDC_SRFO_OUTPUTHORIZON:
					case IDC_HIGHPOLYMATID:
						return TRUE;
//					default:
//						assert(0);
				}
			}
			return TRUE;

		case WM_CLOSE:
			This->m_bQuitting=true;
			return TRUE;
	}

	//	return DefWindowProc( hWnd, uMsg, wParam, lParam );
	return FALSE;
}



void CSRFUserDialog::UpdateWindowTitle()
{
	assert(m_pSRFCompiler);

	string title = string(m_pSRFCompiler->m_pCC->sourceFileFinal);

	SetWindowText(m_hWindow,title.c_str());
}

void CSRFUserDialog::Paint( HDC hdc )
{
	assert(m_pSRFCompiler);

	const CSRFData *pSRFData = &m_pSRFCompiler->m_InputSRFFile;

	if(pSRFData->GetWidth())
	{	
		uint32 dwMaxPreviewSize = 128;
		uint32 dwPreviewSizeY = pSRFData->GetHeight();				

		if(dwPreviewSizeY>dwMaxPreviewSize)
			dwPreviewSizeY=dwMaxPreviewSize;

		std::vector<uint32>	Mem;

		uint32 dwPreviewSizeX = (dwPreviewSizeY*pSRFData->GetWidth())/pSRFData->GetHeight();

		if(dwPreviewSizeX>dwMaxPreviewSize)
		{
			dwPreviewSizeX=dwMaxPreviewSize;
			dwPreviewSizeY=(dwPreviewSizeX*pSRFData->GetHeight())/pSRFData->GetWidth();
		}





		Mem.resize(dwPreviewSizeX*dwPreviewSizeY);

		uint32 dwPreviewId=0;

		for(;;++dwPreviewId)
		{
			if(!pSRFData->RequestPreview(dwPreviewSizeX,dwPreviewSizeY,0,dwPreviewSizeY,&Mem[0],dwPreviewId))
				break;

			DWORD mem[sizeof(BITMAPINFOHEADER)/4+3];	//3 DWORDS for the RGB-Masks
			BITMAPINFO *lpbmi=(BITMAPINFO *)mem; 	

			lpbmi->bmiHeader.biSize   = sizeof(BITMAPINFOHEADER);
			lpbmi->bmiHeader.biWidth  = dwPreviewSizeX;
			lpbmi->bmiHeader.biHeight = (int)dwPreviewSizeY;
			lpbmi->bmiHeader.biPlanes = 1;
			lpbmi->bmiHeader.biBitCount=32;
			lpbmi->bmiHeader.biCompression=BI_BITFIELDS;
			lpbmi->bmiHeader.biSizeImage=0;
			lpbmi->bmiHeader.biXPelsPerMeter=1;
			lpbmi->bmiHeader.biYPelsPerMeter=1;
			lpbmi->bmiHeader.biClrUsed=0;
			lpbmi->bmiHeader.biClrImportant=0;

			mem[sizeof(BITMAPINFOHEADER)/4+0]=0x00ff0000;
			mem[sizeof(BITMAPINFOHEADER)/4+1]=0x0000ff00;
			mem[sizeof(BITMAPINFOHEADER)/4+2]=0x000000ff;

			SetDIBitsToDevice(hdc,dwPreviewId*(dwPreviewSizeX+1),0,lpbmi->bmiHeader.biWidth,lpbmi->bmiHeader.biHeight,0,0,0,-lpbmi->bmiHeader.biHeight,&Mem[0],lpbmi,DIB_RGB_COLORS);

		}			

	}
}