#pragma once

class CImageCompiler;

#include "RelativeMouseInput.h"						// CRelativeMouseInput
#include "UIBumpmapPanel.h"								// CUIBumpmapPanel
#include "UIMIPControlPanel.h"						// CUIMIPControlPanel
#include "Monitor.h"
#include "ImageProperties.h"
#include "TinyDocument.h"

#define DEFAULT_SCALE64 64

//--------------------------------------------------------------------------------------------------
//structure used to store current preview data for calculation and rendering
//--------------------------------------------------------------------------------------------------
struct SPreviewData
{
	float	fShiftX;							//shift in x dir, in texture coordinates [0..1] initially 0.5
	float	fShiftY;							//shift in y dir, in texture coordinates [0..1] initially 0.5
	float	fOldShiftX;						//previous shift in x to recognize movements
	float	fOldShiftY;						//previous shift in y to recognize movements
	int		iScale64;							//current scale (fixpoint)
	int		iCurMaxScale64;				//current maximum scale for which mips are available
	int		iInitialScale64;			//initial scale for current preview image
	int		iOrigWidth;						//original width of current preview image
	int		iOrigHeight;					//original height of current preview image
	bool	bIsUpdating;					//indicates whether preview is currently being updated
	bool	bDelayUpdate;					//indicates whether preview update should be delayed
	DWORD	dwMouseButtonsOld;		//previous state of mouse buttons, for appropriate update on shift movements and update delaying

	SPreviewData()
	{
		fShiftX = 0.5f;
		fShiftY = 0.5f;
		fOldShiftX = 0.5f;
		fOldShiftY = 0.5f;
		iScale64 = DEFAULT_SCALE64;
		iCurMaxScale64 = DEFAULT_SCALE64;
		iInitialScale64 = DEFAULT_SCALE64;
		iOrigWidth = 0;
		iOrigHeight = 0;
		bIsUpdating = false;
		bDelayUpdate = false;
		dwMouseButtonsOld = 0;
	}
};

class CImageUserDialog : public Monitor, public TinyDocument<float>::Listener, public TinyDocument<bool>::Listener
{
public:

	SPreviewData					m_PreviewData;

	//! constructor
	CImageUserDialog();
	//! destructor
	virtual ~CImageUserDialog();

	//!
	bool DoModal( CImageCompiler *inpImageCompiler );
	
	// Returns
	//   true if preview conversion should be canceled
	bool PreviewGenerationCanceled();
	void UpdateProgressBar(const float fProgress);

	// Returns
	//   number of MIP levels that can be skipped because preview display is scaled down (zero if no MIPs available)
	int GetPreviewReduceResolution();
	// Returns
	//   top/left included, right/bottom excluded
	RECT GetPreviewRectangle( const uint32 mipLevel=0 );

private: // ---------------------------------------------------------------------------------------

	enum WorkerAction
	{
		WorkerActionNone,
		WorkerActionPaint,
		WorkerActionUpdatePreview,
		WorkerActionGenerateOutput,
		WorkerActionGenerateOutputAndQuit,
		WorkerActionSaveConfig,
		WorkerActionSaveConfigAndQuit,
		WorkerActionQuit,
		WorkerActionNoCustomSettings
	};
	WorkerAction m_eWorkerAction;

	CImageCompiler *			m_pImageCompiler;				//!< this class is not responsibe for destruction
	bool									m_bQuitting;						//!< 

	HWND									m_hWindow;							//!< main window handle

	// sub windows (in the tab control)
	HWND									m_hTab_Saveformat;			//!< window handle
	HWND									m_hTab_Advanced;				//!< window handle
	CUIBumpmapPanel				m_Bump2NormalPanel;			//
	CUIBumpmapPanel				m_AlphaAsBumpPanel;			//
	CUIMIPControlPanel		m_MIPControlPanel;			//!<

	const int							m_iPreviewWidth;				//!<
	const int							m_iPreviewHeight;				//!<

	CRelativeMouseInput		m_RelMouse;								//!< for relative mouse movement
	bool									m_bDialogIsUpdating;		//!<

	HANDLE								m_hWindowThread;

	//!
	static BOOL CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

	//!
	void Draw( HDC inHdc );

	//!
	void MouseMessage( const DWORD indwButtons, const int iniRelX , const int iniRelY, int iniRelZ );

	//!
	void UpdateWindowTitle();

	void UpdatePixelFormatDesc();

	void UpdatePreview( const bool bFullConvert );

	void CreateDialogItems();

	void GetDataFromDialog();

	//! makes the dialog visible too
	void SetDataToDialog( const bool bInitalUpdate=false );

	void SetPropertyTab( const int iniNo ); 

	void UpdateBumpStrength();

	void UpdateBumpBlur();

	static DWORD WINAPI ThreadStart_Static(void* pThis);
	DWORD ThreadStart();

	void TriggerUpdatePreview();
	void TriggerGenerateOutput(bool bQuit);
	void TriggerSaveConfig(bool bQuit);
	void TriggerRepaint();
	void TriggerCancel();
	void TriggerResetSettings();
	WorkerAction CheckAndResetWorkerAction();

	virtual void OnTinyDocumentChanged(TinyDocument<float>* pDocument);
	virtual void OnTinyDocumentChanged(TinyDocument<bool>* pDocument);

	void ZoomPreviewUpdateCheck();
};
