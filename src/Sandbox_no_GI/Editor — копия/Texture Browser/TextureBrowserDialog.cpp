/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: TextureBrowserDialog.cpp,v 1.0 2008/07/30 17:24:35 PauloZaffari Exp wwwrun $
$DateTime$
Description:  MFC class implementing the texture browser dialog.
-------------------------------------------------------------------------
History:
- 30:7:2008   17:24 : Created by Paulo Zaffari

*************************************************************************/

#include "stdafx.h"

//#include "../Controls/TextureViewer.h"
#include "TextureBrowserPanel.h"

#include "TextureBrowserDialog.h"

#define IDC_DOCKING_PANE 1000

// CTextureBrowserDialog dialog
IMPLEMENT_DYNCREATE(CTextureBrowserDialog,CBaseFrameWnd)

BEGIN_MESSAGE_MAP(CTextureBrowserDialog, CBaseFrameWnd)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
CTextureBrowserDialog::CTextureBrowserDialog()
{
	CTextureBrowserDialog::GetCurrentInstance()=this;
	GetIEditor()->RegisterNotifyListener(this);
	CRect rc(0,0,0,0);
	Create( WS_CHILD|WS_VISIBLE,rc,AfxGetMainWnd() );
}
//////////////////////////////////////////////////////////////////////////
CTextureBrowserDialog::~CTextureBrowserDialog()
{
	GetIEditor()->UnregisterNotifyListener( this );
	CTextureBrowserDialog::GetCurrentInstance()=NULL;
}

//////////////////////////////////////////////////////////////////////////
void CTextureBrowserDialog::RegisterViewClass()
{
	GetIEditor()->GetClassFactory()->RegisterClass( new CTextureBrowserViewClass );
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureBrowserDialog::SetTotalFileSize(unsigned long long nTotalFilzeSize)
{
	string strCaption;
	strCaption.Format("Selected database size: %I64u bytes",nTotalFilzeSize);
	m_oWindowStatusBar.SetPaneText(1,strCaption.c_str());
}
//////////////////////////////////////////////////////////////////////////
void CTextureBrowserDialog::SetCurrentFilename(const char* cszDirectory,const char* cszCurrentFilename)
{
	string strCaption(cszDirectory);
	strCaption+=cszCurrentFilename;
	m_oWindowStatusBar.SetPaneText(0,strCaption.c_str());
	m_poTextureBrowserPanel->SetFilenameDisplay(cszCurrentFilename);
}
//////////////////////////////////////////////////////////////////////////
void CTextureBrowserDialog::SetTextureType(const char* cszTextureType)
{
	m_poTextureBrowserPanel->SetSurfaceTypeDisplay(cszTextureType);
}
//////////////////////////////////////////////////////////////////////////
void CTextureBrowserDialog::SetNumberofMips( const char* szNumberOfMips )
{
	m_poTextureBrowserPanel->SetNumberOfMipsDisplay(szNumberOfMips);
}
//////////////////////////////////////////////////////////////////////////
void CTextureBrowserDialog::SetResolution(const char* szResolution)
{
	m_poTextureBrowserPanel->SetTextureResolutionDisplay(szResolution);
}
//////////////////////////////////////////////////////////////////////////
void CTextureBrowserDialog::SetFileSize(const char* szTotalFilzeSize)
{
	m_poTextureBrowserPanel->SetFileSizeDisplay(szTotalFilzeSize);
}
//////////////////////////////////////////////////////////////////////////
void CTextureBrowserDialog::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	if (event==eNotify_OnIdleUpdate)
	{
		return;
	}

	switch (event)
	{
		
		case eNotify_OnCloseScene:    // Send when the document is about to close.
		case eNotify_OnBeginSceneOpen:// Sent when document is about to be opened.
		case 	eNotify_OnBeginNewScene:// Sent when the document is begin to be cleared.
			{
				// Here we must stop the database creator as many of the current level textures
				// may become invalid. We must also invalidate the current database.

				CTextureBrowserDialog::OnDestroy();

				// We are finishing the database as we can't guarantee the valitity of the items there.
				m_oTextureViewer.FinishDatabase();
			}
		break;

		//case eNotify_OnMissionChange: // Send when the current mission changes. I am not sure if this is necessary.
		case eNotify_OnEndSceneOpen: // Sent after document have been opened.
		case 	eNotify_OnEndNewScene: // Sent after the document have been cleared.
			{
				// We may now restart the database creator...
				if (!m_poTextureDatabaseCreator)
				{
					m_oTextureViewer.RestartThread();

					m_poTextureDatabaseCreator=new CTextureDatabaseCreator();
					m_poTextureDatabaseCreator->SetAssociatedViewer(&m_oTextureViewer);
					m_oTextureViewer.SetDatabaseUpdater(m_poTextureDatabaseCreator);	
					m_poTextureDatabaseCreator->Start();
				}
			}
		break;

		// Sent when document is about to be saved.
		case eNotify_OnBeginSceneSave:          
			{
				// Here we should lock the database creator so it will not interfere with saving.
				m_oTextureViewer.Lock();

				if (m_poTextureDatabaseCreator)
				{
					m_poTextureDatabaseCreator->Lock();				
				}
			}
		break;

		// Sent after document have been saved.
		case eNotify_OnEndSceneSave:
			{
				// We may now resume the database creator.
				m_oTextureViewer.Unlock();

				if (m_poTextureDatabaseCreator)
				{
					m_poTextureDatabaseCreator->Unlock();				
				}
			}
		break;
	}
}
//////////////////////////////////////////////////////////////////////////
CTextureBrowserDialog*& CTextureBrowserDialog::GetCurrentInstance()
{
	static CTextureBrowserDialog* poCurrentInstance(NULL);
	return poCurrentInstance;
}
//////////////////////////////////////////////////////////////////////////
void CTextureBrowserDialog::GetSelectedItems(CTextureViewer::TDTextureDatabase& rcpoSelectedItemArray)
{
	m_oTextureViewer.GetSelectedItems(rcpoSelectedItemArray);
}
//////////////////////////////////////////////////////////////////////////
bool CTextureBrowserDialog::SetSelectedItem( const char* szItemFilename )
{
	bool boIsVisible(false);
	bool boReturn(false);

	boReturn=m_oTextureViewer.SetSelectedItem(szItemFilename,boIsVisible);
	if (!boIsVisible)
	{
		m_poTextureBrowserPanel->ClearFilters();
	}
	
	return boReturn;
}
//////////////////////////////////////////////////////////////////////////
void CTextureBrowserDialog::SetTextureBrowserDoubleClickCallback(CTextureViewer::TDDoubleClickCallback	pfnDoubleClickCallback)
{
	if (m_oTextureViewer)
	{
		m_oTextureViewer.SetDoubleClickCallback( pfnDoubleClickCallback ? pfnDoubleClickCallback : functor(*this,&CTextureBrowserDialog::StandardDoubleClickCallback) );
	}
}
//////////////////////////////////////////////////////////////////////////
void CTextureBrowserDialog::DoDataExchange(CDataExchange* pDX)
{
	CBaseFrameWnd::DoDataExchange(pDX);
}

