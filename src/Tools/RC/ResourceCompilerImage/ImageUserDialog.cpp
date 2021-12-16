#include "StdAfx.h"
#include "ImageUserDialog.h"				// CImageUserDialog
#include "resource.h"								// IDD_ ...
#include <assert.h>									// assert()
#include "ImageCompiler.h"					// CImageCompiler
#include "RelativeMouseInput.h"			// CRelativeMouseInput
#include <windowsx.h>								// ComboBox_GetCurSel
#include "IRCLog.h"									// IRCLog
#include "tools.h"									// CenterWindow()

#include <commctrl.h>								// TCITEM
#include <vector>
#include "ICfgFile.h"
#include <atlbase.h>
#include <atlwin.h>



// Class name for this application's window class.
#define WINDOW_CLASSNAME      "UserDialog"

// constructor
CImageUserDialog::CImageUserDialog() :m_iPreviewWidth(256), m_iPreviewHeight(256), m_bQuitting(false)
{
	m_pImageCompiler=0;
	m_hTab_Saveformat=0;
	m_hTab_Advanced=0;
	m_hWindow=0;
	m_bDialogIsUpdating=false;
	m_hWindowThread=0;
}

// destructor
CImageUserDialog::~CImageUserDialog()
{
	assert(!m_pImageCompiler);			// should be 0 outside of DoModal call
}




bool CImageUserDialog::DoModal( CImageCompiler *inpImageCompiler )
{
	m_pImageCompiler=inpImageCompiler;

	m_eWorkerAction = WorkerActionNone;

	// Start the thread that will control the window and handle the message loop.
	m_hWindowThread = CreateThread(
		0,											//LPSECURITY_ATTRIBUTES lpThreadAttributes,
		0,											//SIZE_T dwStackSize,
		ThreadStart_Static,			//LPTHREAD_START_ROUTINE lpStartAddress,
		this,										//LPVOID lpParameter,
		0,											//DWORD dwCreationFlags,
		0);											//LPDWORD lpThreadId);

	// Loop until the dialog thread has exitted.
	bool bContinue = true;
	while (bContinue)
	{
		// Check whether the ui thread is finished - if so, we can return.
		DWORD dwReturnCode = 0;
		GetExitCodeThread(m_hWindowThread, &dwReturnCode);
		if (dwReturnCode != STILL_ACTIVE)
			bContinue = false;
		else
		{
			// Check whether we need to update the preview.
			WorkerAction eAction = CheckAndResetWorkerAction();
			switch (eAction)
			{
			case WorkerActionNone:
				break;

			case WorkerActionUpdatePreview:
				{
					//clear progress before bar is drawn
					m_pImageCompiler->m_Progress.Start();

					{
						LOCK_MONITOR();

						m_PreviewData.bIsUpdating = true;

						if (m_PreviewData.bDelayUpdate)
						{
							TriggerRepaint();
							continue;
						}
					}

					GetDataFromDialog();
					UpdatePixelFormatDesc();
					SetDataToDialog();		// show settings in dialog

					HDC hPreviewDC = GetDC(this->m_hWindow);
					Draw(hPreviewDC);
					ReleaseDC(this->m_hWindow, hPreviewDC);

					UpdatePreview(true);

					{
						LOCK_MONITOR();
						m_PreviewData.bIsUpdating = false;
					}

					TriggerRepaint();
				
					break;
				}

			case WorkerActionGenerateOutputAndQuit:
			case WorkerActionGenerateOutput:
				{
					GetDataFromDialog();
					m_pImageCompiler->AutoOptimize();			// can change the preset
					SetDataToDialog(true);								// might need to update preset in UI
					bool bRet = m_pImageCompiler->RunWithProperties(true);

					if(bRet)
					{
						if(!m_pImageCompiler->UpdateAndSaveConfig())
							MessageBox(this->m_hWindow,"Error while saving the config file (write protected?)","ResourceCompiler Image Error",MB_OK);
						else
						{
							if (eAction == WorkerActionGenerateOutputAndQuit)
								m_bQuitting=true;
							 else
								RCLog("Output was generated.");
						}
					}
					else 
						RCLog("No Output was generated.");

					break;
				}

			case WorkerActionSaveConfigAndQuit:
			case WorkerActionSaveConfig:
				{
					GetDataFromDialog();
					if(!m_pImageCompiler->UpdateAndSaveConfig())
						MessageBox(this->m_hWindow,"Error while saving the config file (write protected?)","ResourceCompiler Image Error",MB_OK);
					else
					{
						if (eAction == WorkerActionSaveConfigAndQuit)
							m_bQuitting=true;
						else
						{
							MessageBox(this->m_hWindow,"Config saved","ResourceCompiler Image",MB_OK);
						}
					}
					break;
				}

			case WorkerActionPaint:
				{
					HDC hPreviewDC = GetDC(this->m_hWindow);
					Draw(hPreviewDC);
					ReleaseDC(this->m_hWindow, hPreviewDC);
					break;
				}

			case WorkerActionQuit:
				{
					m_bQuitting = true;
				}
				break;

			case WorkerActionNoCustomSettings:
				{
					SetDataToDialog();				// reset to old setting
					MessageBox(this->m_hWindow,"Custom settings are not enabled","ResourceCompilerImage",MB_OK);
				}
				break;
			}

			// Wait some time to avoid hogging the CPU.
			Sleep(100);
		}
	}

	m_pImageCompiler=0;

	return(true);
}


