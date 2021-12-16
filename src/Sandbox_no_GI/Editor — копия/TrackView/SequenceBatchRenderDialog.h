//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File Name        : SequenceBatchRenderDialog.h
//  Author           : Jaewon Jung
//  Time of creation : 3/1/2011   11:23
//  Compilers        : VS2008
//  Description      : A dialog for batch-rendering sequences
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

#ifndef __SEQUENCE_BATCH_RENDER_DIALOG_H__
#define __SEQUENCE_BATCH_RENDER_DIALOG_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMFCButton;

class CSequenceBatchRenderDialog : public CDialog, public IMovieListener
{
public:
	CSequenceBatchRenderDialog(float fps, CWnd *pParent = NULL);
	virtual ~CSequenceBatchRenderDialog();

	enum { IDD = IDD_SEQUENCE_BATCH_RENDER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg void OnAddRenderItem();
	afx_msg void OnRemoveRenderItem();
	afx_msg void OnClearRenderItems();
	afx_msg void OnUpdateRenderItem();
	afx_msg void OnOutputFolder();
	afx_msg void OnLoadPreset();
	afx_msg void OnSavePreset();
	afx_msg void OnGo();
	afx_msg void OnDone();
	afx_msg void OnSequenceSelected();
	afx_msg void OnRenderItemSelChange();
	afx_msg void OnFPSEditChange();
	afx_msg void OnResolutionSelected();
	afx_msg void OnStartFrameChange();
	afx_msg void OnEndFrameChange();
	afx_msg void OnLoadBatch();
	afx_msg void OnSaveBatch();
	afx_msg void OnBuffersSelected();
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM lCount);

	DECLARE_MESSAGE_MAP()

	void SaveOutputOptions(const char *pathname) const;
	bool LoadOutputOptions(const char *pathname);

	CComboBox m_sequenceCombo;
	CComboBox m_shotCombo;
	CListBox m_renderList;
	CNumberCtrl m_startFrame, m_endFrame;
	CComboBox m_resolutionCombo;
	CComboBox m_fpsCombo;
	CComboBox m_imageFormatCombo;
	CComboBox m_buffersToCaptureCombo;
	CStatic m_progressStatusMsg;
	// This is a pointer to avoid including 'afxbutton.h' here
	// because it causes a name clash with the 'CImage' thing.
	CMFCButton *m_pGoBtn;
	CProgressCtrl m_progressBar;
	CEdit m_cvarsEdit;
	CButton m_updateBtn;

	float m_fpsForTimeToFrameConversion;	// FPS setting in TrackView
	struct SRenderItem
	{
		IAnimSequence *pSequence;
		IAnimNode *pDirectorNode;
		Range frameRange;
		int resW, resH;
		int fps;
		int formatIndex;
		int bufferIndex;
		CString folder;
		CString prefix;
		std::vector<CString> cvars;
		SRenderItem() : pSequence(NULL), pDirectorNode(NULL) {}
		bool operator==(const SRenderItem &item)
		{
			if(pSequence == item.pSequence
			&& pDirectorNode == item.pDirectorNode
			&& frameRange == item.frameRange
			&& resW == item.resW && resH == item.resH
			&& fps == item.fps
			&& formatIndex == item.formatIndex
			&& bufferIndex == item.bufferIndex
			&& folder == item.folder
			&& prefix == item.prefix
			&& cvars == item.cvars)
				return true;
			else
				return false;
		}
	};
	std::vector<SRenderItem> m_renderItems;
	struct SRenderContext
	{
		int currentItemIndex;
		float expectedTotalTime;
		float spentTime;
		int flagBU;
		Range rangeBU;
		int cvarCustomResWidthBU, cvarCustomResHeightBU;
		bool bWarmingUpAfterResChange;
		DWORD timeWarmingUpStarted;
		IAnimNode *pActiveDirectorBU;
		ICaptureKey captureOptions;

		bool IsInRendering() const
		{ return currentItemIndex >= 0; }

		SRenderContext() : currentItemIndex(-1), expectedTotalTime(0), spentTime(0), 
											flagBU(0), pActiveDirectorBU(NULL),
											cvarCustomResWidthBU(0), cvarCustomResHeightBU(0), 
											bWarmingUpAfterResChange(false), timeWarmingUpStarted(0) {}
	};
	SRenderContext m_renderContext;
	// Custom values from resolution/FPS combo boxes
	int m_customResW, m_customResH;
	int m_customFPS;

	void InitializeContext();
	virtual void OnMovieEvent(IMovieListener::EMovieEvent event, IAnimSequence* pSequence);
	void StartCaptureItem();
	void ReallyStartCaptureItem();
	void EndCaptureItem(IAnimSequence* pSequence);
	bool SetUpNewRenderItem(SRenderItem& item);
	void AddItem(const SRenderItem& item);
};

#endif // __SEQUENCE_BATCH_RENDER_DIALOG_H__