BOOL CTextureBrowserDialog::OnInitDialog()
{
	CreateTextureViewer();

	m_poDockingPane = GetDockingPaneManager()->CreatePane( IDC_DOCKING_PANE, CRect(0,0,220,200), dockLeftOf );

	m_oRollupControl.Create( WS_CHILD|WS_VISIBLE,CRect(0,0,100,100),this,2 );

	m_poTextureBrowserPanel = new CTextureBrowserPanel(&m_oTextureViewer,&m_oRollupControl);
	m_oRollupControl.InsertPage( "Browse Information",m_poTextureBrowserPanel );

	// We MUST use the ID AFX_IDW_STATUS_BAR if we are speaking about the status bar.
	// We also are reserving 2 slots into the status bar, for currently selected file path
	// AND current dataset size.
	UINT indicators[] =
	{
		AFX_IDW_STATUS_BAR,           
		AFX_IDW_STATUS_BAR,           
	};

	m_oWindowStatusBar.Create( this, WS_CHILD|WS_VISIBLE|CBRS_BOTTOM);
	m_oWindowStatusBar.SetIndicators(indicators,2);

	//////////////////////////////////////////////////////////////////////////
	char szCaption[512];

	sprintf(szCaption, "No selection"); //Placeholder info.
	m_oWindowStatusBar.SetPaneText(0, szCaption);//Placeholder info.

	sprintf(szCaption, "Refreshing texture database...");//Placeholder info.
	m_oWindowStatusBar.SetPaneText(1, szCaption);//Placeholder info.
	//////////////////////////////////////////////////////////////////////////

	return TRUE;
}