void CImageUserDialog::CreateDialogItems()
{
	LOCK_MONITOR();

		// tab control
	{
		HWND hwnd=GetDlgItem(m_hWindow,IDC_PROPTAB);assert(hwnd);
		TCITEM tie; 
		RECT wnp;

    tie.mask = TCIF_TEXT | TCIF_IMAGE; 
    tie.iImage = -1; 

    tie.pszText = "RGB Bump2Normal"; 
    TabCtrl_InsertItem(hwnd,0,&tie);

		tie.pszText = "File Output"; 
    TabCtrl_InsertItem(hwnd,1,&tie);

    tie.pszText = "Advanced"; 
    TabCtrl_InsertItem(hwnd,2,&tie);

    tie.pszText = "Alpha UsedAsBump"; 
    TabCtrl_InsertItem(hwnd,3,&tie);

		tie.pszText = "MIP Control"; 
		TabCtrl_InsertItem(hwnd,4,&tie);

		GetWindowRect(hwnd,&wnp);

		// sub windows
		m_Bump2NormalPanel.InitDialog(m_hWindow,false);
		m_Bump2NormalPanel.m_bumpStrengthValue.AddListener(this);
		m_Bump2NormalPanel.m_bumpBlurValue.AddListener(this);
		m_Bump2NormalPanel.m_bumpInvertValue.AddListener(this);

		m_AlphaAsBumpPanel.InitDialog(m_hWindow,false);
		m_AlphaAsBumpPanel.m_bumpStrengthValue.AddListener(this);
		m_AlphaAsBumpPanel.m_bumpBlurValue.AddListener(this);
		m_AlphaAsBumpPanel.m_bumpInvertValue.AddListener(this);

		m_MIPControlPanel.InitDialog(m_hWindow,false);
		for (int i = 0; i < CUIMIPControlPanel::NUM_CONTROLLED_MIP_MAPS; ++i)
			m_MIPControlPanel.m_MIPAlphaControl[i].GetDocument().AddListener(this);

		m_MIPControlPanel.m_MIPSharpenControl.GetDocument().AddListener(this);
		
		m_hTab_Saveformat=CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_TAB_SAVEFORMAT),m_hWindow,WndProcRedirect); assert(m_hTab_Saveformat);
		SetWindowPos(m_hTab_Saveformat, HWND_TOP, wnp.left, wnp.top, 0,0,SWP_NOSIZE); 

		m_hTab_Advanced=CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_TAB_ADVANCED),m_hWindow,WndProcRedirect); assert(m_hTab_Advanced);
		SetWindowPos(m_hTab_Advanced, HWND_TOP, wnp.left, wnp.top, 0,0,SWP_NOSIZE); 

		// make dialog visible and set right tab
		SetPropertyTab(1);
	}

	// templates
	{
		HWND hwnd=GetDlgItem(m_hWindow,IDC_TEMPLATECOMBO);assert(hwnd);

		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"<CUSTOM>");
		
		for(int i = 1;; i++)
		{
			// I think it is OK to access GetSectionName without synchronization, since it won't change.
			const char *name = m_pImageCompiler->m_pCC->presets->GetSectionName(i);
			if(!name) break;
			SendMessage(hwnd, CB_ADDSTRING, 0,(LPARAM)name);
		};

		SendMessage(hwnd,CB_SETCURSEL,0,0);		// first 0 is setting the used template
	}

	// preview mode
	{
		HWND hwnd=GetDlgItem(m_hWindow,IDC_PREVIEWMODE);assert(hwnd);

		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"RGB");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"Alpha");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"RGB Alpha");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"Debug Normals");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"RGB*A (HDR)");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"AlphaTest");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"mCIE2RGB");

		SendMessage(hwnd,CB_SETCURSEL,0,0);		// first 0 is setting the used template
	}

	// pixelformats
	{
		HWND hwnd=GetDlgItem(m_hTab_Saveformat,IDC_PIXELFORMAT);assert(hwnd);

		for(int i=0;;i++)
		{
			int iPixelFormat=CPixelFormats::GetPixelFormatNoFromDestNo(i);				if(iPixelFormat==-1)break;

			SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)CPixelFormats::GetPixelFormatName(iPixelFormat));
		}

		UpdatePixelFormatDesc();
	}


	// mipmaps
	{
		HWND hwnd=GetDlgItem(m_hTab_Saveformat,IDC_MIPMAPS);assert(hwnd);

		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"none (0)");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"max, tiled (1)");
		//SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"max, mirrored (2)");
	}

	// reduce resolution
	{
		HWND hwnd=GetDlgItem(m_hTab_Saveformat,IDC_REDUCERES);assert(hwnd);

		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"-2");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)"-1");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)" 0 (no change)");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)" 1 (1/4 memory)");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)" 2 (1/16 memory)");
		SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)" 3 (1/64 memory)");
	}
}



void CImageUserDialog::SetDataToDialog( const bool bInitalUpdate )
{
	m_bDialogIsUpdating=true;

	HWND hwnd;
	bool isnotpreset = m_pImageCompiler->m_Props.GetPreset()=="";

	bool enablecustoms = isnotpreset && m_pImageCompiler->m_Props.GetUserDialogCustom();

	// preview part in main window -------------------------------------

	hwnd=GetDlgItem(m_hWindow,IDC_PREVIEWMODE);assert(hwnd);
	SendMessage(hwnd,CB_SETCURSEL,m_pImageCompiler->m_Props.m_ePreviewMode,0);

	hwnd=GetDlgItem(m_hWindow,IDC_PREVIEWFILTERED);assert(hwnd);
	Button_SetCheck(hwnd,m_pImageCompiler->m_Props.m_bPreviewFiltered?1:0);

	hwnd=GetDlgItem(m_hWindow,IDC_PREVIEWTILED);assert(hwnd);
	Button_SetCheck(hwnd,m_pImageCompiler->m_Props.m_bPreviewTiled?1:0);

	hwnd=GetDlgItem(m_hWindow,IDC_PREVIEWMODETEXT);assert(hwnd);
	SetWindowText(hwnd,m_pImageCompiler->m_Props.GetPreviewModeText());

	// main window -----------------------------------------------------

	hwnd=GetDlgItem(m_hWindow,IDC_TEMPLATECOMBO);assert(hwnd);
	SendMessage(hwnd,CB_SETCURSEL,m_pImageCompiler->m_pCC->presets->Find(m_pImageCompiler->m_Props.GetPreset()),0);

	m_Bump2NormalPanel.SetDataToDialog(m_pImageCompiler->m_Props.m_Bump2Normal,enablecustoms,bInitalUpdate);
	m_AlphaAsBumpPanel.SetDataToDialog(m_pImageCompiler->m_Props.m_AlphaAsBump,m_pImageCompiler->InputHasAlpha(),bInitalUpdate);
	m_MIPControlPanel.SetDataToDialog(m_pImageCompiler->m_Props,bInitalUpdate);

	// sub window: advanced --------------------------------------------

	hwnd=GetDlgItem(m_hTab_Advanced,IDC_MAINTAINALPHACOVERAGE);assert(hwnd);
	Button_SetCheck(hwnd,m_pImageCompiler->m_Props.GetMaintainAlphaCoverage()?1:0);
	{
		string str;
		EnableWindow(hwnd,!m_pImageCompiler->m_pCC->config->Get("mc",str,eCP_PriorityNotFile));
	}

	hwnd=GetDlgItem(m_hTab_Advanced,IDC_MIPGAMMA);assert(hwnd);
	Button_SetCheck(hwnd,m_pImageCompiler->m_Props.GetGammaCorrectedMips()?1:0);
	{
		string str;
		EnableWindow(hwnd,!m_pImageCompiler->m_pCC->config->Get("mipgamma",str,eCP_PriorityNotFile));
	}

	hwnd=GetDlgItem(m_hTab_Advanced,IDC_SUPRESSENGINEREDUCE);assert(hwnd);
	Button_SetCheck(hwnd,m_pImageCompiler->m_Props.GetSupressEngineReduce()?1:0);
	{		
		string str;
		EnableWindow(hwnd,!m_pImageCompiler->m_pCC->config->Get("ser",str,eCP_PriorityNotFile));
	}

	hwnd=GetDlgItem(m_hTab_Advanced,IDC_FILEAUTOOPTIMIZE);assert(hwnd);
	Button_SetCheck(hwnd,m_pImageCompiler->m_Props.GetAutoOptimizeFile()?1:0);
	{
		string str;	
		EnableWindow(hwnd,!m_pImageCompiler->m_pCC->config->Get("autooptimizefile",str,eCP_PriorityNotFile));
	}

	hwnd=GetDlgItem(m_hTab_Advanced,IDC_MIPNORMALIZE);assert(hwnd);
	Button_SetCheck(hwnd,m_pImageCompiler->m_Props.GetMipRenormalize()?1:0);
	{
		string str;	
		EnableWindow(hwnd,!m_pImageCompiler->m_pCC->config->Get("mipnormalize",str,eCP_PriorityNotFile));
	}

	// sub window: save format

	hwnd=GetDlgItem(m_hTab_Saveformat,IDC_MIPMAPS);assert(hwnd);
	{	
		int iSel=0;

		if(m_pImageCompiler->m_Props.GetMipMaps())
			iSel=1;

		SendMessage(hwnd,CB_SETCURSEL,iSel,0);

		{
			string str;	
			EnableWindow(hwnd,!m_pImageCompiler->m_pCC->config->Get("mipmaps",str,eCP_PriorityNotFile));
		}
	}

	hwnd=GetDlgItem(m_hTab_Saveformat,IDC_REDUCERES);assert(hwnd);
	SendMessage(hwnd,CB_SETCURSEL,m_pImageCompiler->m_Props.GetReduceResolutionFile()+2,0);

	hwnd=GetDlgItem(m_hTab_Saveformat,IDC_PIXELFORMAT);assert(hwnd);
	SendMessage(hwnd,CB_SETCURSEL,CPixelFormats::GetDestNoFromPixelFormatNo(m_pImageCompiler->m_Props.GetDestPixelFormat(m_pImageCompiler->InputHasAlpha())),0);		// -1 clears the selection
	{
		string str;	
		EnableWindow(hwnd,!m_pImageCompiler->m_pCC->config->Get("pixelformat",str,eCP_PriorityNotFile));
	}

	m_bDialogIsUpdating=false;
}

bool CImageUserDialog::PreviewGenerationCanceled()
{
	LOCK_MONITOR();

	//check whether interface thread has been closed (window closed)
	{
		DWORD dwReturnCode = 0;
		GetExitCodeThread(m_hWindowThread, &dwReturnCode);
		if (dwReturnCode != STILL_ACTIVE)
			return true;
	}

	ZoomPreviewUpdateCheck();

	return (m_eWorkerAction!=WorkerActionNone && m_eWorkerAction!=WorkerActionPaint);
}