void CTextureBrowserDialog::CreateTextureViewer()
{
	// Creates the container itself..OBS: we can't create such containers for only one element...
	// ... so we add to this one a useless beautification element so it will not complain...
	m_oViewportContainer.CreateStatic(this,2,1);
	m_oViewportContainer.SetSplitterStyle(XT_SPLIT_NOFULLDRAG | XT_SPLIT_NOSIZE | XT_SPLIT_NOBORDER);
	// Sizing for splitter
	m_oViewportContainer.SetRowInfo (0, 22, 0);


	// For us to have a caption. As we have must have at leas 2 columns, let's at list put something
	// interesting there.
	m_oWindowCaption.Create(&m_oViewportContainer, _T("Available Textures"), NULL,WS_VISIBLE|SS_CENTER|SS_CENTERIMAGE, CRect(0,0,0,0),m_oViewportContainer.IdFromRowCol(0, 0));
	m_oWindowCaption.SetCaptionColors (GetXtremeColor(COLOR_3DFACE),GetXtremeColor(COLOR_3DSHADOW), GetXtremeColor(COLOR_WINDOW) );
	m_oWindowCaption.SetOffice2003Colors(true);

	// Setting the standard for for our beautification element.
	LOGFONT lf;
	XTAuxData().fontBold.GetLogFont(&lf);
	lf.lfHeight = 18;
	m_fontCaption.CreateFontIndirect(&lf);

	// set the font and initial icon.
	m_oWindowCaption.ModifyCaptionStyle(0,&m_fontCaption, NULL, NULL);

	// 
	//m_oTextureViewer	= new CTextureViewer();
	m_oTextureViewer.Create(0,0,100,100,&m_oViewportContainer);
	m_oTextureViewer.SetDlgCtrlID( m_oViewportContainer.IdFromRowCol(1,0) );
	//m_oTextureViewer->MoveWindow( CRect(20,50,500,500) );
	//m_oTextureViewer->ModifyStyle( WS_POPUP,WS_CHILD,0 );
	m_oTextureViewer.SetParent( &m_oViewportContainer );
	m_oTextureViewer.SetOwner( this );
	m_oTextureViewer.SetDoubleClickCallback(functor(*this,&CTextureBrowserDialog::StandardDoubleClickCallback));

	// We have to do this because this way the owner doesn't need to implement the information
	// display specifics...doing so will allow us to use this control in more places.
	m_oTextureViewer.SetStatusDisplay(this); 
	

	m_poTextureDatabaseCreator=new CTextureDatabaseCreator();
	m_poTextureDatabaseCreator->SetAssociatedViewer(&m_oTextureViewer);
	m_oTextureViewer.SetDatabaseUpdater(m_poTextureDatabaseCreator);	
	m_poTextureDatabaseCreator->Start();
}

void CTextureBrowserDialog::OnDestroy()
{
	m_oTextureViewer.NotifyShutDown();
	m_oTextureViewer.ResumeThread();
	m_oTextureViewer.WaitForThread();
	m_oTextureViewer.Stop();

	// Kill the database updater, if needed.
	if (m_poTextureDatabaseCreator)
	{
		// Tell it to stop what it is doing...
		m_poTextureDatabaseCreator->NotifyShutDown();
		m_poTextureDatabaseCreator->Unlock();
		// And then wait for it to finish, so it won't leak memory or anything.
		m_poTextureDatabaseCreator->WaitForThread();
		m_poTextureDatabaseCreator->Stop(); // This is here only to avoid log warnings, as this is not necessary.
		m_oTextureViewer.SetDatabaseUpdater(NULL);
		delete m_poTextureDatabaseCreator;
		m_poTextureDatabaseCreator=NULL; // Well, we will not call the destructor again, but it's nice to do it.
	}
}

void CTextureBrowserDialog::OnClose()
{	
	DestroyWindow();
}

//////////////////////////////////////////////////////////////////////////
LRESULT CTextureBrowserDialog::OnDockingPaneNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == XTP_DPN_SHOWWINDOW)
	{
		// get a pointer to the docking pane being shown.
		CXTPDockingPane* pwndDockWindow = (CXTPDockingPane*)lParam;    
		if (!pwndDockWindow->IsValid())
		{
			switch (pwndDockWindow->GetID())
			{
			case IDC_DOCKING_PANE:
				pwndDockWindow->Attach(&m_oRollupControl);
				break;
			default:
				return FALSE;
			}
		}
		return TRUE;
	}
	else if (wParam == XTP_DPN_CLOSEPANE)
	{
		// get a pointer to the docking pane being closed.
		CXTPDockingPane* pwndDockWindow = (CXTPDockingPane*)lParam;
		if (pwndDockWindow->IsValid())
		{
		}
	}

	return FALSE;
}

BOOL CTextureBrowserDialog::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message==WM_MOUSEWHEEL)
	{
		CWnd* poChildWindow=WindowFromPoint(pMsg->pt);
		if (poChildWindow==&m_oTextureViewer)
		{
			::SendMessage(m_oTextureViewer.GetSafeHwnd(),pMsg->message,pMsg->wParam,pMsg->lParam);
			return TRUE;
		}		
	}

	return FALSE;
}

void CTextureBrowserDialog::StandardDoubleClickCallback()
{
	CMainFrame*	poMainFrame=((CMainFrame*)AfxGetMainWnd());

	if (!poMainFrame->IsPageDocked("Texture Browser"))
	{
		poMainFrame->ClosePage("Texture Browser");
	}	
}