void CImageUserDialog::ZoomPreviewUpdateCheck()
{
	bool bUpdatePreview = false;

	//if currently updating preview data and the zoom is reduced so MIPs are not needed, cancel processing and trigger update
	if (m_PreviewData.bIsUpdating)
	{
		if (m_PreviewData.iCurMaxScale64 > m_PreviewData.iScale64 && m_PreviewData.iScale64 < DEFAULT_SCALE64)
			bUpdatePreview = true;
	}

	//if zoomed out from partial view, trigger update
	if (m_PreviewData.iCurMaxScale64 > m_PreviewData.iScale64 && m_PreviewData.iCurMaxScale64 > m_PreviewData.iInitialScale64)
		bUpdatePreview = true;

	//if zoomed in beyond previous measures, preview data must be updated (to calculate skipped MIPs)
	if (m_PreviewData.iCurMaxScale64 < m_PreviewData.iScale64 && m_PreviewData.iCurMaxScale64 < DEFAULT_SCALE64)
		bUpdatePreview = true;


	if (bUpdatePreview)
		TriggerUpdatePreview();
}

void CImageUserDialog::UpdateProgressBar(const float fProgress)
{
	assert(fProgress>=0 && fProgress<=1);

	if (PreviewGenerationCanceled())
		return;

	char inszTxt[6];
	sprintf_s(inszTxt, "%3.0f%%", fProgress*100.0f);

	RECT rectProgressBorder;
	rectProgressBorder.left = m_iPreviewWidth+9;
	rectProgressBorder.top = m_iPreviewHeight-14;
	rectProgressBorder.right = rectProgressBorder.left+m_iPreviewWidth;
	rectProgressBorder.bottom = m_iPreviewHeight;

	RECT rectProgressBar = rectProgressBorder;
	rectProgressBar.left += 1;
	rectProgressBar.top += 1;
	rectProgressBar.right -= 1;
	rectProgressBar.bottom -= 1;

	RECT rectProgressCount;
	rectProgressCount.left=m_iPreviewWidth+9;
	rectProgressCount.top=rectProgressBorder.top-1;
	rectProgressCount.right=rectProgressCount.left+m_iPreviewWidth;
	rectProgressCount.bottom=rectProgressCount.top+18;

	HDC hdc=GetDC(m_hWindow);
	FillRect(hdc,&rectProgressBorder,GetSysColorBrush(COLOR_BACKGROUND));
	FillRect(hdc,&rectProgressBar,GetSysColorBrush(COLOR_BTNSHADOW));
	rectProgressBar.right = rectProgressBar.left + (int)((m_iPreviewWidth-2)*fProgress);
	FillRect(hdc,&rectProgressBar,GetSysColorBrush(COLOR_SCROLLBAR));

	SetBkMode(hdc,TRANSPARENT);
	SetTextAlign(hdc,TA_CENTER);
	ExtTextOut(hdc,(rectProgressCount.left+rectProgressCount.right)/2,rectProgressCount.top,ETO_CLIPPED,&rectProgressCount,inszTxt,strlen(inszTxt),0);
	ReleaseDC(m_hWindow,hdc);
}

void CImageUserDialog::GetDataFromDialog()
{
	HWND hwnd;
	// preview part in main window

	m_pImageCompiler->m_Props.SetToDefault();

	hwnd=GetDlgItem(m_hWindow,IDC_PREVIEWMODE);assert(hwnd);
	m_pImageCompiler->m_Props.m_ePreviewMode=(EPreviewMode)ComboBox_GetCurSel(hwnd);

	hwnd=GetDlgItem(m_hWindow,IDC_PREVIEWFILTERED);assert(hwnd);
	m_pImageCompiler->m_Props.m_bPreviewFiltered=Button_GetCheck(hwnd)!=0;

	hwnd=GetDlgItem(m_hWindow,IDC_PREVIEWTILED);assert(hwnd);
	m_pImageCompiler->m_Props.m_bPreviewTiled=Button_GetCheck(hwnd)!=0;

	hwnd=GetDlgItem(m_hWindow,IDC_TEMPLATECOMBO);assert(hwnd);
	const char *pname = m_pImageCompiler->m_pCC->presets->GetSectionName(ComboBox_GetCurSel(hwnd));
	m_pImageCompiler->m_Props.SetPreset(pname ? pname : "");

	hwnd=GetDlgItem(m_hTab_Saveformat,IDC_REDUCERES);assert(hwnd);
	m_pImageCompiler->m_Props.SetReduceResolutionFile(ComboBox_GetCurSel(hwnd)-2);

	hwnd=GetDlgItem(m_hTab_Advanced,IDC_SUPRESSENGINEREDUCE);assert(hwnd);
	m_pImageCompiler->m_Props.SetSupressEngineReduce(Button_GetCheck(hwnd)!=0);

	hwnd=GetDlgItem(m_hTab_Advanced,IDC_FILEAUTOOPTIMIZE);assert(hwnd);
	m_pImageCompiler->m_Props.SetAutoOptimizeFile(Button_GetCheck(hwnd)!=0);

	m_Bump2NormalPanel.GetDataFromDialog(m_pImageCompiler->m_Props.m_Bump2Normal);
	m_AlphaAsBumpPanel.GetDataFromDialog(m_pImageCompiler->m_Props.m_AlphaAsBump);
	m_MIPControlPanel.GetDataFromDialog(m_pImageCompiler->m_Props);

	hwnd=GetDlgItem(m_hTab_Advanced,IDC_MAINTAINALPHACOVERAGE);assert(hwnd);
	m_pImageCompiler->m_Props.SetMaintainAlphaCoverage(Button_GetCheck(hwnd)!=0);

	hwnd=GetDlgItem(m_hTab_Advanced,IDC_MIPGAMMA);assert(hwnd);
	m_pImageCompiler->m_Props.SetGammaCorrectedMips(Button_GetCheck(hwnd)!=0);

	hwnd=GetDlgItem(m_hTab_Advanced,IDC_MIPNORMALIZE);assert(hwnd);
	m_pImageCompiler->m_Props.SetMipRenormalize(Button_GetCheck(hwnd)!=0);

	hwnd=GetDlgItem(m_hTab_Saveformat,IDC_PIXELFORMAT);assert(hwnd);
	m_pImageCompiler->m_Props.SetDestPixelFormat(CPixelFormats::GetPixelFormatNoFromDestNo(ComboBox_GetCurSel(hwnd)),m_pImageCompiler->InputHasAlpha());

	hwnd=GetDlgItem(m_hTab_Saveformat,IDC_MIPMAPS);assert(hwnd);
	m_pImageCompiler->m_Props.SetMipMaps(ComboBox_GetCurSel(hwnd)!=0);
}



void CImageUserDialog::UpdateWindowTitle()
{
	LOCK_MONITOR();

	assert(m_pImageCompiler);

	string Zoom;
	Zoom.Format("  Zoom:%d%%",(100*m_PreviewData.iScale64)/64);

	string filename;
	if(!m_pImageCompiler->m_pCC->config->Get("overwritefilename",filename))
	{
		filename = m_pImageCompiler->m_pCC->sourceFileFinal;
	}

	string title = string(filename) + Zoom;
	
	SetWindowText(m_hWindow,title.c_str());
}


//
void CImageUserDialog::Draw( HDC inHdc )
{
	LOCK_MONITOR();

	int iGap=4;
	int iTexWidth=(int)m_pImageCompiler->GetInputImageWidth();
	int iTexHeight=(int)m_pImageCompiler->GetInputImageHeight();

	RECT rec;

	rec.left=0;
	rec.top=0;
	rec.right=rec.left+m_iPreviewWidth;
	rec.bottom=rec.top+m_iPreviewHeight;

	// left side = original
	assert(m_pImageCompiler);
	if(!m_pImageCompiler->BlitTo(m_hWindow,rec,m_PreviewData.fShiftX,m_PreviewData.fShiftY,m_PreviewData.iScale64,true))
		FillRect(inHdc,&rec,GetSysColorBrush(COLOR_3DFACE));

	rec.left=m_iPreviewWidth+iGap*2+1;
	rec.right=rec.left+m_iPreviewWidth;

	// right side = destination
	if (m_PreviewData.bIsUpdating)
	{
		UpdateProgressBar(m_pImageCompiler->m_Progress.Get());
	}
	else
	{
		if(!m_pImageCompiler->BlitTo(m_hWindow,rec,m_PreviewData.fOldShiftX,m_PreviewData.fOldShiftY,m_PreviewData.iScale64,false))
			FillRect(inHdc,&rec,GetSysColorBrush(COLOR_3DFACE));
	}
}


void CImageUserDialog::MouseMessage( const DWORD indwButtons, const int iniRelX , const int iniRelY, int iniRelZ )
{
	//early out if no processing required because no mouse buttons are currently and previously pressed and not zooming
	if (!indwButtons && !m_PreviewData.dwMouseButtonsOld && !iniRelZ)
		return;

	assert(m_pImageCompiler);

	int iWidth=(int)m_pImageCompiler->GetInputImageWidth(),iHeight=(int)m_pImageCompiler->GetInputImageHeight(); 

	float fOldShiftX=m_PreviewData.fShiftX,fOldShiftY=m_PreviewData.fShiftY;
	int iOldScale64=m_PreviewData.iScale64;

	m_PreviewData.fShiftX-=iniRelX*(64.0f/m_PreviewData.iScale64/iWidth);
	m_PreviewData.fShiftY-=iniRelY*(64.0f/m_PreviewData.iScale64/iHeight);

	while(iniRelZ)
	{
		if(iniRelZ>0)
		{
			m_PreviewData.iScale64/=2;
			iniRelZ-=WHEEL_DELTA;
		}
		else
		{
			m_PreviewData.iScale64*=2;
			iniRelZ+=WHEEL_DELTA;
		}
	}

	if(m_PreviewData.iScale64<1)				m_PreviewData.iScale64=1;
	if(m_PreviewData.iScale64>16*16*4)	m_PreviewData.iScale64=16*16*4;

	bool bNoMovementPossible = m_pImageCompiler->ClampBlitOffset(m_iPreviewWidth,m_iPreviewHeight,m_PreviewData.fShiftX,m_PreviewData.fShiftY,m_PreviewData.iScale64);


	LOCK_MONITOR();

	m_PreviewData.bDelayUpdate = false;


	if ((fOldShiftX!=m_PreviewData.fShiftX || fOldShiftY!=m_PreviewData.fShiftY) && !((m_PreviewData.dwMouseButtonsOld&0x0001) || (m_PreviewData.dwMouseButtonsOld&0x0002))
		&& !m_PreviewData.bIsUpdating)
	{
		m_PreviewData.fOldShiftX = m_PreviewData.fShiftX;
		m_PreviewData.fOldShiftY = m_PreviewData.fShiftY;
	}

	if(fOldShiftX!=m_PreviewData.fShiftX || fOldShiftY!=m_PreviewData.fShiftY || iOldScale64!=m_PreviewData.iScale64)
	{
		UpdateWindowTitle();			// Zoom:%d

		RECT rect;

		GetClientRect(m_hWindow,&rect);
		rect.bottom=m_iPreviewHeight;

		rect.right=m_iPreviewWidth;

		// update window
		InvalidateRect(m_hWindow,&rect,bNoMovementPossible);
		UpdateWindow(m_hWindow);


		if (m_PreviewData.bIsUpdating)
			TriggerUpdatePreview();
		else
			ZoomPreviewUpdateCheck();

		if ((m_PreviewData.dwMouseButtonsOld&0x0001) || (m_PreviewData.dwMouseButtonsOld&0x0002))
			m_PreviewData.bDelayUpdate = true;
	}

	if (((m_PreviewData.dwMouseButtonsOld&0x0001) || (m_PreviewData.dwMouseButtonsOld&0x0002)) && !((indwButtons&0x0001) || (indwButtons&0x0002))
		&& m_PreviewData.iScale64>m_PreviewData.iInitialScale64)
	{
		float deltaX = fabsf(m_PreviewData.fOldShiftX-m_PreviewData.fShiftX);
		float deltaY = fabsf(m_PreviewData.fOldShiftY-m_PreviewData.fShiftY); 

		if (deltaX > 0.0001f || deltaY > 0.0001f)
			TriggerUpdatePreview();

		//if (m_PreviewData.fOldShiftX!=m_PreviewData.fShiftX || m_PreviewData.fOldShiftX!=m_PreviewData.fShiftY)
		//	TriggerUpdatePreview();
	}

	m_PreviewData.dwMouseButtonsOld = indwButtons;
}



void CImageUserDialog::UpdatePixelFormatDesc()
{
	HWND hwnds=GetDlgItem(m_hTab_Saveformat,IDC_PIXELFORMATDESC);			assert(hwnds);

	SetWindowText(hwnds,CPixelFormats::GetPixelFormatDesc(m_pImageCompiler->m_Props.GetDestPixelFormat(m_pImageCompiler->InputHasAlpha()  )));
}



void CImageUserDialog::UpdatePreview( const bool bFullConvert )
{
	m_PreviewData.iCurMaxScale64 =  m_PreviewData.iScale64;

	// preview images
	{
		RECT rect;

		GetClientRect(m_hWindow,&rect);
		rect.bottom=m_iPreviewHeight;

		bool bErase=false;

		if(bFullConvert)
		{
			m_pImageCompiler->AutoOptimize();			// can change the preset
			SetDataToDialog(true);								// might need to update preset in UI

			bErase=m_pImageCompiler->RunWithProperties(false);			// do image conversion
		}
		
		//InvalidateRect(m_hWindow,&rect,bErase);					// don't erase background
	}

	// info text
	if (!PreviewGenerationCanceled())
	{
		string sRightInfo=m_pImageCompiler->GetInfoStringUI(false);

		HWND hwnd2=GetDlgItem(m_hWindow,IDC_RIGHTPREVIEWINFO);		assert(hwnd2);
		SetWindowText(hwnd2,sRightInfo.c_str());

		m_PreviewData.fOldShiftX = m_PreviewData.fShiftX;
		m_PreviewData.fOldShiftY = m_PreviewData.fShiftY;
	}
}


void CImageUserDialog::SetPropertyTab( const int iniNo )
{
	LOCK_MONITOR();

	assert(iniNo>=0 && iniNo<=4);
	assert(m_Bump2NormalPanel.GetHWND());
	assert(m_hTab_Saveformat);
	assert(m_hTab_Advanced);
	assert(m_AlphaAsBumpPanel.GetHWND());
	assert(m_MIPControlPanel.GetHWND());

	HWND hwnd=GetDlgItem(m_hWindow,IDC_PROPTAB);assert(hwnd);
	
	TabCtrl_SetCurSel(hwnd,iniNo);

	ShowWindow(m_Bump2NormalPanel.GetHWND(),iniNo==0 ? SW_SHOW:SW_HIDE);
	ShowWindow(m_hTab_Saveformat,iniNo==1 ? SW_SHOW:SW_HIDE);
	ShowWindow(m_hTab_Advanced,iniNo==2 ? SW_SHOW:SW_HIDE);
	ShowWindow(m_AlphaAsBumpPanel.GetHWND(),iniNo==3 ? SW_SHOW:SW_HIDE);
	ShowWindow(m_MIPControlPanel.GetHWND(),iniNo==4 ? SW_SHOW:SW_HIDE);
}

int CImageUserDialog::GetPreviewReduceResolution()
{
	const int factor = m_PreviewData.iCurMaxScale64 << m_pImageCompiler->m_Props.GetReduceResolution();

	if (factor >= DEFAULT_SCALE64)
		return 0;

	return m_pImageCompiler->m_Props.GetMipMaps() ? ((int)(log((float)DEFAULT_SCALE64)/LN2) - (int)(log((float)factor)/LN2)) : 0;
}

RECT CImageUserDialog::GetPreviewRectangle( const uint32 mipLevel )
{
	RECT rect;

	const int iWidth = m_PreviewData.iOrigWidth >> mipLevel;
	const int iHeight = m_PreviewData.iOrigHeight >> mipLevel;
	const int iScale = m_PreviewData.iCurMaxScale64 << mipLevel;

	float fCoordinateShiftX = 0.5f;
	float fCoordinateShiftY = 0.5f;
	if (m_PreviewData.iCurMaxScale64 > m_PreviewData.iInitialScale64)
	{
		float fScaleShiftX = 1.0f;
		float fScaleShiftY = 1.0f;
		if (iWidth != iHeight)
		{
			if (iWidth < iHeight)
				fScaleShiftX = (float)(iHeight/iWidth);
			else
				fScaleShiftY = (float)(iWidth/iHeight);
		}
	
		const float fTemp = m_PreviewData.iInitialScale64/(float)m_PreviewData.iCurMaxScale64;
		fCoordinateShiftX *= fTemp * fScaleShiftX;
		fCoordinateShiftY *= fTemp * fScaleShiftY;
	}

	const int iTemp = (MAX(m_iPreviewWidth,m_iPreviewHeight)*DEFAULT_SCALE64)/iScale;

	rect.left = (long)((m_PreviewData.fShiftX-fCoordinateShiftX)*iWidth);
	rect.right = rect.left + iTemp;
	rect.right = (rect.right > iWidth) ? iWidth : rect.right;
	rect.top = (long)((m_PreviewData.fShiftY-fCoordinateShiftY)*iHeight);
	rect.bottom = rect.top + iTemp;
	rect.bottom = (rect.bottom > iHeight) ? iHeight : rect.bottom;

	//additional border pixels for mipmap generation
	const int maxBorderSize = 16;
	rect.left -= MIN(maxBorderSize, rect.left);
	rect.right += MIN(maxBorderSize, iWidth-rect.right);
	rect.top -= MIN(maxBorderSize, rect.top);
	rect.bottom += MIN(maxBorderSize, iHeight-rect.bottom);

	// expand to include full 4x4 blocks
	rect.left = rect.left & (~0x3);
	rect.top = rect.top & (~0x3);
	rect.right = (rect.right+3) & (~0x3);
	rect.bottom = (rect.bottom+3) & (~0x3);

	return rect;
}

BOOL CALLBACK CImageUserDialog::WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	CImageUserDialog *This=(CImageUserDialog *)(LONG_PTR)GetWindowLongPtr(hWnd,DWL_USER);

	switch(uMsg)
	{
		case WM_HSCROLL:
			if(	LOWORD(wParam)==TB_PAGEDOWN || LOWORD(wParam)==TB_PAGEUP ||				//
					LOWORD(wParam)==TB_THUMBTRACK ||																	// mouse drag
					LOWORD(wParam)==TB_LINEDOWN || LOWORD(wParam)==TB_LINEUP ||				//
					LOWORD(wParam)==TB_THUMBPOSITION)																	// mousewheel
			{
				This->TriggerUpdatePreview();
			}
			return TRUE;

		case WM_NOTIFY:
			{
				WORD wID=LOWORD(wParam);

				switch(wID)
				{
					case IDC_PROPTAB:
						{
							LockHandle lh = This->Lock();

							HWND hwnd=GetDlgItem(hWnd,IDC_PROPTAB);						assert(hwnd);
							int iPage = TabCtrl_GetCurSel(hwnd); 

							This->SetPropertyTab(iPage);
						}
						return TRUE;
				}
			}
			return(FALSE);

		case WM_COMMAND:											//
			{
				LockHandle lh = This->Lock();

				WORD wID=LOWORD(wParam);

				switch(wID)
				{
					// TODO: Move generation to main thread.
					case IDC_GENERATEOUTPUT:
						This->TriggerGenerateOutput(false);
						return(TRUE);

					case IDOK:
						This->TriggerSaveConfig(true);
						return(TRUE);

					case IDCANCEL:
//						PostQuitMessage(0);
						This->TriggerCancel();
						return(TRUE);

					case IDC_ZOOMIN:
						This->MouseMessage(0,0,0,-WHEEL_DELTA);
						return(TRUE);

					case IDC_ZOOMOUT:
						This->MouseMessage(0,0,0,WHEEL_DELTA);
						return(TRUE);

					case IDC_REDUCERES:											// reduce resolution has changed
						if(HIWORD(wParam)==CBN_SELCHANGE)
						{
							This->TriggerUpdatePreview();
						}
						return(TRUE);

					// combo box change, update all
					case IDC_TEMPLATECOMBO:
					case IDC_MIPMAPS:												// mipmaps has changed
					case IDC_PIXELFORMAT:										// pixelformat has changed
					case IDC_NM_FILTERTYPE:									// bump2normal filter type has changed
						if(HIWORD(wParam)!=CBN_SELCHANGE)
							return TRUE;

					// check box change, update all		
					case IDC_SUPRESSENGINEREDUCE:						// image flags have changed
					case IDC_FILEAUTOOPTIMIZE:							// autooptimize has changed
					case IDC_MIPGAMMA:											// mipgamma has changed
					case IDC_MIPNORMALIZE:									// mipnormalize has changed
					case IDC_MAINTAINALPHACOVERAGE:					// maintainalphacoverage has changed
					case IDC_NM_BUMPINVERT:									// bump map invert has changed
						{
							if(wID==IDC_TEMPLATECOMBO && !This->m_pImageCompiler->m_Props.GetUserDialogCustom())		// no custom
							{
								HWND hwndCombo=GetDlgItem(hWnd,IDC_TEMPLATECOMBO); assert(hwndCombo);
								if(ComboBox_GetCurSel(hwndCombo)==0)
								{
									This->TriggerResetSettings();
								}
							}

							This->TriggerUpdatePreview();
						}
						return(TRUE);

						// preview changes
					case IDC_PREVIEWMODE:
					case IDC_PREVIEWTILED:
					case IDC_PREVIEWFILTERED:
						This->TriggerUpdatePreview();
						return(TRUE);
				}
			}
			return(TRUE);

		case WM_PAINT:											//
			{
				PAINTSTRUCT paint;
				BeginPaint(hWnd, &paint);
				EndPaint(hWnd, &paint);
				This->TriggerRepaint();
				break;
			}

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
			{
				assert(This);			if(!This)return(FALSE);
				LockHandle lh = This->Lock();

				int iY=(int)HIWORD(lParam);

				if(iY<=This->m_iPreviewHeight)
				{
					This->m_RelMouse.OnButtonDown(hWnd);

					SetFocus(This->m_hWindow);
				}
			}
			return(TRUE);

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
			{
				assert(This);			if(!This)return(FALSE);
				LockHandle lh = This->Lock();

				if(wParam==0)
					This->m_RelMouse.OnButtonUp();	
			}
			return(TRUE);

		case WM_MOUSEMOVE:
			{
				assert(This);			if(!This)return(FALSE);
				LockHandle lh = This->Lock();

				bool bButtonDown=(wParam&MK_LBUTTON)!=0 || (wParam&MK_MBUTTON)!=0 || (wParam&MK_RBUTTON)!=0;

				int iY=(int)HIWORD(lParam);

				if(iY<=This->m_iPreviewHeight || This->m_RelMouse.IsCaptured())
				{
					int relX,relY;

					This->m_RelMouse.OnMouseMove(hWnd,bButtonDown,relX,relY);
					This->MouseMessage((DWORD)wParam,relX,relY,0);
				}
			}
			return(TRUE);

		case WM_MOUSEWHEEL:
			{
				assert(This);			if(!This)return(FALSE);
				LockHandle lh = This->Lock();

				int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

				This->MouseMessage(0,0,0,-zDelta);
			}
			return(TRUE);

		case WM_CLOSE:
			{
				LockHandle lh = This->Lock();
				This->m_bQuitting=true;
			}
			return (TRUE);
	}

  return(FALSE);
}

DWORD CImageUserDialog::ThreadStart_Static(void* pThis)
{
	return static_cast<CImageUserDialog*>(pThis)->ThreadStart();
}

DWORD CImageUserDialog::ThreadStart()
{
	// zoom out if image is too big
	{
		int iTexWidth=(int)m_pImageCompiler->GetInputImageWidth(), iTexHeight=(int)m_pImageCompiler->GetInputImageHeight();

		m_PreviewData.iOrigWidth = iTexWidth;
		m_PreviewData.iOrigHeight = iTexHeight;

		while((iTexWidth>256 || iTexHeight>256) && m_PreviewData.iScale64!=1)
		{
			iTexWidth>>=1;
			iTexHeight>>=1;
			m_PreviewData.iScale64>>=1;
		}

		m_PreviewData.iCurMaxScale64 = m_PreviewData.iScale64;
		m_PreviewData.iInitialScale64 = m_PreviewData.iScale64;
	}

	m_hWindow=CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_IMAGEUSERDIALOG),0,CImageUserDialog::WndProc);

	if( !m_hWindow ) 
	{
		DWORD h=GetLastError();
		m_pImageCompiler=0;
		return(false);
	}

	// store this pointer	
	SetWindowLongPtr(m_hWindow,DWL_USER,(LONG)(LONG_PTR)this);

	//	CenterWindow(m_hWindow);			// here I can't see my tab items

	CreateDialogItems();

	CenterWindow(m_hWindow);				// here it's moving

	SetDataToDialog(true);

	UpdateWindowTitle();

	//UpdatePreview(true);			// call after IDC_PIXELFORMAT is created and image conversion is done
	TriggerUpdatePreview();

	ShowWindow(m_hWindow,SW_SHOW);

	//update preview subtitles
	{
		string sLeftInfo=m_pImageCompiler->GetInfoStringUI(true);

		HWND hwnd1=GetDlgItem(m_hWindow,IDC_LEFTPREVIEWINFO);			assert(hwnd1);
		HWND hwnd2=GetDlgItem(m_hWindow,IDC_RIGHTPREVIEWINFO);		assert(hwnd2);

		SetWindowText(hwnd1,sLeftInfo.c_str());
		SetWindowText(hwnd2,"");	
	}

	// message loop
	{
		MSG Msg;

		while(!m_bQuitting)
		{
			//using PeekMessage instead of blocking GetMessage call to be able to recognize quitting without getting input
			if (PeekMessage(&Msg, NULL, 0, WM_USER, PM_REMOVE))
			{
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}

			Sleep(1);
		}
	}

	DestroyWindow(m_hWindow);
	m_hWindow=0;

	return true;
}

void CImageUserDialog::TriggerUpdatePreview()
{
	LOCK_MONITOR();

	m_eWorkerAction = WorkerActionUpdatePreview;
}

void CImageUserDialog::TriggerGenerateOutput(bool bQuit)
{
	LOCK_MONITOR();

	if (bQuit)
		m_eWorkerAction = WorkerActionGenerateOutputAndQuit;
	else
		m_eWorkerAction = WorkerActionGenerateOutput;
}

void CImageUserDialog::TriggerSaveConfig(bool bQuit)
{
	LOCK_MONITOR();

	if (bQuit)
		m_eWorkerAction = WorkerActionSaveConfigAndQuit;
	else
		m_eWorkerAction = WorkerActionSaveConfig;
}

void CImageUserDialog::TriggerRepaint()
{
	LOCK_MONITOR();

	if (m_eWorkerAction == WorkerActionNone)
		m_eWorkerAction = WorkerActionPaint;
}

CImageUserDialog::WorkerAction CImageUserDialog::CheckAndResetWorkerAction()
{
	LOCK_MONITOR();

	WorkerAction eAction = m_eWorkerAction;
	m_eWorkerAction = WorkerActionNone;
	return eAction;
}

void CImageUserDialog::OnTinyDocumentChanged(TinyDocument<float>* pDocument)
{
	TriggerUpdatePreview();
}

void CImageUserDialog::OnTinyDocumentChanged(TinyDocument<bool>* pDocument)
{
	TriggerUpdatePreview();
}

void CImageUserDialog::TriggerCancel()
{
	LOCK_MONITOR();

	m_eWorkerAction = WorkerActionQuit;
}

void CImageUserDialog::TriggerResetSettings()
{
	LOCK_MONITOR();

	m_eWorkerAction = WorkerActionNoCustomSettings;
}
